#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "diagnostics/Diagnostics.hpp"

struct RecordedDiagnostic
{
  DiagnosticSeverity severity = DiagnosticSeverity::Info;
  std::string message;
};

class RecordingDiagnosticSink final : public DiagnosticSink
{
public:
  void report(const DiagnosticSeverity severity, const std::string_view message) override
  {
    messages.push_back(RecordedDiagnostic{
      .severity = severity,
      .message = std::string{message},
    });
  }

  std::vector<RecordedDiagnostic> messages;
};
