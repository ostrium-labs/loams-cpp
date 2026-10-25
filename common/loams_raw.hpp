// Shared helpers for the examples.
//
// These examples demonstrate raw Connect JSON calls through the SDK runtime.
// App modules, builders, collection handles, SQL and streams are also available.
//
// Every call is pinned to the `examples-cpp` namespace.

#ifndef LOAMS_EXAMPLES_RAW_HPP
#define LOAMS_EXAMPLES_RAW_HPP

#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <google/protobuf/struct.pb.h>
#include <google/protobuf/util/json_util.h>

#include "loams/loams.hpp"

namespace ex {

using google::protobuf::Struct;
using google::protobuf::Value;

/// The only namespace the examples read or write.
inline constexpr char kNamespace[] = "examples-cpp";

inline std::string Env(const char* name, const std::string& fallback) {
  const char* const value = std::getenv(name);
  return (value == nullptr || value[0] == '\0') ? fallback : std::string(value);
}

/// Builds a client from LOAMS_ENDPOINT (default http://127.0.0.1:8080) and
/// LOAMS_API_KEY / LOAMS_TOKEN (optional; a local `loams dev` needs none).
inline std::shared_ptr<loams::Loams> Connect() {
  return loams::MakeLoams({.endpoint = Env("LOAMS_ENDPOINT", "http://127.0.0.1:8080"),
                           .token_source = loams::EnvToken()});
}

inline Struct Json(const std::string& text) {
  Struct message;
  const auto status = google::protobuf::util::JsonStringToMessage(text, &message);
  if (!status.ok()) {
    throw std::runtime_error("bad JSON literal: " + std::string(status.message()));
  }
  return message;
}

inline std::string Dump(const Struct& message) {
  std::string out;
  (void)google::protobuf::util::MessageToJsonString(message, &out);
  return out;
}

/// A JSON string literal, escaped.
inline std::string Quote(const std::string& text) {
  Value value;
  value.set_string_value(text);
  std::string out;
  (void)google::protobuf::util::MessageToJsonString(value, &out);
  return out;
}

/// A JSON array of numbers.
inline std::string Floats(const std::vector<float>& values) {
  std::ostringstream out;
  out << "[";
  for (std::size_t i = 0; i < values.size(); ++i) {
    out << (i ? "," : "") << values[i];
  }
  out << "]";
  return out.str();
}

// --- Reading a Struct ---------------------------------------------------------------------------

inline const Value& Get(const Struct& object, const std::string& key) {
  static const Value kNull;
  const auto found = object.fields().find(key);
  return found == object.fields().end() ? kNull : found->second;
}

inline std::string Text(const Value& value) {
  switch (value.kind_case()) {
    case Value::kStringValue:
      return value.string_value();
    case Value::kNumberValue: {
      std::ostringstream out;
      out << value.number_value();
      return out.str();
    }
    case Value::kBoolValue:
      return value.bool_value() ? "true" : "false";
    default:
      return "";
  }
}

// --- Raw calls ----------------------------------------------------------------------------------

/// One raw RPC call (app modules also provide typed calls). `path` empty means the Connect path
/// `/<rpc>`; otherwise it is an HTTP route of the native API.
inline Struct Call(loams::Loams& client, const std::string& rpc, const std::string& request_json,
                   bool read_only, const std::string& path = "") {
  loams::MethodBinding binding;
  binding.rpc = rpc;
  binding.path = path;
  binding.retry_class = read_only ? loams::RetryClass::kSafe : loams::RetryClass::kManual;
  const Struct request = Json(request_json);
  Struct response;
  client.Runtime()->Unary(binding, request, &response, loams::ContentType::kJson);
  return response;
}

/// Prints a SDK error the way an application should branch on it: by reason.
inline void PrintError(const loams::LoamsError& error) {
  std::cerr << "error: code=" << loams::ToString(error.CodeValue())
            << " reason=" << loams::ToString(error.ReasonValue()) << "\n  " << error.what() << "\n";
  if (!error.Hint().empty()) {
    std::cerr << "  hint: " << error.Hint() << "\n";
  }
}

inline void CreateNamespace(loams::Loams& client) {
  try {
    Call(client, "loams.collection.v1.NamespaceService/CreateNamespace",
         std::string("{\"namespace\":") + Quote(kNamespace) + "}", false);
  } catch (const loams::LoamsError& error) {
    // An existing namespace is fine; anything else is a real failure.
    if (error.CodeValue() != loams::Code::kAlreadyExists) throw;
  }
}

inline void DropCollection(loams::Loams& client, const std::string& name) {
  try {
    Call(client, "loams.collection.v1.CollectionService/DropCollection",
         std::string("{\"namespace\":") + Quote(kNamespace) + ",\"collection\":" + Quote(name) + "}", false);
  } catch (const loams::LoamsError& error) {
    if (error.CodeValue() != loams::Code::kNotFound) throw;
  }
}

/// Creates a collection with a text field `body`, a keyword field `tenant` and a
/// dense vector `embedding` of `dim` dimensions, dropping a previous one first.
inline void CreateDocsCollection(loams::Loams& client, const std::string& name, int dim) {
  DropCollection(client, name);
  const std::string schema =
      R"({"fields":[
           {"name":"body","source_path":"body","kind":{"text":{"analyzer":"standard","positions":true}},"indexed":true,"fast":false},
           {"name":"tenant","source_path":"tenant","kind":"keyword","indexed":true,"fast":true}],
          "vectors":[{"name":"embedding","dim":)" + std::to_string(dim) + R"(,"distance":"cosine"}],
          "sparse_vectors":[],"dynamic":"ignore","max_fields":1000})";
  Call(client, "loams.collection.v1.CollectionService/CreateCollection",
       std::string("{\"namespace\":") + Quote(kNamespace) + ",\"name\":" + Quote(name) + ",\"schema\":" + schema + "}",
       false);
}

