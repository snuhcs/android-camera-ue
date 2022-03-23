#pragma once

#include "UObject/ObjectMacros.h"
#include "AndroidCameraFrame.generated.h"

class UAndroidCameraComponent;
UCLASS(Blueprintable)
class ANDROIDCAMERA_API UAndroidCameraFrame : public UObject
{
	GENERATED_BODY()

	virtual void BeginDestroy() override;

public:

	void Initialize(int Width, int Height);

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetWidth() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetHeight() const;

	unsigned char* GetBuffer();
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	UTexture2D* GetTexture2D();

private:
	inline int GetPlaneSize() const;
	void UpdateFrame(unsigned char* Y, unsigned char* U, unsigned char* V, int YRowStride, int UVRowStride, int UVPixelStride,
		int YLength, int ULength, int VLength);
	friend class UAndroidCameraComponent;


	FUpdateTextureRegion2D* UpdateTextureRegion = nullptr;

	bool IsTextureDirty = false;
	UTexture2D* CameraTexture = nullptr;
	bool IsBufferDirty = false;
	unsigned char* ARGBBuffer = nullptr;
	int Width;
	int Height;

	// Buffer ptrs owned by Java
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