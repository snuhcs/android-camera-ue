// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include <vector>

DECLARE_LOG_CATEGORY_EXTERN(LogVideo, Log, All);

class UVideoInputComponent;

/*
VideoInputModule that manages video input components
and communicates with Java side MediaMetadataRetriever (CameraConnectionFragment)

Usage pipeline
1. Load video (UVideoInputComponent)
2. (Java) Video is loaded (MediaMetadataRetriever)
3. GetFrames ()

TODO(@juimdpp) Modify below
Frame pipeline
1. CameraConnectionFragment calls jni interface on frame arrival (OnImageAvailable)
2. FAndroidCameraModule sends specific event to corresponding component
3. UAndroidCameraComponent broadcasts OnFrameAvailable delegate
*/

class FVideoInputModule : public IModuleInterface {
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;

  static FVideoInputModule& Get();

  bool RegisterComponent(FString VideoPath);
  bool UnregisterComponent();

  bool CallJava_LoadVideo(FString Path);
  bool CallJava_GetNFrames(int N, int W, int H, int* OutputBuffer);
  bool CallJava_CloseVideo();

private:
};
