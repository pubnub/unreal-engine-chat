// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubEnumLibrary.h"
#include "PubnubChatInternalConverters.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatInternalConverters : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	static EPubnubChatConnectionStatus SubscriptionStatusToChatConnectionStatus(EPubnubSubscriptionStatus SubscriptionStatus);
	static FPubnubChatConnectionStatusData SubscriptionStatusDataToChatConnectionStatusData(const FPubnubSubscriptionStatusData& SubscriptionStatusData);
	
	static FString ChatMessageActionTypeToString(EPubnubChatMessageActionType ActionType);
	static EPubnubChatMessageActionType StringToChatMessageActionType(const FString& ActionTypeString);
	
};
