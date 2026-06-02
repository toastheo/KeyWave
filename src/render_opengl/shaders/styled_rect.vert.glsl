#version 330 core

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec2 inLocalPosition;
layout (location = 2) in vec4 inTopColor;
layout (location = 3) in vec4 inBottomColor;
layout (location = 4) in vec4 inBorderColor;
layout (location = 5) in float inBorderThicknessPixels;
layout (location = 6) in vec2 inRectSizePixels;
layout (location = 7) in vec4 inCornerRadiiPixels;

out vec2 rectLocalPosition;
out vec4 rectTopColor;
out vec4 rectBottomColor;
out vec4 rectBorderColor;
out float rectBorderThicknessPixels;
out vec2 rectSizePixels;
out vec4 rectCornerRadiiPixels;

void main()
{
  rectLocalPosition = inLocalPosition;
  rectTopColor = inTopColor;
  rectBottomColor = inBottomColor;
  rectBorderColor = inBorderColor;
  rectBorderThicknessPixels = inBorderThicknessPixels;
  rectSizePixels = inRectSizePixels;
  rectCornerRadiiPixels = inCornerRadiiPixels;
  gl_Position = vec4(inPosition, 0.0, 1.0);
}
