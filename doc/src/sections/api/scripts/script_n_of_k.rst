Native Script - Require M Of N
=====================================

.. doxygentypedef:: cardano_script_n_of_k_t

------------

.. doxygenfunction:: cardano_script_n_of_k_new

------------

.. doxygenfunction:: cardano_script_n_of_k_from_cbor

------------

.. doxygenfunction:: cardano_script_n_of_k_to_cbor

------------

.. doxygenfunction:: cardano_script_n_of_k_to_cip116_json

------------

.. doxygenfunction:: cardano_script_n_of_k_from_json

------------

.. doxygenfunction:: cardano_script_n_of_k_get_length

------------

.. doxygenfunction:: cardano_script_n_of_k_get_required

------------

.. doxygenfunction:: cardano_script_n_of_k_set_required

------------

.. doxygenfunction:: cardano_script_n_of_k_get_scripts

------------

.. doxygenfunction:: cardano_script_n_of_k_set_scripts

------------

.. doxygenfunction:: cardano_script_n_of_k_equals

------------

.. doxygenfunction:: cardano_script_n_of_k_clear_cbor_cache

------------

.. doxygenfunction:: cardano_script_n_of_k_unref

------------

.. doxygenfunction:: cardano_script_n_of_k_ref

------------

.. doxygenfunction:: cardano_script_n_of_k_refcount

------------

.. doxygenfunction:: cardano_script_n_of_k_set_last_error

------------

.. doxygenfunction:: cardano_script_n_of_k_get_last_error

------------

.. doxygenfunction:: cardano_script_n_of_k_deep_clone

Threshold values
----------------

The required count uses ``size_t``. CBOR and JSON decoding reject counts larger
than ``SIZE_MAX`` with ``CARDANO_ERROR_INTEGER_OVERFLOW``. Equality compares
both the required count and the ordered child scripts.
