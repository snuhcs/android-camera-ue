#pragma once

#include "Engine.h"
#include "AndroidCameraComponent.generated.h"

class UAndroidCameraFrame;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameAvailableDynamic,
                                            const UAndroidCameraFrame*,
                                            AndroidCameraFrame);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrameAvailable,
                                    const UAndroidCameraFrame*);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTextureAvailableDynamic,
                                            UTexture2D*, Texture);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnTextureAvailable, UTexture2D*);


UCLASS(ClassGroup = (AndroidCamera), meta = (BlueprintSpawnableComponent))
class ANDROIDCAMERA_API UAndroidCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void UninitializeComponent() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction*
	                           ThisTickFunction) override;

	void ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight,
	                       int CameraRotation);

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void StartCamera(int DesiredWidth, int DesiredHeight, int DesiredFPS = 30);
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void EndCamera();

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraId() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraRotation() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraSamplingIntervalMs() const;

	const bool IsCameraRegistered() const;

	UPROPERTY(BlueprintAssignable, Category = AndroidCamera)
	FOnFrameAvailableDynamic OnFrameAvailableDynamic;
	UPROPERTY(BlueprintAssignable, Category = AndroidCamera)
	FOnTextureAvailableDynamic OnTextureAvailableDynamic;

	/*
	CAUTION: This delegate doesn't guarantee access from game thread 
	*/
	FOnFrameAvailable OnFrameAvailable;
	FOnTextureAvailable OnTextureAvailable;

private:
	TWeakObjectPtr<UAndroidCameraFrame> CameraFrame;
	TWeakObjectPtr<UTexture2D> Texture;
	FUpdateTextureRegion2D* TextureUpdateRegion = nullptr;
	
	friend class FAndroidCameraModule;
	// Camera callback from Java side (Android Camera Module)
	void OnImageAvailable(
		unsigned char* Y, unsigned char* U, unsigned char* V,
		int YRowStride, int UVRowStride, int UVPixelStride,
		int YLength, int ULength, int VLength);

	bool bRegistered = false;
	bool bActive = false;;

	int CameraId;
	int CameraRotation;
};
