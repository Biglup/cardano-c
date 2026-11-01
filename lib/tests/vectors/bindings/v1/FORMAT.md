# Conformance adapter protocol

The corpus describes behavior. Each binding owns an adapter implemented against
its **public API**, plus runtime-specific lifetime instrumentation. The adapter
must not load expected results or use a second binding to compute its answers.

## Running an adapter

Run from the cardano-c checkout, with Python 3.9 or newer:

```sh
python3 scripts/check-binding-conformance.py --report conformance.json -- node /path/to/adapter.cjs
```

Replace the command after `--` with any executable: a Go or C++ test program,
`dotnet run`, Java, Python, Ruby, etc. The checker needs only Python's standard
library. It sends JSON lines on stdin and accepts JSON lines on stdout. Diagnostic
logging belongs on stderr. The default process timeout is 600 seconds.

For development, `--suite builders` selects only that suite. Multiple `--suite`
arguments are allowed. The report marks these runs `full_corpus: false`; they
must not be advertised as full conformance. Unknown suites are errors.

A request has `schema_version`, `suite`, `id`, `operation`, `input`, and, where
applicable, `type`, `config`, and `recipes`. Expectations are never sent.
The response is:

```json
{"id":"address-6-0","result":{"address":"the computed address"}}
```

Return exactly one response for each request. Response order is immaterial.
Missing, duplicate, unexpected, malformed, and unsupported responses fail the
run. The result must have the expected fields and value types, without silently
omitting fields. Optional response-level diagnostics may accompany `result`.

The manifest enumerates all files and case counts. Duplicate IDs and count
mismatches fail before an adapter is invoked. Pin the corpus's repository commit
alongside the native version used by the binding.

## Values and comparison

Hex strings use lowercase, without `0x`. Byte strings are not UTF-8 text. Amounts,
large integers, and builder parameter values are decimal strings wherever the
schema uses strings. Convert them to the target language's exact integer type.
JSON array order is significant; object key order and whitespace are not. A
string, boolean, and number with superficially similar values are different.

Normalize idiomatic names and value representations back to the fixture schema
in the adapter. Do not discard fields. Exercise decoding into public values and
encoding those values again, in addition to any preserved-CBOR path. A direct
call to native decode/encode alone does not test the binding's value marshaler.

`native_error` records the native reference result for maintainers and native
tests. The checker removes it from public expectations: bindings return an
idiomatic failure classified as `decode_error`, `build_error`, or `invalid_input`.
Do not expose C enum values to application users just to pass this suite.

## Operations

| Operation | Inputs | Result |
| --- | --- | --- |
| `cbor` | `type`, `input.cbor`; transactions may include `input.signing_seed` | `cbor`, plus the documented expected inspection/hash/signature fields for that type |
| `ed25519` | 32-byte `seed`, arbitrary `message` bytes | `public_key`, deterministic `signature` |
| `verify_ed25519` | `public_key`, `signature`, `message` | `valid` boolean |
| `blake2b` | `message`, digest `size` in bytes | `hash`, or `error: invalid_input` for inputs rejected by the native compute API |
| `bip32` | BIP39 `entropy`, password bytes, unsigned derivation `path` | Derived `private_key`, extended `public_key`, raw `ed25519_public_key` |
| `address` | Raw address `bytes` | Bech32 `address`; also assert decoding it returns the original bytes |
| `apply_params` | Compiled script bytes and `parameters_cbor` (a Plutus list) | Applied `script` bytes |
| `build` | Configuration, transaction type, ordered steps, referenced child recipes | `cbor`, `inspection`, `callback_invoked`; on failure, `error` and `callback_invoked` |
| `lifecycle` | Named scenario, iteration count, behavioral contract | `passed`, measured `native_live_delta`, measured `callback_live_delta` |

### CBOR types

The serialization group covers `transaction`, `sub_transaction`,
`transaction_body`, `transaction_input`, `transaction_output`, `value`,
`credential`, `account_balance_interval`, `certificate`, `proposal_procedure`,
`voting_procedures`, `native_script`, `script`, and `plutus_data`.

For every successful case, return `cbor`. Return `inspection` for each type
whose case carries it. Every currently listed successful serialization type
provides inspection; consult the files when adding a type.

Transactions and subtransactions additionally return `body_cbor`, `hash`,
`verification_key`, and `signature`. Sign the raw 32-byte body hash with the
supplied normal Ed25519 seed. Do not sign the envelope or the ASCII hex.
Scripts and native scripts additionally return their `hash`.

Malformed CBOR returns `{"error":"decode_error"}`. Check the whole operation,
not just whether input is a valid hex string. Some valid non-minimal inputs retain
their original representation; others normalize on serialization. Compare the
explicit expected `cbor`, not an assumed universal canonicalization rule.

### Builder configuration and steps

Each request includes the fixed `config` from `builders.json` and all child
recipe **inputs**. Never substitute a child's expected CBOR for building it.
Reject missing, cyclic, or unsupported recipe references.

