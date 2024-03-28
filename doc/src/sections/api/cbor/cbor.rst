CBOR
==========================

Concise Binary Object Representation (`CBOR`_) is a binary data serialization format that aims for compactness and efficiency. CBOR's design is particularly suited for applications where bandwidth or storage efficiency is crucial, making it a preferred choice within the Cardano ecosystem for serializing domain objects and facilitating seamless data interchange.

This section of the documentation provides detailed information on each function offered by this library to both generate and parse `CBOR`_ encoded data:

.. toctree::
    :maxdepth: 1

    ./cbor_writer
    ./cbor_reader
    ./cbor_major_type
    ./cbor_reader_state
    ./cbor_simple_value
    ./cbor_tag

.. _CBOR: https://www.rfc-editor.org/info/std94