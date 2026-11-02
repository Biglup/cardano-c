Metadatum
==========================

Plain JSON conversion preserves the complete text value, including embedded
null characters. Text and byte-string limits are checked when encoding CBOR;
JSON input is not silently shortened to fit those limits.

``cardano_metadatum_to_json`` writes integer values as exact decimal JSON numbers.
It requires text map keys and rejects byte-string values. CIP-116 inspection
output uses the tagged representation and supports every metadatum kind.

.. doxygentypedef:: cardano_metadatum_t

------------

.. doxygenfunction:: cardano_metadatum_new_map

------------

.. doxygenfunction:: cardano_metadatum_new_list

------------

.. doxygenfunction:: cardano_metadatum_new_integer

------------

.. doxygenfunction:: cardano_metadatum_new_integer_from_int

------------

.. doxygenfunction:: cardano_metadatum_new_integer_from_uint

------------

.. doxygenfunction:: cardano_metadatum_new_integer_from_string

------------

.. doxygenfunction:: cardano_metadatum_new_bytes

------------

.. doxygenfunction:: cardano_metadatum_new_bytes_from_hex

------------

.. doxygenfunction:: cardano_metadatum_new_string

------------

.. doxygenfunction:: cardano_metadatum_from_cbor

------------

.. doxygenfunction:: cardano_metadatum_from_json

------------

.. doxygenfunction:: cardano_metadatum_get_json_size

------------

.. doxygenfunction:: cardano_metadatum_to_json

------------

.. doxygenfunction:: cardano_metadatum_to_cip116_json

------------

.. doxygenfunction:: cardano_metadatum_to_cbor

------------

.. doxygenfunction:: cardano_metadatum_get_kind

------------

.. doxygenfunction:: cardano_metadatum_to_map

------------

.. doxygenfunction:: cardano_metadatum_to_list

------------

.. doxygenfunction:: cardano_metadatum_to_integer

------------

.. doxygenfunction:: cardano_metadatum_to_bounded_bytes

------------

.. doxygenfunction:: cardano_metadatum_get_string_size

------------

.. doxygenfunction:: cardano_metadatum_to_string

------------

.. doxygenfunction:: cardano_metadatum_equals

------------

.. doxygenfunction:: cardano_metadatum_unref

------------

.. doxygenfunction:: cardano_metadatum_ref

------------

.. doxygenfunction:: cardano_metadatum_refcount

------------

.. doxygenfunction:: cardano_metadatum_set_last_error

------------

.. doxygenfunction:: cardano_metadatum_get_last_error

------------

.. doxygenfunction:: cardano_metadatum_deep_clone