`parameters` gives named integer protocol parameters. Values not specified use
the native parameter constructor's defaults, not current network parameters.
`protocol_version` is `[major, minor]`; a recipe may override it.
`max_tx_ex_units`, rational `execution_prices`, `plutus_v4_costs`, and
`slot_config` are explicit. Preserve their units and integer precision. Fees use
the native balancer and script budgets use the native evaluator. No live network
provider, wall clock, wallet, or external evaluator participates.

Top-level `utxos` have `tx_id`, `index`, Bech32 `address`, and decimal `coins`.
Their order is fixed; these recipes have one funding UTxO and therefore do not
depend on random coin selection. Apply `change_address` and then the steps in
order. Each build uses a fresh builder and independent callback state.

| Step | Meaning |
| --- | --- |
| `send` | Send `amount` lovelace to `address`. |
| `deposit` | Add a direct account deposit. |
| `withdraw` | Withdraw the specified amount from a reward account; combined with a deposit this represents an account transfer. |
| `balance`, `starting_balance` | Add the account interval: `exact`, or optional inclusive `lower` and exclusive `upper`. |
| `guard` | Add a `key` or `script` credential by hash; script guards include integer `redeemer` data. |
| `require_guard` | Require a top-level guard from a subtransaction. |
| `script` | Attach the compiled script; the current recipe language is `plutus_v4`. |
| `native_script` | Decode and attach the native script. Exercise the public native-script representation. |
| `native_mint` | Mint `amount` of `asset_name` under the preceding native script's computed policy hash. |
| `input` | Add the supplied resolved UTxO with the declared deferred callback. |
| `mint` | Mint the specified `asset_id` and signed amount with a deferred callback. |
| `sub_transaction` | Build the named child recipe, apply its signing seed if supplied, and add it to the batch. These children have no consumed inputs, so their resolved-input lists are empty. |

A subtransaction recipe with `signing_seed` must compute its body ID, create a
verification-key witness over that ID, and apply the witness before returning or
adding the child. The signed-batch recipe builds two different children with two
different test keys. Preserve their individual witnesses.

Deferred callback kinds are `constant_integer`, `error`, and `null`. The first
returns the specified Plutus integer; the second raises/returns a callback
failure; the third supplies no Plutus value. Record whether the callback actually
ran. Both invalid callback cases must fail the build and release partial output.
Do not fabricate a redeemer without registering and executing the callback.

### Lifecycle cases

These are executable adapter obligations, not precomputed transaction outputs.
For each scenario, implement the contract in `lifecycle.json` using public
operations and internal test-only allocation/callback instrumentation. Warm up
once, record a baseline, perform the requested iterations, drain operation
cleanup, and measure the difference. A stable live allocation baseline is not
zero process RSS or zero WASM heap capacity.

Report `passed: true` only after the behavior assertions and measured deltas pass.
Do not hardcode zeros. Missing instrumentation or an unimplemented scenario is an
unsupported case and fails full conformance. Retain the runtime/tool evidence
alongside the report. Native Valgrind tests cannot demonstrate host GC rooting or
callback trampoline removal in another runtime.

For languages without tracing GC, callback retention is tested with ordinary
scope/owner destruction. If the public API is strictly synchronous and offers no
cancellation, test that cancellation cannot release an active native operation;
record that execution model in response-level diagnostics. A cancellable API
must exercise cancellation while work is actually in flight. Failure injection
belongs in the adapter's test infrastructure, never in the public SDK API.

## Porting unit tests

Port behavioral tests from `lib/tests` as well as running this corpus. Start with
constructor/parse failures, integer boundaries, container ownership, transaction
builder errors, callback failure paths, and cryptographic verification failures.
Useful reference suites include:

- `lib/tests/transaction/` and `lib/tests/transaction_body/`
- `lib/tests/crypto/`, `lib/tests/address/`, and `lib/tests/plutus_data/`
- `lib/tests/providers/provider.cpp` and `lib/tests/key_handlers/secure_key_handler.cpp`
- `lib/tests/transaction_builder/balancing/deferred_redeemer_list.cpp`
- `lib/tests/binding_vectors.cpp` for the native fixture adapter and lifetime probes

Port the observable contract, not a literal C wrapper test. A C null-pointer test
may become an invalid-argument test; a reference-count test becomes a scoped
cleanup or retained-value test. Add target-language tests for exceptions, async
cancellation, GC, thread boundaries, packaging, and integer conversions.

Golden vectors establish agreement with cardano-c for the covered operations.
They do not prove complete API coverage, replace the native unit suite, prove
every possible input correct, or establish ledger acceptance. Maintain a mapping
from public cardano-c types, enum variants, and functions to idiomatic binding
APIs and tests, including operations absent from this corpus. Record intentional
internal mappings and unsupported features explicitly. A new public binding feature should bring its
behavioral tests and, where portable, an additional corpus case.
