<p align="center">
  <img align="middle" src=
  "assets/cardano-c-logo-small.png"
  height="160" /></br></br>
  <sup><sup><sup><sup>The Cardano-C logo is licensed under
  <a href="https://creativecommons.org/licenses/by/3.0/">Creative
  Commons 3.0 Attributions license</a></sup></sup></sup></sup>
</p>

# Cardano-C

![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/unit-test.yml/badge.svg)
![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/static-code-analysis.yml/badge.svg)
[![codecov](https://codecov.io/gh/Biglup/cardano-c/graph/badge.svg?token=A5U3U5KGG7)](https://codecov.io/gh/Biglup/cardano-c)
[![Static Badge](https://img.shields.io/badge/Funded_By-Project_Catalyst-133ff0?logo=cardano&logoColor=ffffff)](https://projectcatalyst.io/)
[![Static Badge](https://img.shields.io/badge/Made_By-Biglup_Labs-pink?&logoColor=bbbbbb&color=815fe4)](https://github.com/Biglup)

"cardano-c" is a C library aiming to be a robust, commercial-grade, full-featured Toolkit for building transaction and interacting with the Cardano blockchain. Compliant with MISRA standards, 
it ensures consistent reliability and safety. With its binding-friendly architecture, "cardano-c" aims for broad compatibility, enabling easy integrations across various programming languages. 

A practical toolkit for developers working with Cardano.

### Features

- **Address Parsing & Generation:** Mnemonic creation/restoration and address derivation functionalities.
- **Ed25519 Cryptography:** Support for the Ed25519 signature scheme is based on [libsodium](https://github.com/jedisct1/libsodium) a powerful library for encryption, decryption, signatures, password hashing and more..
- **Transaction Serialization & Deserialization:** Convert transactions to and from CBOR format for transmission to the blockchain.
- **Powerful Transaction Builder:** A versatile tool with comprehensive protocol support that allows you to easily create Cardano transactions.

### Getting Started

The "cardano-c" library has a single dependency and it links statically with it. The end result
is a library with no dependencies.

#### Compilation of Sodium on Unix-like systems

Sodium is a shared library with a machine-independent set of headers, so it can easily be used by 3rd party projects.

The library is built using Autotools, making it easy to package.

Installation is trivial, and both compilation and testing can take advantage of multiple CPU cores.

Download a [tarball of libsodium](https://download.libsodium.org/libsodium/releases/), preferably the latest stable
version, then follow the ritual:

``````
wget https://download.libsodium.org/libsodium/releases/libsodium-1.0.19.tar.gz -O sodium.tar.gz
mkdir -p sodium
tar -xzf sodium.tar.gz -C sodium
cd ./sodium
./configure CFLAGS=-fPIC CXXFLAGS=-fPIC --enable-shared --with-pic
make && make check
make install
``````

For instructions on how to build and install Sodium on other platforms see: https://doc.libsodium.org/installation
### Memory Management

The Cardano C library uses a simple reference-counting model. The main goal is that the library can be easily integrated
into applications and languages that uses different memory management models (such as garbage collection).

##### Core Concepts

Every object in our library provides functions to increase and decrease its reference count. For instance, you would use cardano_cbor_writer_ref to increase and cardano_cbor_writer_unref to decrease the reference count for a cardano_cbor_writer_t object.

Upon creation through constructors like cardano_cbor_writer_new, an object's reference count is initialized to one. This implies that the caller becomes the sole owner of the newly-created reference.

When the reference count drops to zero—typically when the *_unref function is invoked by the last entity holding a reference—the object gets deallocated.

It's crucial to note that **all** getter functions will consistently increment the reference count of the object they return. Thus, it's the caller's responsibility to invoke *_unref once they're done using the result.

### Prerequisites

TBD

### Installation

TBD

### Set up the Git hooks custom directory

After cloning the repository run the following command in the
repository root:

```shell
git config core.hooksPath .githooks
```

### Clang format integration

Repository comes with always-up-to-date `.clang-format` file, an input configuration
for `clang-format` tool (version 17.x is a minimum). 

- https://apt.llvm.org/

### Unit Tests

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

### Contributing

We welcome contributions from the community. Please read our CONTRIBUTING.md for guidelines.


### License 

[APACHE LICENSE, VERSION 2.0](https://apache.org/licenses/LICENSE-2.0)
