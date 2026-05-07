#pragma once

#include <string_view>

struct OpenGLShaderSources
{
  std::string_view vertex;
  std::string_view fragment;
};

class OpenGLShader final
{
public:
  OpenGLShader() = default;
  ~OpenGLShader();

  OpenGLShader(const OpenGLShader&) = delete;
  OpenGLShader& operator=(const OpenGLShader&) = delete;

  [[nodiscard]] bool create(const OpenGLShaderSources& sources);
  void destroy();

  [[nodiscard]] unsigned int id() const;
  [[nodiscard]] bool valid() const;

private:
  unsigned int m_program = 0;
};
