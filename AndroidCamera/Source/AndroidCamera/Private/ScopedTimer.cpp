#include "ScopedTimer.h"
#include "HAL/PlatformTime.h"

ScopedTimer::ScopedTimer(const FString& LogMessage)
	:LogMessage(LogMessage), StartCycle(FPlatformTime::Cycles64())
{
	
}

ScopedTimer::~ScopedTimer()
{
	double Elapsed = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - StartCycle);
	UE_LOG(LogCamera, Log, TEXT("%s %f ms"), *LogMessage, Elapsed);
}
