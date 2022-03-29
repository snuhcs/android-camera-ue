#pragma once

#include "UObject/ObjectMacros.h"
#include "AndroidCameraFrame.generated.h"

class UAndroidCameraComponent;
UCLASS(Blueprintable)
class ANDROIDCAMERA_API UAndroidCameraFrame : public UObject
{
	GENERATED_BODY()
public:
	virtual void BeginDestroy() override;

	void Initialize(int Width, int Height);

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetWidth() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetHeight() const;

	unsigned char* GetARGBBuffer() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	UTexture2D* GetTexture2D() const;

	struct NV12Frame
	{
		uint8* Y;
		uint8* U;
		uint8* V;
		int YRowStride;
		int UVRowStride;
		int UVPixelStride;
	};

	NV12Frame GetData() const;

private:
	inline int GetPlaneSize() const;
	void UpdateFrame(unsigned char* Y, unsigned char* U, unsigned char* V, int YRowStride, int UVRowStride, int UVPixelStride,
		int YLength, int ULength, int VLength);
	friend class UAndroidCameraComponent;


	FUpdateTextureRegion2D* UpdateTextureRegion = nullptr;

	// Mutable temporal member ptrs for lazy update
	mutable bool IsTextureDirty = false;
	mutable UTexture2D* CameraTexture = nullptr;
	mutable bool IsBufferDirty = false;
	mutable unsigned char* ARGBBuffer = nullptr;

	int Width;
	int Height;

	uint8* Y = nullptr;
	uint8* U = nullptr;
	uint8* V = nullptr;

	int YLength = 0;
	int ULength = 0;
	int VLength = 0;

	int YRowStride;
	int UVRowStride;
	int UVPixelStride;
};