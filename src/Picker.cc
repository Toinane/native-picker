#pragma once

#include "Picker.h"

using namespace Napi;

Value picker::Init(const CallbackInfo& info) {
    Env env = info.Env();
    Function emit = info[0].As<Function>();

    #ifdef _WIN32
        WinPicker picker;
        picker.Init(info);
    #endif

    return Boolean::New(env, true);
}

Object Init(Env env, Object exports) {
    exports.Set(
        String::New(env, "init"),
        Function::New(env, picker::Init)
    );

    return exports;
}

NODE_API_MODULE(picker, Init)