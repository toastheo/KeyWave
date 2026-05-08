#pragma once

struct Vec2
{
  double x = 0.0;
  double y = 0.0;
};

struct Rect
{
  double x = 0.0;
  double y = 0.0;
  double width = 0.0;
  double height = 0.0;
};

struct Color
{
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct FramebufferSize
{
  int width = 0;
  int height = 0;
};
