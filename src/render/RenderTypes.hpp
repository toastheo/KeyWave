#pragma once

#include "core/CoreTypes.hpp"

using NativeProcAddress = void (*)();
using NativeProcAddressLoader = NativeProcAddress (*)(const char* name);
