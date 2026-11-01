Binding author guide
====================

Design contract
---------------

Cometa bindings must feel like libraries written in their target language.
Use that language's naming, value types, collections, errors, concurrency model,
and established design patterns. C declarations are an internal implementation
layer, not the public API design.

Transactions, addresses, assets, protocol parameters, and inspection results
should be language-owned values. Preserve integers exactly, including unsigned
64-bit amounts and arbitrary-precision Plutus integers. Use appropriate integer
or decimal types; never route these through floating-point numbers. Distinguish
missing values, null values, zero, and empty collections when the protocol does.
Use tagged variants for credentials, scripts, certificates, and Plutus data.

Expose read-only transaction snapshots to signing and deferred-redeemer
callbacks. A snapshot must remain usable after its native source is released;
copy its data or retain an immutable internal owner. Mutating a public collection must not
silently mutate a transaction whose hash or witnesses have already been computed.
Use builders for constructing transactions, with explicit validation and errors.

Users must not manage C pointers, reference counts, or native allocations.
Do not require disposal of ordinary domain values or builders merely because
cardano-c is used underneath. Prefer builders that accumulate language-owned
state and marshal during build. Native allocations belong to an internal scope
that is cleaned up on success, error, exception, or cancellation.

Language conventions still apply: C++ RAII, Ada controlled types, and Zig/Odin
allocators and scoped cleanup can express ownership naturally. Managed-language
finalizers may be a fallback, but must not be the only mechanism releasing
operation-scoped native allocations. Explicit close/dispose may be appropriate
for genuine resources such as a hardware-wallet session; document that resource,
not the C implementation. Do not promise that garbage collection zeroes secrets.

Architecture
------------

Separate these responsibilities:

* Generated or audited ABI declarations: sizes, signedness, alignment, calling
  conventions, exported symbols, and callback signatures.
* Internal ownership and marshaling: construction, retention, release, byte
  buffers, text encoding, and error translation.
* Language-level domain values, builders, providers, and signing interfaces.

Generate declarations where possible, but review ownership separately: a C type
or function name does not encode whether a result is borrowed or owned. Pin the
native release and test the actual binaries distributed to users. Share the
conformance corpus across bindings; do not generate expected results using the
binding under test during its test run.

The existing Cometa.js and Cometa.py implementations provide behavioral examples
and regression cases. They are not normative public API templates: new bindings
must not reproduce their historical pointer wrappers or cleanup requirements.

Ownership at the C boundary
---------------------------

The following rules describe the RC6 API. The documentation and implementation
of each individual function remain authoritative. Record ownership for every
function admitted to the internal binding layer, including failure paths.

.. list-table:: Ownership categories
   :header-rows: 1
   :widths: 22 38 40

   * - Category
     - Current example
     - Binding obligation
   * - Owned result
     - ``cardano_transaction_from_cbor``; ``cardano_transaction_get_body``;
       ``cardano_transaction_get_id``
     - Release the returned reference exactly once. A getter can return an owned
       reference; do not infer borrowing from its name.
   * - Borrowed storage
     - ``cardano_blake2b_hash_get_data`` and object error-message strings
     - Do not free it. Copy before its owner is released or modified, or retain
       the owner internally for the entire view lifetime.
   * - Retained argument
     - ``cardano_deferred_redeemer_list_add`` retains its redeemer
     - Release the binding's temporary reference after a successful call;
       the list has its own reference. Check each setter/container separately.
   * - Transferred context
     - ``cardano_provider_new`` and ``cardano_secure_key_handler_new`` copy
       their implementation and adopt its ``context`` reference on success
     - Do not release the transferred reference after success. On construction
       failure the binding still owns it. Destruction unreferences the context.
   * - Borrowed callback context
     - Deferred redeemer ``user_context``
     - The C list neither retains nor destroys it. Keep its host object and
       callback bridge rooted until no further invocation is possible.
   * - Caller-owned output storage
     - ``cardano_cbor_writer_encode_hex``
     - Allocate the documented capacity, including any terminator; release the
       allocation in the same allocator domain that created it.

