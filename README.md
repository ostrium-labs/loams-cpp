# loams-cpp

Example apps for the [Loams](https://loams.dev) C++ SDK.

Loams is an open-source data engine for AI apps: hybrid (vector + BM25 + filter) search, SQL and live tables over your own bucket. The SDK talks to it over Connect (HTTP), with no gRPC dependency.

## Examples

| Folder | What it shows |
|---|---|
| [`examples/quickstart`](examples/quickstart) | Connect, create a collection, upsert documents, hybrid search with a filter |
| [`examples/sql`](examples/sql) | SQL over a collection: `SELECT`, `vector_search`, `text_search`, `rrf` |
| [`examples/kb`](examples/kb) | `kb`, a small CLI: ingest text files, ask questions, print the retrieved context for an LLM prompt |

The examples read and write only a namespace named `examples-cpp`.

## A note on the SDK

The C++ SDK's typed facade covers the `instance`, `live`, `approvals`, `devices`, `operations` and `notifications` modules. Collections, documents, search and SQL have no typed C++ module yet, so the examples call them through the SDK's raw call path (`Client::Unary` with a `MethodBinding` and `google.protobuf.Struct` messages over Connect JSON). Auth, retries, timeouts and typed errors still come from the SDK runtime. The helpers are in [`common/loams_raw.hpp`](common/loams_raw.hpp).

## Install the SDK

The `loams-cpp-0.1.0.tar.gz` release asset is **not published yet**, so these examples build against the SDK in the monorepo, [`ostrium-labs/loams`](https://github.com/ostrium-labs/loams) under `sdks/cpp`. Once the tarball is out, install it and drop `-DLOAMS_SDK_DIR` (the build then uses `find_package(loams CONFIG)`).

Requirements: C++17, CMake 3.20+, libcurl, pkg-config, and **protobuf 36.1** exactly (the SDK's generated stubs need that runtime; Arch ships it, Ubuntu 24.04 does not, and the [CI workflow](.github/workflows/ci.yml) shows how to build it).

```sh
git clone https://github.com/ostrium-labs/loams
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
- Running 0.1.0: [docs/release/preview.md](https://github.com/ostrium-labs/loams/blob/main/docs/release/preview.md)
- The SDK: [`sdks/cpp`](https://github.com/ostrium-labs/loams/tree/main/sdks/cpp)

## Licence

Apache-2.0, see [LICENSE](LICENSE).
