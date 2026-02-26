// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatChannel.h"
#include "PubnubChatThreadChannel.generated.h"


class UPubnubChatMessage;
class UPubnubChatThreadMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatThreadMessageReceived, UPubnubChatThreadMessage*, ThreadMessage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatThreadMessageReceivedNative, UPubnubChatThreadMessage* ThreadMessage);


/**
 * A channel for thread replies attached to a parent message. Extends UPubnubChatChannel and inherits send, subscribe, and other channel APIs.
 * Thread-specific APIs: parent channel/message access, thread history, and pin/unpin a thread message to the parent channel.
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatThreadChannel : public UPubnubChatChannel
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:
	
	/* DELEGATES */
	
	/**
	 * Broadcast when a new message is received in this thread (replies to the parent message).
	 * @param ThreadMessage The received thread message object.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatThreadMessageReceived OnThreadMessageReceived;
	FOnPubnubChatThreadMessageReceivedNative OnThreadMessageReceivedNative;
	
	/* PUBLIC FUNCTIONS */
	
	/**
	 * Returns the channel ID of the parent channel (the channel that contains the parent message).
	 * Local: does not perform any network requests.
	 *
	 * @return Parent channel ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadChannel")
	FString GetParentChannelID() const { return ParentChannelID; }
	
	/**
	 * Returns the parent message object (the message this thread replies to).
	 * Local: does not perform any network requests.
	 *
	 * @return The parent message, or null if not set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadChannel")
	UPubnubChatMessage* GetParentMessage() const { return ParentMessage; }

	/**
	 * Fetches thread message history within a timetoken range from the server. Messages are returned in descending order (newest first).
	 * Blocking: performs a FetchHistory request on the calling thread. Blocks for the duration of the operation.
	 * StartTimetoken must be higher (newer) than EndTimetoken. IsMore is true when the returned count equals Count, indicating more messages may exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param Count Maximum number of messages to return (default 25).
	 * @return Operation result, list of thread messages, and IsMore flag.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatGetThreadHistoryResult GetThreadHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 25);

	/**
	 * Fetches thread message history asynchronously within a timetoken range from the server. Messages are returned in descending order (newest first).
	 * StartTimetoken must be higher (newer) than EndTimetoken. IsMore is true when the returned count equals Count, indicating more messages may exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnThreadHistoryResponse Callback executed when the operation completes.
	 * @param Count Maximum number of messages to return (default 25).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	
	void GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponse OnThreadHistoryResponse, const int Count = 25);
	/**
	 * Fetches thread message history asynchronously within a timetoken range from the server. Messages are returned in descending order (newest first).
	 * StartTimetoken must be higher (newer) than EndTimetoken. IsMore is true when the returned count equals Count, indicating more messages may exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnThreadHistoryResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Count Maximum number of messages to return (default 25).
	 */
	void GetThreadHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetThreadHistoryResponseNative OnThreadHistoryResponseNative, const int Count = 25);

	/**
	 * Pins the given thread message to the parent channel so it appears as the pinned message on the parent. Resolves the parent channel, then calls PinMessage(ThreadMessage) on it.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the thread message is from another thread channel, or if the parent channel cannot be resolved.
	 *
	 * @param ThreadMessage The thread message to pin to the parent channel. Must be valid and belong to this thread channel.
	 * @return Operation result. Success if the message was pinned to the parent channel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatOperationResult PinMessageToParentChannel(UPubnubChatThreadMessage* ThreadMessage);
	
	/**
	 * Pins the given thread message asynchronously to the parent channel. Fails if the thread message is from another thread channel, or if the parent channel cannot be resolved.
	 *
	 * @param ThreadMessage The thread message to pin to the parent channel. Must be valid and belong to this thread channel.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	
	void PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pins the given thread message asynchronously to the parent channel. Fails if the thread message is from another thread channel, or if the parent channel cannot be resolved.
	 *
	 * @param ThreadMessage The thread message to pin to the parent channel. Must be valid and belong to this thread channel.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void PinMessageToParentChannelAsync(UPubnubChatThreadMessage* ThreadMessage, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Unpins the currently pinned message from the parent channel. Resolves the parent channel, then calls UnpinMessage() on it.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * No-op if the parent channel has no pinned message.
	 *
	 * @return Operation result. Success if the message was unpinned or the parent had no pinned message.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel")
	FPubnubChatOperationResult UnpinMessageFromParentChannel();
	
	/**
	 * Unpins the currently pinned message asynchronously from the parent channel. No-op if the parent channel has no pinned message.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|ThreadChannel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unpins the currently pinned message asynchronously from the parent channel. No-op if the parent channel has no pinned message.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
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
	virtual FString CreateMentionEventPayload(FString Timetoken, FString Text) override;
	virtual void AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak) override;
};