Initialize local output pointers to null before calls. On failure, follow the
specific function's output contract; never assume all APIs leave outputs in the
same state. Keep a cleanup scope active until ownership transfer succeeds.
Do not mix C allocator buffers, runtime buffers, and WebAssembly heap addresses.
In WASM, refresh heap views after growth and avoid JavaScript bitwise operations
that truncate amounts to signed 32-bit integers.

The provider/key-handler context rules above follow
``lib/src/providers/provider.c`` and
``lib/src/key_handlers/secure_key_handler.c``. Deferred redeemer retention and
release follow ``lib/src/transaction_builder/balancing/deferred_redeemer_list.c``.

Callbacks and asynchronous operations
-------------------------------------

The C callback ABI returns synchronously. An asynchronous language-level provider
requires an explicit bridge (for example, a supported WASM suspension mechanism
or a worker executing blocking native work). Never return a promise or task where
C expects an error code. Document which execution context invokes user callbacks
and whether concurrent operations are supported. Do not assume native objects
or reference counts are thread-safe.

For every bridge:

#. Root the callable, captured state, and trampoline while C can invoke them.
   Do not store movable runtime pointers in C without the runtime's supported
   handle/pinning mechanism.
#. Treat callback inputs as borrowed unless their API explicitly says otherwise.
   Convert them to language-level values without exposing native mutation.
#. Construct owned native output objects in a local cleanup scope. On success,
   transfer the required reference to the C caller. On failure, release partial
   results and leave output pointers null where the callback contract permits.
#. Catch exceptions, panics, and runtime errors before they cross the C ABI.
   Return an appropriate ``cardano_error_t`` and preserve the original cause for
   the outer language-level operation. Copy diagnostics before releasing owners.
#. Keep registrations and native operation state alive until the native call has
   actually finished, even if a caller cancels its await. Cancellation does not
   authorize freeing a context still in use by C. Release each registration once.
#. Prevent unsupported reentry into the same mutable builder. A callback must
   not concurrently mutate or dispose of the object executing it.

Provider object outputs and key-handler witness-set outputs are owned results
for the C caller. When returning an existing internally owned object, acquire a
separate reference for that transfer. Signing callbacks receive a transaction or
subtransaction; present a read-only language-level object to the user. Sign the
appropriate body hash, not the complete transaction CBOR or its ASCII hex text.

Deferred redeemers have a stricter contract, defined by
``cardano_deferred_redeemer_fn_t``:

* ``draft_tx`` and ``resolved_inputs`` are borrowed and must not be modified.
* The returned Plutus data is transferred to C. The resolver releases that
  reference after applying it; the bridge must not also release it on success.
* Top-level balancing can invoke a callback on every iteration. It must be a
  pure, deterministic function of its arguments; no network calls, randomness,
  signing prompts, or mutable counters that affect the result.
* Subtransaction callbacks run once per build and use indices local to that
  subtransaction. Do not substitute top-level input positions.
* Keep context alive through the last possible build, not merely registration
  or the first callback. A reusable native builder may retain callbacks.

Conformance fixtures
--------------------

The versioned corpus is in ``lib/tests/vectors/bindings/v1``. Its README defines
fields, provenance, and comparisons. The native ``binding_conformance`` tests
consume the same JSON files that downstream bindings should use. The initial
cases cover Conway key guards, Dijkstra account deposits and balance intervals
(including UINT64_MAX), and a transaction containing a signed subtransaction.

For each case, a binding must check:

#. Exact transaction CBOR round-trip and exact body CBOR bytes.
#. Transaction ID and deterministic Ed25519 signature over the raw ID bytes,
   plus signature rejection after changing one message byte.
#. Structural equality of inspection JSON, preserving array order, types, and
   missing fields. Object key order and whitespace are insignificant.
#. Language-level values reconstructed from the fixture retain all fields and
   integer precision. Do not satisfy the test solely by forwarding CBOR to C
   while bypassing the public value marshaler.

