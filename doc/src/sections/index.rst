.. _Cardano-C-Library:

Documentation for version |version|, updated on |today|.

Cardano C
===================

.. raw:: html

  <p align="center">
    <img src="_static/logo.png"><br><br>
  </p>

.. important::

   The development of the Cardano C Library is ongoing, and as such, the documentation is continuously being updated and improved. Some sections may be incomplete or subject to change. We appreciate your patience and welcome contributions to enhance the documentation and library features.

Overview
--------

Cardano C is a library for the Cardano blockchain. Our mission is to provide a solid foundation for building mission-critical blockchain applications, with a steadfast commitment to quality, safety, and cross-platform compatibility.

Main features
~~~~~~~~~~~~~

- Address Parsing & Generation
- Ed25519 Cryptography
- Transaction Serialization & Deserialization
- Powerful Transaction Builder
- Robust C99 implementation
- Layered architecture offers both control and convenience
- Flexible memory management
- No shared global state - threading friendly
- Proper handling of UTF-8
- Extensive documentation and test suite
- It has no runtime dependencies (The library depends on `libsodium`_ and `libgmp`_ but they are all statically linked)

Engineered for Excellence
~~~~~~~~~~~~~~~~~~~~~~~~~

Adhering to the rigorous MISRA C guidelines, the Cardano C Library is designed with strict coding standards to ensure reliability, maintainability, and security. Our development process prioritizes bug-free, leak-free code, underpinned by comprehensive unit tests that guarantee stability and performance. This uncompromising approach to quality makes the Cardano C Library the ideal choice for developers building high-stakes applications, infrastructure projects, and more.

Cross-Platform Compatibility and Language Bindings
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The Cardano C Library is engineered with a flexible memory model to facilitate seamless bindings with multiple programming languages. This ensures that developers can easily integrate Cardano's robust blockchain capabilities into a diverse range of projects and ecosystems, broadening the accessibility and utility of the Cardano blockchain across different platforms and programming languages.

Community-Driven
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Developed by and for the community, the Cardano C Library embodies the spirit of open-source collaboration. By contributing to the library, you're not just building upon a robust technical foundation – you're joining a movement to pioneer new frontiers in blockchain technology.

Start Building Today
~~~~~~~~~~~~~~~~~~~~~~~~

Join us on this exciting journey to unlock the full potential of the Cardano blockchain. With comprehensive guides, detailed API documentation, and practical examples, the Cardano C Library equips you with the tools to create secure, efficient, and scalable blockchain solutions. Dive into the world of Cardano development and become a part of a community dedicated to building a decentralized and prosperous future.

License
~~~~~~~~~~~~~~~~~~~~~~~~

The source code of this project is licensed under the Apache License 2.0. The Apache 2.0 License allows for commercial use, modification, distribution, patent use, and private use.

See `APACHE LICENSE, VERSION 2.0`_

.. toctree::
    :hidden:

    getting_started

.. toctree::
    :caption: API Reference
    :hidden:

    api/address/index
    api/assets/index
    api/auxiliary_data/index
    api/certs/index
    api/common/index
    api/cryptography/index
    api/cbor/index
    api/encoding/index
    api/json/index
    api/key_handlers/index
    api/plutus_data/index
    api/pool_params/index
    api/proposal_procedures/index
    api/protocol_params/index
    api/providers/index
    api/scripts/index
    api/transaction/index
    api/transaction_body/index
    api/transaction_builder/index
    api/voting_procedures/index
    api/witness_set/index
    api/bip39
    api/buffer
    api/error
    api/object
    api/time

.. _libsodium: https://github.com/jedisct1/libsodium
.. _libjsonc: https://github.com/json-c/json-c
.. _libgmp: https://gmplib.org/

.. _APACHE LICENSE, VERSION 2.0: https://apache.org/licenses/LICENSE-2.0