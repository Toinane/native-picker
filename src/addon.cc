#include <iostream>
#include "addon.h"

Napi::Value addon::Init(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  Napi::Function emit = info[0].As<Napi::Function>();
  Napi::Object pickerParams = info[1].As<Napi::Object>();

  emit.Call({
    Napi::String::New(env, "start")
  });

  // start picker here.

  std::string color = (std::string) pickerParams.Get("previousColor").ToString();

  emit.Call({
    Napi::String::New(env, "update"),
    Napi::String::New(env, color)
  });

  Picker(NULL, NULL, NULL, 1);

  // end picker here.

  emit.Call({
    Napi::String::New(env, "end")
  });

  return Napi::Boolean::New(env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(
    Napi::String::New(env, "init"),
    Napi::Function::New(env, addon::Init)
  );

  return exports;
}

NODE_API_MODULE(picker, Init)