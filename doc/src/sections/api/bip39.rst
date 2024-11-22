BIP-039
==========================

The functions in this section implement support for the BIP-39 standard, which defines the process of converting entropy into a human-readable mnemonic phrase and vice versa. These mnemonics are widely used in cryptocurrency wallets for secure key generation and backup.

Currently, the implementation only supports the **English BIP-39 wordlist**. This limitation is intentional to simplify the library and focus on the most widely used configuration. Future versions of the library may include support for additional languages.

------------

.. doxygenfunction:: cardano_bip39_entropy_to_mnemonic_words

------------

.. doxygenfunction:: cardano_bip39_mnemonic_words_to_entropy
