// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"

#include "PubnubChatMembership.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatChannel;
class UPubnubSubscription;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnPubnubChatMembershipUpdateReceived, EPubnubChatStreamedUpdateType, UpdateType, FString, ChannelID, FString, UserID, FPubnubChatMembershipData, MembershipData);
DECLARE_MULTICAST_DELEGATE_FourParams(FOnPubnubChatMembershipUpdateReceivedNative, EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, FString UserID, const FPubnubChatMembershipData& MembershipData);

/**
 * Represents a user's membership in a channel in the PubNub Chat SDK. Provides access to membership metadata (custom, status, type),
 * last-read message timetoken, unread count, and streaming updates for membership changes (update/delete).
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMembership : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;

	/* DELEGATES */

	/**
	 * Broadcast when this membership is updated or deleted after StreamUpdates is active.
	 * @param UpdateType Indicates the change type (e.g. Added, Updated, Deleted).
	 * @param ChannelID The channel ID of this membership.
	 * @param UserID The user ID of this membership.
	 * @param MembershipData Updated membership data (custom, status, type). Empty when the membership was deleted.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMembershipUpdateReceived OnMembershipUpdateReceived;
	FOnPubnubChatMembershipUpdateReceivedNative OnMembershipUpdateReceivedNative;

	/* PUBLIC FUNCTIONS */

	/**
	 * Returns the current membership data (custom, status, type) from the local cache.
	 * Local: does not perform any network requests. Data may be stale if the membership was updated elsewhere.
	 *
	 * @return Membership data struct, or empty struct if membership is not initialized or not in cache.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FPubnubChatMembershipData GetMembershipData() const;
	
	/**
	 * Returns the user object for this membership.
	 * Local: does not perform any network requests.
	 *
	 * @return The user associated with this membership, or null if not set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	UPubnubChatUser* GetUser() const { return User; }

	/**
	 * Returns the channel object for this membership.
	 * Local: does not perform any network requests.
	 *
	 * @return The channel associated with this membership, or null if not set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	UPubnubChatChannel* GetChannel() const { return Channel; }

	/**
	 * Returns the user ID of this membership.
	 * Local: does not perform any network requests.
	 *
	 * @return User ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FString GetUserID() const;

	/**
	 * Returns the channel ID of this membership.
	 * Local: does not perform any network requests.
	 *
	 * @return Channel ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Membership")
	FString GetChannelID() const;
	
	/**
	 * Returns the last-read message timetoken for this membership from the local cache.
	 * Local: reads from membership data in the local cache. Does not perform any network requests.
	 *
	 * @return Last-read message timetoken string, or empty if not set.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FString GetLastReadMessageTimetoken();
	
	/**
	 * Removes this membership from the channel on the server (the user is removed from the channel's members).
	 * Similar to Channel->Leave() for the current user: both remove the user's membership from the channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @return Operation result. Success if the membership was removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult Delete();
	
	/**
	 * Removes this membership asynchronously from the channel on the server (the user is removed from the channel's members).
	 * Similar to Channel->Leave() for the current user: both remove the user's membership from the channel.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Removes this membership asynchronously from the channel on the server (the user is removed from the channel's members).
	 * Similar to Channel->Leave() for the current user: both remove the user's membership from the channel.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Updates this membership on the server with the given data (custom, status, type) and updates the local cache.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param UpdateMembershipData The new membership data (custom, status, type) and optional ForceSet flags for full replacement.
	 * @return Operation result. Success if the membership was updated on the server and in the local cache.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult Update(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData);
	
	/**
	 * Updates this membership asynchronously on the server with the given data (custom, status, type) and updates the local cache.
	 *
	 * @param UpdateMembershipData The new membership data (custom, status, type) and optional ForceSet flags for full replacement.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UpdateAsync(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Updates this membership asynchronously on the server with the given data (custom, status, type) and updates the local cache.
	 *
	 * @param UpdateMembershipData The new membership data (custom, status, type) and optional ForceSet flags for full replacement.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UpdateAsync(const FPubnubChatUpdateMembershipInputData& UpdateMembershipData, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Sets the last-read message timetoken for this membership on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event so other clients can track read position (if the user has write permission).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Timetoken The message timetoken to set as last read. Must be non-empty.
	 * @return Operation result. Success if the membership was updated (and receipt emitted for non-public channels when permitted).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessageTimetoken(const FString Timetoken);
	
	/**
	 * Sets the last-read message timetoken asynchronously for this membership on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event so other clients can track read position (if the user has write permission).
	 *
	 * @param Timetoken The message timetoken to set as last read. Must be non-empty.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetLastReadMessageTimetokenAsync(const FString Timetoken, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Sets the last-read message timetoken asynchronously for this membership on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event so other clients can track read position (if the user has write permission).
	 *
	 * @param Timetoken The message timetoken to set as last read. Must be non-empty.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void SetLastReadMessageTimetokenAsync(const FString Timetoken, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Sets the last-read message for this membership to the given message's timetoken on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event (if the user has write permission). Equivalent to SetLastReadMessageTimetoken(Message->GetMessageTimetoken()).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Message The message to set as last read. Must be valid (non-null).
	 * @return Operation result. Success if the membership was updated (and receipt emitted for non-public channels when permitted).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership")
	FPubnubChatOperationResult SetLastReadMessage(UPubnubChatMessage* Message);
	
	/**
	 * Sets the last-read message asynchronously for this membership to the given message's timetoken on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event (if the user has write permission).
	 *
	 * @param Message The message to set as last read. Must be valid (non-null).
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetLastReadMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Sets the last-read message asynchronously for this membership to the given message's timetoken on the server and updates the local cache.
	 * For non-public channels, also emits a Receipt chat event (if the user has write permission).
	 *
	 * @param Message The message to set as last read. Must be valid (non-null).
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void SetLastReadMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for membership metadata updates (update/delete) for this membership. Updates are delivered via OnMembershipUpdateReceived / OnMembershipUpdateReceivedNative.
	 * Blocking: subscribes on the calling thread. Blocks until the subscription is established.
	 * No-op if already streaming updates.
	 *
	 * @return Operation result. Success if subscribe succeeded or already streaming.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult StreamUpdates();
	
	/**
	 * Starts listening asynchronously for membership metadata updates (update/delete) for this membership. Updates are delivered via OnMembershipUpdateReceived / OnMembershipUpdateReceivedNative.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for membership metadata updates (update/delete) for this membership. Updates are delivered via OnMembershipUpdateReceived / OnMembershipUpdateReceivedNative.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for membership metadata updates on each of the given memberships. Calls StreamUpdates() on each membership.
	 * Blocking: performs StreamUpdates on each membership on the calling thread. Blocks for the duration of all operations.
	 *
	 * @param Memberships Array of membership objects on which to start streaming updates.
	 * @return Combined operation result from all memberships.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatMembership*>& Memberships);
	
	/**
	 * Stops listening for membership metadata updates for this membership. OnMembershipUpdateReceived will no longer fire.
	 * Blocking: unsubscribes on the calling thread. Blocks for the duration of the operation.
	 * No-op if not streaming updates.
	 *
	 * @return Operation result. Success if unsubscribe succeeded or was not streaming.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	/**
	 * Stops listening asynchronously for membership metadata updates for this membership. OnMembershipUpdateReceived will no longer fire.
	 * No-op if not streaming updates. Success if unsubscribe succeeded or was not streaming.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for membership metadata updates for this membership. OnMembershipUpdateReceived will no longer fire.
	 * No-op if not streaming updates. Success if unsubscribe succeeded or was not streaming.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Returns the count of unread messages in this channel for the current user, based on the last-read message timetoken.
	 * Blocking: performs a MessageCounts request on the calling thread. Blocks for the duration of the operation.
	 * Uses the membership's last-read timetoken (or empty timetoken if not set) to compute messages after that point.
	 *
	 * @return Operation result and the unread message count.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	FPubnubChatGetUnreadMessagesCountResult GetUnreadMessagesCount();
	
	/**
	 * Returns the count of unread messages asynchronously in this channel for the current user, based on the last-read message timetoken.
	 * Uses the membership's last-read timetoken (or empty timetoken if not set) to compute messages after that point.
	 *
	 * @param OnUnreadMessagesCountResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Membership")
	void GetUnreadMessagesCountAsync(FOnPubnubChatGetUnreadMessagesCountResponse OnUnreadMessagesCountResponse);
	/**
	 * Returns the count of unread messages asynchronously in this channel for the current user, based on the last-read message timetoken.
	 * Uses the membership's last-read timetoken (or empty timetoken if not set) to compute messages after that point.
	 *
	 * @param OnUnreadMessagesCountResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetUnreadMessagesCountAsync(FOnPubnubChatGetUnreadMessagesCountResponseNative OnUnreadMessagesCountResponseNative);
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatUser> User = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatChannel> Channel = nullptr;
	
	UPROPERTY()
	bool IsInitialized = false;
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;
	
	bool IsStreamingUpdates = false;

	void InitMembership(UPubnubClient* InPubnubClient, UPubnubChat* InChat, UPubnubChatUser* InUser, UPubnubChatChannel* InChannel);

	/**
	 * Gets the internal composite membership ID used for repository operations.
	 * Format: [ChannelID].[UserID]
	 * @return Composite membership identifier
	 */
	FString GetInternalMembershipID() const;
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

