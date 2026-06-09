#version 330 core

in vec2 rectLocalPosition;
in vec4 rectTopColor;
in vec4 rectBottomColor;
in vec4 rectBorderColor;
in float rectBorderThicknessPixels;
in vec2 rectSizePixels;
in vec4 rectCornerRadiiPixels;

out vec4 outColor;

void main()
{
  // Vertical fill gradient.
  vec4 fillColor = mix(rectBottomColor, rectTopColor, rectLocalPosition.y);

  // Work in pixel space so radii and border thickness behave consistently.
  vec2 pixelPosition = rectLocalPosition * rectSizePixels;
  vec2 halfSize = rectSizePixels * 0.5;

  // Select per-corner radius:
  // x = top-left, y = top-right,
  // z = bottom-right, w = bottom-left.
  bool isRightSide = pixelPosition.x > halfSize.x;
  bool isTopSide = pixelPosition.y > halfSize.y;

  float selectedRadius = isTopSide
  ? (isRightSide ? rectCornerRadiiPixels.y
  : rectCornerRadiiPixels.x)
  : (isRightSide ? rectCornerRadiiPixels.z
  : rectCornerRadiiPixels.w);

  float radius = clamp(selectedRadius, 0.0, min(halfSize.x, halfSize.y));

  // Signed distance to rounded rectangle.
  // Negative = inside, zero = edge, positive = outside.
  vec2 cornerSize = halfSize - vec2(radius);
  vec2 cornerDistance = abs(pixelPosition - halfSize) - cornerSize;

  float signedDistance =
  length(max(cornerDistance, vec2(0.0))) + min(max(cornerDistance.x, cornerDistance.y), 0.0) -
  radius;

  // Smooth the outer edge using screen-space derivatives.
  float antialiasWidth = max(fwidth(signedDistance), 0.5);
  float shapeAlpha = 1.0 - smoothstep(0.0, antialiasWidth, signedDistance);

  if (shapeAlpha <= 0.0) {
    discard;
  }

  // Border is measured inward from the outer edge.
  float borderMix = 0.0;
  if (rectBorderThicknessPixels > 0.0) {
    float innerBorderDistance = -rectBorderThicknessPixels;
    borderMix =
    smoothstep(innerBorderDistance - antialiasWidth,
               innerBorderDistance + antialiasWidth,
               signedDistance);
  }

  outColor = mix(fillColor, rectBorderColor, borderMix);
  outColor.a *= shapeAlpha;
}
