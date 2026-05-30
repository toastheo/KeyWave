#include "diagnostics/Diagnostics.hpp"

#include <iostream>
#include <ostream>

void NullDiagnosticSink::report(DiagnosticSeverity, std::string_view) {}

OstreamDiagnosticSink::OstreamDiagnosticSink(const OstreamDiagnosticStreams streams)
    : m_infoStream(&streams.info)
    , m_warningErrorStream(&streams.warningError)
{}

void OstreamDiagnosticSink::report(const DiagnosticSeverity severity,
                                   const std::string_view message)
{
  auto& stream = severity == DiagnosticSeverity::Info ? *m_infoStream : *m_warningErrorStream;
  stream << message;
  if (message.empty() || message.back() != '\n') {
    stream << '\n';
  }
}

DiagnosticSink& nullDiagnosticSink()
{
  static NullDiagnosticSink sink;
  return sink;
}

DiagnosticSink& consoleDiagnosticSink()
{
  static OstreamDiagnosticSink sink(OstreamDiagnosticStreams{
    .info = std::cout,
    .warningError = std::cerr,
  });
  return sink;
}

void reportDiagnostic(DiagnosticSink& diagnostics,
                      const DiagnosticSeverity severity,
                      const std::string_view message)
{
  diagnostics.report(severity, message);
}

void reportInfo(DiagnosticSink& diagnostics, const std::string_view message)
{
  reportDiagnostic(diagnostics, DiagnosticSeverity::Info, message);
}

void reportWarning(DiagnosticSink& diagnostics, const std::string_view message)
{
  reportDiagnostic(diagnostics, DiagnosticSeverity::Warning, message);
}

void reportError(DiagnosticSink& diagnostics, const std::string_view message)
{
  reportDiagnostic(diagnostics, DiagnosticSeverity::Error, message);
}
