// kb: a tiny knowledge-base CLI, the retrieval half of a RAG app.
//
//   ./kb ingest <file.txt>...        one paragraph (blank-line separated) = one document
//   ./kb ask "<question>" [--tenant T] [--top N]
//   ./kb drop
//
// `ask` runs hybrid search (embedding + BM25, RRF) and prints the top passages as the
// context block you would put in an LLM prompt. Loams hosts no models, so the
// embedding is a toy hashed bag-of-words; swap `ex::Embed` for a real model.
// Reads and writes only the `examples-cpp` namespace.

#include <fstream>

#include "loams_raw.hpp"

namespace {

constexpr int kDim = 32;
const std::string kCollection = "kb";

int Usage() {
  std::cerr << "usage: kb ingest <file>... | kb ask \"question\" [--tenant T] [--top N] | kb drop\n";
  return 2;
}

int Ingest(loams::Loams& client, const std::vector<std::string>& files) {
  ex::CreateNamespace(client);
  ex::CreateDocsCollection(client, kCollection, kDim);
  std::vector<ex::Doc> docs;
  for (const std::string& file : files) {
    std::ifstream in(file);
    if (!in) {
      std::cerr << "cannot read " << file << "\n";
      return 1;
    }
    std::string line, paragraph;
    auto flush = [&] {
      if (!paragraph.empty()) docs.push_back({docs.size() + 1, paragraph, file});
      paragraph.clear();
    };
    while (std::getline(in, line)) {
      if (line.empty()) flush();
      else paragraph += (paragraph.empty() ? "" : " ") + line;
    }
    flush();
  }
  if (docs.empty()) {
    std::cerr << "nothing to ingest\n";
    return 1;
  }
  ex::Upsert(client, kCollection, docs, kDim);
  std::cout << "ingested " << docs.size() << " passage(s)\n";
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
  const auto hits = ex::HybridSearch(client, kCollection, question, kDim, tenant, top);
  std::cout << "Context for: " << question << "\n";
  for (const auto& hit : hits) std::cout << "  " << ex::HitLine(hit) << "\n";
  if (hits.empty()) std::cout << "  (no passages)\n";
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return Usage();
  const std::string command = argv[1];
  const std::vector<std::string> rest(argv + 2, argv + argc);
  auto client = ex::Connect();
  try {
    if (command == "ingest" && !rest.empty()) return Ingest(*client, rest);
    if (command == "ask") return Ask(*client, rest);
    if (command == "drop") {
      ex::DropCollection(*client, kCollection);
      return 0;
    }
  } catch (const loams::LoamsError& error) {
    ex::PrintError(error);
    return 1;
  }
  return Usage();
}
