#pragma once

UENUM(BlueprintType)
enum class EVideoStatus : uint8 {
  NOT_INITIALIZED UMETA(DisplayName = "NotInitialized"),
  NOT_STARTED UMETA(DisplayName = "NotStarted"),
  PLAYING UMETA(DisplayName = "Playing"),
  FINISHED UMETA(DisplayName = "Finished"),
};
