# sql

Writes four documents to the `sqldemo` collection, then runs SQL over it: a filtered `SELECT`, an aggregate, `vector_search`, `text_search` and `rrf` fusing the two. Each query's result is printed as a table.

The example uses the published `loams.sql` module and prints the SDK's column-name/value records; it does not make a raw HTTP or dynamic RPC call.

```sh
./build/sql
```
