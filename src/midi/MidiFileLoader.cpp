#include "midi/MidiFileLoader.hpp"

#include <MidiFile.h>

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace {

constexpr int kPreviewNoteCount = 5;

void printSummary(const std::filesystem::path& path, const MidiTimeline& timeline) {
  std::cout << "MIDI file loaded\n";
  std::cout << "  path: " << path.string() << '\n';
  std::cout << "  tracks: " << timeline.trackCount() << '\n';
  std::cout << "  ticks per quarter note: " << timeline.ticksPerQuarterNote() << '\n';
  std::cout << "  parsed notes: " << timeline.notes().size() << '\n';
  std::cout << "  length seconds: " << timeline.lengthSeconds() << '\n';

  const auto previewCount = std::min(static_cast<int>(timeline.notes().size()), kPreviewNoteCount);
  for (int index = 0; index < previewCount; ++index) {
    const auto&[pitch, velocity, channel, track, startSeconds, durationSeconds] = timeline.notes()[static_cast<std::size_t>(index)];
    std::cout << "  note[" << index << "]:"
              << " pitch=" << pitch
              << " velocity=" << velocity
              << " channel=" << channel
              << " track=" << track
              << " start=" << startSeconds
              << " duration=" << durationSeconds
              << '\n';
  }
}

} // namespace

std::optional<MidiTimeline> MidiFileLoader::loadFromFile(const std::filesystem::path& path) {
  if (path.empty()) {
    std::cerr << "MIDI load failed: file path is empty.\n";
    return std::nullopt;
  }

  if (!std::filesystem::exists(path)) {
    std::cerr << "MIDI load skipped: file does not exist: " << path.string() << '\n';
    return std::nullopt;
  }

  if (!std::filesystem::is_regular_file(path)) {
    std::cerr << "MIDI load failed: path is not a regular file: " << path.string() << '\n';
    return std::nullopt;
  }

  smf::MidiFile midiFile;
  if (!midiFile.read(path.string())) {
    std::cerr << "MIDI load failed: could not parse file: " << path.string() << '\n';
    return std::nullopt;
  }

  midiFile.absoluteTicks();
  midiFile.sortTracksNoteOffsBeforeOns();
  midiFile.doTimeAnalysis();
  midiFile.linkNotePairsFIFO();

  MidiTimeline timeline;
  timeline.setTrackCount(midiFile.getTrackCount());
  timeline.setTicksPerQuarterNote(midiFile.getTicksPerQuarterNote());

  for (int track = 0; track < midiFile.getTrackCount(); ++track) {
    for (int eventIndex = 0; eventIndex < midiFile.getEventCount(track); ++eventIndex) {
      const auto& event = midiFile.getEvent(track, eventIndex);
      if (!event.isNoteOn() || !event.isLinked()) {
        continue;
      }

      const auto* linkedEvent = event.getLinkedEvent();
      if (linkedEvent == nullptr) {
        continue;
      }

      const auto startSeconds = event.seconds;
      const auto durationSeconds = linkedEvent->seconds - event.seconds;
      if (durationSeconds < 0.0) {
        std::cerr << "MIDI note ignored: linked note-off occurs before note-on"
                  << " track=" << track
                  << " pitch=" << event.getKeyNumber()
                  << '\n';
        continue;
      }

      timeline.addNote(Note{
        .pitch = event.getKeyNumber(),
        .velocity = event.getVelocity(),
        .channel = event.getChannel(),
        .track = track,
        .startSeconds = startSeconds,
        .durationSeconds = durationSeconds,
      });
    }
  }

  printSummary(path, timeline);
  return timeline;
}
