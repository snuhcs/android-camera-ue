#include "AndroidCameraComponent.h"
#include "AndroidCamera.h"

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
		androidCameraTexture = UTexture2D::CreateTransient(width, height, PF_B8G8R8A8);
		echoUpdateTextureRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, width, height);
		androidCameraTexture->UpdateResource();
		rawData.Init(FColor(255, 0, 0, 255), width*height);
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
	RegionData->SrcPitch = (uint32)(4 * width);
	RegionData->SrcBpp = 4;

	char* yuv420sp = (char*)data;
	int* rgb = new int[width * height];
	if (!rawDataAndroid) return;

	int size = width*height;
	int offset = size;

	int u, v, y1, y2, y3, y4;

	for (int i = 0, k = 0; i < size; i += 2, k += 2) {
		y1 = yuv420sp[i] & 0xff;
		y2 = yuv420sp[i + 1] & 0xff;
		y3 = yuv420sp[width + i] & 0xff;
		y4 = yuv420sp[width + i + 1] & 0xff;

		u = yuv420sp[offset + k] & 0xff;
		v = yuv420sp[offset + k + 1] & 0xff;
		u = u - 128;
		v = v - 128;

		rgb[i] = YUVtoRGB(y1, u, v);
		rgb[i + 1] = YUVtoRGB(y2, u, v);
		rgb[width + i] = YUVtoRGB(y3, u, v);
		rgb[width + i + 1] = YUVtoRGB(y4, u, v);

		if (i != 0 && (i + 2) % width == 0)
			i += width;
	}

	RegionData->SrcData = (uint8*)rgb;

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

	delete[] rgb;

#endif
}

int UAndroidCameraComponent::YUVtoRGB(int y, int u, int v)
{
	int r, g, b;
	r = y + (int)1.402f*v;
	g = y - (int)(0.344f*u + 0.714f*v);
	b = y + (int)1.772f*u;
	r = r>255 ? 255 : r<0 ? 0 : r;
	g = g>255 ? 255 : g<0 ? 0 : g;
	b = b>255 ? 255 : b<0 ? 0 : b;
	return 0xff000000 | (b << 16) | (g << 8) | r;
}

