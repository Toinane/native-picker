#pragma once

#define WIN32_LEAN_AND_MEAN

#include <napi.h>
#include <windows.h>
#include <wingdi.h>

class WinPicker {
    public:

    void Init(const Napi::CallbackInfo& info);

    private:

    BOOL PICKER_ACTIVATED;
    Napi::Function emit;
    std::string previousColor;
    int sizeGrid;

    void Test();

    void SendColor(Napi::Env env);
    std::string GetColor(int relativeX, int relativeY);
    void KeyEventProc(KEY_EVENT_RECORD ker, Napi::Env env);
    void MouseEventProc(MOUSE_EVENT_RECORD mer, Napi::Env env);
};