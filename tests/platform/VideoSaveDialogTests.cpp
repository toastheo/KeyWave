#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "platform/VideoSaveDialog.hpp"

namespace {
struct SaveDirectory
{
  std::filesystem::path path =
    std::filesystem::temp_directory_path() /
    ("keywave-save-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  SaveDirectory()
  {
    std::filesystem::create_directory(path);
  }
  ~SaveDirectory()
  {
    std::error_code error;
    std::filesystem::remove_all(path, error);
  }
};

TEST_CASE("Accepted native save selection records confirmed replacement", "[platform][export]")
{
  SaveDirectory const temp;
  const auto path = temp.path / std::filesystem::path(u8"Klavier ä 日本.mp4");
  {
    std::ofstream output(path);
    output << "old video";
  }
  const auto selection = VideoSaveDialog::selectionForConfirmedPath(path);
  CHECK(selection.path == path);
  CHECK(selection.allowOverwrite);
  CHECK(std::filesystem::file_size(path) == 9);
}

TEST_CASE("New native save selection does not authorize replacing a later file",
          "[platform][export]")
{
  SaveDirectory const temp;
  const auto path = temp.path / "new.mp4";
  const auto selection = VideoSaveDialog::selectionForConfirmedPath(path);
  CHECK(selection.path == path);
  CHECK_FALSE(selection.allowOverwrite);
}

TEST_CASE("Appending MP4 extension does not grant consent for a different existing file",
          "[platform][export]")
{
  SaveDirectory const temp;
  const auto path = temp.path / "song.mp4";
  {
    std::ofstream output(path);
    output << "old video";
  }
  const auto selection = VideoSaveDialog::selectionForConfirmedPath(temp.path / "song");
  CHECK(selection.path == path);
  CHECK_FALSE(selection.allowOverwrite);
  CHECK(std::filesystem::file_size(path) == 9);
}
} // namespace
