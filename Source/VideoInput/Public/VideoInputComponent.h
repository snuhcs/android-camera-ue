﻿#pragma once
#include <array>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "VideoInputEnum.h"
#include "AndroidCamera/Public/AndroidCameraFrame.h"
#include "VideoInputModule.h"
#include "VideoInputComponent.generated.h"

class UAndroidCameraFrame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameAvailableDelegateDynamic,
                                            const UAndroidCameraFrame*,
                                            CameraFrame);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrameAvailableDelegate,
                                    const UAndroidCameraFrame*);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVideoEndDelegateDynamic,
                                            FString,
                                            Filepath);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVideoEndDelegate,
                                    FString /* Video file path */);

UCLASS(ClassGroup = (VideoInput), meta = (BlueprintSpawnableComponent))
class VIDEOINPUT_API UVideoInputComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = VideoInput)
  bool Initialize(FString path, int64 frameDuration, bool instantStart = false,
                  bool requireHandshake = false);

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  void StartVideo();

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  EVideoStatus GetStatus() const;

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  FString GetVideoFilePath() const;

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  int GetTotalFrameCount() const;

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  float GetProgress() const;

  // This function is called by the client to indicate that it has processed the previous frame
  // and is ready to receive the next frame from the consumer thread.
  UFUNCTION(BlueprintCallable, Category = VideoInput)
  void SetConsumeReady();

  UFUNCTION(BlueprintCallable, Category = VideoInput)
  bool RequiresHandshake() const;

  UPROPERTY(BlueprintAssignable, Category = VideoInput)
  FOnFrameAvailableDelegateDynamic OnFrameAvailableDynamic;
  // CAUTION: This delegate doesn't guarantee access from game thread
  FOnFrameAvailableDelegate OnFrameAvailable;

  UPROPERTY(BlueprintAssignable, Category = VideoInput)
  FOnVideoEndDelegateDynamic OnVideoEndDynamic;
  // CAUTION: This delegate doesn't guarantee access from game thread
  FOnVideoEndDelegate OnVideoEnd;

  void KillEngine();

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
  void BroadcastImageAvailability(int Idx);
  void BroadcastEndVideo();


  UAndroidCameraFrame* GetCameraFrame();
  // Simple circular buffer
  // UAndroidCameraFrame* is safe from UE GC as it belongs to this component
  std::array<UAndroidCameraFrame*, 10> CameraFrames;

  // FetchEngine related:
  void FetchLoop();
  void ConsumeLoop();
  int FrameToInt(int frame) const;

  // Video-specific parameters, set by Initialize
  FString VideoFilePath;
  int W, H;
  int TotalFrameCnt;
  long long FrameDuration; // in milliseconds

  static constexpr int BufferFrameCnt = 150;
  // Fetch thread will fetch `BatchFrameCnt` frames at a time
  static constexpr int BatchFrameCnt = 30; // 1s
  // Fetch thread will fetch if remaining frames are less than `BatchFrameCnt * RemainingPercent`
  static constexpr float RemainingPercent = 0.8f; // between [0 ~ 1]

  // Image pool, reserves memory for `BufferFrameCnt` images
  int GetBufferSize() const;
  std::vector<int> Buffer;

  int ConsumeHead = 0;
  int FetchHead = 0;

  std::thread FetchThread;
  std::thread ConsumeThread;
  bool KillThread = false;

  std::condition_variable FetchSignalCV;
  std::mutex FetchSignalMtx;
  bool RequireFetch = false;     // Always fetch first few frames at StartVideo
  bool RequireHandshake = false; // Always consume when client is ready 

  std::condition_variable ConsumeSignalCV;
  std::mutex ConsumeSignalMtx;
  bool CanConsume = false;
  EVideoStatus Status = EVideoStatus::NOT_INITIALIZED;

  std::condition_variable ConsumeReadyCV;
  std::mutex ConsumeReadyMtx;
  bool ConsumeReady = false;
};
