// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AndroidCamera.h"
#include "AndroidCameraPermission.h"
#include "ImageFormatUtils.h"
#include "ScopedTimer.h"

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "Rendering/Texture2DResource.h"
#include <android/log.h>

#define LOG_TAG "CameraLOG"

int SetupJNICamera(JNIEnv* env);
JNIEnv* ENV = NULL;
static jmethodID jToast;
static jmethodID AndroidThunkJava_startCamera;
static jmethodID AndroidThunkJava_stopCamera;
static bool newFrame = false;
static unsigned char* rawDataAndroid;
static unsigned char* yuvDataAndroid;
#endif

static int WIDTH = 512;
static int HEIGHT = 512;

#define LOCTEXT_NAMESPACE "FAndroidCameraModule"

DEFINE_LOG_CATEGORY(LogCamera);

void FAndroidCameraModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if PLATFORM_ANDROID
	check_android_permission("CAMERA");
	JNIEnv* env = FAndroidApplication::GetJavaEnv();
	SetupJNICamera(env);
#endif
}

void FAndroidCameraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAndroidCameraModule, AndroidCamera)

#if PLATFORM_ANDROID
int SetupJNICamera(JNIEnv* env)
{
	if (!env) return JNI_ERR;

	ENV = env;

	AndroidThunkJava_startCamera = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_startCamera", "(II)V", false);
	if (!AndroidThunkJava_startCamera)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_startCamera Method cant be found T_T ");
		return JNI_ERR;
	}

	AndroidThunkJava_stopCamera = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_stopCamera", "()V", false);
	if (!AndroidThunkJava_stopCamera)
	{
		__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ERROR: AndroidThunkJava_stopCamera Method cant be found T_T ");
		return JNI_ERR;
	}

	//FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, jToast);
	__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "module load success!!! ^_^");

	return JNI_OK;
}

void  AndroidThunkCpp_startCamera()
{
	if (!AndroidThunkJava_startCamera || !ENV) return;
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_startCamera, WIDTH, HEIGHT);
}

void AndroidThunkCpp_stopCamera()
{
	if (!AndroidThunkJava_stopCamera || !ENV) return;
	FJavaWrapper::CallVoidMethod(ENV, FJavaWrapper::GameActivityThis, AndroidThunkJava_stopCamera);
}

extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeGetFrameData(JNIEnv* LocalJNIEnv, jobject LocalThiz, 
	jobject y_byte_buffer, jint y_row_stride, jint y_pixel_stride,
	jobject u_byte_buffer, jint u_row_stride, jint u_pixel_stride,
	jobject v_byte_buffer, jint v_row_stride, jint v_pixel_stride,
	jint width, jint height)
{
	auto y_buffer = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(y_byte_buffer));
	auto u_buffer = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(u_byte_buffer));
	auto v_buffer = reinterpret_cast<unsigned char*>(LocalJNIEnv->GetDirectBufferAddress(v_byte_buffer));
    
	if (rawDataAndroid == nullptr) {
		rawDataAndroid = new unsigned char[width * height * 4];
		WIDTH = width;
		HEIGHT = height;
	}

	if (yuvDataAndroid == nullptr) {
		yuvDataAndroid = new unsigned char[width * height + (width * height / 2)];
	}

	{
		ScopedTimer(TEXT("YUV to RGB"));
		UE_LOG(LogCamera, Log, TEXT("Width %d Height %d Stride %d %d %d"), width, height, y_row_stride, u_row_stride, u_pixel_stride);
		ImageFormatUtils::YUV420ToARGB8888(y_buffer, u_buffer, v_buffer, width, height, y_row_stride, u_row_stride, u_pixel_stride, (int*)rawDataAndroid);
	}

	std::memcpy(yuvDataAndroid, y_buffer, width * height);
	std::memcpy(yuvDataAndroid + width * height, u_buffer, (width * height) / 4);
	std::memcpy(yuvDataAndroid + width * height + (width * height) / 4, v_buffer, (width * height) / 4);

	newFrame = true;
	//__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "new frame arrive ^_^");
	return JNI_TRUE;
}

#endif
