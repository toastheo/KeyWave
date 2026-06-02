#version 330 core

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec4 inColor;

out vec4 vertexColor;

void main()
{
  vertexColor = inColor;
  gl_Position = vec4(inPosition, 0.0, 1.0);
}
