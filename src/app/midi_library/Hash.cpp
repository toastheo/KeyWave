#include "app/midi_library/Hash.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <optional>
#include <sstream>
#include <string>

#include "diagnostics/Diagnostics.hpp"

namespace midi_library {
namespace {

// These are the standard published constants for the 64-bit Fowler–Noll–Vo hash family:
// fnvOffsetBasis is the standard starting value.
// fnvPrime is the standard multiplier.
// This is enough here because the hash is a stable content identifier, not a security boundary.
constexpr std::uint64_t fnvOffsetBasis = 14695981039346656037ull;
constexpr std::uint64_t fnvPrime = 1099511628211ull;

std::string lowercaseHex(const std::uint64_t value)
{
  std::ostringstream output;
  output << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << value;
  return output.str();
}

} // namespace

std::string lowercaseHexSize(const std::uintmax_t value)
{
  std::ostringstream output;
  output << std::hex << std::nouppercase << value;
  return output.str();
}

std::optional<FileSignature> computeSignature(const std::filesystem::path& path,
                                              DiagnosticSink& diagnostics)
{
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    reportWarning(diagnostics, "Warning: could not open MIDI file for import: " + path.string());
    return std::nullopt;
  }

  std::uint64_t hash = fnvOffsetBasis;
  std::uintmax_t size = 0;
  // Hash in chunks because midi files can be very large, and we don't want them fully into memory.
  std::array<char, 8192> buffer{}; // 8 KiB chunk size
  while (input) {
    input.read(buffer.data(), buffer.size());
    const auto bytesRead = input.gcount();
    for (std::streamsize index = 0; index < bytesRead; ++index) {
      hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
      hash *= fnvPrime;
    }
    size += static_cast<std::uintmax_t>(bytesRead);
  }

  if (!input.eof()) {
    reportWarning(diagnostics, "Warning: could not read MIDI file for import: " + path.string());
    return std::nullopt;
  }

  return FileSignature{
    .contentHash = lowercaseHex(hash),
    .sizeBytes = size,
  };
}

bool sameFileBytes(const FileComparisonPaths& paths, DiagnosticSink& diagnostics)
{
  std::ifstream leftInput(paths.source, std::ios::binary);
  std::ifstream rightInput(paths.importedCopy, std::ios::binary);
  if (!leftInput || !rightInput) {
    return false;
  }

  // We compare here in chunks again: Same reason as for the signature hashing.
  std::array<char, 8192> leftBuffer{};  // 8 KiB
  std::array<char, 8192> rightBuffer{}; // 8 KiB
  while (leftInput && rightInput) {
    leftInput.read(leftBuffer.data(), leftBuffer.size());
    rightInput.read(rightBuffer.data(), rightBuffer.size());
    const auto leftCount = leftInput.gcount();
    const auto rightCount = rightInput.gcount();
    if (leftCount != rightCount) {
      return false;
    }
    if (!std::equal(leftBuffer.begin(),
                    leftBuffer.begin() + leftCount,
                    rightBuffer.begin(),
                    rightBuffer.begin() + rightCount)) {
      return false;
    }
  }

  if (!leftInput.eof() || !rightInput.eof()) {
    reportWarning(diagnostics, "Warning: could not compare imported MIDI file bytes.");
    return false;
  }

  return true;
}

} // namespace midi_library
