/**
 * \file native_tx_evaluator.cpp
 *
 * \author angel.castillo
 * \date   Jun 20, 2026
 *
 * Copyright 2026 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


// Exercise the packaged WASM runtime, including native Plutus cost-model setup.
const assert = require('node:assert/strict');
const createModule = require(process.argv[2]);

(async () => {
  const module = await createModule();
  for (const name of ['addFunction', 'removeFunction', 'getTempRet0',
    'getValue', 'setValue', 'UTF8ToString', 'stringToUTF8', 'lengthBytesUTF8',
    '_malloc', '_free']) assert.equal(typeof module[name], 'function', name);
  assert.equal(typeof module.Asyncify, 'object');
  assert.equal(module.UTF8ToString(module.get_lib_version()), process.argv[3]);
  assert.equal(module._mallinfo, undefined, 'Allocator statistics must be opt-in');
  const callbacks = [];
  try {
    for (let i = 0; i < 1024; ++i) {
      callbacks.push(module.addFunction((context, tx, inputs, redeemer) => i, 'iiiii'));
    }
    assert.equal(new Set(callbacks).size, callbacks.length);
  } finally {
    for (const callback of callbacks) module.removeFunction(callback);
  }

  const buffers = [];
  const objects = [];
  let deferredCallback = 0;
  const allocate = size => {
    const ptr = module._malloc(size);
    assert.ok(ptr);
    buffers.push(ptr);
    module.HEAPU8.fill(0, ptr, ptr + size);
    return ptr;
  };
  const slot = allocate(4);
  const construct = (fn, ...args) => {
    assert.equal(fn(...args, slot), 0);
    const ptr = module.getValue(slot, 'i32');
    assert.ok(ptr);
    objects.push(ptr);
    return ptr;
  };
  const decode = (hex, fn) => {
    const text = allocate(hex.length + 1);
    module.stringToUTF8(hex, text, hex.length + 1);
    const reader = module.cbor_reader_from_hex(text, hex.length);
    assert.ok(reader);
    objects.push(reader);
    return construct(fn, reader);
  };
  try {
    // One Plutus V4 always-succeeding spending script (lambda context -> unit).
    // This synthetic transaction is for evaluation only, not network submission.
    const transaction = decode('84a400d90102818258200101010101010101010101010101010101010101010101010101010101010101000181a200581d6011111111111111111111111111111111111111111111111111111111011a001e848002000b58200000000000000000000000000000000000000000000000000000000000000000a205a182000082d8798082000008d901028146450100002499f5f6', module.transaction_from_cbor);
    const utxo = decode('82825820010101010101010101010101010101010101010101010101010101010101010100a200581d701fd0190f1816f66a28208ce5836f64f54d99bdc2a088fee8e14547ed011a05f5e100', module.utxo_from_cbor);
    const inputs = construct(module.utxo_list_new);
    assert.equal(module.utxo_list_add(inputs, utxo), 0);
    // Synthetic positive coefficients force cost_model_from_params, the path
    // that exceeded the default 64 KiB stack. These are not network parameters.
    const costs = decode('a10398b3' + '01'.repeat(179), module.costmdls_from_cbor);
    const slots = allocate(24);
    module.setValue(slots + 16, 1000, 'i32');
    const evaluator = construct(module.tx_evaluator_new_native, slots, costs, 12, 0);
    const redeemers = construct(module.tx_evaluator_evaluate, evaluator, transaction, inputs);
    assert.equal(module.redeemer_list_get_length(redeemers), 1);

    const params = construct(module.protocol_parameters_new);
    const version = construct(module.protocol_version_new, 12, 0, 0, 0);
    const budget = construct(module.ex_units_new, 14000000, 0, 1410065408, 2);
    assert.equal(module.protocol_parameters_set_protocol_version(params, version), 0);
    assert.equal(module.protocol_parameters_set_cost_models(params, costs), 0);
    assert.equal(module.protocol_parameters_set_max_tx_ex_units(params, budget), 0);
    const builder = module.sub_tx_builder_new(params, slots);
    assert.ok(builder);
    objects.push(builder);
    const script = decode('820446450100002499', module.script_from_cbor);
    module.sub_tx_builder_add_script(builder, script);
    const zero = allocate(2);
    module.stringToUTF8('0', zero, 2);
    let invocations = 0;
    deferredCallback = module.addFunction((context, draft, resolved, out) => {
      assert.ok(draft);
      assert.ok(resolved);
      ++invocations;
      return module.plutus_data_new_integer_from_string(zero, 1, 10, out);
    }, 'iiiii');
    module.sub_tx_builder_add_input_with_deferred_redeemer(builder, utxo, deferredCallback, 0, 0);
    construct(module.sub_tx_builder_build, builder);
    assert.equal(invocations, 1);
  } finally {
    for (const ptr of objects.reverse()) {
      module.setValue(slot, ptr, 'i32');
      module.object_unref(slot);
    }
    if (deferredCallback) module.removeFunction(deferredCallback);
    for (const ptr of buffers.reverse()) module._free(ptr);
  }
  console.log('Packaged WASM callbacks and native Plutus evaluation passed');
})().catch(error => { console.error(error); process.exitCode = 1; });
