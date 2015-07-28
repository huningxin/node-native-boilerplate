/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

#include <nan.h>
#include <uv.h>
#include <stdio.h>
#include <vector>
#include <exception>

#include <DepthSense.hxx>

using namespace DepthSense;
using namespace std;

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

Context g_context;
DepthNode g_dnode;
ColorNode g_cnode;
AudioNode g_anode;

uint32_t g_aFrames = 0;
uint32_t g_cFrames = 0;
uint32_t g_dFrames = 0;

bool g_bDeviceFound = false;

ProjectionHelper* g_pProjHelper = NULL;
StereoCameraParameters g_scp;

static uv_thread_t thread_id;
static uv_async_t async;
static int cmd = 0;

/*----------------------------------------------------------------------------*/
// New audio sample event handler
void onNewAudioSample(AudioNode node, AudioNode::NewSampleReceivedData data)
{
    printf("A#%u: %d\n",g_aFrames,data.audioData.size());
    g_aFrames++;
}

/*----------------------------------------------------------------------------*/
// New color sample event handler
void onNewColorSample(ColorNode node, ColorNode::NewSampleReceivedData data)
{
    printf("C#%u: %d\n",g_cFrames,data.colorMap.size());
    g_cFrames++;
}

/*----------------------------------------------------------------------------*/
// New depth sample event handler
void onNewDepthSample(DepthNode node, DepthNode::NewSampleReceivedData data)
{
    printf("Z#%u: %d\n",g_dFrames,data.vertices.size());

    // Project some 3D points in the Color Frame
    if (!g_pProjHelper)
    {
        g_pProjHelper = new ProjectionHelper (data.stereoCameraParameters);
        g_scp = data.stereoCameraParameters;
    }
    else if (g_scp != data.stereoCameraParameters)
    {
        g_pProjHelper->setStereoCameraParameters(data.stereoCameraParameters);
        g_scp = data.stereoCameraParameters;
    }

    int32_t w, h;
    FrameFormat_toResolution(data.captureConfiguration.frameFormat,&w,&h);
    int cx = w/2;
    int cy = h/2;

    Vertex p3DPoints[4];

    p3DPoints[0] = data.vertices[(cy-h/4)*w+cx-w/4];
    p3DPoints[1] = data.vertices[(cy-h/4)*w+cx+w/4];
    p3DPoints[2] = data.vertices[(cy+h/4)*w+cx+w/4];
    p3DPoints[3] = data.vertices[(cy+h/4)*w+cx-w/4];
    
    Point2D p2DPoints[4];
    g_pProjHelper->get2DCoordinates ( p3DPoints, p2DPoints, 4, CAMERA_PLANE_COLOR);
    
    g_dFrames++;

    // Quit the main loop after 200 depth frames received
    if (g_dFrames == 200)
        g_context.quit();
}

/*----------------------------------------------------------------------------*/
void configureAudioNode()
{
    g_anode.newSampleReceivedEvent().connect(&onNewAudioSample);

    AudioNode::Configuration config = g_anode.getConfiguration();
    config.sampleRate = 44100;

    
    g_context.requestControl(g_anode,0);

    g_anode.setConfiguration(config);
    
    g_anode.setInputMixerLevel(0.5f);
}

/*----------------------------------------------------------------------------*/
void configureDepthNode()
{
    g_dnode.newSampleReceivedEvent().connect(&onNewDepthSample);

    DepthNode::Configuration config = g_dnode.getConfiguration();
    config.frameFormat = FRAME_FORMAT_QVGA;
    config.framerate = 25;
    config.mode = DepthNode::CAMERA_MODE_CLOSE_MODE;
    config.saturation = true;

    g_dnode.setEnableVertices(true);

    g_context.requestControl(g_dnode,0);

    g_dnode.setConfiguration(config);
}

/*----------------------------------------------------------------------------*/
void configureColorNode()
{
    // connect new color sample handler
    g_cnode.newSampleReceivedEvent().connect(&onNewColorSample);

    ColorNode::Configuration config = g_cnode.getConfiguration();
    config.frameFormat = FRAME_FORMAT_VGA;
    config.compression = COMPRESSION_TYPE_MJPEG;
    config.powerLineFrequency = POWER_LINE_FREQUENCY_50HZ;
    config.framerate = 25;

    g_cnode.setEnableColorMap(true);

    g_context.requestControl(g_cnode,0);

    g_cnode.setConfiguration(config);
}

/*----------------------------------------------------------------------------*/
void configureNode(Node node)
{
    if ((node.is<DepthNode>())&&(!g_dnode.isSet()))
    {
        g_dnode = node.as<DepthNode>();
        configureDepthNode();
        g_context.registerNode(node);
    }

    if ((node.is<ColorNode>())&&(!g_cnode.isSet()))
    {
        g_cnode = node.as<ColorNode>();
        configureColorNode();
        g_context.registerNode(node);
    }

    if ((node.is<AudioNode>())&&(!g_anode.isSet()))
    {
        g_anode = node.as<AudioNode>();
        configureAudioNode();
        g_context.registerNode(node);
    }
}

/*----------------------------------------------------------------------------*/
void onNodeConnected(Device device, Device::NodeAddedData data)
{
    configureNode(data.node);
}

/*----------------------------------------------------------------------------*/
void onNodeDisconnected(Device device, Device::NodeRemovedData data)
{
    if (data.node.is<AudioNode>() && (data.node.as<AudioNode>() == g_anode))
        g_anode.unset();
    if (data.node.is<ColorNode>() && (data.node.as<ColorNode>() == g_cnode))
        g_cnode.unset();
    if (data.node.is<DepthNode>() && (data.node.as<DepthNode>() == g_dnode))
        g_dnode.unset();
    printf("Node disconnected\n");
}

/*----------------------------------------------------------------------------*/
void onDeviceConnected(Context context, Context::DeviceAddedData data)
{
    if (!g_bDeviceFound)
    {
        data.device.nodeAddedEvent().connect(&onNodeConnected);
        data.device.nodeRemovedEvent().connect(&onNodeDisconnected);
        g_bDeviceFound = true;
    }
}

/*----------------------------------------------------------------------------*/
void onDeviceDisconnected(Context context, Context::DeviceRemovedData data)
{
    g_bDeviceFound = false;
    printf("Device disconnected\n");
}

static void thread_main(void* arg) {
  printf("thread_main\n");

  g_context = Context::create("localhost");

  g_context.deviceAddedEvent().connect(&onDeviceConnected);
  g_context.deviceRemovedEvent().connect(&onDeviceDisconnected);

  // Get the list of currently connected devices
  vector<Device> da = g_context.getDevices();

  // We are only interested in the first device
  if (da.size() >= 1)
  {
      g_bDeviceFound = true;

      da[0].nodeAddedEvent().connect(&onNodeConnected);
      da[0].nodeRemovedEvent().connect(&onNodeDisconnected);

      vector<Node> na = da[0].getNodes();
      
      printf("Found %u nodes\n",na.size());
      
      for (int n = 0; n < (int)na.size();n++)
          configureNode(na[n]);
  }

  g_context.startNodes();

  g_context.run();

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
