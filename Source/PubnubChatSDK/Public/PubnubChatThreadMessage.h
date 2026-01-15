// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatMessage.h"
#include "PubnubChatThreadMessage.generated.h"


class UPubnubChatMessage;


UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatThreadMessage : public UPubnubChatMessage
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadMessage")
	FString GetParentChannelID() const { return ParentChannelID; }

	
private:
	
	UPROPERTY()
	FString ParentChannelID = "";
	
	void InitThreadMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken, const FString InParentChannelID);
	
};