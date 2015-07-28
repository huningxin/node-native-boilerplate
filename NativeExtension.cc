/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>
#include <uv.h>

class DepthCamera : public node::ObjectWrap {
 public:
  static void Init(v8::Handle<v8::Object> exports);

 private:
  DepthCamera();
  ~DepthCamera();

  static NAN_METHOD(New);
  static NAN_METHOD(Start);
  static NAN_METHOD(Stop);
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
  NODE_SET_PROTOTYPE_METHOD(tpl, "start", Start);
  NODE_SET_PROTOTYPE_METHOD(tpl, "stop", Stop);

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

static uv_thread_t thread_id;
static uv_async_t async;
static int cmd = 0;

static void thread_main(void* arg) {
  printf("thread_main\n");
  cmd = 1;
  async.data = (void*) &cmd;
  uv_async_send(&async);
}

v8::Persistent<v8::Function> startCallback;

void handle_async_send(uv_async_t *handle, int) {
    int data = *((int*) handle->data);
    printf("handle_async_send %d\n", data);
    switch (data) {
      case 1: {
        // start success
        v8::Handle<v8::Value> arg = NanNew("success");
        NanMakeCallback(NanGetCurrentContext()->Global(), startCallback, 1, &arg);
        break;
      }
      case 2:
        // stop success
        break;
    }
}

NAN_METHOD(DepthCamera::Start) {
  NanScope();
  NanAssignPersistent<v8::Function>(startCallback, args[0].As<v8::Function>());

  uv_async_init(uv_default_loop(), &async, handle_async_send);

  uv_thread_create(&thread_id, thread_main, NULL);

  NanReturnUndefined();
}

NAN_METHOD(DepthCamera::Stop) {
  NanScope();
  v8::Local<v8::Function> callbackHandle = args[0].As<v8::Function>();

  uv_thread_join(&thread_id);

  v8::Handle<v8::Value> arg = NanNew("success");
  NanMakeCallback(NanGetCurrentContext()->Global(), callbackHandle, 1, &arg);
  NanReturnUndefined(); 
}

void Init(v8::Handle<v8::Object> exports) {
  DepthCamera::Init(exports);
}

NODE_MODULE(DepthCamera, Init)
