#include "AndroidCameraComponent.h"
#include "AndroidCamera.h"
#include "ImageFormatUtils.h"
#include "ScopedTimer.h"

#if PLATFORM_ANDROID
extern void AndroidThunkCpp_startCamera();
extern void AndroidThunkCpp_stopCamera();
extern bool newFrame;
extern unsigned char* rawDataAndroid;
#endif

#define print(txt) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green, txt)

UTexture2D*  UAndroidCameraComponent::getAndroidCameraTexture()
{
	if (!bActive)
	{
		//create texture
		androidCameraTexture = UTexture2D::CreateTransient(WIDTH, HEIGHT, PF_B8G8R8A8);
		echoUpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, WIDTH, HEIGHT);
		androidCameraTexture->UpdateResource();
		rawData.Init(FColor(255, 0, 0, 255), WIDTH*HEIGHT);
#if PLATFORM_ANDROID
		AndroidThunkCpp_startCamera();
#endif
		bActive = true;
	}
	return androidCameraTexture;
}

void UAndroidCameraComponent::shutDownCamera()
{
	if (bActive)
	{
		bActive = false;
#if PLATFORM_ANDROID
		AndroidThunkCpp_stopCamera();
#endif
	}
}

// Called every frame
void UAndroidCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


}

void UAndroidCameraComponent::updateCamera(float delta)
{
	timer += delta;
	if (timer >= (1.f / frameRate))
	{
		timer = 0;
		if (androidCameraTexture && bActive)
		{
#if PLATFORM_ANDROID
			void* image = NULL;
			if (newFrame == true)
			{
				image = (void*)rawDataAndroid;
			}

			if (image != NULL)
			{
				updateTexture(image);
				newFrame = false;
			}
#endif
		}
	}

}


void UAndroidCameraComponent::updateTexture(void* data)
{
	if (data == NULL) return;

#if PLATFORM_ANDROID
	// fill my Texture Region data
	FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;
	RegionData->Texture2DResource = (FTexture2DResource*)androidCameraTexture->Resource;
	RegionData->MipIndex = 0;
	RegionData->NumRegions = 1;
	RegionData->Regions = echoUpdateTextureRegion;
	RegionData->SrcPitch = (uint32)(4 * WIDTH);
	RegionData->SrcBpp = 4;

	if (!rawDataAndroid) return;

	RegionData->SrcData = (uint8*)rawDataAndroid;

	bool bFreeData = false;

	// call the RHIUpdateTexture2D to refresh the texture with new info
	ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
		[RegionData, bFreeData](FRHICommandListImmediate& RHICmdList)
	{
		for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
		{
			int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
			if (RegionData->MipIndex >= CurrentFirstMip)
			{
				RHIUpdateTexture2D(
					RegionData->Texture2DResource->GetTexture2DRHI(),
					RegionData->MipIndex - CurrentFirstMip,
					RegionData->Regions[RegionIndex],
					RegionData->SrcPitch,
					RegionData->SrcData
					+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
					+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
				);
			}
		}
		if (bFreeData)
		{
			FMemory::Free(RegionData->Regions);
			FMemory::Free(RegionData->SrcData);
		}
		delete RegionData;
	});

#endif
}