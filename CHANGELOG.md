Next (V1.1.10)
---------------------

V1.1.9
---------------------

- Fix a bug in _cardano_set_collateral_output that was causing the collateral output to sometimes be set when not needed.

V1.1.8
---------------------

- Expose cardano_transaction_get_unique_signers to the public API

V1.1.7
---------------------

- Add donate to treasury function to tx builder.
- Increase url max size on pool metadata from 64 to 128 bytes.

V1.1.6
---------------------

- Pin Emscripten to version 3.1.49.

V1.1.5
---------------------

- Added emscripten coin selector.
- Added emscripten tx evaluator.
- Fixed a bug in the transaction balancer that was setting wrong redeemer index sometimes.

V1.1.4
---------------------
- Added emscripten blockchain provider.

V1.1.3
---------------------
- Switched to using MODULARIZE=1 for the Emscripten build process. The library is now initialized via a factory function.

V1.1.2
---------------------
- EMSCRIPTEN: Wasm file is now embedded in the cardano-c.js file.

V1.1.1
---------------------
- Adjusted exports for Emscripten builds.
- Added a WASM release target.

V1.1.0
---------------------
- Added `cardano_json_writer_t` to write JSON to a buffer.
- Added `cardano_json_object_t` to parse JSON from a strings.
- Added new fuzzing targets for JSON parsing.
- Removed JSON-C dependency.

V1.0.0
---------------------
- First release of cardano-c
- Address Parsing & Generation
- Ed25519 Cryptography
- Transaction Serialization & Deserialization
- Powerful Transaction Builder
- Full Conway era support, including registering DReps, voting on proposals, and submitting proposals.