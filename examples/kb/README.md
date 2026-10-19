# kb

A small knowledge-base CLI: the retrieval half of a RAG app.

```sh
./build/kb ingest examples/kb/sample.txt          # one blank-line separated paragraph = one passage
./build/kb ask "how do I get a refund" --top 2    # hybrid search, prints the context block
./build/kb ask "reset password" --tenant examples/kb/sample.txt
./build/kb drop
```

`ask` prints the passages you would paste into an LLM prompt; Loams hosts no models, so generation is left to your own client. The `tenant` field holds the source file name, so `--tenant` filters to one file.
