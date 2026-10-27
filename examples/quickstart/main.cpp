// Published SDK 0.1.2 tour: system, namespaces, collections, documents,
// search/vector, optional graph, SQL, and streams. Writes stay in examples-cpp.

#include <iostream>
#include <utility>

#include <loams/loams.hpp>

int main() {
  try {
    loams::Options options = loams::OptionsFromEnv();
    options.default_namespace = "examples-cpp";
    options.session_consistency = true;
    auto client = loams::MakeLoams(std::move(options));

    loams::instance::v1::GetInstanceResponse info;
    client->Instance()->GetInstance(&info);
    std::cout << "connected to " << info.name() << " " << info.server_version() << "\n";
    const auto version = client->System()->Version();
    std::cout << "SDK/server compatible: " << (version.compatible ? "yes" : "no") << "\n";
    std::cout << "served packages: " << client->System()->Served().size() << "\n";

    try { client->Namespaces()->Create("examples-cpp"); }
    catch (const loams::AlreadyExistsError&) { /* safe to rerun */ }
    try {
      client->Collections()->Create("sdk_tour",
          loams::Schema().Text("title").Text("body").Keyword("tags").I64("n").Vector("embedding", 4));
    } catch (const loams::AlreadyExistsError&) { /* same schema is reused; never drop data */ }

    auto notes = client->Collection("sdk_tour");
    notes.Upsert({
        loams::Document::FromJson(1, R"({"title":"Refund policy","body":"refund policy for orders","tags":["policy"],"n":1})",
                                  {{"embedding", {1, 0, 0, 0}}}),
        loams::Document::FromJson(2, R"({"title":"Shipping","body":"shipping times","tags":["ops"],"n":2})",
                                  {{"embedding", {0, 1, 0, 0}}}),
        loams::Document::FromJson(3, R"({"title":"Refund window","body":"the refund window is thirty days","tags":["policy"],"n":3})",
                                  {{"embedding", {0.9f, 0.1f, 0, 0}}}),
    });
    std::cout << "documents: " << notes.Count().count() << "\n";
    const auto selected = notes.Get({loams::DocId(1), loams::DocId(3)});
    std::cout << "get returned " << selected.documents_size() << " docs\n";

    loams::SelectOptions title;
    title.include = {"title"};
    const auto text = client->Search()->Query(loams::Search().Text("refund", {{"body"}, 10})
        .Where(loams::Filter::Eq("tags", loams::Json("policy"))).Select(title).Limit(5));
    loams::SearchShorthand vector_options;
    vector_options.k = 10;
    const auto vector = client->Vector()->Search("sdk_tour", {1, 0, 0, 0}, "embedding", vector_options);
    loams::VectorOptions ten;
    ten.k = 10;
    const auto hybrid = notes.Search(loams::Search().Text("refund", {{"body"}, 10})
        .Vector("embedding", {1, 0, 0, 0}, ten)
        .Fuse(loams::Fusion::Rrf(60)).Limit(5));
    std::cout << "text/vector/hybrid hits: " << text.hits_size() << "/" << vector.hits_size() << "/"
              << hybrid.hits_size() << "\n";

    const auto sql = client->Sql()->Query("SELECT n, title FROM sdk_tour WHERE n > $1 ORDER BY n", {loams::Json(1)});
    std::cout << "SQL rows: " << loams::SqlModule::Records(sql).size() << "\n";

    const auto stream = client->Streams()->Create("sdk-tour-events", 1);
    loams::ProduceOptions produce_options;
    produce_options.producer_id = "examples-cpp-sdk-tour";
    produce_options.producer_seq = 1;
    const auto event = client->Streams()->Produce("sdk-tour-events", {{"1", R"({"kind":"seeded","id":1})"}},
        produce_options);
    loams::FetchOptions fetch;
    fetch.offset = event.base_offset();
    const auto read = client->Streams()->Fetch("sdk-tour-events", fetch);
    std::cout << "stream created=" << stream.created() << ", fetched=" << read.records_size() << " record(s)\n";

    // Graph is experimental and feature-detected; query only, without deleting it.
    if (client->System()->Available("graph")) {
      try { client->Graph()->Create("examples_cpp_sdk_tour"); }
      catch (const loams::AlreadyExistsError&) { }
      loams::GraphQueryOptions read_only;
      read_only.read_only = true;
      const auto graph = client->Graph()->Query("examples_cpp_sdk_tour", "RETURN 1 AS ready", {}, read_only);
      std::cout << "graph rows: " << loams::GraphModule::Records(graph).size() << "\n";
    } else {
      std::cout << "graph: not in this build\n";
    }
  } catch (const loams::LoamsError& error) {
    std::cerr << loams::ToString(error.CodeValue()) << " / " << loams::ToString(error.ReasonValue())
              << ": " << error.what() << "\n";
    return 1;
  } catch (const std::exception& error) {
    std::cerr << error.what() << "\n";
    return 1;
  }
  return 0;
}
