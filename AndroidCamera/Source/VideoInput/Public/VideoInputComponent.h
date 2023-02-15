#pragma once
#include <condition_variable>
#include <mutex>
#include <thread>
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
  // Step 1.
  UFUNCTION(BlueprintCallable, Category = VideoInput)
  void Initialize(FString path, int w, int h, int totalFrameCnt,
                  int blockFrameCnt, int buffFrameCnt, float remainingPercent,
                  int64 frameDuration);

  // UFUNCTION(BlueprintCallable, Category = VideoInput)
  // void StartVideo();
  //
  // UFUNCTION(BlueprintCallable, Category = VideoInput)
  // void CloseVideo();

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

  UAndroidCameraFrame* CameraFrame = nullptr;

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
};
