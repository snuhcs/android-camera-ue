// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include <map>
#include <queue>

DECLARE_LOG_CATEGORY_EXTERN(LogCamera, Log, All);

DECLARE_STATS_GROUP(TEXT("AndroidCamera"), STATGROUP_AndroidCamera, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("YUV420toARGB"), STAT_AndroidCameraYUV420toARGB, STATGROUP_AndroidCamera, ANDROIDCAMERA_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("ARGBtoTexture2D"), STAT_AndroidCameraARGBtoTexture2D, STATGROUP_AndroidCamera, ANDROIDCAMERA_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("CopyBuffer"), STAT_AndroidCameraCopyBuffer, STATGROUP_AndroidCamera, ANDROIDCAMERA_API);

class UAndroidCameraComponent;

/*
AndroidCameraModule that manages multiple camera components
and communicates with Java side Camera2 API (CameraConnectionFragment)

Initialization order of the AndroidCamera
1. Initialize component (UAndroidCameraComponent)
2. Start Java side camera (CameraConnectionFragment)
3. Activate component with corresponding component (ActivateComponent)

Frame pipeline
1. CameraConnectionFragment calls jni interface on frame arrival (OnImageAvailable)
2. FAndroidCameraModule sends specific event to corresponding component
3. UAndroidCameraComponent broadcasts OnFrameAvailable delegate
*/
class ANDROIDCAMERA_API FAndroidCameraModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static FAndroidCameraModule &Get();
	void RegisterComponent(UAndroidCameraComponent& Component, int DesiredWidth, int DesiredHeight);
	void ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight, int CameraRotation);
	void UnregisterComponent(int CameraId);

	UAndroidCameraComponent* GetComponent(int CameraId);

	bool OnImageAvailable(int CameraId, unsigned char* Y, unsigned char* U, unsigned char* V, 
		int YRowStride, int UVRowStride, int UVPixelStride,
		int YLength, int ULength, int VLength);
private:
	// Map of activated components
	std::map<int, UAndroidCameraComponent*> IdToComponents;
	// Queue of not-activated components; 
	std::queue<UAndroidCameraComponent*> PendingComponents;
};