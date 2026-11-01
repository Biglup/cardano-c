Account Balance Interval
==========================

.. doxygentypedef:: cardano_account_balance_interval_t

------------

.. doxygenfunction:: cardano_account_balance_interval_new

------------

.. doxygenfunction:: cardano_account_balance_interval_new_exact

------------

.. doxygenfunction:: cardano_account_balance_interval_from_cbor

------------

.. doxygenfunction:: cardano_account_balance_interval_to_cbor

------------

.. doxygenfunction:: cardano_account_balance_interval_to_cip116_json

JSON representation
-------------------

An exact balance uses the ``exact`` tag and a decimal-string lovelace amount:

.. code-block:: json

   {"tag": "exact", "value": "12000000"}

A range has an inclusive lower bound and an exclusive upper bound. Bounds are
also decimal strings; ``null`` denotes an unbounded endpoint:

.. code-block:: json

   {"tag": "range", "inclusive_lower_bound": "12000000", "exclusive_upper_bound": null}

.. code-block:: json

   {"tag": "range", "inclusive_lower_bound": null, "exclusive_upper_bound": "20000000"}


------------

.. doxygenfunction:: cardano_account_balance_interval_get_inclusive_lower_bound

------------

.. doxygenfunction:: cardano_account_balance_interval_get_exclusive_upper_bound

------------

.. doxygenfunction:: cardano_account_balance_interval_get_exact_balance

------------

.. doxygenfunction:: cardano_account_balance_interval_is_exact

------------

.. doxygenfunction:: cardano_account_balance_interval_unref

------------

.. doxygenfunction:: cardano_account_balance_interval_ref

------------

.. doxygenfunction:: cardano_account_balance_interval_refcount

------------

.. doxygenfunction:: cardano_account_balance_interval_set_last_error

------------

.. doxygenfunction:: cardano_account_balance_interval_get_last_error
