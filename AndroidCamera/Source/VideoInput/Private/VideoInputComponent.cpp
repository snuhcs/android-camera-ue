#include "VideoInputComponent.h"

#include <cassert>

void UVideoInputComponent::BeginPlay() {
  Super::BeginPlay();
}

void UVideoInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);
  KillEngine();
}

void UVideoInputComponent::Initialize(FString path, int64 frameDuration,
                                      bool instantStart) {
  VideoFilePath = path;
  if (!FVideoInputModule::Get().CallJava_LoadVideo(VideoFilePath)) {
    UE_LOG(LogVideo, Display,
           TEXT("LoadVideo failed! Please check video path (%s)"),
           *VideoFilePath);
    return;
  }
  RequireFetch = true;
  IsStarted = false;
  // No need to access with mutex because StartVideo writes first before any
  // other function

  TotalFrameCnt = FVideoInputModule::Get().CallJava_GetFrameCount();
  if (TotalFrameCnt < 0) {
    UE_LOG(
        LogVideo, Display,
        TEXT("Coudn't get frame count of video... Make sure video is loaded"));
    return;
  }
  W = FVideoInputModule::Get().CallJava_GetVideoWidth();
  H = FVideoInputModule::Get().CallJava_GetVideoHeight();
  if (W < 0 || H < 0) {
    UE_LOG(LogVideo, Display,
           TEXT("Coudn't get video size... Make sure video is loaded"));
    return;
  }

  BufferFrameCnt = 0.25 * TotalFrameCnt;
  BatchFrameCnt = BufferFrameCnt * 0.5;
  BufferSize = BufferFrameCnt * W * H;
  Buffer.resize(BufferSize);

  RemainingPercent = 0.3;
  FrameDuration = frameDuration;

  CameraFrame = NewObject<UAndroidCameraFrame>(this);
  CameraFrame->Initialize(W, H, false, PF_R8G8B8A8);

  if (instantStart) {
    StartVideo();
  }
}

void UVideoInputComponent::StartVideo() {
  if (!IsStarted) {
    FetchThread = std::thread([this] { this->FetchLoop(); });
    ConsumeThread = std::thread([this] { this->ConsumeLoop(); });
    IsStarted = true;
  } else {
    UE_LOG(LogVideo, Display, TEXT("Video already started!"));
  }
}

int UVideoInputComponent::GetTotalFrameCount() const {
  return TotalFrameCnt;
}

void UVideoInputComponent::KillEngine() {
  CameraFrame = nullptr;
  KillThread = true;
  if (IsStarted) {
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
}

void UVideoInputComponent::FetchLoop() {
  while (true) {
    // Wait for fetch signal (fetch directly only at beginning)
    {
      std::unique_lock<std::mutex> FetchLock(FetchSignalMtx);
      FetchSignalCV.wait(FetchLock,
                         [this] { return KillThread || RequireFetch; });
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
      ConsumeSignalCV.wait(ConsumeLock,
                           [this] { return KillThread || CanConsume; });
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

    AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask,
              [&]() { OnFrameAvailable.Broadcast(CameraFrame); });

    // This code is on a Java thread
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [&]() { OnFrameAvailableDynamic.Broadcast(CameraFrame); }, TStatId(),
        nullptr, ENamedThreads::GameThread);
  }
}
