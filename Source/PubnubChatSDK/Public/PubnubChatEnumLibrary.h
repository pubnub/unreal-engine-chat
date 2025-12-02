// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatEnumLibrary.generated.h"


UENUM(BlueprintType)
enum class EPubnubChatConnectionStatus : uint8
{
	PCCS_CONNECTION_ONLINE		UMETA(DisplayName="ConnectionOnline"),
	PCCS_CONNECTION_OFFLINE		UMETA(DisplayName="ConnectionOffline"),
	PCCS_CONNECTION_ERROR		UMETA(DisplayName="ConnectionError")
};