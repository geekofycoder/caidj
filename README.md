# CAIDJ — Concurrency-Aware Indexing for Distributed Joins

**[Live results dashboard](https://geekofycoder.github.io/caidj/)**

A distributed join builds an index on the join key, then probes it. Classic join
algorithms assume the data holds still while that happens. In a live system it
does not — writes keep arriving, and a lock on the index serialises them against
every probe.

This is a C++20 simulator that implements three index protocols designed to avoid
that, and measures them against a lock-based baseline.

| | idea |
|---|---|
| **NHJ** | baseline: one global reader-writer lock |
| **ECHI** | batch writes into epochs; probes read a frozen snapshot, never lock |
| **MPI-MVCC** | per-key version chains; probes read at their own timestamp |
| **BF-CSI** | Bloom filter in front of a concurrent skip-list |

Headline result: the baseline blocks **38.2%** of probes at 16 writer threads.
All three protocols block **none**, at every concurrency level.

## Build

Needs CMake 3.22+ and a C++20 compiler. No external dependencies.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Run the tests:

```bash
cd build && ctest --output-on-failure
```

## Run

```bash
./build/caidj --protocol all --concurrency 4 --duration 2000 --runs 1 --output out/
```

That takes a few seconds and prints a table. The full benchmark from the report
is 30-second trials, 3 runs, at 1/2/4/8/16 writers, and takes about 30 minutes:

```bash
./scripts/run_all.sh ./build/caidj
```

Output defaults to `results/`, which already holds the recorded runs, so pass
`--output` to somewhere else as above.

Useful flags:

```
--protocol nhj|echi|mpimvcc|bfcsi|all
--concurrency <n>      writer threads
--duration <ms>        trial length
--runs <n>             repetitions
--output <dir>         where results land
--help                 everything else
```

## Layout

```
src/index/      the four index protocols
src/join/       join executor, protocol-agnostic
src/txn/        transaction manager
src/bench/      benchmark harness
include/caidj/  headers
configs/        TOML benchmark configs
results/        full_benchmark/ is the run reported in the project;
                the other files are from a separate run
tests/          unit tests
docs/           the results dashboard and the project report (PDF)
DESIGN.md       full implementation design document
```

## Results

`results/full_benchmark/results.csv` holds 60 runs — 4 protocols × 5 concurrency
levels × 3 repetitions. The dashboard is generated from that file.

Mean join latency (ms), lower is better:

| writers | NHJ | ECHI | MPI-MVCC | BF-CSI |
|---|---|---|---|---|
| 1 | 150.4 | **56.5** | 7,287.0 | 97.9 |
| 4 | 121.7 | **94.4** | 2,802.8 | 144.4 |
| 16 | 1,855.8 | **61.5** | 2,475.8 | 678.6 |

ECHI is fastest from 4 writers up and uses the least memory. MPI-MVCC never
blocks a reader but version-chain traversal makes it the slowest protocol
measured — a negative result worth keeping.

## Changes from the original

This repository is a modified copy. GPL-3.0 asks that modifications be stated:

- headers moved from `data/include/` to `include/`, which is where
  `CMakeLists.txt` already expected them
- a portability shim for `std::atomic<std::shared_ptr<T>>`
  (`include/caidj/util/atomic_shared_ptr.hpp`). libstdc++ implements the C++20
  specialisation; libc++ does not, so the build failed on clang. The shim keeps
  the native path where it exists and falls back to the `atomic_load`/
  `atomic_store` free functions elsewhere
- the `docs/` dashboard, generated from `results/full_benchmark/results.csv`
- build artefacts and `results.tar` left out

## Authors

B.Tech project, International Institute of Information Technology Bhubaneswar, May 2026.

- Soubhik Kumar Gon (B422056)
- Hitanshu Satpathy (B422028)
- Amit Bhagat (B422007)

Supervised by Dr. Rakesh Chandra Balabantaray, Department of CSE, IIIT Bhubaneswar.


## Note on scope

Everything runs in one process on one machine. Network latency, node failure and
partition rebalancing are not modelled, so the results describe relative protocol
behaviour rather than absolute production performance.

## License

GPL-3.0. See [LICENSE](LICENSE).
