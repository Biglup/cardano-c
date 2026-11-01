# Binding conformance corpus v1

This corpus contains **229 cases** in six suites:

| Suite | Cases | Coverage |
| --- | ---: | --- |
| Transactions | 3 | Original Conway key-guard and Dijkstra account/nested-signature regression vectors |
| Serialization | 114 | Transaction and subtransaction forms, outputs, values, credentials, certificates, governance proposals/votes, native and Plutus scripts, Plutus data, and balance intervals |
| Invalid inputs | 45 | Wrong types, truncated CBOR, malformed proposals, intervals, and subtransaction envelopes |
| Primitives | 45 | Ed25519 signing and positive/negative verification, BIP32 derivation, Blake2b, Shelley address credential/network combinations, and script parameter application |
| Builders | 14 | Payments, account deposits/transfers/intervals, signed subtransactions and batching, native guard minting, deferred redeemers, script guards, and failure paths |
| Lifecycle | 8 | Runtime-specific ownership, callback, partial-construction, cancellation, and cleanup obligations |

See [FORMAT.md](FORMAT.md) for the request/result protocol, exact field semantics,
builder operations, lifecycle instrumentation requirements, and unit-test porting
instructions. The [binding guide](../../../../../BINDINGS.md) defines the public
API and native memory model.

## Run

The native reference checks are part of the normal test target:

```sh
/path/to/test-cardano-c --gtest_filter='binding_*.*'
```

A binding supplies an executable JSON-lines adapter against its public API:

```sh
python3 scripts/check-binding-conformance.py --report conformance.json -- /path/to/adapter
```

The runner sends inputs, never expectations. It verifies every response against
the corpus and fails on mismatches or omitted cases. Partial development runs can
select `--suite builders`; their reports explicitly say they are partial.

The native tests execute the 221 deterministic cases and additional ownership and
repeated-failure probes. The eight lifecycle requests must be implemented and
instrumented inside each target runtime; native tests are not evidence that a
binding retains callbacks correctly through that runtime's GC or cancellation.

## Provenance and scope

Expected results target cardano-c **2.0.0-rc.6**, release commit
`e321e98bbc7f4e1f2d9177e6bafe3d884f5eb00e`, except builder fees and change,
which follow the exact ledger minimum fee introduced after 2.0.0-rc.6. Pin the
containing corpus commit when consuming it. The native version belongs in this
provenance, not in the binding architecture guide.

Serialization cases are promoted from native unit-test vectors, identified by
their `source` fields, plus explicit integer boundary cases. Builder expectations
were produced by directly invoking the native C API with the declared inputs.
The C++ reference adapter independently rebuilds them from those same recipes;
it does not feed expected CBOR back into a builder. Parameter-application and
cryptographic expectations use the native API; Blake2b digests and transaction
body hashes are additionally checked with Python's independent implementation.

The original three transaction files retain their compact format:
`transaction_cbor`, `body_cbor`, `transaction_id`, `signing_seed`,
`verification_key`, `signature`, and `inspection`, with `schema_version`, `id`,
and `native_version` metadata. The runner normalizes them to the common `cbor`
operation. Other files contain `cases` arrays. `manifest.json` enumerates every
file and expected case count; expectation changes require review and must never
be generated automatically during a binding's tests.

All keys and seeds are public test data. Never use them for funds. Inputs and
parameters are synthetic; these are not network-submit fixtures or evidence of
hardfork availability. Some cases intentionally test historically supported
encodings or rejected input. Original nested witnesses must be preserved; a
standalone transaction test signature signs the outer body and does not replace
those witnesses.

The suite is a broad binding acceptance baseline, not an exhaustive enumeration
of every native function, script program, network provider, or ledger rule.
Network integration and full provider/signer behavior require ported unit tests
and separate end-to-end tests. Add cases as APIs and supported features grow.
