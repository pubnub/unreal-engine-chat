// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatInternalUtilities.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatInternalUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:


	/* SOFT DELETE */
	
	UFUNCTION()
	static FString GetSoftDeletedObjectPropertyKey();

	UFUNCTION()
	static FString AddDeletedPropertyToCustom(FString CurrentCustom);

	UFUNCTION()
	static FString RemoveDeletedPropertyFromCustom(FString CurrentCustom);

	/* PUBLISH MESSAGE */

	UFUNCTION()
	static FString ChatMessageToPublishString(const FString ChatMessage);

	UFUNCTION()
	static FString PublishedStringToChatMessage(const FString PublishedMessage);

	UFUNCTION()
	static FString SendTextMetaFromParams(const FPubnubChatSendTextParams& SendTextParams);

	/* EVENTS */
	
	UFUNCTION()
	static EPubnubChatEventMethod GetDefaultChatEventMethodForEventType(EPubnubChatEventType EventType);

	UFUNCTION()
	static FPubnubChatEvent GetEventFromPubnubMessageData(const FPubnubMessageData& MessageData);
};
