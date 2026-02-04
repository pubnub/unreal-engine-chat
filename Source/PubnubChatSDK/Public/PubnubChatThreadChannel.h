// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatChannel.h"
#include "PubnubChatThreadChannel.generated.h"


class UPubnubChatMessage;
class UPubnubChatThreadMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatThreadChannelMessageReceived, UPubnubChatThreadMessage*, ThreadMessage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatThreadChannelMessageReceivedNative, UPubnubChatThreadMessage* ThreadMessage);


UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatThreadChannel : public UPubnubChatChannel
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:
	
	/* DELEGATES */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatThreadChannelMessageReceived OnThreadMessageReceived;
	FOnPubnubChatThreadChannelMessageReceivedNative OnThreadMessageReceivedNative;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadChannel")
	FString GetParentChannelID() const { return ParentChannelID; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadChannel")
	UPubnubChatMessage* GetParentMessage() const { return ParentMessage; }

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatGetThreadHistoryResult GetThreadHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 25);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	void GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponse OnThreadHistoryResponse, const int Count = 25);
	void GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponseNative OnThreadHistoryResponseNative, const int Count = 25);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatOperationResult PinMessageToParentChannel(UPubnubChatThreadMessage* ThreadMessage);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponse OnOperationResponse);
	void PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatOperationResult UnpinMessageFromParentChannel();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
private:
	
	// Hide Channel OnMessageReceived delegates as in this class we have OnThreadMessageReceived
	using UPubnubChatChannel::OnMessageReceived;
	using UPubnubChatChannel::OnMessageReceivedNative;
	
	UPROPERTY()
	FString ParentChannelID = "";
	
	UPROPERTY()
	UPubnubChatMessage* ParentMessage = nullptr;
	
	bool IsThreadConfirmed = false;
	
	void InitThreadChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InThreadChannelID, UPubnubChatMessage* InParentMessage, bool InIsThreadConfirmed);
	
	virtual FPubnubChatOperationResult OnSendText() override;
	virtual void AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak) override;
};