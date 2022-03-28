#pragma once
#include "Engine.h"
#include "AndroidCameraComponent.generated.h"

class UAndroidCameraFrame;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFrameAvailableDynamic, UAndroidCameraFrame*, AndroidCameraFrame);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFrameAvailable, UAndroidCameraFrame*);


UCLASS(ClassGroup = (AndroidCamera), meta = (BlueprintSpawnableComponent))
class ANDROIDCAMERA_API UAndroidCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void UninitializeComponent() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight, int CameraRotation);

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void StartCamera(int DesiredWidth, int DesiredHeight);
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void EndCamera();

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraId() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraRotation() const;

	UPROPERTY(BlueprintAssignable, Category = AndroidCamera)
	FOnFrameAvailableDynamic OnFrameAvailableDynamic;
	FOnFrameAvailable OnFrameAvailable;

	void OnImageAvailable(
		unsigned char* Y, unsigned char* U, unsigned char* V, 
		int YRowStride, int UVRowStride, int UVPixelStride,
		int YLength, int ULength, int VLength);

private:
	bool bRegistered = false;
	bool bActive = false;;

	UAndroidCameraFrame* CameraFrame = nullptr;

	int CameraId;
	int CameraRotation;
};