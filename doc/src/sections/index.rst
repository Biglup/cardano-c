.. _Cardano-C-Library:

Documentation for version |version|, updated on |today|.

Cardano C
===================

.. raw:: html

  <p align="center">
    <img src="_static/logo.png"><br><br>
    <sup>The Cardano-C logo is licensed under
    <a href="https://creativecommons.org/licenses/by/4.0/">Creative Commons 4.0 Attributions license</a>.</sup>
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
- No runtime dependencies, small footprint

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

.. toctree::
    :hidden:

    getting_started

.. toctree::
    :caption: API Reference
    :hidden:

    api/cbor/cbor
    api/cryptography/cryptography
    api/buffer
    api/error
    api/object
    api/allocators