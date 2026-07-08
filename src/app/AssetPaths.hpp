#pragma once

#include <filesystem>

[[nodiscard]] std::filesystem::path keywaveAssetPath(const std::filesystem::path& relativePath);
[[nodiscard]] std::filesystem::path defaultPianoSoundFontPath();
