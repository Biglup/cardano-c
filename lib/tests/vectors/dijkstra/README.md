# Dijkstra era test vectors

Reference material for the Dijkstra (protocol version 12) serialization work, see
https://github.com/Biglup/cardano-c/issues/133.

## Sources

- `dijkstra.cddl` and `golden/`: IntersectMBO/cardano-ledger, commit
  `965cc47d7bbb1c86975c6e73738a9593881fcb58` (master, 2026-07-15), from
  `eras/dijkstra/impl/cddl/data/dijkstra.cddl` and `eras/dijkstra/impl/golden/`.
  The 5.7MB `translations.cbor` golden was deliberately not vendored (Plutus
  translation contexts, not needed for serialization round trips).
- `js-sdk-1735/`: TypeScript test files from input-output-hk/cardano-js-sdk
  PR #1735 (merge commit `af5626565c3061b1802b9b75c4a729b6808179ef`). These are
  NOT compiled; they are vendored verbatim as a source of CBOR hex vectors for
  the Dijkstra types (sub transactions, guards, PlutusV4, direct deposits,
  account balance intervals, redeemer tightenings).

## Re-sync

The era is still under development upstream. When the ledger tags a final CDDL
(or IntersectMBO/cardano-node#6558 flips cardano-testnet to Dijkstra), re-fetch
these files at the new pin and diff before continuing past phase 2 of the plan
in issue #133.
