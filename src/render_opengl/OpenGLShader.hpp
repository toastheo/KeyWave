#pragma once

#include <filesystem>
#include <string_view>

#include "diagnostics/Diagnostics.hpp"

struct OpenGLShaderFilePaths
{
  std::filesystem::path vertex;
  std::filesystem::path fragment;
};

class OpenGLShader final
{
public:
  explicit OpenGLShader(DiagnosticSink& diagnostics = nullDiagnosticSink());
  ~OpenGLShader();

  OpenGLShader(const OpenGLShader&) = delete;
  OpenGLShader& operator=(const OpenGLShader&) = delete;

  [[nodiscard]] bool createFromFiles(const OpenGLShaderFilePaths& paths);
  void destroy();

  [[nodiscard]] unsigned int id() const;
  [[nodiscard]] bool valid() const;

private:
  struct Sources
  {
    std::string_view vertex;
    std::string_view fragment;
  };

  [[nodiscard]] bool create(const Sources& sources);

  DiagnosticSink& m_diagnostics;
  unsigned int m_program = 0;
};
