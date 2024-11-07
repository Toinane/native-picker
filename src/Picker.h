#pragma once

#include <string>
#include <napi.h>

#ifdef _WIN32
    #include "./windows/WinPicker.h"
#endif

namespace picker {
    Napi::Value Init(const Napi::CallbackInfo& info);
    COLORREF GetColor();
}

Napi::Object Init(Napi::Env env, Napi::Object exports);