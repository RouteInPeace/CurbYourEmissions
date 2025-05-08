#pragma once

#include <filesystem>
#include <vector>
#include <memory>
#include "rapidjson/document.h"

namespace serial {

class JSONArchive {
 public:
  JSONArchive(std::filesystem::path path);

  auto operator[](std::string_view name) -> JSONArchive;

  template <typename T>
  auto get(std::string_view name) -> T = delete;
  
  template <typename T> requires std::is_same_v<T, std::vector<typename T::value_type>>
  auto get(std::string_view name) -> T;

 private:
  JSONArchive(std::shared_ptr<rapidjson::Document> document_, rapidjson::Value *value_);

  std::shared_ptr<rapidjson::Document> document_;
  rapidjson::Value *value_;
};

}  // namespace serial