// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatMessage.h"
#include "PubnubChatThreadMessage.generated.h"


class UPubnubChatMessage;


/**
 * A message that belongs to a thread (reply to a parent message). Extends UPubnubChatMessage and inherits all message APIs (edit, delete, reactions, etc.).
 * Thread-specific APIs: parent channel ID and pin/unpin this message to the parent channel.
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatThreadMessage : public UPubnubChatMessage
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:
	
	/**
	 * Returns the channel ID of the parent channel (the channel that contains the message this thread replies to).
	 * Local: does not perform any network requests.
	 *
	 * @return Parent channel ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|ThreadMessage")
	FString GetParentChannelID() const { return ParentChannelID; }

	/**
	 * Pins this thread message to the parent channel so it appears as the pinned message on the parent. Resolves the parent channel, then calls PinMessage(this) on it.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the parent channel cannot be resolved.
	 *
	 * @return Operation result. Success if this message was pinned to the parent channel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult PinMessageToParentChannel();
	
	/**
	 * Pins this thread message asynchronously to the parent channel. Fails if the parent channel cannot be resolved.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageToParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pins this thread message asynchronously to the parent channel. Fails if the parent channel cannot be resolved.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void PinMessageToParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Unpins this message from the parent channel only if this message is currently the pinned message on the parent. Resolves the parent channel and pinned message; if this message is pinned, unpins it.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * No-op if this message is not the one currently pinned on the parent channel.
	 *
	 * @return Operation result. Success if this message was unpinned or it was not pinned on the parent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult UnpinMessageFromParentChannel();
	
	/**
	 * Unpins this message asynchronously from the parent channel only if this message is currently the pinned message on the parent. No-op if this message is not pinned on the parent.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unpins this message asynchronously from the parent channel only if this message is currently the pinned message on the parent. No-op if this message is not pinned on the parent.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UnpinMessageFromParentChannelAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
private:
	
	UPROPERTY()
	FString ParentChannelID = "";
	
	void InitThreadMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken, const FString InParentChannelID);
	
};