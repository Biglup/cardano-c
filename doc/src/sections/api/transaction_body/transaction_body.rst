Transaction Body
==========================

.. doxygentypedef:: cardano_transaction_body_t

------------

.. doxygenfunction:: cardano_transaction_body_new

------------

.. doxygenfunction:: cardano_transaction_body_from_cbor

------------

.. doxygenfunction:: cardano_transaction_body_to_cbor

------------

.. doxygenfunction:: cardano_transaction_body_to_cip116_json

Dijkstra JSON fields
--------------------

The transaction body serializer includes Dijkstra fields when present. These
fields extend the JSON representation using CIP-116 conventions; they are not
part of the published Babbage schema.

* ``sub_transactions`` is an array of nested transactions. Each entry contains
  ``body`` and ``witness_set``, plus ``auxiliary_data`` when present. Nested
  transactions do not contain a top-level ``is_valid`` flag.
* ``required_top_level_guards`` is an array of ``key``/``value`` objects. The key
  is a credential tagged ``pubkey_hash`` or ``script_hash``; the value is Plutus
  data, or ``null`` when no data is attached.
* ``direct_deposits`` is an array of ``key``/``value`` objects mapping Bech32
  reward addresses to lovelace amounts encoded as decimal strings.
* ``account_balance_intervals`` and ``starting_account_balance_intervals`` map
  Bech32 reward addresses to balance intervals, using the same ``key``/``value``
  array representation. See :doc:`account_balance_interval` for interval forms.
* ``guards`` contains the full credential array when a script guard is present.
  Key guards also remain in ``required_signers``. Bodies containing only key
  guards retain the existing ``required_signers`` representation.

Sub-transaction bodies use ``guards`` for their credential array and serialize
only their own fields; they do not add a fee, collateral, or nested
sub-transactions. Lovelace amounts are decimal strings to preserve unsigned
64-bit precision.


------------

.. doxygenfunction:: cardano_transaction_body_get_inputs

------------

.. doxygenfunction:: cardano_transaction_body_set_inputs

------------

.. doxygenfunction:: cardano_transaction_body_get_outputs

------------

.. doxygenfunction:: cardano_transaction_body_set_outputs

------------

.. doxygenfunction:: cardano_transaction_body_get_fee

------------

.. doxygenfunction:: cardano_transaction_body_set_fee

------------

.. doxygenfunction:: cardano_transaction_body_get_invalid_after

------------

.. doxygenfunction:: cardano_transaction_body_set_invalid_after

------------

.. doxygenfunction:: cardano_transaction_body_get_certificates

------------

.. doxygenfunction:: cardano_transaction_body_set_certificates

------------

.. doxygenfunction:: cardano_transaction_body_get_withdrawals

------------

.. doxygenfunction:: cardano_transaction_body_set_withdrawals

------------

.. doxygenfunction:: cardano_transaction_body_get_update

------------

.. doxygenfunction:: cardano_transaction_body_set_update

------------

.. doxygenfunction:: cardano_transaction_body_get_aux_data_hash

------------

.. doxygenfunction:: cardano_transaction_body_set_aux_data_hash

------------

.. doxygenfunction:: cardano_transaction_body_get_invalid_before

------------

.. doxygenfunction:: cardano_transaction_body_set_invalid_before

------------

.. doxygenfunction:: cardano_transaction_body_get_mint

------------

.. doxygenfunction:: cardano_transaction_body_set_mint

------------

.. doxygenfunction:: cardano_transaction_body_get_script_data_hash

------------

.. doxygenfunction:: cardano_transaction_body_set_script_data_hash

------------

.. doxygenfunction:: cardano_transaction_body_get_collateral

------------

.. doxygenfunction:: cardano_transaction_body_set_collateral

------------

.. doxygenfunction:: cardano_transaction_body_get_required_signers

------------

.. doxygenfunction:: cardano_transaction_body_set_required_signers

------------

.. doxygenfunction:: cardano_transaction_body_get_guards

------------

.. doxygenfunction:: cardano_transaction_body_set_guards

------------

.. doxygenfunction:: cardano_transaction_body_get_network_id

------------

.. doxygenfunction:: cardano_transaction_body_set_network_id

------------

.. doxygenfunction:: cardano_transaction_body_get_collateral_return

------------

.. doxygenfunction:: cardano_transaction_body_set_collateral_return

------------

.. doxygenfunction:: cardano_transaction_body_get_total_collateral

------------

.. doxygenfunction:: cardano_transaction_body_set_total_collateral

------------

.. doxygenfunction:: cardano_transaction_body_get_reference_inputs

------------

.. doxygenfunction:: cardano_transaction_body_set_reference_inputs

------------

.. doxygenfunction:: cardano_transaction_body_get_voting_procedures

------------

.. doxygenfunction:: cardano_transaction_body_set_voting_procedures

------------

.. doxygenfunction:: cardano_transaction_body_get_proposal_procedures

------------

.. doxygenfunction:: cardano_transaction_body_set_proposal_procedure

------------

.. doxygenfunction:: cardano_transaction_body_get_treasury_value

------------

.. doxygenfunction:: cardano_transaction_body_set_treasury_value

------------

.. doxygenfunction:: cardano_transaction_body_get_donation

------------

.. doxygenfunction:: cardano_transaction_body_set_donation

------------

.. doxygenfunction:: cardano_transaction_body_get_sub_transactions

------------

.. doxygenfunction:: cardano_transaction_body_set_sub_transactions

------------

.. doxygenfunction:: cardano_transaction_body_get_required_top_level_guards

------------

.. doxygenfunction:: cardano_transaction_body_set_required_top_level_guards

------------

.. doxygenfunction:: cardano_transaction_body_get_direct_deposits

------------

.. doxygenfunction:: cardano_transaction_body_set_direct_deposits

------------

.. doxygenfunction:: cardano_transaction_body_get_account_balance_intervals

------------

.. doxygenfunction:: cardano_transaction_body_set_account_balance_intervals

------------

.. doxygenfunction:: cardano_transaction_body_get_starting_account_balance_intervals

------------

.. doxygenfunction:: cardano_transaction_body_set_starting_account_balance_intervals

------------

.. doxygenfunction:: cardano_transaction_body_get_hash

------------

.. doxygenfunction:: cardano_transaction_body_has_tagged_sets

------------

.. doxygenfunction:: cardano_transaction_body_clear_cbor_cache

------------

.. doxygenfunction:: cardano_transaction_body_unref

------------

.. doxygenfunction:: cardano_transaction_body_ref

------------

.. doxygenfunction:: cardano_transaction_body_refcount

------------

.. doxygenfunction:: cardano_transaction_body_set_last_error

------------

.. doxygenfunction:: cardano_transaction_body_get_last_error
