#include "AndroidCameraFrame.h"

#include <cassert>

#include "AndroidCamera.h"
#include "ImageFormatUtils.h"

bool UAndroidCameraFrame::HasYUV() const {
  return Y && U && V;
}

void UAndroidCameraFrame::BeginDestroy() {
  if (ARGBBuffer) {
    delete[] ARGBBuffer;
    ARGBBuffer = nullptr;
  }

  if (UpdateTextureRegion) {
    delete UpdateTextureRegion;
    UpdateTextureRegion = nullptr;
  }

  if (Y) {
    delete[] Y;
  }

  if (U) {
    delete[] U;
  }

  if (V) {
    delete[] V;
  }

  // No need to deallocate as UE4 owns transient texture
  CameraTexture = nullptr;

  Super::BeginDestroy();
}

void UAndroidCameraFrame::Initialize(int PreviewWidth, int PreviewHeight,
                                     const bool hasYUV,
                                     const EPixelFormat InFormat,
                                     bool CreateTexture) {
  UpdateTextureRegion = new FUpdateTextureRegion2D(
      0, 0, 0, 0, PreviewWidth, PreviewHeight);

  Width = PreviewWidth;
  Height = PreviewHeight;

  if (!hasYUV) {
    Y = U = V = nullptr;
  }
  PixelFormat = InFormat;

  ARGBBuffer = new unsigned char[GetPlaneSize() * 4];

  if (CreateTexture) {
    if (IsInGameThread()) {
      CameraTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
      CameraTexture->UpdateResource();
      CameraTexture->WaitForPendingInitOrStreaming();
    } else {
      UE_LOG(LogCamera, Error,
             TEXT(
               "CameraTexture should be created in the game thread. Use CreateTexture2DAsync instead"
             ));
    }
  }
}

int UAndroidCameraFrame::GetWidth() const {
  return Width;
}

int UAndroidCameraFrame::GetHeight() const {
  return Height;
}

unsigned char* UAndroidCameraFrame::GetARGBBuffer() const {
  if (IsBufferDirty && ARGBBuffer && HasYUV()) {
    SCOPE_CYCLE_COUNTER(STAT_AndroidCameraYUV420toARGB);
    ImageFormatUtils::YUV420ToARGB8888(Y, U, V, Width, Height, YRowStride,
                                       UVRowStride, UVPixelStride,
                                       reinterpret_cast<int*>(ARGBBuffer));
    IsBufferDirty = false;
  } else if (IsBufferDirty && ARGBBuffer && !HasYUV()) {
    IsBufferDirty = false;
  }

  return ARGBBuffer;
}

UTexture2D* UAndroidCameraFrame::GetTexture2D() const {
  if (!CameraTexture.Get()) {
    CameraTexture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
    CameraTexture->UpdateResource();
    CameraTexture->WaitForPendingInitOrStreaming();
  }

  if (IsTextureDirty && CameraTexture.Get()) {
    SCOPE_CYCLE_COUNTER(STAT_AndroidCameraARGBtoTexture2D);
    CameraTexture->UpdateTextureRegions(0, 1, UpdateTextureRegion, 4 * Width, 4,
                                        GetARGBBuffer());
    IsTextureDirty = false;
  }
  return CameraTexture.Get();
}

UAndroidCameraFrame* UAndroidCameraFrame::FromTexture2D(UTexture2D* Texture) {
  CameraTexture = Texture;
  Height = Texture->GetSizeX();
  Width = Texture->GetSizeY();
  return this;
}

UAndroidCameraFrame::NV12Frame UAndroidCameraFrame::GetData() const {
  if (!HasYUV()) {
    UE_LOG(LogCamera, Error,
           TEXT(
             "For frames that only have buffer (no YUV), use GetARGBBuffer() instead"
           ));
  }
  return {Y, U, V, YRowStride, UVRowStride, UVPixelStride};
}

void UAndroidCameraFrame::UpdateFrame(unsigned char* NewY, unsigned char* NewU,
                                      unsigned char* NewV, int NewYRowStride,
                                      int NewUVRowStride, int NewUVPixelStride,
                                      int NewYLength, int NewULength,
                                      int NewVLength) {
  SCOPE_CYCLE_COUNTER(STAT_AndroidCameraCopyBuffer);

  // Lazily initialize YUV buffer
  // TODO(dostos): move this to `Initialize` if
  // there is a way to determine plane sizes in advance
  if (!Y || NewYLength > YLength) {
    if (Y) {
      delete[] Y;
    }
    Y = new uint8[NewYLength];
    YLength = NewYLength;
  }

  if (!U || NewULength > ULength) {
    if (U) {
      delete[] U;
    }
    U = new uint8[NewULength];
    ULength = NewULength;
  }

  if (!V || NewVLength > VLength) {
    if (V) {
      delete[] V;
    }
    V = new uint8[NewVLength];
    VLength = NewVLength;
  }

  std::memcpy(Y, NewY, YLength);
  std::memcpy(U, NewU, ULength);
  std::memcpy(V, NewV, VLength);

  YRowStride = NewYRowStride;
  UVRowStride = NewUVRowStride;
  UVPixelStride = NewUVPixelStride;

  IsTextureDirty = true;
  IsBufferDirty = true;
}

void UAndroidCameraFrame::UpdateFrame(int* NewARGB) const {
  if (HasYUV()) {
    UE_LOG(LogCamera, Error,
           TEXT(
             "For frames that have YUV buffer, use UpdateFrame(unsigned char *NewY, unsigned char *NewU, unsigned char *NewV, ...) instead"
           ));
    return;
  }

  if (NewARGB == nullptr) {
    UE_LOG(LogCamera, Error, TEXT("NewARGB is null"));
    return;
  }

  // Width && Height do not change
  std::memcpy(ARGBBuffer, NewARGB, GetPlaneSize() * 4);
  IsTextureDirty = true;
  IsBufferDirty = true;
}


inline int UAndroidCameraFrame::GetPlaneSize() const {
  return Width * Height;
}
