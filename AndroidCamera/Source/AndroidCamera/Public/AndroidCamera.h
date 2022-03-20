// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include <map>
#include <queue>

DECLARE_LOG_CATEGORY_EXTERN(LogCamera, Log, All);

class UAndroidCameraComponent;
class FAndroidCameraModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	static FAndroidCameraModule &Get();

	/*
	Initialization order of the AndroidCamera
	1. Initialize component (UAndroidCameraComponent)
	2. Start Java side camera (CameraConnectionFragment)
	3. Activate component with corresponding component (ActivateComponent)
	*/
	void RegisterComponent(UAndroidCameraComponent& Component, int DesiredWidth, int DesiredHeight);
	void ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight, int CameraRotation);
	void UnregisterComponent(int CameraId);

	UAndroidCameraComponent* GetComponent(int CameraId);

private:
	// Map of activated components
	std::map<int, UAndroidCameraComponent*> IdToComponents;
	// Queue of not-activated components; 
	std::queue<UAndroidCameraComponent*> PendingComponents;
};