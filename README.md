<p align="center">
  <img align="middle" src=
  "assets/cardano-c-logo-small.png"
  height="160" />
</p>

# Cardano-C

**cardano-c** is a C library aiming to be a robust, commercial-grade, full-featured toolkit for building transaction and interacting with the Cardano blockchain. Its compliant with the MISRA 2012 standard,
and was designed with a binding-friendly architecture to enable easy integrations across various programming languages.

Ready to learn? Get the latest documentation at [cardano-c.readthedocs.io](https://cardano-c.readthedocs.io/) or jump to [code examples!](examples/)

---
<br>

![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/unit-test.yml/badge.svg)
![Post-Integration](https://github.com/Biglup/cardano-c/actions/workflows/static-code-analysis.yml/badge.svg)
[![codecov](https://codecov.io/gh/Biglup/cardano-c/graph/badge.svg?token=A5U3U5KGG7)](https://codecov.io/gh/Biglup/cardano-c)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/36ac650a4b694421bf6780a110e2f65a)](https://app.codacy.com/gh/Biglup/cardano-c/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
[![Documentation Status](https://readthedocs.org/projects/cardano-c/badge/?version=latest)](https://cardano-c.readthedocs.io/en/latest/?badge=latest)
[![Static Badge](https://img.shields.io/badge/Funded_By-Project_Catalyst-133ff0?logo=cardano&logoColor=ffffff)](https://projectcatalyst.io/)

## Features

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
- It has no runtime dependencies (The library depends on [libsodium](https://github.com/jedisct1/libsodium) and [libgmp](https://gmplib.org/), but they are all statically linked)

## Basic Example

This is a basic cardano-c example, it sends `LOVELACE_TO_SEND` coins to `RECEIVING_ADDRESS`. Check full example at [send lovelace](examples/src/send_lovelace_example.c).

```c
// 2 hours from now in UNIX time (seconds)
const uint64_t invalid_after = cardano_utils_get_time() + SECONDS_IN_TWO_HOURS;

// 1.- Build transaction
cardano_tx_builder_t* tx_builder = cardano_tx_builder_new(protocol_params, provider);

cardano_tx_builder_set_utxos(tx_builder, utxo_list);
cardano_tx_builder_set_change_address(tx_builder, payment_address);
cardano_tx_builder_set_invalid_after_ex(tx_builder, invalid_after);
cardano_tx_builder_send_lovelace_ex(tx_builder, RECEIVING_ADDRESS, cardano_utils_safe_strlen(RECEIVING_ADDRESS, 128), LOVELACE_TO_SEND);

cardano_transaction_t* transaction = NULL;
cardano_error_t        result      = cardano_tx_builder_build(tx_builder, &transaction);

if (result != CARDANO_SUCCESS)
{
 console_error("Failed to build transaction");
 console_error("Error [%d]: %s", result, cardano_error_to_string(result));
 console_error("%s", cardano_tx_builder_get_last_error(tx_builder));

 return result;
}

// 2.- Sign transaction
sign_transaction(key_handler, SIGNER_DERIVATION_PATH, transaction);

// 3.- Submit transaction & confirm
submit_transaction(provider, CONFIRM_TX_TIMEOUT_MS, transaction);
```

## Conway Era Support

Cardano-C supports all features up to the Conway era, which is the current era of the Cardano blockchain. Conway era
brought to Cardano decentralized governance. You can see some of the governance related examples in the [examples](examples/) directory:

- [Register as DRep (PubKey)](examples/src/drep_pubkey_example.c)
- [Register as DRep (Script)](examples/src/drep_script_example.c)
- [Submit governance action proposal (Withdrawing from treasury)](examples/src/propose_treasury_withdrawal_example.c)
- [Vote for proposal (PubKey DRep)](examples/src/vote_for_proposal_pubkey_example.c)
- [Vote for proposal (Script DRep)](examples/src/vote_for_proposal_script_example.c)

These are some of the examples illustrated in the [examples](examples/) directory. However, you should
be able to build any valid transaction for the current era. See the [Documentation](https://cardano-c.readthedocs.io/) for more information.

## Memory Management

The Cardano C library uses a simple reference-counting model. The main goal is that the library can be easily integrated
into applications and languages that uses different memory management models (such as garbage collection).

Every object in our library provides functions to increase and decrease its reference count. For instance, you would use `cardano_cbor_writer_ref` to increase and `cardano_cbor_writer_unref` to decrease the reference count for a `cardano_cbor_writer_t` object.

Upon creation through constructors like `cardano_cbor_writer_new`, an object's reference count is initialized to one.
This implies that the caller becomes the sole owner of the newly created reference.

When the reference count drops to zero—typically when the `*_unref` function is invoked by the last entity
holding a reference—the object gets deallocated.

Note that **all** getter functions will consistently increment the reference count of the object they return. Thus, it's the caller's responsibility to invoke `*_unref` once they're done using the result.

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

<a href="https://github.com/Biglup/cardano-c/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=Biglup/cardano-c&max=500&columns=20&anon=1" />
</a>

We welcome contributions from the community. Please read our [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License 

[APACHE LICENSE, VERSION 2.0](https://apache.org/licenses/LICENSE-2.0)
