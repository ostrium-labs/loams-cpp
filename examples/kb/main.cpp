// A small knowledge-base CLI using only the high-level 0.1.2 SDK modules.
//
//   kb ingest <file.txt>...        blank-line-separated paragraphs become docs
//   kb ask "<question>" [--tenant T] [--top N]
//   kb drop                         explicitly drops only examples-cpp/kb

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <loams/loams.hpp>

namespace {
constexpr int kDim = 32;
constexpr char kCollection[] = "kb";

std::vector<float> Embed(const std::string& text) {
  std::vector<float> vector(kDim, 0.0f);
  std::string word;
  auto flush = [&] {
    if (word.empty()) return;
    std::uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : word) hash = (hash ^ c) * 1099511628211ull;
    vector[hash % kDim] += 1.0f;
    word.clear();
  };
  for (unsigned char c : text) {
    if (std::isalnum(c)) word.push_back(static_cast<char>(std::tolower(c)));
    else flush();
  }
  flush();
  float norm = 0.0f;
  for (float x : vector) norm += x * x;
  norm = std::sqrt(norm);
  if (norm > 0) for (float& x : vector) x /= norm;
  return vector;
}

std::shared_ptr<loams::Loams> Connect() {
  auto options = loams::OptionsFromEnv();
  options.default_namespace = "examples-cpp";
  options.session_consistency = true;
  return loams::MakeLoams(std::move(options));
}

void EnsureKB(loams::Loams& client) {
  try { client.Namespaces()->Create("examples-cpp"); }
  catch (const loams::AlreadyExistsError&) { }
  try {
    client.Collections()->Create(kCollection,
        loams::Schema().Text("body").Keyword("tenant").Vector("embedding", kDim));
  } catch (const loams::AlreadyExistsError&) { }
}

int Usage() {
  std::cerr << "usage: kb ingest <file>... | kb ask \"question\" [--tenant T] [--top N] | kb drop\n";
  return 2;
}

int Ingest(loams::Loams& client, const std::vector<std::string>& files) {
  EnsureKB(client);
  std::vector<loams::Document> docs;
  for (const auto& file : files) {
    std::ifstream in(file);
    if (!in) { std::cerr << "cannot read " << file << "\n"; return 1; }
    std::string line, paragraph;
    auto flush = [&] {
      if (!paragraph.empty()) {
        const auto id = static_cast<std::uint64_t>(docs.size() + 1);
        const std::string source = std::string("{\"body\":") + loams::ToJson(loams::Json(paragraph)) +
                                  ",\"tenant\":" + loams::ToJson(loams::Json(file)) + "}";
        docs.push_back(loams::Document::FromJson(id, source, {{"embedding", Embed(paragraph)}}));
      }
      paragraph.clear();
    };
    while (std::getline(in, line)) {
      if (line.empty()) flush();
      else paragraph += (paragraph.empty() ? "" : " ") + line;
    }
    flush();
  }
  if (docs.empty()) { std::cerr << "nothing to ingest\n"; return 1; }
  client.Documents()->Upsert(kCollection, docs);
  std::cout << "ingested/upserted " << docs.size() << " passage(s)\n";
  return 0;
}

int Ask(loams::Loams& client, const std::vector<std::string>& args) {
  std::string question, tenant;
  int top = 3;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--tenant" && i + 1 < args.size()) tenant = args[++i];
    else if (args[i] == "--top" && i + 1 < args.size()) top = std::atoi(args[++i].c_str());
    else question += (question.empty() ? "" : " ") + args[i];
  }
  if (question.empty()) return Usage();
  loams::VectorOptions vector_options;
  vector_options.k = 10;
  auto search = loams::Search().Text(question, {{"body"}, 10})
      .Vector("embedding", Embed(question), vector_options)
      .Fuse(loams::Fusion::Rrf(60)).Limit(static_cast<std::uint32_t>(top));
  if (!tenant.empty()) search.Where(loams::Filter::Eq("tenant", loams::Json(tenant)));
  const auto hits = client.Collection(kCollection).Search(search);
  std::cout << "Context for: " << question << "\n";
  for (const auto& hit : hits.hits()) {
    std::cout << "  " << loams::DocId::From(hit.pk()).Text() << "  "
              << hit.source().fields().at("tenant").string_value() << "  "
              << hit.source().fields().at("body").string_value() << "\n";
  }
  if (hits.hits().empty()) std::cout << "  (no passages)\n";
  return 0;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return Usage();
  std::vector<std::string> args(argv + 2, argv + argc);
  try {
    auto client = Connect();
    const std::string command = argv[1];
    if (command == "ingest" && !args.empty()) return Ingest(*client, args);
    if (command == "ask") return Ask(*client, args);
    if (command == "drop") {
      client->Collection(kCollection).Drop();
      return 0;
    }
  } catch (const loams::NotFoundError&) {
    std::cerr << "the kb collection does not exist; run kb ingest first\n";
    return 1;
  } catch (const loams::LoamsError& error) {
    std::cerr << loams::ToString(error.CodeValue()) << " / " << loams::ToString(error.ReasonValue()) << ": "
              << error.what() << "\n";
    return 1;
  }
  return Usage();
}
