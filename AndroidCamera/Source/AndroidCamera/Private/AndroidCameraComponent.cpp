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
	bActive = true;

	CameraFrame = NewObject<UAndroidCameraFrame>(this);
	CameraFrame->Initialize(PreviewWidth, PreviewHeight);
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

void UAndroidCameraComponent::OnImageAvailable(
	unsigned char* Y, unsigned char* U, unsigned char* V,
	int YRowStride, int UVRowStride, int UVPixelStride,
	int YLength, int ULength, int VLength)
{
	if (OnFrameAvailable.IsBound() || OnFrameAvailableDynamic.IsBound())
	{
		CameraFrame->UpdateFrame(Y, U, V, YRowStride, UVRowStride, UVPixelStride, YLength, ULength, VLength);
		
		
		AsyncTask(ENamedThreads::AnyHiPriThreadHiPriTask, [&]()
		{
			OnFrameAvailable.Broadcast(CameraFrame);
		});
		
		// This code is on a Java thread
		FFunctionGraphTask::CreateAndDispatchWhenReady([&]()
		{
			OnFrameAvailableDynamic.Broadcast(CameraFrame);
		}, TStatId(), nullptr, ENamedThreads::GameThread);
		
	}
}
