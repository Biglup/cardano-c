Next (V1.2.3)
---------------------

- Added the Dijkstra era (protocol version 12) serialization layer: CIP-118 nested transactions (`cardano_sub_transaction_body_t`, `cardano_sub_transaction_t`, `cardano_sub_transaction_set_t` and transaction body key 23), CIP-112 guards (`cardano_guard_set_t` at body key 14 with dual wire form, `cardano_required_guards_map_t` at key 24 on both body levels, the `RequireGuard` native script kind 6 and the `guarding` redeemer tag), direct deposits (key 25), account balance intervals (key 26), PlutusV4 serialization plumbing (script union tag 4, hash prefix 0x04, cost models key 3, auxiliary data key 5; no witness set entry exists in this era) and the four reference script protocol parameters (keys 34-37). The library stays era-less: decoders remain permissive for historical forms and encoders keep producing Conway-valid bytes unless a Dijkstra-only feature is used. Byte-exact round trips are pinned against the cardano-ledger golden vectors.
- Account balance intervals now support the exact balance form of the Dijkstra CDDL: `cardano_account_balance_interval_new_exact`, `cardano_account_balance_interval_get_exact_balance` and `cardano_account_balance_interval_is_exact` model an interval that requires an account balance to be exactly N lovelace, encoded on the wire as a bare unsigned integer instead of the two element bounds array. Range intervals keep their existing encoding.
- Added the Dijkstra protocol parameters 38 to 48 to `cardano_protocol_parameters_t` with getters and setters: `max_pledge_leverage` (CIP-50, nullable; `NULL` means unbounded and is the default), `min_pool_margin` (CIP-23) and the nine Leios parameters (CIP-164): `leios_announcement_period_length`, `leios_vote_period_length`, `leios_diffusion_period_length`, `leios_committee_size`, `leios_quorum_stake_threshold`, `max_endorser_block_references_size`, `max_endorser_block_txs_size`, `max_endorser_block_execution_units` and `max_ref_script_size_per_endorser_block`. Integer parameters are bounded to their wire width on set (16 bits for the committee size, 32 bits otherwise); a fresh parameter set defaults the new rationals to zero and the new execution units to (0, 0).
- Added `cardano_bls_key_t`, the optional BLS key (a 96 byte public key plus a 48 byte proof of possession) that Dijkstra stake pool registrations may carry for Leios voting. Pool parameters expose it through `cardano_pool_params_has_bls_key`, `cardano_pool_params_get_bls_key` and `cardano_pool_params_set_bls_key`, the pool registration certificate decodes both the 10 element (legacy) and the 11 element (with key) forms, and it keeps emitting the legacy form unless a key is set. The CIP-116 JSON of pool parameters gains a `bls_key` object only when a key is present.
- Account balance intervals maps are keyed by reward address (the serialized reward account bytes) as in the Dijkstra CDDL: `cardano_account_balance_intervals_map_t` maps each reward account to its interval. `get`, `insert`, `get_key_at` and `get_key_value_at` take and return `cardano_reward_address_t`, `get_keys` returns a `cardano_reward_address_list_t` in insertion order, and the new `cardano_account_balance_intervals_map_insert_ex` accepts a Bech32 reward address. The vendored Dijkstra golden transaction vector was re-pinned to the ledger commit that introduced this form.
- `cardano_protocol_param_update_t` now decodes, stores and re-encodes the same parameters 38 to 48 (CBOR keys, CIP-116 JSON with the snake_case names and the changed-parameters Plutus data map) so that Dijkstra parameter change proposals round trip and guardrail scripts see them. The pledge leverage update is nullable: `set_max_pledge_leverage` proposes a bound, `set_max_pledge_leverage_unbounded` proposes lifting it (CBOR null, JSON null, Plutus `Constr 1 []`), `clear_max_pledge_leverage` drops the key and `has_max_pledge_leverage` reports presence; the getter succeeds with a `NULL` interval for an unbounded proposal. Bounded integer parameters are validated to their wire width on decode and on set.
- Added the starting account balance intervals (transaction body key 27) to `cardano_transaction_body_t` through `cardano_transaction_body_get_starting_account_balance_intervals` and `cardano_transaction_body_set_starting_account_balance_intervals`. The field reuses `cardano_account_balance_intervals_map_t` and constrains, per reward account, the balance an account had before the batch started, while key 26 constrains the balance at the point the transaction is applied. It is encoded after key 26, only top level bodies carry it, and a sub transaction body that declares key 27 is rejected like the other top-level-only keys.
- The transaction decoder now preserves the Dijkstra 3-element frame (no `is_valid` flag) on re-encode; a transaction whose `is_valid` flag is false always serializes as the 4-element mempool frame so the flag is never lost. Witness sets preserve their decoded map key order for byte-exact round trips (fresh sets keep encoding canonically).
- **BREAKING (behavioral)**: `cardano_transaction_body_get_required_signers` now returns a derived, caller-owned view over the key-hash members of the body's guards (key 14 stores guards from this release); `set_required_signers` replaces the guards with key-hash credentials. Key-hash-only bodies keep serializing in the legacy Conway form. Multi-signer transactions built with `cardano_tx_builder_add_signer` now encode key 14 in insertion order instead of sorted order (wire-valid, but built transaction bytes can differ from prior releases), and adding a duplicate signer is now a no-op instead of emitting a duplicate entry.
- Wire tightenings per the Dijkstra CDDL: bootstrap witness chain codes must be exactly 32 bytes, protocol version minor is bounded to uint32, redeemer indices are bounded to uint32 on fresh encodes, empty multiasset maps and empty policy maps are never emitted (values collapse to plain coin), and cost model arrays decode in indefinite-length form.
- Added `cardano_random_improve_coin_selector_new` (and `_new_with_seed` for deterministic behavior): a Round-Robin Random-Improve coin selector ported from cardano-wallet's coin selection algorithm. It selects UTxOs at random per required asset in round-robin order, improving each selection toward twice the minimum, and generates one min-ADA compliant change output per user-specified output, with change sized to mimic the user's payments. The selector RNG is re-seeded on every selection so that repeated invocations from the balancing loop are reproducible.
- **BREAKING**: `cardano_coin_selector_select` and the `cardano_coin_select_func_t` callback now receive all selection inputs through a single `cardano_coin_selection_request_t` structure (pre-selected UTxOs, available UTxOs, the authoritative target, an optional `outputs_to_cover` shape hint for change generation, the change address and the protocol parameters). Future request fields will be appended with zero-value defaults, so this is intended to be the last signature change to the selector interface.
- **BREAKING**: Refactored coin selection to support multiple change outputs (in preparation for Round-Robin Random-Improve). `cardano_coin_selector_select` and the `cardano_coin_select_func_t` callback now take the change address and the protocol parameters, and return the change outputs explicitly through a new `cardano_transaction_output_list_t** change_outputs` out parameter. Selectors are now responsible for producing locally balanced (`sum(selection) = target + sum(change_outputs)`), min-ADA compliant change outputs; the balancer keeps full responsibility for fee convergence, implicit coin accounting and the final transaction balance check. The min-ADA padding retry loop was removed from the balancer accordingly.
- Added deferred redeemer callbacks (`cardano_deferred_redeemer_fn_t`): `cardano_tx_builder_add_input_with_deferred_redeemer`, `cardano_tx_builder_mint_token_with_deferred_redeemer`, `cardano_tx_builder_withdraw_rewards_with_deferred_redeemer`, `cardano_tx_builder_add_certificate_with_deferred_redeemer` and `cardano_tx_builder_vote_with_deferred_redeemer` register a callback that builds the redeemer payload from the balanced draft transaction, once the final canonical input order, change outputs and fee are known. Callbacks run on every balancing iteration and must be pure. This enables redeemers that carry script-context indices (witness-driven design patterns), for every redeemer purpose the builder can attach: spends, mints, withdrawals, certificates and votes.
- Added canonical index lookup helpers on transactions: `cardano_transaction_find_input_index`, `find_reference_input_index`, `find_output_index`, `find_mint_policy_index`, `find_withdrawal_index`, `find_certificate_index`, `find_vote_index` and `find_redeemer_index`.
- **BREAKING**: `cardano_balance_transaction` takes an additional (nullable) `cardano_deferred_redeemer_list_t*` parameter.
- Added `cardano_random_improve_coin_selector_new_with_options` and `cardano_selection_strategy_t`: the random improve selector can now use the "minimal" selection strategy (select just enough of each asset) in addition to the default "optimal" strategy (improve toward twice the minimum).
- **BREAKING (behavioral)**: The transaction builder now uses the Round-Robin Random-Improve selector by default instead of the large-first selector. Use `cardano_tx_builder_set_coin_selector` with `cardano_large_first_coin_selector_new` to restore the previous behavior.
- Coin selectors now split change outputs whose assets would exceed the protocol's maximum output value size (`max_value_size`): oversized change bundles are recursively halved until every change output fits, for both the large-first and the random-improve selectors. A `max_value_size` of zero disables the check.
- Added property-based tests for coin selection (ported from the cardano-js-sdk input-selection property tests): generative scenarios validate coverage, local balance, min-ADA compliance of change outputs, UTxO conservation and honest failure reporting against an independent integer-arithmetic oracle.
- Added `cardano_tx_builder_add_guard` and `cardano_tx_builder_add_guard_ex` to the transaction builder: they add an arbitrary CIP-112 guard credential to the transaction body. A key hash guard is a required signer and produces the same transaction as `cardano_tx_builder_add_signer`; a script hash guard requires the ledger to run the script with the guarding redeemer purpose and switches body key 14 to the Dijkstra credential wire form. Duplicate guards are ignored and the builder does not attach the guard script or its redeemer.
- Added transaction imbalance helpers to the balancing module: `cardano_compute_transaction_imbalance` and `cardano_compute_sub_transaction_imbalance` return the value a transaction or a CIP-118 sub transaction consumes minus the value it produces (resolved inputs, withdrawals, deposit refunds and minted assets against outputs, fee, deposits, donation, direct deposits and burned assets) as a signed `cardano_value_t`, so that parties can hand-assemble sub transactions and batchers can match their intents. `cardano_compute_sub_transaction_implicit_coin` reports the withdrawals, deposits and refunds of a sub transaction body. `cardano_is_transaction_balanced` is now defined as a zero imbalance and therefore accounts for direct deposits (body key 25) as produced value.
- Added direct deposit and account balance interval setters to the transaction builder: `cardano_tx_builder_add_direct_deposit` and `cardano_tx_builder_add_direct_deposit_ex` pay lovelace straight into a reward account without creating a UTxO (body key 25), `cardano_tx_builder_add_account_balance_interval` and `cardano_tx_builder_add_account_balance_interval_ex` make the transaction valid only while a reward account balance sits inside an interval (key 26), and `cardano_tx_builder_add_starting_account_balance_interval` and `cardano_tx_builder_add_starting_account_balance_interval_ex` assert the balance an account had before any sub transaction was applied (key 27). Direct deposits to the same reward account accumulate (an accumulated amount that overflows 64 bits is reported as `CARDANO_ERROR_INTEGER_OVERFLOW` and a zero amount is rejected), while a new interval replaces the previous one of the same account. `cardano_balance_transaction` now funds direct deposits from the selected inputs together with the outputs and the fee. Transactions that do not use these functions keep their exact bytes.
- Added `cardano_compute_transaction_batch_imbalance` to the balancing module: it returns the imbalance of a whole CIP-118 batch, the imbalance of the top level transaction body plus the imbalance of every sub transaction it carries (coin and multi assets), which is the figure the ledger checks for value conservation. The resolved inputs must cover the top level body and every sub transaction. `cardano_is_transaction_balanced` is now defined as a zero batch imbalance, so it gives the whole batch answer for a transaction that carries sub transactions; `cardano_compute_transaction_imbalance` still reports the top level body alone, and results for transactions without sub transactions do not change.
- Added `cardano_sub_tx_builder_t`, a dedicated builder for CIP-118 sub transactions that produces a `cardano_sub_transaction_t` through `cardano_sub_tx_builder_build`. A sub transaction is an intent, so the builder never balances: it performs no coin selection and adds no change, fee or collateral, the result spends exactly the inputs and produces exactly the outputs that were added, and its imbalance (`cardano_compute_sub_transaction_imbalance`) is what a batcher matches against the rest of the batch. Because it is a separate type, the fields that only exist on a top level transaction (fee, collateral, starting account balance intervals) have no functions on it. The core surface mirrors the transaction builder: `cardano_sub_tx_builder_new`, `_ref`, `_unref`, `_refcount`, `_set_last_error`, `_get_last_error`, `_set_network_id`, `_set_donation`, `_set_invalid_after`, `_set_invalid_after_ex`, `_set_invalid_before`, `_set_invalid_before_ex`, `_add_input`, `_add_reference_input`, `_add_output`, `_send_lovelace`, `_send_lovelace_ex`, `_send_value`, `_send_value_ex`, `_set_metadata`, `_set_metadata_ex`, `_mint_token`, `_mint_token_ex`, `_mint_token_with_id`, `_mint_token_with_id_ex`, `_add_script`, `_add_guard`, `_add_guard_ex`, `_add_direct_deposit`, `_add_direct_deposit_ex`, `_add_account_balance_interval` and `_add_account_balance_interval_ex`. `cardano_sub_tx_builder_require_top_level_guard` and `cardano_sub_tx_builder_require_top_level_guard_ex` require a guard credential, with an optional datum, from the top level transaction that carries the sub transaction (body key 24). Plutus scripts can not run inside a sub transaction, so no function takes a redeemer and `cardano_sub_tx_builder_add_script` reports a Plutus script of any version as `CARDANO_ERROR_INVALID_SCRIPT_LANGUAGE` when building; native scripts are accepted.
- Fixed `cardano_utxo_list_clone` returning `NULL` for an empty list, which caused coin selection to report `CARDANO_ERROR_MEMORY_ALLOCATION_FAILED` instead of `CARDANO_ERROR_BALANCE_INSUFFICIENT` when the available UTxO list was empty.
- Fixed `cardano_governance_action_id_new` leaking the new object when the CIP-129 encoding of the id fails under memory pressure, which also affected `cardano_governance_action_id_from_bech32` and the builder proposal functions that take the governance action id as a string.

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