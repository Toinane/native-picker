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

Napi::String N_GetHexPixelColor(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  MMSignedPoint pos;

  if(info[0].IsNumber() && info[1].IsNumber()) {
    pos.x = info[0].As<Napi::Number>().Int32Value();
    pos.y = info[1].As<Napi::Number>().Int32Value();
  } else {
    pos = GetMousePos();
  }

  Napi::String hexColor = Napi::String::New(env, GetHexPixelColor(pos));

  return hexColor;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("getMousePos", Napi::Function::New(env, N_GetMousePos));
  exports.Set("getPixelColor", Napi::Function::New(env, N_GetHexPixelColor));

  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)