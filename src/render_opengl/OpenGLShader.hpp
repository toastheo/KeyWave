#pragma once

#include <string_view>

#include "diagnostics/Diagnostics.hpp"

struct OpenGLShaderSources
{
  std::string_view vertex;
  std::string_view fragment;
};

class OpenGLShader final
{
public:
  explicit OpenGLShader(DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~OpenGLShader();

  OpenGLShader(const OpenGLShader&) = delete;
  OpenGLShader& operator=(const OpenGLShader&) = delete;

  [[nodiscard]] bool create(const OpenGLShaderSources& sources);
  void destroy();

  [[nodiscard]] unsigned int id() const;
  [[nodiscard]] bool valid() const;

private:
  DiagnosticSink* m_diagnostics = nullptr;
  unsigned int m_program = 0;
};
