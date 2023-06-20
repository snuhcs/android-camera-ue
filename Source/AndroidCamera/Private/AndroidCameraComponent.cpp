#include "AndroidCameraComponent.h"
#include "AndroidCameraFrame.h"
#include "AndroidCamera.h"
#include "ImageFormatUtils.h"
#include "Rendering/Texture2DResource.h"
#include "HAL/FileManager.h"

#include <fstream>

#define print(txt) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, txt)

void UAndroidCameraComponent::UninitializeComponent()
{
	EndCamera();
	if (TextureUpdateRegion)
	{
		delete TextureUpdateRegion;
		TextureUpdateRegion = nullptr;
	}

	Super::UninitializeComponent();
}

// Called every frame
void UAndroidCameraComponent::TickComponent(float DeltaTime,
                                            ELevelTick TickType,
                                            FActorComponentTickFunction*
                                            ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UAndroidCameraComponent::ActivateComponent(int TargetCameraId,
                                                int PreviewWidth,
                                                int PreviewHeight,
                                                int TargetCameraRotation)
{
	UE_LOG(LogCamera, Display, TEXT("ActivateComponent id %d %dx%d"),
	       TargetCameraId, PreviewWidth, PreviewHeight);
	CameraId = TargetCameraId;
	CameraRotation = TargetCameraRotation;
	bActive = true;

	CameraFrame = NewObject<UAndroidCameraFrame>(this);
	CameraFrame->Initialize(PreviewWidth, PreviewHeight, true, PF_B8G8R8A8,
	                        true);

	UTexture2D* NewTexture = UTexture2D::CreateTransient(
		PreviewWidth, PreviewHeight, PF_B8G8R8A8);
	// Must wait for render thread to create the texture
	AsyncTask(ENamedThreads::GameThread, [this, NewTexture]()
	{
		NewTexture->UpdateResource();
		Texture = NewTexture;
	});
	TextureUpdateRegion = new FUpdateTextureRegion2D(
		0, 0, 0, 0, PreviewWidth, PreviewHeight);
}

void UAndroidCameraComponent::StartCamera(int DesiredWidth, int DesiredHeight,
                                          int DesiredFPS)
{
	if (!bRegistered)
	{
		UE_LOG(LogCamera, Display,
		       TEXT("StartCamera desired size %dx%d, FPS %d"),
		       DesiredWidth, DesiredHeight, DesiredFPS);
		FAndroidCameraModule::Get().RegisterComponent(
			*this, DesiredWidth, DesiredHeight, DesiredFPS);
		bRegistered = true;
	}
}

void UAndroidCameraComponent::EndCamera()
{
	CameraFrame = nullptr;
	bRegistered = false;
	bActive = false;

	FAndroidCameraModule::Get().UnregisterComponent(CameraId);
}

int UAndroidCameraComponent::GetCameraId() const
{
	return CameraId;
}

int UAndroidCameraComponent::GetCameraRotation() const
{
	return CameraRotation;
}

int UAndroidCameraComponent::GetCameraSamplingIntervalMs() const
{
	// TODO(dostos): Get from actual Camera2 interface
	return 33;
}

const bool UAndroidCameraComponent::IsCameraRegistered() const
{
	return bRegistered;
}

void UAndroidCameraComponent::OnImageAvailable(
	unsigned char* Y, unsigned char* U, unsigned char* V,
	int YRowStride, int UVRowStride, int UVPixelStride,
	int YLength, int ULength, int VLength)
{
	if (OnFrameAvailable.IsBound() || OnFrameAvailableDynamic.IsBound())
	{
		CameraFrame->UpdateFrame(Y, U, V, YRowStride, UVRowStride,
		                         UVPixelStride,
		                         YLength, ULength, VLength);

		AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [&]()
		{
			OnFrameAvailable.Broadcast(CameraFrame.Get());
		});

		// This code is on a Java thread
		AsyncTask(ENamedThreads::GameThread, [&]()
		{
			OnFrameAvailableDynamic.Broadcast(CameraFrame.Get());
			if (Texture.IsValid())
			{
				Texture->UpdateTextureRegions(0, 1, TextureUpdateRegion,
				                              4 * CameraFrame->GetWidth(), 4,
				                              CameraFrame->GetARGBBuffer());

				AsyncTask(ENamedThreads::GameThread, [&]()
				{
					OnTextureAvailableDynamic.Broadcast(Texture.Get());
				});

				AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [&]()
				{
					OnTextureAvailable.Broadcast(Texture.Get());
				});
			}
		});
	}
}