struct Doc {
  std::uint64_t id;
  std::string body;
  std::string tenant;
};

/// Toy embedding: hashed bag of words, L2-normalised. Loams hosts no models, so a
/// real application would put its embedding model here. Deterministic, so runs repeat.
inline std::vector<float> Embed(const std::string& text, int dim) {
  std::vector<float> vector(static_cast<std::size_t>(dim), 0.0f);
  std::string word;
  auto flush = [&] {
    if (word.empty()) return;
    std::uint64_t hash = 1469598103934665603ull;  // FNV-1a
    for (const char c : word) hash = (hash ^ static_cast<unsigned char>(c)) * 1099511628211ull;
    vector[hash % static_cast<std::uint64_t>(dim)] += 1.0f;
    word.clear();
  };
  for (const char c : text) {
    if (std::isalnum(static_cast<unsigned char>(c))) {
      word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    } else {
      flush();
    }
  }
  flush();
  float norm = 0.0f;
  for (const float x : vector) norm += x * x;
  norm = std::sqrt(norm);
  if (norm > 0.0f) for (float& x : vector) x /= norm;
  return vector;
}

/// Upserts documents with their embeddings; returns the consistency token.
inline std::string Upsert(loams::Loams& client, const std::string& collection, const std::vector<Doc>& docs,
                          int dim) {
  std::string ops;
  for (const Doc& doc : docs) {
    ops += std::string(ops.empty() ? "" : ",") + "{\"upsert\":{\"id\":{\"uint\":" + std::to_string(doc.id) +
           "},\"source\":{\"body\":" + Quote(doc.body) + ",\"tenant\":" + Quote(doc.tenant) +
           "},\"vectors\":{\"embedding\":" + Floats(Embed(doc.body, dim)) + "}}}";
  }
  const Struct response =
      Call(client, "loams.collection.v1.DocumentService/WriteDocuments",
           std::string("{\"namespace\":") + Quote(kNamespace) + ",\"collection\":" + Quote(collection) +
               ",\"ops\":[" + ops + "]}",
           false);
  return Text(Get(response, "token"));
}

/// Hybrid search: dense vector + BM25 text, fused with RRF, optionally filtered to
/// one tenant. Returns the `hits` array of the response.
inline std::vector<Struct> HybridSearch(loams::Loams& client, const std::string& collection,
                                        const std::string& query, int dim, const std::string& tenant,
                                        int limit) {
  const std::string filter =
      tenant.empty() ? "" : ",\"filter\":{\"term\":{\"field\":\"tenant\",\"value\":" + Quote(tenant) + "}}";
  const Struct response = Call(
      client, "loams.collection.v1.QueryService/Search",
      std::string("{\"namespace\":") + Quote(kNamespace) + ",\"collection\":" + Quote(collection) +
          ",\"retrievers\":[{\"vector\":{\"field\":\"embedding\",\"query\":" + Floats(Embed(query, dim)) +
          ",\"k\":10}},{\"text\":{\"query\":{\"match\":{\"field\":\"body\",\"text\":" + Quote(query) +
          "}},\"k\":10}}],\"fusion\":{\"rrf\":{\"k\":60}}" + filter + ",\"limit\":" + std::to_string(limit) + "}",
      true);
  std::vector<Struct> hits;
  for (const Value& hit : Get(response, "hits").list_value().values()) hits.push_back(hit.struct_value());
  return hits;
}

inline std::string HitLine(const Struct& hit) {
  std::ostringstream out;
  out << "#" << Text(Get(Get(hit, "pk").struct_value(), "uint")) << "  score=" << std::fixed << std::setprecision(4)
      << Get(hit, "score").number_value() << "  tenant=" << Text(Get(Get(hit, "source").struct_value(), "tenant"))
      << "  " << Text(Get(Get(hit, "source").struct_value(), "body"));
  return out.str();
}

}  // namespace ex

#endif  // LOAMS_EXAMPLES_RAW_HPP
