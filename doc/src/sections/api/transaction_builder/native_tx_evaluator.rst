Native Transaction Evaluator
============================

A transaction evaluator backed by the in-process Untyped Plutus Core virtual
machine. It resolves each redeemer's script and datum, builds the
version-appropriate Plutus script context, evaluates the program on the CEK
machine against the supplied cost model, and writes the consumed execution
units back into the redeemer. It implements the standard transaction evaluator
interface, so it is used exactly like the provider-backed evaluator and requires
no network access.

.. doxygenfunction:: cardano_tx_evaluator_new_native
