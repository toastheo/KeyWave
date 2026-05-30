#include "midi/MidiFileLoader.hpp"

#include <MidiFile.h>

#include <filesystem>
#include <sstream>

std::optional<MidiTimeline> MidiFileLoader::loadFromFile(const std::filesystem::path& path,
                                                         DiagnosticSink& diagnostics) {
  if (path.empty()) {
    reportError(diagnostics, "MIDI load failed: file path is empty.");
    return std::nullopt;
  }

  if (!std::filesystem::exists(path)) {
    reportWarning(diagnostics, "MIDI load skipped: file does not exist: " + path.string());
    return std::nullopt;
  }

  if (!std::filesystem::is_regular_file(path)) {
    reportError(diagnostics, "MIDI load failed: path is not a regular file: " + path.string());
    return std::nullopt;
  }

  // Keep smf::MidiFile local to this loader.
  // craigsapp/midifile has known copy/move assignment issues with raw-owned event lists:
  // https://github.com/craigsapp/midifile/issues/69
  // Extract values into MidiTimeline instead of storing or exposing smf::MidiFile.
  smf::MidiFile midiFile;
  if (!midiFile.read(path.string())) {
    reportError(diagnostics, "MIDI load failed: could not parse file: " + path.string());
    return std::nullopt;
  }

  // Prepare smf::MidiFile for reliable note extraction: normalize timing, order same-tick
  // note-offs before note-ons, compute seconds, then link each note-on to its FIFO note-off.
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
        std::ostringstream message;
        message << "MIDI note ignored: linked note-off occurs before note-on"
                << " track=" << track
                << " pitch=" << event.getKeyNumber();
        reportWarning(diagnostics, message.str());
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
