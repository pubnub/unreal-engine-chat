// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatChannel.h"
#include "PubnubChatThreadChannel.generated.h"


class UPubnubChatMessage;


UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatThreadChannel : public UPubnubChatChannel
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FString GetParentChannelID() const { return ParentChannelID; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	UPubnubChatMessage* GetParentMessage() const { return ParentMessage; }
	
private:
	
	UPROPERTY()
	FString ParentChannelID = "";
	
	UPROPERTY()
	UPubnubChatMessage* ParentMessage = nullptr;
	
	bool IsThreadConfirmed = false;
	
	void InitThreadChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InThreadChannelID, UPubnubChatMessage* InParentMessage, bool InIsThreadConfirmed);
	
	
	virtual FPubnubChatOperationResult OnSendText() override;
};