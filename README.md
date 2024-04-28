<p align="center">
  <img align="middle" src=
  "assets/cardano-c-logo-small.png"
  height="160" />
</p>

# Cardano-C

![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/unit-test.yml/badge.svg)
![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/static-code-analysis.yml/badge.svg)
[![codecov](https://codecov.io/gh/Biglup/cardano-c/graph/badge.svg?token=A5U3U5KGG7)](https://codecov.io/gh/Biglup/cardano-c)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/36ac650a4b694421bf6780a110e2f65a)](https://app.codacy.com/gh/Biglup/cardano-c/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![Documentation Status](https://readthedocs.org/projects/cardano-c/badge/?version=latest)](https://cardano-c.readthedocs.io/en/latest/?badge=latest)
[![Static Badge](https://img.shields.io/badge/Funded_By-Project_Catalyst-133ff0?logo=cardano&logoColor=ffffff)](https://projectcatalyst.io/)

`Cardano C` is a C library aiming to be a robust, commercial-grade, full-featured Toolkit for building transaction and interacting with the Cardano blockchain. Compliant with MISRA standards, 
it ensures consistent reliability and safety. With its binding-friendly architecture, `Cardano C` aims for broad compatibility, enabling easy integrations across various programming languages. 

Get the latest documentation at [cardano-c.readthedocs.io](https://cardano-c.readthedocs.io/)

> \[!IMPORTANT\]
> This library is in the early stages of development.
> The documentation is a work in progress and may not be complete. We will
> actively work on improving the documentation as we progress.

## Features

- **Address Parsing & Generation:** Mnemonic creation/restoration and address derivation functionalities.
- **Ed25519 Cryptography:** Support for the Ed25519 signature scheme is based on [libsodium](https://github.com/jedisct1/libsodium) a powerful library for encryption, decryption, signatures, password hashing and more..
- **Transaction Serialization & Deserialization:** Convert transactions to and from CBOR format for transmission to the blockchain.
- **Powerful Transaction Builder:** A versatile tool with comprehensive protocol support that allows you to easily create Cardano transactions.

## Memory Management

The Cardano C library uses a simple reference-counting model. The main goal is that the library can be easily integrated
into applications and languages that uses different memory management models (such as garbage collection).

Every object in our library provides functions to increase and decrease its reference count. For instance, you would use `cardano_cbor_writer_ref` to increase and `cardano_cbor_writer_unref` to decrease the reference count for a `cardano_cbor_writer_t` object.

Upon creation through constructors like `cardano_cbor_writer_new`, an object's reference count is initialized to one.
This implies that the caller becomes the sole owner of the newly created reference.

When the reference count drops to zero—typically when the `*_unref` function is invoked by the last entity
holding a reference—the object gets deallocated.

It's crucial to note that **all** getter functions will consistently increment the reference count of the object they return. Thus, it's the caller's responsibility to invoke `*_unref` once they're done using the result.

## Set up the Git hooks custom directory

After cloning the repository run the following command in the
repository root:

```shell
git config core.hooksPath .githooks
```

## Clang format integration

Repository comes with always-up-to-date `.clang-format` file, an input configuration
for `clang-format` tool (version 15.x is a minimum). 

- https://apt.llvm.org/

## Unit Tests

This project uses Google test framework for writing unit tests.

Start by installing the gtest development package:

```bash
sudo apt-get install libgtest-dev
```

Note that this package only install source files. You have to compile the code yourself to create the necessary
library files.

These source files should be located at /usr/src/googletest.

```bash
cd $(mktemp -d)
cmake /usr/src/googletest
make
sudo make install
```

## Documentation

To generate the documentation for this project, we utilize Doxygen for source code processing, Breathe to integrate Doxygen content with Sphinx, and Sphinx to compile the entire documentation into a web-friendly format.

### Setting Up Your Environment

1. **Install Python 3 and pip**: Required for Sphinx and Breathe. Most Linux distributions include Python 3 by default. For Windows, download from the [Python website](https://www.python.org/downloads/), ensuring you add Python to your PATH.

   Linux example (Ubuntu):
   ```bash
   sudo apt install python3 python3-pip
   ```

2. **Install Sphinx and Breathe**: These Python packages are installed via pip. Breathe bridges Doxygen-generated XML with Sphinx for seamless documentation integration. Additionally, the Sphinx Immaterial theme is installed for a modern web design.
   ```bash
   pip install sphinx sphinx-immaterial breathe
   ```

3. **Install Doxygen**: Necessary for generating XML files from source code. Install via your Linux package manager or download from the [Doxygen website](https://www.doxygen.nl/download.html) for Windows.

   Linux example (Ubuntu):
   ```bash
   sudo apt-get install doxygen
   ```

### Generating Documentation

After setting up your environment, generate the project's documentation by navigating to the repository root and executing:

```bash
cmake -DDOXYGEN_ENABLED=ON .
make doc
```

The generated documentation will be available at `build/release/doc/html/index.html`, providing a comprehensive guide to the project's API and architecture.

## Contributing

We welcome contributions from the community. Please read our CONTRIBUTING.md for guidelines.

## License 

[APACHE LICENSE, VERSION 2.0](https://apache.org/licenses/LICENSE-2.0)
