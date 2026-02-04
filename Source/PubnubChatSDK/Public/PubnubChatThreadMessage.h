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

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult PinMessageToParentChannel();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageToParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void PinMessageToParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult UnpinMessageFromParentChannel();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
private:
	
	UPROPERTY()
	FString ParentChannelID = "";
	
	void InitThreadMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken, const FString InParentChannelID);
	
};