// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"

#include "PubnubChatMessage.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubSubscription;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageUpdated, FString, Timetoken, FPubnubChatMessageData, MessageData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageUpdatedNative, FString Timetoken, const FPubnubChatMessageData& MessageData);

/**
 * Represents a chat message in the PubNub Chat SDK. Provides access to message content, edits, reactions, pin/unpin,
 * forward, report, thread operations, and streaming updates for message actions (edits, reactions, etc.).
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMessage : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChatThreadChannel;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	/**
	 * Broadcast when this message is updated (e.g. edit, reaction, delete action) after StreamUpdates is active.
	 * @param Timetoken The timetoken of this message.
	 * @param MessageData Updated message data (text, message actions, etc.).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageUpdated OnUpdated;
	FOnPubnubChatMessageUpdatedNative OnUpdatedNative;
	
	/* PUBLIC FUNCTIONS */
	
	/**
	 * Returns the current message data (text, channel ID, user ID, message actions such as edits and reactions) from the local cache.
	 * Local: does not perform any network requests. Data may be stale if the message was updated elsewhere.
	 *
	 * @return Message data struct, or empty struct if message is not initialized or not in cache.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatMessageData GetMessageData() const;
	
	/**
	 * Returns the unique timetoken of this message.
	 * Local: does not perform any network requests.
	 *
	 * @return Message timetoken string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetMessageTimetoken() const { return Timetoken; }
	
	/**
	 * Returns the current display text of this message, including the latest edit if any.
	 * Local: reads from the local cache. Resolves the most recent Edited message action to determine the displayed text.
	 *
	 * @return Current text (original or latest edited value). Empty if not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetCurrentText();
	
	/**
	 * Returns message elements (text and mentions) parsed from this message's current text.
	 * When the message was sent from a MessageDraft, the text contains markdown links [text](url); this method parses them
	 * and returns the same structure as MessageDraft's GetMessageElements (plain text segments and mention elements with Start, Length, Text, MentionTarget).
	 * Local: parses from the local cache text (GetCurrentText()). Returns empty array if not initialized.
	 *
	 * @return Array of message elements; concatenating element.Text yields the display text; Start/Length match draft-style indices.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	TArray<FPubnubChatMessageElement> GetMessageElements();

	/**
	 * Returns the message type. For standard chat messages this is "text".
	 * Local: does not perform any network requests.
	 *
	 * @return Message type string (e.g. "text").
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetType() { return "text"; }
	
	/**
	 * Edits this message by adding an Edited message action with the new text on the server and updating the local cache.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param NewText The new text content for the message. Must be non-empty.
	 * @return Operation result. Success if the edit action was added.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult EditText(const FString NewText);
	
	/**
	 * Edits this message asynchronously by adding an Edited message action with the new text on the server and updating the local cache.
	 *
	 * @param NewText The new text content for the message. Must be non-empty.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void EditTextAsync(const FString NewText, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Edits this message asynchronously by adding an Edited message action with the new text on the server and updating the local cache.
	 *
	 * @param NewText The new text content for the message. Must be non-empty.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void EditTextAsync(const FString NewText, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Deletes this message on the server. Hard delete removes the message; soft delete adds a Deleted message action.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * If this message has a thread, the thread is also deleted (after the message delete succeeds).
	 *
	 * @param Soft When true, adds a Deleted message action instead of removing the message from the server (default false).
	 * @return Operation result. Success if the message (and thread if any) was deleted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	/**
	 * Deletes this message asynchronously on the server. Hard delete removes the message; soft delete adds a Deleted message action.
	 * If this message has a thread, the thread is also deleted (after the message delete succeeds).
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Soft When true, adds a Deleted message action instead of removing the message from the server (default false).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse, bool Soft = false);
	/**
	 * Deletes this message asynchronously on the server. Hard delete removes the message; soft delete adds a Deleted message action.
	 * If this message has a thread, the thread is also deleted (after the message delete succeeds).
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Soft When true, adds a Deleted message action instead of removing the message from the server (default false).
	 */
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, bool Soft = false);
	
	/**
	 * Restores a soft-deleted message by removing all Deleted message actions from the server and updating the local cache.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * If this message has a thread, the thread is also restored.
	 *
	 * @return Operation result. Success if the delete actions were removed (and thread restored if any).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Restore();
	
	/**
	 * Restores a soft-deleted message asynchronously by removing all Deleted message actions from the server and updating the local cache.
	 * If this message has a thread, the thread is also restored.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RestoreAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Restores a soft-deleted message asynchronously by removing all Deleted message actions from the server and updating the local cache.
	 * If this message has a thread, the thread is also restored.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void RestoreAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Checks whether this message is marked as deleted (soft-deleted) by looking for a Deleted message action in the local cache.
	 * Local: does not perform any network requests.
	 *
	 * @return Operation result and whether the message has a Deleted message action.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatIsDeletedResult IsDeleted();
	
	/**
	 * Pins this message to its channel. Resolves the channel, then calls PinMessageToChannel on the chat.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the channel for this message does not exist.
	 *
	 * @return Operation result. Success if the message was pinned to the channel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Pin();
	
	/**
	 * Pins this message asynchronously to its channel. Resolves the channel, then pins the message.
	 * Fails if the channel for this message does not exist.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pins this message asynchronously to its channel. Resolves the channel, then pins the message.
	 * Fails if the channel for this message does not exist.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void PinAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Unpins this message from its channel if it is currently the pinned message. Resolves the channel and pinned message; if this message is pinned, unpins it.
	 * Blocking: may perform network requests on the calling thread (e.g. to get pinned message). Blocks for the duration of the operation.
	 * No-op if this message is not the one currently pinned on the channel.
	 *
	 * @return Operation result. Success if the message was unpinned or it was not pinned.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Unpin();
	
	/**
	 * Unpins this message asynchronously from its channel if it is currently the pinned message. Resolves the channel and pinned message; if this message is pinned, unpins it.
	 * No-op if this message is not the one currently pinned on the channel.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unpins this message asynchronously from its channel if it is currently the pinned message. Resolves the channel and pinned message; if this message is pinned, unpins it.
	 * No-op if this message is not the one currently pinned on the channel.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UnpinAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Toggles a reaction (e.g. emoji) for the current user on this message. If the current user already has this reaction, removes it; otherwise adds it.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Reaction The reaction value (e.g. emoji code or string). Must be non-empty.
	 * @return Operation result. Success if the reaction was added or removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult ToggleReaction(const FString Reaction);
	
	/**
	 * Toggles a reaction asynchronously for the current user on this message. If the current user already has this reaction, removes it; otherwise adds it.
	 *
	 * @param Reaction The reaction value (e.g. emoji code or string). Must be non-empty.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ToggleReactionAsync(const FString Reaction, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Toggles a reaction asynchronously for the current user on this message. If the current user already has this reaction, removes it; otherwise adds it.
	 *
	 * @param Reaction The reaction value (e.g. emoji code or string). Must be non-empty.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void ToggleReactionAsync(const FString Reaction, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Returns all reactions (message actions of type Reaction) on this message from the local cache.
	 * Local: does not perform any network requests. Data may be stale if reactions were updated elsewhere.
	 *
	 * @return Operation result and list of reaction message actions (value, user ID, timetoken, etc.).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatGetReactionsResult GetReactions() const;
	
	/**
	 * Checks whether the current user has the given reaction on this message. Reads from the local cache.
	 * Local: does not perform any network requests.
	 *
	 * @param Reaction The reaction value to check (e.g. emoji code or string). Must be non-empty.
	 * @return Operation result and whether the current user has this reaction on this message.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatHasReactionResult HasUserReaction(const FString Reaction) const;
	
	/**
	 * Forwards this message to the given channel. Publishes the message to the target channel with forwarding metadata (original channel, timetoken).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * The target channel must be different from this message's channel.
	 *
	 * @param Channel The channel to forward this message to. Must be valid (non-null) and different from this message's channel.
	 * @return Operation result. Success if the message was published to the target channel.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Forward(UPubnubChatChannel* Channel);
	
	/**
	 * Forwards this message asynchronously to the given channel. Publishes the message to the target channel with forwarding metadata (original channel, timetoken).
	 * The target channel must be different from this message's channel.
	 *
	 * @param Channel The channel to forward this message to. Must be valid (non-null) and different from this message's channel.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ForwardAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Forwards this message asynchronously to the given channel. Publishes the message to the target channel with forwarding metadata (original channel, timetoken).
	 * The target channel must be different from this message's channel.
	 *
	 * @param Channel The channel to forward this message to. Must be valid (non-null) and different from this message's channel.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void ForwardAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Reports this message by emitting a Report chat event to the channel's moderation/restrictions stream (e.g. for moderation review).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Reason Optional reason or context for the report (e.g. for moderators).
	 * @return Operation result. Success if the report event was emitted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message")
	FPubnubChatOperationResult Report(const FString Reason = "");
	
	/**
	 * Reports this message asynchronously by emitting a Report chat event to the channel's moderation/restrictions stream (e.g. for moderation review).
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Reason Optional reason or context for the report (e.g. for moderators).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ReportAsync(FOnPubnubChatOperationResponse OnOperationResponse, const FString Reason = "");
	/**
	 * Reports this message asynchronously by emitting a Report chat event to the channel's moderation/restrictions stream (e.g. for moderation review).
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Reason Optional reason or context for the report (e.g. for moderators).
	 */
	void ReportAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, const FString Reason = "");
	
	/**
	 * Starts listening for message action updates (edits, reactions, delete actions) for this message. Updates are delivered via OnMessageUpdateReceived / OnMessageUpdateReceivedNative.
	 * Blocking: subscribes on the calling thread. Blocks until the subscription is established.
	 * No-op if already streaming updates.
	 *
	 * @return Operation result. Success if subscribe succeeded or already streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StreamUpdates();
	
	/**
	 * Starts listening asynchronously for message action updates (edits, reactions, delete actions) for this message. Updates are delivered via OnMessageUpdateReceived / OnMessageUpdateReceivedNative.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for message action updates (edits, reactions, delete actions) for this message. Updates are delivered via OnMessageUpdateReceived / OnMessageUpdateReceivedNative.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for message action updates on each of the given messages. Calls StreamUpdates() on each message.
	 * Blocking: performs StreamUpdates on each message on the calling thread. Blocks for the duration of all operations.
	 *
	 * @param Messages Array of message objects on which to start streaming updates.
	 * @return Combined operation result from all messages.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatMessage*>& Messages);
	
	/**
	 * Stops listening for message action updates for this message. OnMessageUpdateReceived will no longer fire.
	 * Blocking: unsubscribes on the calling thread. Blocks for the duration of the operation.
	 * No-op if not streaming updates.
	 *
	 * @return Operation result. Success if unsubscribe succeeded or was not streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	/**
	 * Stops listening asynchronously for message action updates for this message. OnMessageUpdateReceived will no longer fire.
	 * No-op if not streaming updates. Success if unsubscribe succeeded or was not streaming.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for message action updates for this message. OnMessageUpdateReceived will no longer fire.
	 * No-op if not streaming updates. Success if unsubscribe succeeded or was not streaming.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Creates a thread channel for replying to this message. Blocking: may perform a network check to ensure the thread does not already exist.
	 * Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 * The thread is created on the server when the first reply is sent.
	 *
	 * @return Operation result and the thread channel object (thread is created on the server when the first reply is sent).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult CreateThread();
	
	/**
	 * Creates a thread channel asynchronously for replying to this message. Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 * The thread is created on the server when the first reply is sent.
	 *
	 * @param OnThreadChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	void CreateThreadAsync(FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	/**
	 * Creates a thread channel asynchronously for replying to this message. Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 * The thread is created on the server when the first reply is sent.
	 *
	 * @param OnThreadChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void CreateThreadAsync(FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	/**
	 * Retrieves the thread channel for this message if it already exists on the server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if no thread exists for this message.
	 *
	 * @return Operation result and the thread channel object (if the thread exists).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatThreadChannelResult GetThread();
	
	/**
	 * Retrieves the thread channel asynchronously for this message if it already exists on the server. Fails if no thread exists for this message.
	 *
	 * @param OnThreadChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	void GetThreadAsync(FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	/**
	 * Retrieves the thread channel asynchronously for this message if it already exists on the server. Fails if no thread exists for this message.
	 *
	 * @param OnThreadChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetThreadAsync(FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	/**
	 * Checks whether this message has a thread by looking for a thread root message action in the local cache.
	 * Local: does not perform any network requests.
	 *
	 * @return Operation result and whether this message has a thread (HasThread true/false).
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|Message")
	FPubnubChatHasThreadResult HasThread() const;
	
	/**
	 * Removes the thread from this message: deletes the thread root message action and the thread channel on the server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @return Operation result. Success if the thread was removed.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message")
	FPubnubChatOperationResult RemoveThread();
	
	/**
	 * Removes the thread asynchronously from this message: deletes the thread root message action and the thread channel on the server.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Message", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RemoveThreadAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Removes the thread asynchronously from this message: deletes the thread root message action and the thread channel on the server.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void RemoveThreadAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

protected:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString Timetoken = "";
	UPROPERTY()
	FString ChannelID = "";
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;

	void InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken);
	void UpdateMessageData(const FPubnubChatMessageData& NewMessageData);

	/**
	 * Gets the internal composite message ID used for repository operations.
	 * Format: [ChannelID].[Timetoken]
	 * @return Composite message identifier
	 */
	FString GetInternalMessageID() const;
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

