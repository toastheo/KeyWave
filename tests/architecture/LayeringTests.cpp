#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string readTextFile(const std::filesystem::path& path)
{
  std::ifstream const input(path);
  REQUIRE(input.good());

  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

bool contains(const std::string_view haystack, const std::string_view needle)
{
  return haystack.find(needle) != std::string_view::npos;
}

std::string targetLinkBlock(const std::string& cmakeLists, const std::string& targetName)
{
  const auto start = cmakeLists.find("target_link_libraries(" + targetName);
  REQUIRE(start != std::string::npos);

  const auto end = cmakeLists.find(')', start);
  REQUIRE(end != std::string::npos);
  return cmakeLists.substr(start, end - start);
}

std::vector<std::filesystem::path> sourceFilesUnder(const std::filesystem::path& root)
{
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
    if (!entry.is_regular_file()) {
      continue;
    }

    const auto extension = entry.path().extension();
    if (extension == ".cpp" || extension == ".hpp") {
      files.push_back(entry.path());
    }
  }

  return files;
}

TEST_CASE("Visualization targets do not publicly depend on app settings", "[architecture][layers]")
{
  const auto sourceRoot = std::filesystem::path{KEYWAVE_SOURCE_DIR};
  const auto appSettingsHeader = readTextFile(sourceRoot / "src" / "app" / "AppSettings.hpp");

  CHECK_FALSE(contains(appSettingsHeader, "#include \"midi/"));
  CHECK_FALSE(contains(appSettingsHeader, "#include \"render/"));

  for (const auto& header : std::vector{
         sourceRoot / "src" / "keyboard" / "KeyboardGeometry.hpp",
         sourceRoot / "src" / "keyboard" / "KeyboardRenderAdapter.hpp",
         sourceRoot / "src" / "fallingnotes" / "FallingNotesRenderAdapter.hpp",
         sourceRoot / "src" / "fallingnotes" / "PianoRollSceneBuilder.hpp",
       }) {
    CAPTURE(header.string());
    CHECK_FALSE(contains(readTextFile(header), "#include \"app/AppSettings.hpp\""));
  }

  const auto srcCmakeLists = readTextFile(sourceRoot / "src" / "CMakeLists.txt");
  CHECK_FALSE(contains(targetLinkBlock(srcCmakeLists, "keywave_keyboard"), "keywave_app_settings"));
  CHECK_FALSE(
    contains(targetLinkBlock(srcCmakeLists, "keywave_fallingnotes"), "keywave_app_settings"));
}

TEST_CASE("First-party code routes diagnostics through the diagnostics boundary",
          "[architecture][diagnostics]")
{
  const auto sourceRoot = std::filesystem::path{KEYWAVE_SOURCE_DIR};

  for (const auto& file : sourceFilesUnder(sourceRoot / "src")) {
    const auto relativePath = std::filesystem::relative(file, sourceRoot).generic_string();
    if (relativePath == "src/diagnostics/Diagnostics.cpp") {
      continue;
    }

    CAPTURE(relativePath);
    const auto contents = readTextFile(file);
    CHECK_FALSE(contains(contents, "std::cout"));
    CHECK_FALSE(contains(contents, "std::cerr"));
  }
}

TEST_CASE("Ostream diagnostics use named streams instead of adjacent stream parameters",
          "[architecture][diagnostics]")
{
  const auto sourceRoot = std::filesystem::path{KEYWAVE_SOURCE_DIR};
  const auto diagnosticsHeader =
    readTextFile(sourceRoot / "src" / "diagnostics" / "Diagnostics.hpp");

  CHECK_FALSE(contains(diagnosticsHeader,
                       "OstreamDiagnosticSink(std::ostream& infoStream, "
                       "std::ostream& warningErrorStream)"));
}

} // namespace
