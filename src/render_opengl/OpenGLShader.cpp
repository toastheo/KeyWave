#include "render_opengl/OpenGLShader.hpp"

#include <glad/glad.h>

#include <cstddef>
#include <sstream>
#include <string>

namespace {

std::string shaderInfoLog(const unsigned int shader)
{
  int logLength = 0;
  glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength <= 1) {
    return {};
  }

  std::string log(static_cast<std::size_t>(logLength), '\0');
  glGetShaderInfoLog(shader, logLength, nullptr, log.data());
  return log;
}

std::string programInfoLog(const unsigned int program)
{
  int logLength = 0;
  glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
  if (logLength <= 1) {
    return {};
  }

  std::string log(static_cast<std::size_t>(logLength), '\0');
  glGetProgramInfoLog(program, logLength, nullptr, log.data());
  return log;
}

unsigned int compileShader(const unsigned int shaderType,
                           const std::string_view source,
                           DiagnosticSink& diagnostics)
{
  const auto shader = glCreateShader(shaderType);
  const char* sourceData = source.data();
  const auto sourceLength = static_cast<int>(source.size());
  glShaderSource(shader, 1, &sourceData, &sourceLength);
  glCompileShader(shader);

  int compileStatus = GL_FALSE;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compileStatus);
  if (compileStatus != GL_TRUE) {
    reportError(diagnostics,
                "OpenGL shader compilation failed:\n" + shaderInfoLog(shader));
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

} // namespace

OpenGLShader::OpenGLShader(DiagnosticSink& diagnostics)
    : m_diagnostics(&diagnostics)
{}

OpenGLShader::~OpenGLShader()
{
  destroy();
}

bool OpenGLShader::create(const OpenGLShaderSources& sources)
{
  destroy();

  const auto vertexShader = compileShader(GL_VERTEX_SHADER, sources.vertex, *m_diagnostics);
  if (vertexShader == 0) {
    return false;
  }

  const auto fragmentShader = compileShader(GL_FRAGMENT_SHADER, sources.fragment, *m_diagnostics);
  if (fragmentShader == 0) {
    glDeleteShader(vertexShader);
    return false;
  }

  const auto program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  glDetachShader(program, vertexShader);
  glDetachShader(program, fragmentShader);
  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  int linkStatus = GL_FALSE;
  glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
  if (linkStatus != GL_TRUE) {
    reportError(*m_diagnostics, "OpenGL shader link failed:\n" + programInfoLog(program));
    glDeleteProgram(program);
    return false;
  }

  m_program = program;
  return true;
}

void OpenGLShader::destroy()
{
  if (m_program != 0) {
    glDeleteProgram(m_program);
    m_program = 0;
  }
}

unsigned int OpenGLShader::id() const
{
  return m_program;
}

bool OpenGLShader::valid() const
{
  return m_program != 0;
}
