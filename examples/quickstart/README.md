# quickstart

Connects, creates the `quickstart` collection in the `examples-cpp` namespace (a text field, a keyword `tenant` field and a 32-dimension cosine vector), upserts four documents, and runs hybrid search (vector + BM25, fused with RRF), once filtered to `tenant = acme` and once unfiltered.

```sh
./build/quickstart
```

Embeddings are a toy hashed bag-of-words (Loams hosts no models); replace `ex::Embed` with your model.
