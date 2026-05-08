#include "midi/MidiFileLoader.hpp"

#include <MidiFile.h>

#include <filesystem>
#include <iostream>

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

  return timeline;
}
