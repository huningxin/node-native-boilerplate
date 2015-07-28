/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>

class DepthCamera : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> exports);

 private:
  DepthCamera();
  ~DepthCamera();

  static NAN_METHOD(New);
  static NAN_METHOD(CallEmit);
  static v8::Persistent<v8::Function> constructor;
};

v8::Persistent<v8::Function> DepthCamera::constructor;

DepthCamera::DepthCamera() {
}

DepthCamera::~DepthCamera() {
}

void DepthCamera::Init(v8::Handle<v8::Object> exports) {
  NanScope();

  // Prepare constructor template
  v8::Local<v8::FunctionTemplate> tpl = NanNew<v8::FunctionTemplate>(New);
  tpl->SetClassName(NanNew<v8::String>("DepthCamera"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  NODE_SET_PROTOTYPE_METHOD(tpl, "call_emit", CallEmit);

  NanAssignPersistent<v8::Function>(constructor, tpl->GetFunction());
  exports->Set(NanNew<v8::String>("DepthCamera"), tpl->GetFunction());
}

NAN_METHOD(DepthCamera::New) {
  NanScope();

  if (args.IsConstructCall()) {
    DepthCamera* obj = new DepthCamera();
    obj->Wrap(args.This());
    NanReturnValue(args.This());
  } else {
    v8::Local<v8::Function> cons = NanNew<v8::Function>(constructor);
    NanReturnValue(cons->NewInstance());
  }
}

NAN_METHOD(DepthCamera::CallEmit) {
  NanScope();
  v8::Handle<v8::Value> argv[1] = {
    NanNew("event"),  // event name
  };

  NanMakeCallback(args.This(), "emit", 1, argv);
  NanReturnUndefined();
}

void Init(v8::Handle<v8::Object> exports) {
  DepthCamera::Init(exports);
}

NODE_MODULE(DepthCamera, Init)
