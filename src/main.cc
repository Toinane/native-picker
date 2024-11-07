#include "main.h"

using namespace Napi;

Napi::Object N_GetMousePos(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Object mousePos = Napi::Object::New(env);

  MMSignedPoint pos = GetMousePos();

  mousePos.Set("x", Napi::Number::New(env, pos.x));
  mousePos.Set("y", Napi::Number::New(env, pos.y));

  return mousePos;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getMousePos", Napi::Function::New(env, N_GetMousePos));

  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)