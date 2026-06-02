#version 330 core

in vec2 rectLocalPosition;
in vec4 rectTopColor;
in vec4 rectBottomColor;
in vec4 rectBorderColor;
in float rectBorderThicknessPixels;
in vec2 rectSizePixels;

out vec4 outColor;

void main()
{
  vec4 fillColor = mix(rectBottomColor, rectTopColor, rectLocalPosition.y);
  vec2 pixelPosition = rectLocalPosition * rectSizePixels;
  vec2 edgeDistance = min(pixelPosition, rectSizePixels - pixelPosition);
  float nearestEdgeDistance = min(edgeDistance.x, edgeDistance.y);
  float borderMix = rectBorderThicknessPixels > 0.0 &&
                            nearestEdgeDistance <= rectBorderThicknessPixels
                          ? 1.0
                          : 0.0;
  outColor = mix(fillColor, rectBorderColor, borderMix);
}
