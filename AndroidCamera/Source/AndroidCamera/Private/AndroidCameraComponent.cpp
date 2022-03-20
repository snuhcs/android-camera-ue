#include "AndroidCameraComponent.h"
#include "AndroidCamera.h"
#include "ImageFormatUtils.h"
#include "ScopedTimer.h"
#include "Rendering/Texture2DResource.h"
#include "HAL/FileManager.h"

#include <fstream>

#define print(txt) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, txt)

void UAndroidCameraComponent::UninitializeComponent()
{
	EndCamera();
	Super::UninitializeComponent();
}

// Called every frame
void UAndroidCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAndroidCameraComponent::ActivateComponent(int TargetCameraId, int PreviewWidth, int PreviewHeight, int TargetCameraRotation)
{
	UE_LOG(LogCamera, Display, TEXT("ActivateComponent id %d %dx%d"), TargetCameraId, PreviewWidth, PreviewHeight);
	CameraId = TargetCameraId;
	CameraRotation = TargetCameraRotation;
	Width = PreviewWidth;
	Height = PreviewHeight;
	bActive = true;

	ARGBBuffer = new unsigned char[PreviewWidth * PreviewHeight * 4];
	CameraTexture = UTexture2D::CreateTransient(PreviewWidth, PreviewHeight, PF_B8G8R8A8);
	CameraTexture->UpdateResource();
	UpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, PreviewWidth, PreviewHeight);
}

bool UAndroidCameraComponent::UpdateCamera(float delta)
{
	Timer += delta;
	if (Timer >= (1.f / FrameRate))
	{
		Timer = 0;
		if (CameraTexture && bActive && NewFrame)
		{
			return true;
		}
	}

	return false;
}

void UAndroidCameraComponent::StartCamera(int DesiredWidth, int DesiredHeight)
{
	if (!bRegistered)
	{
		UE_LOG(LogCamera, Display, TEXT("StartCamera desired size %dx%d"), DesiredWidth, DesiredHeight);
		FAndroidCameraModule::Get().RegisterComponent(*this, DesiredWidth, DesiredHeight);
		bRegistered = true;
	}
}

void UAndroidCameraComponent::EndCamera()
{
	bRegistered = false;
	bActive = false;

	if (ARGBBuffer)
	{
		delete[] ARGBBuffer;
		ARGBBuffer = nullptr;
	}

	if (UpdateTextureRegion)
	{
		delete UpdateTextureRegion;
		UpdateTextureRegion = nullptr;
	}

	// No need to deallocate as UE4 owns transient texture
	CameraTexture = nullptr;

	FAndroidCameraModule::Get().UnregisterComponent(CameraId);
}

unsigned char* UAndroidCameraComponent::GetBuffer()
{
	std::lock_guard<std::mutex> Guard(BufferMutex);
	NewFrame = false;
	return ARGBBuffer;
}

UTexture2D* UAndroidCameraComponent::GetTexture2D()
{
	if (!bActive)
	{
		return nullptr;
	}

	std::lock_guard<std::mutex> Guard(BufferMutex);
	NewFrame = false;
	CameraTexture->UpdateTextureRegions(0, 1, UpdateTextureRegion, 4 * Width, 4, ARGBBuffer);
	return CameraTexture;
}

int UAndroidCameraComponent::GetPreviewWidth() const
{
	return Width;
}

int UAndroidCameraComponent::GetPreviewHeight() const
{
	return Height;
}

int UAndroidCameraComponent::GetCameraId() const
{
	return CameraId;
}

int UAndroidCameraComponent::GetCameraRotation() const
{
	return CameraRotation;
}


void UAndroidCameraComponent::OnImageAvailable(unsigned char* Y, unsigned char* U, unsigned char* V, int YRowStride, int URowStride, int VRowStride, int YPixelStride, int UPixelStride, int VPixelStride)
{
	std::lock_guard<std::mutex> Guard(BufferMutex);
	// TODO(dostos): use worker thread?
	ScopedTimer("OnImageAvailable-YUV420 to RGB");
	ImageFormatUtils::YUV420ToARGB8888(Y, U, V, Width, Height, YRowStride, URowStride, UPixelStride, reinterpret_cast<int*>(ARGBBuffer));
	NewFrame = true;
}
