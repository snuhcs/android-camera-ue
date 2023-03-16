#pragma once
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

UCLASS(ClassGroup = (VideoInput), meta = (BlueprintSpawnableComponent))
class VIDEOINPUT_API UVideoInputComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = VideoInput)
  void Initialize(FString path, int64 frameDuration, bool instantStart = false);

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

  UPROPERTY(BlueprintAssignable, Category = VideoInput)
  FOnFrameAvailableDelegateDynamic OnFrameAvailableDynamic;
  // CAUTION: This delegate doesn't guarantee access from game thread
  FOnFrameAvailableDelegate OnFrameAvailable;

  void KillEngine();

protected:
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
  void BroadcastImageAvailability(int Idx);


  UAndroidCameraFrame* GetCameraFrame();
  // simple circular buffer
  // UAndroidCameraFrame* is safe from UE GC as it belongs to this component
  std::array<UAndroidCameraFrame*, 5> CameraFrames;

  // FetchEngine related:
  void FetchLoop();
  void ConsumeLoop();
  int FrameToInt(int frame) const;

  // Must be initialized from FetchEngine caller
  FString VideoFilePath;
  int W, H;
  int TotalFrameCnt;
  int BatchFrameCnt;
  int BufferFrameCnt;
  int BufferSize; // in int
  std::vector<int> Buffer;
  float RemainingPercent;  // between [0 ~ 1]
  long long FrameDuration; // int milliseconds

  int ConsumeHead = 0;
  int FetchHead = 0;

  std::thread FetchThread;
  std::thread ConsumeThread;
  bool KillThread = false;

  std::condition_variable FetchSignalCV;
  std::mutex FetchSignalMtx;
  bool RequireFetch = false; // Always fetch first few frames at StartVideo

  std::condition_variable ConsumeSignalCV;
  std::mutex ConsumeSignalMtx;
  bool CanConsume = false;
  EVideoStatus Status = EVideoStatus::NOT_INITIALIZED;
};
