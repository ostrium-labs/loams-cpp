// SQL through the published loams.sql module. The app seeds its own collection
// in examples-cpp; stable IDs make repeated runs safe.

#include <iostream>
#include <utility>

#include <loams/loams.hpp>

int main() {
  try {
    auto options = loams::OptionsFromEnv();
    options.default_namespace = "examples-cpp";
    options.session_consistency = true;
    auto client = loams::MakeLoams(std::move(options));
    try { client->Namespaces()->Create("examples-cpp"); }
    catch (const loams::AlreadyExistsError&) { }
    try { client->Collections()->Create("sqldemo", loams::Schema().Text("body").Keyword("tenant").Vector("embedding", 4)); }
    catch (const loams::AlreadyExistsError&) { }

    auto docs = client->Collection("sqldemo");
    docs.Upsert({
        loams::Document::FromJson(1, R"({"body":"refund policy for damaged items","tenant":"acme"})",
                                  {{"embedding", {1, 0, 0, 0}}}),
        loams::Document::FromJson(2, R"({"body":"shipping times and carriers","tenant":"acme"})",
                                  {{"embedding", {0, 1, 0, 0}}}),
        loams::Document::FromJson(3, R"({"body":"refund window for digital goods","tenant":"globex"})",
                                  {{"embedding", {0, 0, 1, 0}}}),
    });

    const auto result = client->Sql()->Query("SELECT tenant, count(*) AS n FROM sqldemo GROUP BY tenant ORDER BY tenant");
    for (const auto& row : loams::SqlModule::Records(result)) {
      std::cout << row.at("tenant").string_value() << " | " << row.at("n").number_value() << "\n";
    }
    const auto hybrid = client->Sql()->Query(
        "SELECT _id, body, _score FROM rrf(vector_search('sqldemo', [1.0, 0.0, 0.0, 0.0], 'embedding', 10), "
        "text_search('sqldemo', 'refund', 'body', 10)) LIMIT 3");
    std::cout << "hybrid result rows: " << loams::SqlModule::Records(hybrid).size() << "\n";
  } catch (const loams::LoamsError& error) {
    std::cerr << loams::ToString(error.CodeValue()) << " / " << loams::ToString(error.ReasonValue()) << ": "
              << error.what() << "\n";
    return 1;
  }
  return 0;
}
