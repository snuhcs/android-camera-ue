#pragma once
#include "Engine.h"
#include <mutex>
#include "AndroidCameraComponent.generated.h"

UCLASS(ClassGroup = (AndroidCamera), meta = (BlueprintSpawnableComponent))
class UAndroidCameraComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	virtual void UninitializeComponent() override;
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ActivateComponent(int CameraId, int PreviewWidth, int PreviewHeight, int CameraRotation);

	// Returns whether there is a new buffer or not
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	bool UpdateCamera(float delta);

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void StartCamera(int DesiredWidth, int DesiredHeight);
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	void EndCamera();

	unsigned char* GetBuffer();
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	UTexture2D* GetTexture2D();

	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetPreviewWidth() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetPreviewHeight() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraId() const;
	UFUNCTION(BlueprintCallable, Category = AndroidCamera)
	int GetCameraRotation() const;


	void OnImageAvailable(
		unsigned char* Y, unsigned char* U, unsigned char* V, 
		int YRowStride, int URowStride, int VRowStride,
		int YPixelStride, int UPixelStride, int VPixelStride);

private:
	bool bRegistered = false;
	bool bActive = false;;

	std::mutex BufferMutex;
	unsigned char* ARGBBuffer = nullptr;
	bool NewFrame = false;
	UTexture2D* CameraTexture = nullptr;
	FUpdateTextureRegion2D *UpdateTextureRegion = nullptr;

	const float FrameRate = 30.f;
	float Timer = 0;
	int Width;
	int Height;
	int CameraId;
	int CameraRotation;
};