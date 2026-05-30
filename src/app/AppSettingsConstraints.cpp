#include "app/AppSettingsConstraints.hpp"

const AppSettingsConstraints& appSettingsConstraints()
{
  static constexpr AppSettingsConstraints constraints;
  return constraints;
}