The native JSON format is a wire-level oracle, not a requirement to expose
snake_case fields or decimal strings in every binding. Normalize a binding's
idiomatic inspection value to the fixture schema in its test adapter, without
silently dropping unknown fields. RC6 inspection uses ``guards`` for both key and
script credentials and does not emit ``required_signers``.

These are serialization and cryptographic fixtures, not funded transactions or
proof of ledger validity. They do not establish hardfork availability. Existing
nested signatures are preserved; each fixture's test signature is over its
outer transaction ID and is not inserted into its witnesses.

Builder conformance requires additional deterministic recipes: fixed protocol
parameters, slot configuration, UTxOs, requested outputs, change address, explicit
input/coin-selection order, script evaluation results, and signing keys. Never
compare an automatically balanced transaction against an unrelated serialization
fixture. Add a recipe and native reference build when a new binding implements
that surface. No live network provider should be needed by conformance CI.

Lifecycle conformance
---------------------

Identical output does not prove correct cleanup. Each binding must implement the
following tests using its own runtime and allocation instrumentation:

.. list-table:: Required lifecycle scenarios
   :header-rows: 1
   :widths: 25 40 35

   * - Scenario
     - Exercise
     - Required observation
   * - Owned and borrowed values
     - Read nested values, release native parents, keep public snapshots
     - Snapshots remain valid; no double release or dangling views
   * - Repeated success
     - Warm up, then repeat build/sign/inspect and discard results
     - Live native allocations and callback registrations return to baseline
   * - Partial construction
     - Fail allocation or conversion after earlier allocations succeeded
     - Every acquired resource is released; no partial result escapes
   * - Callback failure
     - Provider, signer, and redeemer raise/return errors; also fail output marshaling
     - Error reaches the caller; temporary outputs and registrations are released
   * - Callback retention
     - Collect host garbage between registration and invocation, then release owner
     - Callback remains callable while needed and becomes collectible afterward
   * - Cancellation
     - Cancel during a suspended provider or signer operation
     - No early free; cleanup occurs when native execution has safely ended
   * - Repeated balancing
     - Invoke a deferred redeemer with identical drafts and multiple iterations
     - Identical data; no per-iteration accumulation or premature bridge removal
   * - Reuse and concurrency
     - Rebuild/retry according to the public contract; overlap supported operations
     - No stale error, shared mutable context, or cross-operation callback delivery

Use ASan/LSan or Valgrind for native accesses and leaks, and runtime-specific
checks for host references and callback tables. For WASM, measure live allocator
usage after warm-up; linear-memory capacity and process RSS are not live-allocation
counts. Document intentional caches and test bounded behavior. An output-only
fixture pass must not be advertised as lifecycle conformance.

Release checklist
-----------------

* Pin the native release, fixture revision, compiler/runtime versions, and native
  artifact checksums. Test installed packages on every supported platform.
* Run the shared fixtures through the public API and the lifecycle scenarios.
  Report unsupported features explicitly; do not silently skip them.
* Verify symbols and callback layouts against the shipped binary, including
  32-bit targets, signed/unsigned boundaries, and optional exports.
* Document installation, ordinary transaction workflows, errors, cancellation,
  supported eras, and thread-safety with examples in the target language.
* Review API ergonomics with a maintainer experienced in that language. Existing
  bindings may migrate incrementally; this guide is not a claim that they already
  meet every requirement.

Implementation references
-------------------------

* `Cometa.js builder lifetime and deferred-redeemer bridge
  <https://github.com/Biglup/cometa.js/blob/v2.0.0-rc.3/src/txBuilder/TransactionBuilderBase.ts>`_.
* `Cometa.py provider callback retention and error translation
  <https://github.com/Biglup/cometa.py/blob/v2.0.0rc3/src/cometa/providers/python_provider_adapter.py>`_.
* `Cometa.py deferred-redeemer bridge
  <https://github.com/Biglup/cometa.py/blob/v2.0.0rc3/src/cometa/transaction_builder/transaction_builder_base.py>`_.

These references explain existing bridge mechanics. The design and lifecycle
requirements above apply independently of the cleanup strategy used by those
particular releases.
