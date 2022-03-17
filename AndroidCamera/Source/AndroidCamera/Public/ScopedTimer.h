#pragma once

#include "CoreMinimal.h"

class ScopedTimer
{
public:
	ScopedTimer(const FString& LogMessage);
	~ScopedTimer();

private:
	FString LogMessage;
	uint64 StartCycle;
};