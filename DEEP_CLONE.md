# Deep copying native objects

Use `cardano_<type>_deep_clone` to obtain an independent, owned copy of a native
value. The implementation copies fields and owned children directly; it does not
serialize and parse the value.

```c
cardano_transaction_t* copy = NULL;
cardano_error_t result = cardano_transaction_deep_clone(transaction, &copy);

if (result == CARDANO_SUCCESS)
{
  /* Use or modify copy independently of transaction. */
  cardano_transaction_unref(&copy);
}
```

## Contract

* A successful clone owns one reference. Release it with the type's `*_unref`.
* The source is unchanged. Mutable children, collections, buffers, keys, and
  caches belong to the copy; they are not retained from the source.
* Values, optional-field presence, collection order, encoding flags, cached CBOR,
  transaction framing, and stored hashes are preserved. Cloning does not
  canonicalize, validate, repair, or rebalance the source.
* A copy of a serializable value produces the same CBOR as its source before
  either is modified. Transaction IDs and signatures therefore remain unchanged.
* Copying does not require the object to be ready for serialization. Reader and
  writer copies preserve their positions and nested processing state.
* The output pointer is set to `NULL` on failure, provided the output argument
  itself is non-null. Partial copies are released. Do not pass an output slot
  that already owns a reference, including `deep_clone(value, &value)`.
* The reference count and object-local diagnostic message are not copied. A new
  copy starts with a reference count of one and an empty diagnostic message.
* Source graphs must be acyclic. Repeated references within the source may become
  separate copies; preservation of pointer identity or alias relationships is
  not part of the contract.

Mutations still follow the normal native API rules. In particular, explicitly
clear CBOR caches where the existing mutation API requires it. Cloning neither
changes cache-invalidation rules nor makes existing signatures valid after the
signed body changes.

Source access must be synchronized with mutation and destruction. Deep cloning
does not make cardano-c's reference counting atomic or establish thread safety.
Private-key clones duplicate secret material and use the same destruction rules
as keys constructed through the ordinary APIs.

## Supported types

All public reference-counted domain values, typed collections, addresses,
cryptographic values, buffers, and JSON/CBOR readers and writers provide a
`deep_clone` function. The individual API reference pages document their signatures.

These types intentionally do not provide deep cloning:

| Type | Reason |
| --- | --- |
| `cardano_object_t` | The ownership base has no concrete-type copy contract. |
| Transaction and subtransaction builders | Construction sessions include callbacks and external services. |
| Deferred redeemer lists | Callback contexts are borrowed and their duplication contract is unknown. |
| Providers, evaluators, and coin selectors | Implementations may own external resources or opaque user contexts. |
| Secure key handlers | Implementations may own hardware sessions or opaque user contexts. |

Enums and pointer-free public structs already support ordinary C value copying.
Arena-owned UPLC nodes and borrowed views follow their owner's lifetime contract;
this API does not introduce independently owned copies of arena allocations.
Internal generic collections require a typed element copier rather than guessing
how to duplicate arbitrary user objects.

Existing `*_clone` APIs retain their documented behavior. For example,
`cardano_utxo_list_clone` remains shallow; use `cardano_utxo_list_deep_clone` for
independent UTxOs and their children.

## Correctness checks

Run the copy tests:

```sh
# From the build directory containing test-cardano-c:
./test-cardano-c --gtest_filter='*_deep_clone.*'
```

The tests cover:

* Null arguments, fresh ownership, and independent source/copy destruction.
* Existing native CBOR fixtures, including nonminimal encodings, ordered witnesses,
  Dijkstra fields, unknown cost-model language IDs, and numeric boundaries.
* Nested mutation and distinct child pointers, so shallow retention cannot satisfy
  the copy contract merely by returning identical bytes.
* Keys and other values without CBOR interfaces, JSON text caches, buffer read
  positions, and nested reader/writer state.
* Failure at each cardano-c allocator call in representative populated instances,
  including cleanup of partial children and preservation of the source.

Run the suite under Valgrind or sanitizers to check invalid accesses and leaks.
Allocation fault injection covers cardano-c's allocator hooks. The existing
mini-GMP backend uses its own allocator and aborts on internal allocation failure;
these tests do not claim to turn that backend behavior into a recoverable error.

When adding fields or variants, add regression tests with those values populated.
Tests should check observable behavior independently of the copy implementation,
including mutation isolation and cleanup on allocation failure.
