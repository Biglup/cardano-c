Next (V1.2.3)
---------------------

- Added `cardano_random_improve_coin_selector_new` (and `_new_with_seed` for deterministic behavior): a Round-Robin Random-Improve coin selector ported from cardano-wallet's coin selection algorithm. It selects UTxOs at random per required asset in round-robin order, improving each selection toward twice the minimum, and generates one min-ADA compliant change output per user-specified output, with change sized to mimic the user's payments. The selector RNG is re-seeded on every selection so that repeated invocations from the balancing loop are reproducible.
- **BREAKING**: `cardano_coin_selector_select` and the `cardano_coin_select_func_t` callback now receive all selection inputs through a single `cardano_coin_selection_request_t` structure (pre-selected UTxOs, available UTxOs, the authoritative target, an optional `outputs_to_cover` shape hint for change generation, the change address and the protocol parameters). Future request fields will be appended with zero-value defaults, so this is intended to be the last signature change to the selector interface.
- **BREAKING**: Refactored coin selection to support multiple change outputs (in preparation for Round-Robin Random-Improve). `cardano_coin_selector_select` and the `cardano_coin_select_func_t` callback now take the change address and the protocol parameters, and return the change outputs explicitly through a new `cardano_transaction_output_list_t** change_outputs` out parameter. Selectors are now responsible for producing locally balanced (`sum(selection) = target + sum(change_outputs)`), min-ADA compliant change outputs; the balancer keeps full responsibility for fee convergence, implicit coin accounting and the final transaction balance check. The min-ADA padding retry loop was removed from the balancer accordingly.
- Added `cardano_random_improve_coin_selector_new_with_options` and `cardano_selection_strategy_t`: the random improve selector can now use the "minimal" selection strategy (select just enough of each asset) in addition to the default "optimal" strategy (improve toward twice the minimum).
- Coin selectors now split change outputs whose assets would exceed the protocol's maximum output value size (`max_value_size`): oversized change bundles are recursively halved until every change output fits, for both the large-first and the random-improve selectors. A `max_value_size` of zero disables the check.
- Added property-based tests for coin selection (ported from the cardano-js-sdk input-selection property tests): generative scenarios validate coverage, local balance, min-ADA compliance of change outputs, UTxO conservation and honest failure reporting against an independent integer-arithmetic oracle.
- Fixed `cardano_utxo_list_clone` returning `NULL` for an empty list, which caused coin selection to report `CARDANO_ERROR_MEMORY_ALLOCATION_FAILED` instead of `CARDANO_ERROR_BALANCE_INSUFFICIENT` when the available UTxO list was empty.

V1.2.2
---------------------

- Added a native, in-process Untyped Plutus Core (UPLC) virtual machine: flat decoding and encoding, the CEK evaluation machine, the full V1 through V4 builtin set, ledger cost-model and execution-budget accounting, and phase-2 script-context construction.
- Added `cardano_tx_evaluator_new_native`, a transaction evaluator backed by the in-process VM. The transaction builder now defaults to it, so transactions that interact with Plutus scripts are evaluated locally without a provider round-trip; an evaluator set with `cardano_tx_builder_set_tx_evaluator` still takes precedence.
- Added `cardano_uplc_apply_params_to_script` to apply parameters to a parameterized Plutus script.

V1.2.0
---------------------

- cardano_tx_builder_new no longer takes a provider and instead take a cardano_slot_config_t to configure slot settings.
- cardano_tx_builder no properly bubbles up errors from the coin selection process.

V1.1.13
---------------------

- Fixes a bug on the JSON writer that was causing a segfault when calling `cardano_json_writer_encode_in_buffer` on an empty writer.
- `cardano_json_object_t` now properly initializes `last_error` as an empty string when created with `cardano_json_object_parse`.
- Fixed a bug in the transaction builder that was preventing two mint policies with the same script hash from being added to the transaction.

V1.1.12
---------------------

- Added support for CIP-08 message signing.

V1.1.11
---------------------

- Added support for CIP-116 to serialize domain objects to JSON.

V1.1.10
---------------------

- Fixed several bugs with redeemer indices calculation while building the transaction.

V1.1.9
---------------------

- Fix a bug in _cardano_set_collateral_output that was causing the collateral output to sometimes be set when not needed.

V1.1.8
---------------------

- Expose cardano_transaction_get_unique_signers to the public API

V1.1.7
---------------------

- Add donate to treasury function to tx builder.
- Increase url max size on pool metadata from 64 to 128 bytes.

V1.1.6
---------------------

- Pin Emscripten to version 3.1.49.

V1.1.5
---------------------

- Added emscripten coin selector.
- Added emscripten tx evaluator.
- Fixed a bug in the transaction balancer that was setting wrong redeemer index sometimes.

V1.1.4
---------------------
- Added emscripten blockchain provider.

V1.1.3
---------------------
- Switched to using MODULARIZE=1 for the Emscripten build process. The library is now initialized via a factory function.

V1.1.2
---------------------
- EMSCRIPTEN: Wasm file is now embedded in the cardano-c.js file.

V1.1.1
---------------------
- Adjusted exports for Emscripten builds.
- Added a WASM release target.

V1.1.0
---------------------
- Added `cardano_json_writer_t` to write JSON to a buffer.
- Added `cardano_json_object_t` to parse JSON from a strings.
- Added new fuzzing targets for JSON parsing.
- Removed JSON-C dependency.

V1.0.0
---------------------
- First release of cardano-c
- Address Parsing & Generation
- Ed25519 Cryptography
- Transaction Serialization & Deserialization
- Powerful Transaction Builder
- Full Conway era support, including registering DReps, voting on proposals, and submitting proposals.