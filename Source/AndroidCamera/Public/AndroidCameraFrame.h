#pragma once

#include "UObject/ObjectMacros.h"
#include "AndroidCameraFrame.generated.h"

class UAndroidCameraComponent;

UCLASS(Blueprintable)
class ANDROIDCAMERA_API UAndroidCameraFrame : public UObject {
  GENERATED_BODY()

public:
  virtual void BeginDestroy() override;

  void Initialize(int Width, int Height, bool hasYUV = true,
                  EPixelFormat InFormat = PF_B8G8R8A8,
                  bool CreateTexture = false);

  UFUNCTION(BlueprintCallable, Category = AndroidCamera)
  bool HasYUV() const;

  UFUNCTION(BlueprintCallable, Category = AndroidCamera)
  int GetWidth() const;
  UFUNCTION(BlueprintCallable, Category = AndroidCamera)
  int GetHeight() const;

  unsigned char* GetARGBBuffer() const;

  struct NV12Frame {
    uint8* Y;
    uint8* U;
    uint8* V;
    int YRowStride;
    int UVRowStride;
    int UVPixelStride;
  };

  // For !HasYUV (i.e. frames that only have buffer), use GetARGBBuffer instead
  NV12Frame GetData() const;

private:
  inline int GetPlaneSize() const;
  void UpdateFrame(unsigned char* Y, unsigned char* U, unsigned char* V,
                   int YRowStride, int UVRowStride, int UVPixelStride,
                   int YLength, int ULength, int VLength);
  void UpdateFrame(int* ARGBData) const;

  friend class UAndroidCameraComponent;
  friend class UVideoInputComponent;
  
  mutable bool IsBufferDirty = false;
  mutable unsigned char* ARGBBuffer = nullptr;

  int Width = 0;
  int Height = 0;

  uint8* Y = nullptr;
  uint8* U = nullptr;
  uint8* V = nullptr;

  int YLength = 0;
  int ULength = 0;
  int VLength = 0;

  int YRowStride;
  int UVRowStride;
  int UVPixelStride;

  EPixelFormat PixelFormat = PF_B8G8R8A8;
};
