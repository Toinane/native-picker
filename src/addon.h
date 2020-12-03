#pragma once
#include <napi.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <Windows.h>
  #include "windows/Picker.h"
#endif

namespace addon {
    Napi::Value Init(const Napi::CallbackInfo& info);
}

Napi::Object Init(Napi::Env env, Napi::Object exports);