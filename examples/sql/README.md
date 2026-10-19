# sql

Writes four documents to the `sqldemo` collection, then runs SQL over it: a filtered `SELECT`, an aggregate, `vector_search`, `text_search` and `rrf` fusing the two. Each query's result is printed as a table.

SQL is served by the native API route `POST /v1/namespaces/{ns}/sql`; the example calls it through the SDK's raw call path.

```sh
./build/sql
```
