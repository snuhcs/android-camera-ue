#include "AndroidCameraFrame.h"

#include <cassert>

#include "AndroidCamera.h"
#include "ImageFormatUtils.h"

bool UAndroidCameraFrame::HasYUV() const
{
	return Y && U && V;
}

void UAndroidCameraFrame::BeginDestroy()
{
	if (ARGBBuffer)
	{
		delete[] ARGBBuffer;
		ARGBBuffer = nullptr;
	}
	if (Y)
	{
		delete[] Y;
	}

	if (U)
	{
		delete[] U;
	}

	if (V)
	{
		delete[] V;
	}
	Super::BeginDestroy();
}

void UAndroidCameraFrame::Initialize(int PreviewWidth, int PreviewHeight,
                                     const bool hasYUV,
                                     const EPixelFormat InFormat,
                                     bool CreateTexture)
{
	Width = PreviewWidth;
	Height = PreviewHeight;

	if (!hasYUV)
	{
		Y = U = V = nullptr;
	}
	PixelFormat = InFormat;

	ARGBBuffer = new unsigned char[GetPlaneSize() * 4];
}


int UAndroidCameraFrame::GetWidth() const
{
	return Width;
}

int UAndroidCameraFrame::GetHeight() const
{
	return Height;
}

unsigned char* UAndroidCameraFrame::GetARGBBuffer() const
{
	if (IsBufferDirty && ARGBBuffer && HasYUV())
	{
		SCOPE_CYCLE_COUNTER(STAT_AndroidCameraYUV420toARGB);
		ImageFormatUtils::YUV420ToARGB8888(Y, U, V, Width, Height, YRowStride,
		                                   UVRowStride, UVPixelStride,
		                                   reinterpret_cast<int*>(ARGBBuffer));
		IsBufferDirty = false;
	}
	else if (IsBufferDirty && ARGBBuffer && !HasYUV())
	{
		IsBufferDirty = false;
	}

	return ARGBBuffer;
}

UAndroidCameraFrame::NV12Frame UAndroidCameraFrame::GetData() const
{
	if (!HasYUV())
	{
		UE_LOG(LogCamera, Error,
		       TEXT(
			       "For frames that only have buffer (no YUV), use GetARGBBuffer() instead"
		       ));
	}
	return {Y, U, V, YRowStride, UVRowStride, UVPixelStride};
}

void UAndroidCameraFrame::UpdateFrame(unsigned char* NewY, unsigned char* NewU,
                                      unsigned char* NewV, int NewYRowStride,
                                      int NewUVRowStride, int NewUVPixelStride,
                                      int NewYLength, int NewULength,
                                      int NewVLength)
{
	SCOPE_CYCLE_COUNTER(STAT_AndroidCameraCopyBuffer);

	// Lazily initialize YUV buffer
	// TODO(dostos): move this to `Initialize` if
	// there is a way to determine plane sizes in advance
	if (!Y || NewYLength > YLength)
	{
		if (Y)
		{
			delete[] Y;
		}
		Y = new uint8[NewYLength];
		YLength = NewYLength;
	}

	if (!U || NewULength > ULength)
	{
		if (U)
		{
			delete[] U;
		}
		U = new uint8[NewULength];
		ULength = NewULength;
	}

	if (!V || NewVLength > VLength)
	{
		if (V)
		{
			delete[] V;
		}
		V = new uint8[NewVLength];
		VLength = NewVLength;
	}

	std::memcpy(Y, NewY, YLength);
	std::memcpy(U, NewU, ULength);
	std::memcpy(V, NewV, VLength);

	YRowStride = NewYRowStride;
	UVRowStride = NewUVRowStride;
	UVPixelStride = NewUVPixelStride;

	IsBufferDirty = true;
}

void UAndroidCameraFrame::UpdateFrame(int* NewARGB) const
{
	if (HasYUV())
	{
		UE_LOG(LogCamera, Error,
		       TEXT(
			       "For frames that have YUV buffer, use UpdateFrame(unsigned char *NewY, unsigned char *NewU, unsigned char *NewV, ...) instead"
		       ));
		return;
	}

	if (NewARGB == nullptr)
	{
		UE_LOG(LogCamera, Error, TEXT("NewARGB is null"));
		return;
	}

	// Width && Height do not change
	std::memcpy(ARGBBuffer, NewARGB, GetPlaneSize() * 4);
	IsBufferDirty = true;
}


inline int UAndroidCameraFrame::GetPlaneSize() const
{
	return Width * Height;
}
