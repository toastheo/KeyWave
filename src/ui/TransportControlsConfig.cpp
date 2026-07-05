#include "ui/TransportControlsConfig.hpp"

TransportControlsWindowFlags transportControlsWindowFlags()
{
  return transportControlsWindowFlagValue(TransportControlsWindowFlag::NoDecoration) |
         transportControlsWindowFlagValue(TransportControlsWindowFlag::AlwaysAutoResize) |
         transportControlsWindowFlagValue(TransportControlsWindowFlag::NoSavedSettings) |
         transportControlsWindowFlagValue(TransportControlsWindowFlag::NoMove) |
         transportControlsWindowFlagValue(TransportControlsWindowFlag::NoFocusOnAppearing) |
         transportControlsWindowFlagValue(TransportControlsWindowFlag::NoNavFocus);
}
