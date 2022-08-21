#ifndef _H_GG_OPENXR
#define _H_GG_OPENXR

namespace wiGraphics
{
	struct Texture;
	class GraphicsDevice;
}

typedef enum OpenXRRenderSide
{
	OPENXR_RENDER_UNDEFINED = -1,
	OPENXR_RENDER_LEFT = 0,
	OPENXR_RENDER_RIGHT = 1
} OpenXRRenderSide;

int  OpenXRInit( wiGraphics::GraphicsDevice* device );
void OpenXRStartSession();
void OpenXREndSession();
void OpenXRDestroy();
void OpenXRHandleEvents();

const char* OpenXRGetRuntimeName();

bool OpenXRIsInitialised();
bool OpenXRIsSessionSetup();
bool OpenXRIsSessionActive();
void OpenXRGetViewMat( OpenXRRenderSide side, float outMat[16] );
void OpenXRGetInvViewMat( OpenXRRenderSide side, float outMat[16] );
void OpenXRGetProjMat( OpenXRRenderSide side, float fNear, float fFar, float outMat[16], int invertedDepth=1 );
void OpenXRGetResolution( OpenXRRenderSide side, int* width, int* height );

#define OPENXR_CONTROLLER_UNKNOWN       0
#define OPENXR_CONTROLLER_OCULUS_TOUCH  1
#define OPENXR_CONTROLLER_HTC_VIVE      2
#define OPENXR_CONTROLLER_VALVE_INDEX   3
#define OPENXR_CONTROLLER_WMR           4

// controller functions
bool OpenXRHasLeftAim();
bool OpenXRHasLeftStick();
bool OpenXRHasLeftTouchPad();
bool OpenXRHasLeftTrigger();
bool OpenXRHasLeftSqueeze();
bool OpenXRHasLeftButtonA();
bool OpenXRHasLeftButtonB();

int   OpenXRGetLeftHandActive();
int   OpenXRGetLeftHandControllerType();
void  OpenXRGetLeftHandPos( float* x, float* y, float* z );
void  OpenXRGetLeftHandQuat( float* w, float* x, float* y, float* z );
void  OpenXRGetLeftHandMatrix( float* m, float scale );
void  OpenXRGetLeftHandAimPos( float* x, float* y, float* z );
void  OpenXRGetLeftHandAimQuat( float* w, float* x, float* y, float* z );
void  OpenXRGetLeftHandAimMatrix( float* m, float scale );
float OpenXRGetLeftStickX();
float OpenXRGetLeftStickY();
bool  OpenXRGetLeftStickClick();
bool  OpenXRGetLeftStickTouch();
float OpenXRGetLeftTouchPadX();
float OpenXRGetLeftTouchPadY();
bool  OpenXRGetLeftTouchPadClick();
bool  OpenXRGetLeftTouchPadTouch();
float OpenXRGetLeftTrigger();
float OpenXRGetLeftSqueeze();
bool  OpenXRGetLeftButtonA();
bool  OpenXRGetLeftButtonB();

bool OpenXRHasRightAim();
bool OpenXRHasRightStick();
bool OpenXRHasRightTouchPad();
bool OpenXRHasRightTrigger();
bool OpenXRHasRightSqueeze();
bool OpenXRHasRightButtonA();
bool OpenXRHasRightButtonB();

int   OpenXRGetRightHandActive();
int   OpenXRGetRightHandControllerType();
void  OpenXRGetRightHandPos( float* x, float* y, float* z );
void  OpenXRGetRightHandQuat( float* w, float* x, float* y, float* z );
void  OpenXRGetRightHandMatrix( float* m, float scale );
void  OpenXRGetRightHandAimPos( float* x, float* y, float* z );
void  OpenXRGetRightHandAimQuat( float* w, float* x, float* y, float* z );
void  OpenXRGetRightHandAimMatrix( float* m, float scale );
float OpenXRGetRightStickX();
float OpenXRGetRightStickY();
bool  OpenXRGetRightStickClick();
bool  OpenXRGetRightStickTouch();
float OpenXRGetRightTouchPadX();
float OpenXRGetRightTouchPadY();
bool  OpenXRGetRightTouchPadClick();
bool  OpenXRGetRightTouchPadTouch();
float OpenXRGetRightTrigger();
float OpenXRGetRightSqueeze();
bool  OpenXRGetRightButtonA();
bool  OpenXRGetRightButtonB();

// Rendering functions, must be called in this order
//   OpenXRStartFrame
//   RenderView = OpenXRStartRender( OPENXR_RENDER_LEFT )
//   Draw to RenderView
//   OpenXREndRender
//   RenderView = OpenXRStartRender( OPENXR_RENDER_RIGHT )
//   Draw to RenderView
//   OpenXREndRender
//   OpenXREndFrame

bool OpenXRStartFrame();
int OpenXRStartRender( OpenXRRenderSide side, unsigned char cmd );
void OpenXREndRender( unsigned char cmd );
void OpenXREndFrame();


#endif //_H_GG_OPENXR