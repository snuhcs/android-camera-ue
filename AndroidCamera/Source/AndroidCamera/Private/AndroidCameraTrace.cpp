#include "AndroidCameraTrace.h"
#if ANDROID_CAMERA_TRACE_ENABLED
#include "Trace/Trace.inl"

UE_TRACE_CHANNEL_DEFINE(TraceAndroidCameraChannel)

UE_TRACE_EVENT_BEGIN(AndroidCamera, OnCameraFrame)
	UE_TRACE_EVENT_FIELD(int32, CameraId)
UE_TRACE_EVENT_END()

void FAndroidCameraTraceReporter::ReportOnCameraFrame(int32 CameraId)
{
	UE_TRACE_LOG(AndroidCamera, OnCameraFrame, TraceAndroidCameraChannel)
		<< OnCameraFrame.CameraId(CameraId);
}

#endif
