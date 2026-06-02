#pragma once

#include <cstdint>
#include <iosfwd>
#include <string_view>

enum class DiagnosticSeverity : std::uint8_t
{
  Info,
  Warning,
  Error,
};

class DiagnosticSink
{
public:
  virtual ~DiagnosticSink() = default;

  virtual void report(DiagnosticSeverity severity, std::string_view message) = 0;
};

class NullDiagnosticSink final : public DiagnosticSink
{
public:
  void report(DiagnosticSeverity severity, std::string_view message) override;
};

struct OstreamDiagnosticStreams
{
  std::ostream& info;
  std::ostream& warningError;
};

class OstreamDiagnosticSink final : public DiagnosticSink
{
public:
  explicit OstreamDiagnosticSink(OstreamDiagnosticStreams streams);

  void report(DiagnosticSeverity severity, std::string_view message) override;

private:
  std::ostream& m_infoStream;
  std::ostream& m_warningErrorStream;
};

[[nodiscard]] DiagnosticSink& nullDiagnosticSink();
[[nodiscard]] DiagnosticSink& consoleDiagnosticSink();

void reportDiagnostic(DiagnosticSink& diagnostics,
                      DiagnosticSeverity severity,
                      std::string_view message);
void reportInfo(DiagnosticSink& diagnostics, std::string_view message);
void reportWarning(DiagnosticSink& diagnostics, std::string_view message);
void reportError(DiagnosticSink& diagnostics, std::string_view message);
