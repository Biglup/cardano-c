# Binding conformance corpus v1

These public test vectors target cardano-c **2.0.0-rc.6**, release commit
`e321e98bbc7f4e1f2d9177e6bafe3d884f5eb00e`. Pin the containing repository commit
when consuming them. Fixture changes require review alongside native tests;
never refresh expected values automatically during a binding test run.

Each JSON file is an independent transaction case:

| Field | Meaning |
| --- | --- |
| `schema_version` | Integer `1`; reject unsupported schema versions. |
| `id` | Stable case identifier, matching the filename without `.json`. |
| `native_version` | Native release used to produce expected results. |
| `transaction_cbor` | Original complete transaction, lowercase hexadecimal. |
| `body_cbor` | Serialized transaction body, lowercase hexadecimal. |
| `transaction_id` | Blake2b-256 of the body bytes, lowercase hexadecimal. |
| `signing_seed` | Public 32-byte Ed25519 test seed, **never use for funds**. |
| `verification_key` | Derived 32-byte Ed25519 public key, lowercase hexadecimal. |
| `signature` | Deterministic 64-byte Ed25519 signature over the raw 32-byte transaction ID. |
| `inspection` | Native transaction JSON, compared structurally. |

The seed is the byte sequence 0 through 31. The signature does not authorize
spending the fixture inputs and is not attached to the transaction. Inputs are
synthetic; these cases are not intended for submission to a network.

## Coverage and provenance

- `conway-key-guard`: synthetic Conway-format transaction with one input, one
  output, and a key guard. Confirms that RC6 JSON emits `guards`, not
  `required_signers`, even for key-only transactions.
- `dijkstra-account-balances`: promoted from the Cometa RC3 cross-binding
  inspection checks. Covers direct deposits, a lower-only account interval,
  and an exact starting balance of `18446744073709551615`.
- `dijkstra-signed-subtransaction`: promoted from the same checks. Preserves a
  signed nested transaction and its required top-level guard with Plutus integer
  data. The fixture signature signs the outer body; it does not replace the
  nested witnesses.

Expected bodies, IDs, signatures, public keys, and inspection output were
exported through Cometa.py using the official RC6 native library. The native
`binding_conformance` tests independently call the C API against these files.
The body hashes are also cross-checked with Python's Blake2b-256 implementation.
JS/native comparison is useful additional evidence, not an independent ledger
implementation: both bindings ultimately use cardano-c.

## Running and consuming

Build the repository's normal native test target and run:

```sh
/path/to/test-cardano-c --gtest_filter='binding_conformance.*'
```

Downstream suites should load each JSON file, decode and re-encode the
transaction through their public API, and compare body bytes, ID, signature,
verification key, and inspection. Verify the signature, then flip bit 0 of
message byte 0 and require verification to fail. Compare hex as bytes and JSON
as structured data: object order is irrelevant, array order and value types
are significant. Do not convert decimal amount strings through floating point.

These cases preserve original CBOR; they do not require rewriting every legal
CBOR encoding into a single canonical representation. Hash the actual body
bytes. Public-value reconstruction must be tested separately from any cached
CBOR round-trip path.

This initial corpus does not yet contain deterministic balancing recipes,
standalone subtransaction signing vectors, or all script/certificate variants.
Lifecycle and callback conformance requires runtime tests, not additional golden
JSON fields. See the [binding guide](../../../../../doc/src/sections/binding_guide.rst)
for the required ownership rules, callback behavior, and lifecycle scenarios.
