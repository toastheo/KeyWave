#pragma once

#include <nlohmann/json.hpp>

#include "app/AppSettings.hpp"

class AppSettingsSerializer
{
public:
  static nlohmann::json serialize(const AppSettings& settings);
  static AppSettings deserialize(const nlohmann::json& json, const AppSettings& defaults = {});
};
