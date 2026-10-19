// Quickstart: connect, create a collection, upsert documents, hybrid search.
//
//   ./quickstart                      # LOAMS_ENDPOINT defaults to http://127.0.0.1:8080
//
// Reads and writes only the `examples-cpp` namespace.

#include "loams_raw.hpp"

int main() {
  constexpr int kDim = 32;
  const std::string collection = "quickstart";
  auto client = ex::Connect();

  try {
    // 1. Connect: what is on the other end? (needs no auth)
    loams::instance::v1::GetInstanceResponse info;
    client->Instance()->GetInstance(&info);
    std::cout << "connected to " << client->Runtime()->Endpoint() << " (server " << info.server_version() << ")\n";

    // 2. A collection: a text field, a keyword field and a dense vector.
    ex::CreateNamespace(*client);
    ex::CreateDocsCollection(*client, collection, kDim);

    // 3. Upsert documents. Each carries its text, a tenant and an embedding.
    const std::string token = ex::Upsert(*client, collection,
                                         {{1, "refund policy for damaged items", "acme"},
                                          {2, "shipping times and carriers", "acme"},
                                          {3, "refund window for digital goods", "globex"},
                                          {4, "how to reset your password", "acme"}},
                                         kDim);
    std::cout << "wrote 4 documents, consistency token " << token << "\n";

    // 4. Hybrid search: vector + BM25, fused with RRF, filtered to one tenant.
    const std::string query = "refund policy";
    std::cout << "\nhybrid search for '" << query << "' (tenant=acme):\n";
    for (const auto& hit : ex::HybridSearch(*client, collection, query, kDim, "acme", 3)) {
      std::cout << "  " << ex::HitLine(hit) << "\n";
    }
    std::cout << "\nwithout the filter:\n";
    for (const auto& hit : ex::HybridSearch(*client, collection, query, kDim, "", 3)) {
      std::cout << "  " << ex::HitLine(hit) << "\n";
    }
  } catch (const loams::LoamsError& error) {
    ex::PrintError(error);
    return 1;
  }
  return 0;
}
