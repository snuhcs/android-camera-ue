#include "AndroidCameraComponent.h"
#include "AndroidCamera.h"
#include "ImageFormatUtils.h"
#include "ScopedTimer.h"
#include "Rendering/Texture2DResource.h"
#include "HAL/FileManager.h"

#include <fstream>

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

UTexture2D* UAndroidCameraComponent::GetRawTexture(UTexture2D* texture)
{
	const int inputWidth = texture->Resource->GetSizeX();
	const int inputHeight = texture->Resource->GetSizeY();

	const int outputWidth = 512;
	const int outputHeight = 512;
	UTexture2D* newTexture = UTexture2D::CreateTransient(outputWidth, outputHeight);

	FUpdateTextureRegion2D* updateRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, outputWidth, outputHeight);
	FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;
	RegionData->Texture2DResource = (FTexture2DResource*)texture->Resource;
	RegionData->MipIndex = 0;
	RegionData->NumRegions = 1;
	RegionData->Regions = updateRegion;
	RegionData->SrcPitch = (uint32)(4 * inputWidth);
	RegionData->SrcBpp = 4;
	RegionData->SrcData = (uint8*)texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_ONLY);
	
	// call the RHIUpdateTexture2D to refresh the texture with new info
	ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)(
		[RegionData, updateRegion](FRHICommandListImmediate& RHICmdList)
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
			delete updateRegion;
			delete RegionData;
		});

	texture->PlatformData->Mips[0].BulkData.Unlock();


	return texture;
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


	UE_LOG(LogCamera, Display, TEXT("Input texture width %d height %d"), WIDTH, HEIGHT);
	FString path = FString(TEXT("sdcard/UE4Game/BandExample/image.bin"));
	FString yuvPath = FString(TEXT("sdcard/UE4Game/BandExample/yuvImage.bin"));

	{
		std::ofstream file;
		file.open(TCHAR_TO_ANSI(*path), std::ios::binary);
		if (file.is_open()) {
			file.write((char*)rawDataAndroid, WIDTH * HEIGHT * 4);
			file.close();
			UE_LOG(LogCamera, Display, TEXT("Write texture to %s"), *path);
		}
		else {
			UE_LOG(LogCamera, Display, TEXT("Failed to write texture to %s"), *path);
		}
	}

	{
		std::ofstream file;
		file.open(TCHAR_TO_ANSI(*yuvPath), std::ios::binary);
		if (file.is_open()) {
			file.write((char*)yuvDataAndroid, WIDTH * HEIGHT + (WIDTH * HEIGHT) / 2);
			file.close();
			UE_LOG(LogCamera, Display, TEXT("Write texture to %s"), *path);
		}
		else {
			UE_LOG(LogCamera, Display, TEXT("Failed to write texture to %s"), *path);
		}
	}

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