// SQL over a collection: a plain SELECT, then vector_search, text_search and rrf.
//
//   ./sql
//
// Demonstrate the native SQL HTTP route through the SDK runtime. The SDK also
// provides the loams.sql.v1 SQL RPC module.
// Reads and writes only the `examples-cpp` namespace.

#include "loams_raw.hpp"

namespace {

void Run(loams::Loams& client, const std::string& label, const std::string& sql) {
  std::cout << "\n-- " << label << "\n" << sql << "\n";
  const ex::Struct result =
      ex::Call(client, "loams.native.v1.Sql/Query", "{\"query\":" + ex::Quote(sql) + "}", true,
               std::string("/v1/namespaces/") + ex::kNamespace + "/sql");
  std::string header;
  for (const auto& column : ex::Get(result, "columns").list_value().values()) {
    header += ex::Text(ex::Get(column.struct_value(), "name")) + "\t";
  }
  std::cout << header << "\n";
  for (const auto& row : ex::Get(result, "rows").list_value().values()) {
    std::string line;
    for (const auto& cell : row.list_value().values()) line += ex::Text(cell) + "\t";
    std::cout << line << "\n";
  }
}

}  // namespace

int main() {
  constexpr int kDim = 32;
  const std::string collection = "sqldemo";
  auto client = ex::Connect();
  try {
    ex::CreateNamespace(*client);
    ex::CreateDocsCollection(*client, collection, kDim);
    ex::Upsert(*client, collection,
               {{1, "refund policy for damaged items", "acme"},
                {2, "shipping times and carriers", "acme"},
                {3, "refund window for digital goods", "globex"},
                {4, "how to reset your password", "acme"}},
               kDim);

    const std::string vec = ex::Floats(ex::Embed("refund policy", kDim));
    Run(*client, "a plain SELECT", "SELECT _id, body, tenant FROM sqldemo WHERE tenant = 'acme' ORDER BY _id");
    Run(*client, "an aggregate", "SELECT tenant, count(*) AS n FROM sqldemo GROUP BY tenant ORDER BY tenant");
    Run(*client, "vector_search", "SELECT _id, body, _score FROM vector_search('sqldemo', " + vec + ", 'embedding', 3)");
    Run(*client, "text_search (BM25)", "SELECT _id, body, _score FROM text_search('sqldemo', 'refund', 'body', 3)");
    Run(*client, "rrf: vector + text fused",
        "SELECT _id, body, _score FROM rrf(vector_search('sqldemo', " + vec +
            ", 'embedding', 10), text_search('sqldemo', 'refund policy', 'body', 10)) LIMIT 3");
  } catch (const loams::LoamsError& error) {
    ex::PrintError(error);
    return 1;
  }
  return 0;
}
