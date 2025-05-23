#include "file_container.h"

#include <fstream>
#include <sstream>
#include <string>

namespace cpputils {

std::string FileContainer::read_file(const std::filesystem::path& file_path) {
  std::ifstream file(
      file_path,
      std::ios::in |
          std::ios::binary);  // Open in binary mode to preserve all bytes
  if (!file.is_open()) {
    return "";
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

std::string FileContainer::GetFileContent(const std::string& filename) {
  std::lock_guard<std::mutex> lock(mtx);
  // Check cache first
  auto it = data.find(filename);
  if (it != data.end()) {
    return it->second;
  }

  std::filesystem::path file_path = abs_path / filename;
  std::string content = read_file(file_path);
  data[filename] = content;
  return content;
}

std::string FileContainer::SetFileContent(std::string filename,
                                          std::optional<std::string> content) {
  std::lock_guard<std::mutex> lock(mtx);
  if (content) {
    data[filename] = *content;
    return *content;
  }

  std::filesystem::path file_path = abs_path / filename;
  std::string file_content = read_file(file_path);
  return data[filename] = file_content;
  return file_content;
}

}  // namespace cpputils
