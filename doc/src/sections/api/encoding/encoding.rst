Encoding
========

This section of the documentation explores two significant encoding schemes used within the Cardano ecosystem: Base58 and Bech32.

**Base58**

Base58 is a binary-to-text encoding scheme primarily used for encoding binary data in a compact, human-friendly format. It was notably used for representing cryptocurrency addresses in Bitcoin and within the Cardano ecosystem during the Byron era. The design of Base58 excludes visually similar characters, helping to prevent reading and typing errors, making it an ideal choice for presenting cryptographic keys and addresses to end-users.

.. note::
   With the advent of the Shelley era and the introduction of Bech32 encoding for addresses, Base58 usage in the Cardano ecosystem has been deprecated.

**Bech32**

Bech32 is a newer encoding format developed for use with Segregated Witness (SegWit) addresses in Bitcoin, and it has been adopted by other blockchain projects for its efficiency and error detection capabilities. Bech32 addresses consist of a human-readable part (HRP) and a data part encoded using a BCH code for error detection and correction. In the Cardano ecosystem, Bech32 encoding is employed for wallet addresses.

For in-depth information about the functionalities related to Base58 and Bech32 encoding, please refer to the sections below:

.. toctree::
   :maxdepth: 1

   base58
   bech32