#include "VideoInputComponent.h"

#include <cassert>

void UVideoInputComponent::BeginPlay() {
  Super::BeginPlay();
}

void UVideoInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);

  for (size_t i = 0; i < CameraFrames.size(); i++) {
    CameraFrames[i]->RemoveFromRoot();
  }
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

  // magic numbers
  BufferFrameCnt = 150;
  BatchFrameCnt = BufferFrameCnt * 0.2;
  BufferSize = BufferFrameCnt * W * H;
  Buffer.resize(BufferSize);

  RemainingPercent = 0.8;
  FrameDuration = frameDuration;

  for (size_t i = 0; i < CameraFrames.size(); i++) {
    CameraFrames[i] = NewObject<UAndroidCameraFrame>(this);
    CameraFrames[i]->Initialize(W, H, false, PF_R8G8B8A8, true);
    CameraFrames[i]->AddToRoot();
  }

  Status = EVideoStatus::NOT_STARTED;

  if (instantStart) {
    StartVideo();
  }
}

void UVideoInputComponent::StartVideo() {
  if (EVideoStatus::NOT_STARTED == Status) {
    FetchThread = std::thread([this] { this->FetchLoop(); });
    ConsumeThread = std::thread([this] { this->ConsumeLoop(); });
    Status = EVideoStatus::PLAYING;
  } else {
    UE_LOG(LogVideo, Display, TEXT("Video already started!"));
  }
}

EVideoStatus UVideoInputComponent::GetStatus() const {
  return Status;
}

FString UVideoInputComponent::GetVideoFilePath() const {
  return VideoFilePath;
}

int UVideoInputComponent::GetTotalFrameCount() const {
  return TotalFrameCnt;
}

float UVideoInputComponent::GetProgress() const {
  return (float)(ConsumeHead) / TotalFrameCnt;
}

void UVideoInputComponent::KillEngine() {
  // no need to clear frames, it belongs to the UE4 GC
  KillThread = true;
  if (Status == EVideoStatus::PLAYING || Status == EVideoStatus::FINISHED) {
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
    int NumFrames = BatchFrameCnt;
    if (FetchHead + BatchFrameCnt > TotalFrameCnt) {
      NumFrames = TotalFrameCnt - FetchHead;
    }
    if (FVideoInputModule::Get().CallJava_GetNFrames(
        NumFrames, W, H, &Buffer[FrameToInt(FetchHead)])) {
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
      UE_LOG(LogVideo, Display, TEXT("Fetched all frames. Closing video..."));
      Status = EVideoStatus::FINISHED;
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
      BroadcastEndVideo();
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
    UAndroidCameraFrame* CameraFrame = GetCameraFrame();
    CameraFrame->UpdateFrame(&Buffer[FrameToInt(Idx)]);

    AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask,
              [&]() { OnFrameAvailable.Broadcast(CameraFrame); });

    // This code is on a Java thread
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [&]() { OnFrameAvailableDynamic.Broadcast(CameraFrame); }, TStatId(),
        nullptr, ENamedThreads::GameThread);
  }
}

void UVideoInputComponent::BroadcastEndVideo() {
  if (OnVideoEnd.IsBound() || OnVideoEndDynamic.IsBound()) {
    AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask,
              [&]() { OnVideoEnd.Broadcast(VideoFilePath); });

    // This code is on a Java thread
    FFunctionGraphTask::CreateAndDispatchWhenReady(
        [&]() { OnVideoEndDynamic.Broadcast(VideoFilePath); }, TStatId(),
        nullptr,
        ENamedThreads::GameThread);
  }
}

UAndroidCameraFrame* UVideoInputComponent::GetCameraFrame() {
  static size_t CurrentHead = 0;
  auto CurrentFrame = CameraFrames[CurrentHead];
  CurrentHead = (CurrentHead + 1) % CameraFrames.size();
  return CurrentFrame;
}
