#pragma once

#include <filesystem>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>

namespace cpputils {
class FileContainer {
 private:
  std::map<std::string, std::string> data;
  const std::filesystem::path abs_path;
  std::mutex mtx;

  static std::string read_file(const std::filesystem::path& file_path);

 public:
  explicit FileContainer(std::string_view folder)
      : abs_path(std::filesystem::absolute(folder)) {}

  // Returns cache or reads file then returns content
  std::string GetFileContent(const std::string& filename);

  // Updates cache either to the value of content or by reading the file
  std::string SetFileContent(std::string filename,
                             std::optional<std::string> content = std::nullopt);
};
}  // namespace cpputils
