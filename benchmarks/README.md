# Benchmarks

Benchmark harnesses for cardano-c. Not built by default; enable with:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_STATIC_LIBS=ON -DBENCHMARKS_ENABLED=ON
cmake --build build --target uplc-bench
```

## uplc-bench

Measures flat-decode + CEK evaluation per script over a directory of raw
`.flat` UPLC programs (the [cardano-plutus-vm-benchmark](https://github.com/saib-inc/cardano-plutus-vm-benchmark)
`plutus_use_cases` corpus) and emits the JSON schema consumed by that suite:

```sh
./build/build/release/benchmarks/uplc-bench --quiet -o results.json <flat-dir>
```

Protocol matches the suite's other custom harnesses (plutuz, llvm-uplc):
5 warmup iterations, at least 50 measured iterations, a 5 second time budget
per script and a 10000 iteration cap. Evaluation runs under Plutus V3
semantics with an unlimited budget.

`--verify` evaluates each script once and prints `name,status,cpu,mem` CSV,
useful for differential comparison against other VMs (spent-budget equality
is a very strong proxy for execution-trace equality).
