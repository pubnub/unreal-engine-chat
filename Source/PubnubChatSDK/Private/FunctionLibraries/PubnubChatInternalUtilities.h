// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubChatInternalUtilities.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatInternalUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	UFUNCTION()
	static FString GetSoftDeletedObjectPropertyKey();

	UFUNCTION()
	static FString AddDeletedPropertyToCustom(FString CurrentCustom);

	UFUNCTION()
	static FString RemoveDeletedPropertyFromCustom(FString CurrentCustom);
};
