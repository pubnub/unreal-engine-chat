// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.generated.h"



USTRUCT(BlueprintType)
struct FPubnubStringArrayWrapper
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FString> Strings;
};

USTRUCT(BlueprintType)
struct FPubnubChatConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") FString AuthKey = "";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int TypingTimeout = 5000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int TypingTimeoutDifference = 1000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int StoreUserActivityInterval = 600000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") bool StoreUserActivityTimestamps = false;
	
};


