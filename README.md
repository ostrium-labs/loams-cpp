# loams-cpp

These examples target the published **Loams SDK 0.1.2** release.

Example apps for the [Loams](https://loams.dev) C++ SDK.

Loams is an open-source data engine for AI apps: hybrid (vector + BM25 + filter) search, SQL and live tables over your own bucket. The SDK talks to it over Connect (HTTP), with no gRPC dependency.

## Examples

| Folder | What it shows |
|---|---|
| [`examples/quickstart`](examples/quickstart) | Instance/system, namespaces, collections, documents, text/vector/hybrid search, optional graph, SDK SQL and streams |
| [`examples/sql`](examples/sql) | SQL module with aggregation and `rrf`, `vector_search`, `text_search` table functions |
| [`examples/kb`](examples/kb) | SDK-backed CLI: ingest text files, ask questions, print retrieved context; explicit `kb drop` only affects this demo collection |

The examples read and write only a namespace named `examples-cpp`.

## A note on the SDK

The examples use the published 0.1.2 high-level modules and builders throughout; they do not hand-build Connect JSON or call the native SQL HTTP route. The SDK's `Live()` and `System()` facade remain available; quickstart demonstrates `System()` for version and feature discovery. The upstream [SDK notes example](https://github.com/ostrium-labs/loams/blob/v0.1.2/sdks/cpp/examples/notes/main.cpp) documents the same published signatures.

## Install the SDK

Download the [`loams-cpp-0.1.2.tar.gz` release asset](https://github.com/ostrium-labs/loams/releases/download/v0.1.2/loams-cpp-0.1.2.tar.gz), or build against `sdks/cpp` from the matching `v0.1.2` monorepo tag. An installed SDK is resolved with `find_package(loams 0.1.2 EXACT CONFIG)`. CI checks out the exact release tag.

Requirements: C++17, CMake 3.20+, libcurl, pkg-config, and **protobuf 36.1** exactly (the SDK's generated stubs need that runtime; Arch ships it, Ubuntu 24.04 does not, and the [CI workflow](.github/workflows/ci.yml) shows how to build it).

```sh
git clone --branch v0.1.2 --depth 1 https://github.com/ostrium-labs/loams
cmake -S . -B build -DLOAMS_SDK_DIR=$PWD/loams/sdks/cpp   # path to sdks/cpp
cmake --build build -j 4
```

## Run against a local engine

Start an engine (from a checkout of `ostrium-labs/loams`) or the compose stack from [docs/release/preview.md](https://github.com/ostrium-labs/loams/blob/main/docs/release/preview.md):

```sh
cargo run -p loams -- dev          # serves http://127.0.0.1:8080, auth off
```

```sh
export LOAMS_ENDPOINT=http://127.0.0.1:8080   # the default
# export LOAMS_API_KEY=...                    # only if the engine has auth on
./build/quickstart
./build/sql
./build/kb ingest examples/kb/sample.txt
./build/kb ask "how do I get a refund" --top 2
```

## Docs

- Loams docs: <https://loams.dev/docs>
- Running 0.1.2: [docs/release/preview.md](https://github.com/ostrium-labs/loams/blob/main/docs/release/preview.md)
- The SDK: [`sdks/cpp`](https://github.com/ostrium-labs/loams/tree/main/sdks/cpp)

## Licence

Apache-2.0, see [LICENSE](LICENSE).
