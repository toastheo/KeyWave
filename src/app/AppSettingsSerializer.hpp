#pragma once

#include <string>
#include <string_view>

#include "app/AppSettings.hpp"

class AppSettingsSerializer
{
public:
  static std::string serialize(const AppSettings& settings, int indentation = -1);
  static AppSettings deserialize(std::string_view text, const AppSettings& defaults = {});
};
