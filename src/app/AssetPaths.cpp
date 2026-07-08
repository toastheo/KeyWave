#include "app/AssetPaths.hpp"

#include <filesystem>

#ifndef KEYWAVE_ASSET_DIR
#define KEYWAVE_ASSET_DIR "assets"
#endif

std::filesystem::path keywaveAssetPath(const std::filesystem::path& relativePath)
{
  return std::filesystem::path{KEYWAVE_ASSET_DIR} / relativePath;
}

std::filesystem::path defaultPianoSoundFontPath()
{
  return keywaveAssetPath(std::filesystem::path{"soundfonts"} / "default.sf2");
}
