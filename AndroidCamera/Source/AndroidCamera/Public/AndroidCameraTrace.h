#pragma once

#include "CoreTypes.h"
#include "ObjectTrace.h"
#include "CoreMinimal.h"
#include "Trace/Trace.h"

#if !defined(ANDROID_CAMERA_TRACE_ENABLED)
#if UE_TRACE_ENABLED && !UE_BUILD_SHIPPING
#define ANDROID_CAMERA_TRACE_ENABLED 1
#else
#define ANDROID_CAMERA_TRACE_ENABLED 0
#endif
#endif

#if ANDROID_CAMERA_TRACE_ENABLED

UE_TRACE_CHANNEL_EXTERN(TraceAndroidCameraChannel, ANDROIDCAMERA_API)

// Emits trace events denoting scope/lifetime of an activity on the cooking channel.
#define UE_SCOPED_ANDROIDCAMERATIMER(name)						TRACE_CPUPROFILER_EVENT_SCOPE_ON_CHANNEL(name, BandChannel)
#define UE_SCOPED_ANDROIDCAMERATIMER_AND_DURATION(name, durationStorage) \
FScopedDurationTimer name##Timer(durationStorage); UE_SCOPED_COOKTIMER(name)

struct FAndroidCameraTraceReporter
{
	static void ReportOnCameraFrame(int32 CameraId);
};


#define TRACE_ANDROIDCAMERA_ON_FRAME(CameraId) \
	FAndroidCameraTraceReporter::ReportOnCameraFrame(CameraId);

#else

#define UE_SCOPED_ANDROIDCAMERATIMER(...)
#define UE_SCOPED_ANDROIDCAMERATIMER_AND_DURATION(...)
#define TRACE_ANDROIDCAMERA_ON_FRAME(...)

#endif // ANDROID_CAMERA_TRACE_ENABLED