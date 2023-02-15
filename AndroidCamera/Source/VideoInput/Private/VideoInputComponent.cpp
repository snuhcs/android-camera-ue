#include "VideoInputComponent.h"

#include <cassert>

void UVideoInputComponent::BeginPlay() {
  Super::BeginPlay();
  UE_LOG(LogVideo, Display,
         TEXT("UVideoInputComponent BeginPlay! Don't forget to OpenVideo"));
}


void UVideoInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);
  UE_LOG(LogVideo, Display,
         TEXT("UVideoInputComponent EndPlay! Killing FetchEngine..."));

  KillEngine();
}

void UVideoInputComponent::Initialize(FString path, int w, int h,
                                      int totalFrameCnt, int blockFrameCnt,
                                      int buffFrameCnt, float remainingPercent,
                                      int64 frameDuration) {
  UE_LOG(LogVideo, Display,
         TEXT(
           "TotalFrameCnt must be divisble by BufferFrameCnt && BufferFrameCnt must be divisible by BlockFrameCnt."
         ))

  VideoFilePath = path;
  if (!FVideoInputModule::Get().CallJava_LoadVideo(VideoFilePath)) {
    UE_LOG(LogVideo, Display,
           TEXT("LoadVideo failed! Please check video path (%s)"),
           *VideoFilePath);
    return;
  }
  RequireFetch = true;
  // No need to access with mutex because StartVideo writes first before any other function

  W = w;
  H = h;

  TotalFrameCnt = totalFrameCnt;
  BatchFrameCnt = blockFrameCnt;

  BufferFrameCnt = buffFrameCnt;
  BufferSize = BufferFrameCnt * W * H;
  Buffer.resize(BufferSize);

  RemainingPercent = remainingPercent;
  FrameDuration = frameDuration;

  CameraFrame = NewObject<UAndroidCameraFrame>(this);
  CameraFrame->Initialize(w, h, false, PF_R8G8B8A8);

  FetchThread = std::thread([this] { this->FetchLoop(); });
  ConsumeThread = std::thread([this] { this->ConsumeLoop(); });
}


void UVideoInputComponent::KillEngine() {
  CameraFrame = nullptr;

  KillThread = true;
  FetchSignalCV.notify_all();
  ConsumeSignalCV.notify_all();
  if (ConsumeThread.joinable()) {
    ConsumeThread.join();
  }
  if (FetchThread.joinable()) {
    FetchThread.join();
  }

  FVideoInputModule::Get().CallJava_CloseVideo();
}


void UVideoInputComponent::FetchLoop() {
  while (true) {
    // Wait for fetch signal (fetch directly only at beginning)
    {
      std::unique_lock<std::mutex> FetchLock(FetchSignalMtx);
      FetchSignalCV.wait(FetchLock, [this] {
        return KillThread || RequireFetch;
      });
      RequireFetch = false;
      // FetchLock.unlock();
    }
    if (KillThread) {
      return;
    }

    UE_LOG(LogVideo, Display, TEXT("Fetching frames %d ~ %d"), FetchHead,
           FetchHead + BatchFrameCnt);
    if (FVideoInputModule::Get().CallJava_GetNFrames(
        BatchFrameCnt, W, H, &Buffer[FrameToInt(FetchHead)])) {
      UE_LOG(LogVideo, Display, TEXT("Successfully fetched frames!"));
      FetchHead += BatchFrameCnt;
    } else {
      UE_LOG(LogVideo, Display, TEXT("Error in fetching frames..."));
      return;
    }

    // Notify Consumer
    {
      std::unique_lock<std::mutex> ConsumeLock(ConsumeSignalMtx);
      CanConsume = true;
      ConsumeSignalCV.notify_all();
    }

    // Terminate FetchThread if FetchEngine fetched all frames
    if (FetchHead >= TotalFrameCnt) {
      UE_LOG(LogVideo, Display, TEXT("Fetched all frames"));
      return;
    }
  }
}

void UVideoInputComponent::ConsumeLoop() {
  while (true) {
    // Wait for Consume signal (wait even at beginning)
    {
      std::unique_lock<std::mutex> ConsumeLock(ConsumeSignalMtx);
      ConsumeSignalCV.wait(ConsumeLock, [this] {
        return KillThread || CanConsume;
      });
      CanConsume = false;
      // ConsumeLock.unlock();
    }
    if (KillThread) {
      return;
    }

    // Process
    int cnt = BatchFrameCnt;
    bool signalSent = false;
    while (cnt > 0) {
      UE_LOG(LogVideo, Display, TEXT("VideoInput consumer"));
      cnt--;

      // Convert to YUV && Trigger Delegate Event
      UE_LOG(LogVideo, Display,
             TEXT("OnImageAvailableDelegate called on frame %d"), ConsumeHead);
      BroadcastImageAvailability(ConsumeHead);

      ConsumeHead++;
      if (!signalSent && cnt <= BatchFrameCnt * RemainingPercent)
      // If frames left to read are less than RemainingPercent
      {
        // Notify Fetcher
        std::unique_lock<std::mutex> FetchLock(FetchSignalMtx);
        RequireFetch = true;
        UE_LOG(LogVideo, Display, TEXT("Fetch another batch of frames"));
        signalSent = true;
        FetchSignalCV.notify_all();
      }
      // Sleep to simulate framerate (yield sleep)
      std::this_thread::sleep_for(std::chrono::milliseconds(FrameDuration));
    }

    // Terminate ConsumeThread if FetchEngine has consumed all frames
    if (ConsumeHead >= TotalFrameCnt) {
      UE_LOG(LogVideo, Display, TEXT("Consumed all frames"));
      return;
    }
  }
}

int UVideoInputComponent::FrameToInt(int frame) const {
  const int rangeStart = frame % BufferFrameCnt * W * H;
  assert(rangeStart < BufferSize);
  return rangeStart;
}


void UVideoInputComponent::BroadcastImageAvailability(int Idx) {
  if (OnFrameAvailable.IsBound() || OnFrameAvailableDynamic.IsBound()) {
    CameraFrame->UpdateFrame(&Buffer[FrameToInt(Idx)]);

    AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [&]() {
      OnFrameAvailable.Broadcast(CameraFrame);
    });

    // This code is on a Java thread
    FFunctionGraphTask::CreateAndDispatchWhenReady([&]() {
      OnFrameAvailableDynamic.Broadcast(CameraFrame);
    }, TStatId(), nullptr, ENamedThreads::GameThread);
  }
}
