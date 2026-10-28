Native Transaction Evaluator
============================

A transaction evaluator backed by the in-process Untyped Plutus Core virtual
machine. It resolves each redeemer's script and datum, builds the
version-appropriate Plutus script context, evaluates the program on the CEK
machine against the supplied cost model, and writes the consumed execution
units back into the redeemer. It implements the standard transaction evaluator
interface, so it is used exactly like the provider-backed evaluator and requires
no network access.

From protocol version 12 the evaluator also runs Plutus V4 scripts, the
guarding redeemers of a transaction, and the scripts of its sub transactions. A
transaction and its sub transactions are evaluated together, as the ledger
does: every script of every level is deserialised before any of them runs, and
a failure at any level fails the whole evaluation. The evaluator created with
``cardano_tx_evaluator_new_native`` returns the redeemers of the top level
transaction, and the one created with
``cardano_tx_evaluator_new_native_sub_transaction`` returns the redeemers of one
sub transaction.

.. doxygenfunction:: cardano_tx_evaluator_new_native

------------

.. doxygenfunction:: cardano_tx_evaluator_new_native_sub_transaction
