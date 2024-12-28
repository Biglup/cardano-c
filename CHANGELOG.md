Next (V1.x.x)
---------------------

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