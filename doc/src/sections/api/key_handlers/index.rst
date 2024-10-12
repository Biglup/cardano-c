Key Handlers
=============

A key handler securely manages cryptographic key operations, providing a unified interface for handling both BIP32 (HD) and Ed25519 keys. It ensures that keys are stored encrypted, only decrypted when needed for operations like signing, and cleared from memory afterward. Key handlers can also abstract interactions with hardware wallets, where keys never leave the device.

.. toctree::
   :maxdepth: 1

   account_derivation_path
   derivation_path
   secure_key_handler
   secure_key_handler_impl
   secure_key_handler_type
   software_secure_key_handler