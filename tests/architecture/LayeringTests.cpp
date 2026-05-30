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

TEST_CASE("Visualization targets do not publicly depend on app settings",
          "[architecture][layers]")
{
  const auto sourceRoot = std::filesystem::path{KEYWAVE_SOURCE_DIR};
  const auto appSettingsHeader =
    readTextFile(sourceRoot / "src" / "app" / "AppSettings.hpp");

  CHECK_FALSE(contains(appSettingsHeader, "#include \"midi/"));
  CHECK_FALSE(contains(appSettingsHeader, "#include \"render/"));

  for (const auto& header : std::vector{
         sourceRoot / "src" / "keyboard" / "KeyboardGeometry.hpp",
         sourceRoot / "src" / "keyboard" / "KeyboardRenderAdapter.hpp",
         sourceRoot / "src" / "fallingnotes" / "FallingNotesRenderAdapter.hpp",
         sourceRoot / "src" / "fallingnotes" / "FallingNotesSceneBuilder.hpp",
       }) {
    CAPTURE(header.string());
    CHECK_FALSE(contains(readTextFile(header), "#include \"app/AppSettings.hpp\""));
  }

  const auto srcCmakeLists = readTextFile(sourceRoot / "src" / "CMakeLists.txt");
  CHECK_FALSE(contains(targetLinkBlock(srcCmakeLists, "keywave_keyboard"),
                       "keywave_app_settings"));
  CHECK_FALSE(contains(targetLinkBlock(srcCmakeLists, "keywave_fallingnotes"),
                       "keywave_app_settings"));
}

} // namespace

