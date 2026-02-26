// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "HAL/CriticalSection.h"

#include "PubnubChatChannel.generated.h"

class UPubnubClient;
class UPubnubSubscription;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;
class UPubnubChatCallbackStop;
class UPubnubChatMessageDraft;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReceived, UPubnubChatMessage*, PubnubMessage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReceivedNative, UPubnubChatMessage* PubnubMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatChannelUpdated, FString, ChannelID, FPubnubChatChannelData, ChannelData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatChannelUpdatedNative, FString ChannelID, const FPubnubChatChannelData& ChannelData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatTypingChanged, const TArray<FString>&, TypingUserIDs);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatTypingChangedNative, const TArray<FString>& TypingUserIDs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatReadReceiptReceived, FPubnubChatReadReceipt, ReadReceipt);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatReadReceiptReceivedNative, const FPubnubChatReadReceipt& ReadReceipt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReported, FPubnubChatReportEvent, ReportEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReportedNative, const FPubnubChatReportEvent& ReportEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatPresenceChanged, const TArray<FString>&, UserIDs);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatPresenceChangedNative, const TArray<FString>& UserIDs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatCustomEventReceived, FPubnubChatCustomEvent, CustomEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatCustomEventReceivedNative, const FPubnubChatCustomEvent& CustomEvent);


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatChannel : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChatMessageDraft;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	/**
	 * Broadcast when a new message is received on this channel (after Connect or Join).
	 * @param PubnubMessage The received message object.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageReceived OnMessageReceived;
	FOnPubnubChatMessageReceivedNative OnMessageReceivedNative;
	
	/**
	 * Broadcast when this channel's metadata is updated (after StreamUpdates is active).
	 * For channel deletion, use OnDeleted instead.
	 * @param ChannelID The channel ID (this channel).
	 * @param ChannelData Updated channel metadata.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatChannelUpdated OnUpdated;
	FOnPubnubChatChannelUpdatedNative OnUpdatedNative;
	
	/**
	 * Broadcast when typing indicators change on this channel (after StreamTyping is active). Delivers the list of user IDs currently typing.
	 * @param TypingUserIDs User IDs that are currently typing on this channel.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatTypingChanged OnTypingChanged;
	FOnPubnubChatTypingChangedNative OnTypingChangedNative;
	
	/**
	 * Broadcast when read receipt events are received on this channel (after StreamReadReceipts is active).
	 * @param ReadReceipts Read receipt data (e.g. user, message timetoken, channel).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatReadReceiptReceived OnReadReceiptReceived;
	FOnPubnubChatReadReceiptReceivedNative OnReadReceiptReceivedNative;
	
	/**
	 * Broadcast when a message report event is received for this channel (after StreamMessageReports is active).
	 * @param ReportEvent The report event (user, payload, etc.).
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageReported OnMessageReported;
	FOnPubnubChatMessageReportedNative OnMessageReportedNative;

	/**
	 * Broadcast when this channel is deleted on the server (after StreamUpdates is active).
	 * Subscribe to remove the channel from local UI or lists when it is deleted elsewhere.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatObjectDeleted OnDeleted;
	FOnPubnubChatObjectDeletedNative OnDeletedNative;
	
	/**
	 * Broadcast when the set of users present on this channel changes.
	 * Presence reflects active subscriptions; use WhoIsPresent() or IsPresent() to query current state.
	 * @param UserIDs User IDs currently subscribed to this channel.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatPresenceChanged OnPresenceChanged;
	FOnPubnubChatPresenceChangedNative OnPresenceChangedNative;

	/**
	 * Broadcast when a custom chat event is received on this channel (after StreamCustomEvents is active).
	 * @param CustomEvent Parsed custom event data.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatCustomEventReceived OnCustomEventReceived;
	FOnPubnubChatCustomEventReceivedNative OnCustomEventReceivedNative;
	
	
	/* PUBLIC FUNCTIONS */
	
	/**
	 * Returns the current channel metadata (name, description, custom, status, type) from the local cache.
	 * Local: does not perform any network requests. Data may be stale if the channel was updated elsewhere.
	 *
	 * @return Channel metadata struct, or empty struct if channel is not initialized or not in cache.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatChannelData GetChannelData() const;
	
	/**
	 * Returns the unique identifier of this channel.
	 * Local: does not perform any network requests.
	 *
	 * @return Channel ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FString GetChannelID() const { return ChannelID; }
	
	/**
	 * Updates this channel's metadata on the PubNub server and updates the local cache.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the channel is not initialized or the server request fails.
	 *
	 * @param UpdateChannelData Fields to update (ChannelName, Description, Custom, Status, Type) and ForceSet flags
	 *        (ForceSetChannelName, ForceSetDescription, ForceSetCustom, ForceSetStatus, ForceSetType) to control
	 *        whether each field is merged or fully replaced. Use ForceSetAllFields() for full replacement.
	 * @return Operation result with success or error details.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Update(FPubnubChatUpdateChannelInputData UpdateChannelData);
	
	/**
	 * Updates this channel's metadata asynchronously on the PubNub server and updates the local cache.
	 * Fails if the channel is not initialized or the server request fails.
	 *
	 * @param UpdateChannelData Fields to update (ChannelName, Description, Custom, Status, Type) and ForceSet flags
	 *        (ForceSetChannelName, ForceSetDescription, ForceSetCustom, ForceSetStatus, ForceSetType) to control
	 *        whether each field is merged or fully replaced. Use ForceSetAllFields() for full replacement.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UpdateAsync(FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Updates this channel's metadata asynchronously on the PubNub server and updates the local cache.
	 * Fails if the channel is not initialized or the server request fails.
	 *
	 * @param UpdateChannelData Fields to update (ChannelName, Description, Custom, Status, Type) and ForceSet flags
	 *        (ForceSetChannelName, ForceSetDescription, ForceSetCustom, ForceSetStatus, ForceSetType) to control
	 *        whether each field is merged or fully replaced. Use ForceSetAllFields() for full replacement.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UpdateAsync(FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Subscribes to this channel so that new messages are delivered via OnMessageReceived / OnMessageReceivedNative.
	 * Blocking: performs subscription on the calling thread. Blocks until the subscription is established.
	 * No-op if already connected. Does not create or update membership; use Join() to join as a member.
	 *
	 * @return Operation result. Success if subscribe succeeded or channel was already connected.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Connect();
	
	/**
	 * Subscribes asynchronously to this channel so that new messages are delivered via OnMessageReceived / OnMessageReceivedNative.
	 * No-op if already connected. Does not create or update membership; use JoinAsync() to join as a member.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ConnectAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Subscribes asynchronously to this channel so that new messages are delivered via OnMessageReceived / OnMessageReceivedNative.
	 * No-op if already connected. Does not create or update membership; use JoinAsync() to join as a member.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void ConnectAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Adds the current user as a member of this channel, then connects to the channel.
	 * Blocking: performs network requests and subscription on the calling thread. Blocks for the duration of the operation.
	 * Sets the user's membership (with optional metadata), subscribes for messages, and sets last-read-message timetoken.
	 * For public channels this establishes membership; for group/direct it typically confirms an invite.
	 *
	 * @param MembershipData Optional membership metadata (Custom, Status, Type) for the current user in this channel.
	 *        If not provided, defaults are used. Status is forced to be set so a previously invited user is updated correctly.
	 * @return Operation result, and the created or updated membership object for the current user.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatJoinResult Join(FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());
	
	/**
	 * Adds the current user as a member of this channel asynchronously, then connects to the channel.
	 * Sets the user's membership (with optional metadata), subscribes for messages, and sets last-read-message timetoken.
	 * For public channels this establishes membership; for group/direct it typically confirms an invite.
	 *
	 * @param OnJoinResponse Callback executed when the operation completes.
	 * @param MembershipData Optional membership metadata (Custom, Status, Type) for the current user in this channel.
	 *        If not provided, defaults are used. Status is forced to be set so a previously invited user is updated correctly.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	void JoinAsync(FOnPubnubChatJoinResponse OnJoinResponse, FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());
	/**
	 * Adds the current user as a member of this channel asynchronously, then connects to the channel.
	 * Sets the user's membership (with optional metadata), subscribes for messages, and sets last-read-message timetoken.
	 * For public channels this establishes membership; for group/direct it typically confirms an invite.
	 *
	 * @param OnJoinResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param MembershipData Optional membership metadata (Custom, Status, Type) for the current user in this channel.
	 *        If not provided, defaults are used. Status is forced to be set so a previously invited user is updated correctly.
	 */
	void JoinAsync(FOnPubnubChatJoinResponseNative OnJoinResponseNative, FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());
	
	/**
	 * Unsubscribes from this channel. Stops receiving new messages; OnMessageReceived will no longer fire.
	 * Blocking: performs unsubscribe on the calling thread. Blocks for the duration of the operation.
	 * Does not remove membership; use Leave() to leave the channel and remove membership.
	 *
	 * @return Operation result. Success if unsubscribe succeeded or channel was not connected.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Disconnect();
	
	/**
	 * Unsubscribes asynchronously from this channel. Stops receiving new messages when the operation completes; OnMessageReceived will no longer fire after that.
	 * Does not remove membership; use LeaveAsync() to leave the channel and remove membership.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DisconnectAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unsubscribes asynchronously from this channel. Stops receiving new messages when the operation completes; OnMessageReceived will no longer fire after that.
	 * Does not remove membership; use LeaveAsync() to leave the channel and remove membership.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DisconnectAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Removes the current user's membership from this channel and unsubscribes (Disconnect).
	 * Blocking: performs network requests and unsubscribe on the calling thread. Blocks for the duration of the operation.
	 *
	 * @return Operation result combining Disconnect and RemoveMemberships steps.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Leave();
	
	/**
	 * Removes the current user's membership from this channel asynchronously and unsubscribes (Disconnect).
	 * Result combines Disconnect and RemoveMemberships steps.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void LeaveAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Removes the current user's membership from this channel asynchronously and unsubscribes (Disconnect).
	 * Result combines Disconnect and RemoveMemberships steps.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void LeaveAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Publishes a text message to this channel on the PubNub server.
	 * Blocking: performs network request on the calling thread. May block additionally if rate limiting applies (sleep).
	 * Validates that a quoted message, if provided, belongs to this channel and has a valid timetoken.
	 *
	 * @param Message The text content to send.
	 * @param SendTextParams Optional settings: StoreInHistory (default true), SendByPost (default false), Meta (JSON string),
	 *        QuotedMessage (message to quote; must be from this channel and have a valid timetoken).
	 * @return Operation result. Success if publish succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult SendText(const FString Message, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
	/**
	 * Publishes a text message asynchronously to this channel on the PubNub server.
	 * Validates that a quoted message, if provided, belongs to this channel and has a valid timetoken.
	 *
	 * @param Message The text content to send.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param SendTextParams Optional settings: StoreInHistory (default true), SendByPost (default false), Meta (JSON string),
	 *        QuotedMessage (message to quote; must be from this channel and have a valid timetoken).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SendTextAsync(const FString Message, FOnPubnubChatOperationResponse OnOperationResponse,  FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	/**
	 * Publishes a text message asynchronously to this channel on the PubNub server.
	 * Validates that a quoted message, if provided, belongs to this channel and has a valid timetoken.
	 *
	 * @param Message The text content to send.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param SendTextParams Optional settings: StoreInHistory (default true), SendByPost (default false), Meta (JSON string),
	 *        QuotedMessage (message to quote; must be from this channel and have a valid timetoken).
	 */
	void SendTextAsync(const FString Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
	/**
	 * Invites a user to this channel. Adds the user as a member with "pending" status, emits an Invite event, and sets last-read timetoken.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * If the user is already a member, returns their existing membership without sending another invite.
	 *
	 * @param User The chat user object to invite. Must be valid (non-null).
	 * @return Operation result and the created or existing membership for the invited user.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatInviteResult Invite(UPubnubChatUser* User);
	
	/**
	 * Invites a user asynchronously to this channel. Adds the user as a member with "pending" status, emits an Invite event, and sets last-read timetoken.
	 * If the user is already a member, returns their existing membership without sending another invite.
	 *
	 * @param User The chat user object to invite. Must be valid (non-null).
	 * @param OnInviteResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	void InviteAsync(UPubnubChatUser* User, FOnPubnubChatInviteResponse OnInviteResponse);
	/**
	 * Invites a user asynchronously to this channel. Adds the user as a member with "pending" status, emits an Invite event, and sets last-read timetoken.
	 * If the user is already a member, returns their existing membership without sending another invite.
	 *
	 * @param User The chat user object to invite. Must be valid (non-null).
	 * @param OnInviteResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void InviteAsync(UPubnubChatUser* User, FOnPubnubChatInviteResponseNative OnInviteResponseNative);
	
	/**
	 * Invites multiple users to this channel in one request. Adds each user as a member with "pending" status, emits Invite events, and sets last-read timetoken per user.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Invalid or null users in the array are skipped. At least one valid user is required.
	 *
	 * @param Users Array of chat user objects to invite. At least one valid user required; null/invalid entries are ignored.
	 * @return Operation result and array of created memberships for the invited users.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
    FPubnubChatInviteMultipleResult InviteMultiple(TArray<UPubnubChatUser*> Users);
	
	/**
	 * Invites multiple users asynchronously to this channel in one request. Adds each user as a member with "pending" status, emits Invite events, and sets last-read timetoken per user.
	 * Invalid or null users in the array are skipped. At least one valid user is required.
	 *
	 * @param Users Array of chat user objects to invite. At least one valid user required; null/invalid entries are ignored.
	 * @param OnInviteMultipleResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	void InviteMultipleAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatInviteMultipleResponse OnInviteMultipleResponse);
	/**
	 * Invites multiple users asynchronously to this channel in one request. Adds each user as a member with "pending" status, emits Invite events, and sets last-read timetoken per user.
	 * Invalid or null users in the array are skipped. At least one valid user is required.
	 *
	 * @param Users Array of chat user objects to invite. At least one valid user required; null/invalid entries are ignored.
	 * @param OnInviteMultipleResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void InviteMultipleAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatInviteMultipleResponseNative OnInviteMultipleResponseNative);
	
	/**
	 * Pins a message to this channel. Stores the pinned message reference in the channel's custom metadata and updates the server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * The message must belong to this channel or to a thread whose parent is this channel. Fails if the message is from another channel.
	 *
	 * @param Message The message to pin. Must be from this channel or from a thread on this channel. Must be valid (non-null).
	 * @return Operation result. Success if the channel metadata was updated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult PinMessage(UPubnubChatMessage* Message);
	
	/**
	 * Pins a message asynchronously to this channel. Stores the pinned message reference in the channel's custom metadata and updates the server.
	 * The message must belong to this channel or to a thread whose parent is this channel. Fails if the message is from another channel.
	 *
	 * @param Message The message to pin. Must be from this channel or from a thread on this channel. Must be valid (non-null).
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pins a message asynchronously to this channel. Stores the pinned message reference in the channel's custom metadata and updates the server.
	 * The message must belong to this channel or to a thread whose parent is this channel. Fails if the message is from another channel.
	 *
	 * @param Message The message to pin. Must be from this channel or from a thread on this channel. Must be valid (non-null).
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void PinMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Unpins the currently pinned message from this channel. Removes the pinned message from the channel's custom metadata and updates the server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * No-op if there is no pinned message.
	 *
	 * @return Operation result. Success if metadata was updated or there was nothing to unpin.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult UnpinMessage();
	
	/**
	 * Unpins the currently pinned message asynchronously from this channel. Removes the pinned message from the channel's custom metadata and updates the server.
	 * No-op if there is no pinned message.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unpins the currently pinned message asynchronously from this channel. Removes the pinned message from the channel's custom metadata and updates the server.
	 * No-op if there is no pinned message.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UnpinMessageAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Returns the message currently pinned to this channel, if any. Reads from the local cache; if the pinned message is in a thread, fetches it from the server.
	 * Blocking: may perform network requests on the calling thread if the pinned message is in a thread channel.
	 * Returns an empty result if no message is pinned.
	 *
	 * @return Operation result and the pinned message object, or empty result if no pinned message or on error.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMessageResult GetPinnedMessage();
	
	/**
	 * Returns asynchronously the message currently pinned to this channel, if any. Reads from the local cache; if the pinned message is in a thread, fetches it from the server.
	 * Returns an empty result if no message is pinned. May perform network requests if the pinned message is in a thread channel.
	 *
	 * @param OnMessageResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetPinnedMessageAsync(FOnPubnubChatMessageResponse OnMessageResponse);
	/**
	 * Returns asynchronously the message currently pinned to this channel, if any. Reads from the local cache; if the pinned message is in a thread, fetches it from the server.
	 * Returns an empty result if no message is pinned. May perform network requests if the pinned message is in a thread channel.
	 *
	 * @param OnMessageResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetPinnedMessageAsync(FOnPubnubChatMessageResponseNative OnMessageResponseNative);
	
	/**
	 * Lists user IDs currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of user IDs to return (default 1000).
	 * @param Offset Number of users to skip for pagination (default 0).
	 * @return Operation result and list of user IDs present on this channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatWhoIsPresentResult WhoIsPresent(int Limit = 1000, int Offset = 0);
	
	/**
	 * Lists user IDs asynchronously currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param OnWhoIsPresentResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of user IDs to return (default 1000).
	 * @param Offset Number of users to skip for pagination (default 0).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void WhoIsPresentAsync(FOnPubnubChatWhoIsPresentResponse OnWhoIsPresentResponse, int Limit = 1000, int Offset = 0);
	/**
	 * Lists user IDs asynchronously currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param OnWhoIsPresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of user IDs to return (default 1000).
	 * @param Offset Number of users to skip for pagination (default 0).
	 */
	void WhoIsPresentAsync(FOnPubnubChatWhoIsPresentResponseNative OnWhoIsPresentResponseNative, int Limit = 1000, int Offset = 0);
	
	/**
	 * Checks whether a user is currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @return Operation result and whether the user is present on this channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatIsPresentResult IsPresent(const FString UserID);
	
	/**
	 * Checks asynchronously whether a user is currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @param OnIsPresentResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void IsPresentAsync(const FString UserID, FOnPubnubChatIsPresentResponse OnIsPresentResponse);
	/**
	 * Checks asynchronously whether a user is currently present (subscribed) on this channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @param OnIsPresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void IsPresentAsync(const FString UserID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative);

	/**
	 * Deletes this channel on the PubNub server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Delete();

	/**
	 * Deletes this channel asynchronously on the PubNub server.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Deletes this channel asynchronously on the PubNub server.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Retrieves members of this channel from the PubNub server. Returns user and membership data for each member.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of members to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 * @return Operation result, list of memberships (user + membership data), pagination data, and total count.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMembershipsResult GetMembers(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Retrieves members of this channel asynchronously from the PubNub server. Returns user and membership data for each member.
	 *
	 * @param OnMembershipsResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of members to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetMembersAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves members of this channel asynchronously from the PubNub server. Returns user and membership data for each member.
	 *
	 * @param OnMembershipsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of members to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	void GetMembersAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Retrieves invitees of this channel: members with "pending" status. Same as GetMembers with a filter for status == "pending".
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of invitees to return. Pass 0 to use the server default.
	 * @param Filter Additional filter expression combined with status == "pending". Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 * @return Operation result, list of memberships (invitees), pagination data, and total count.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMembershipsResult GetInvitees(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Retrieves invitees of this channel asynchronously (members with "pending" status). Same as GetMembers with a filter for status == "pending".
	 *
	 * @param OnMembershipsResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of invitees to return. Pass 0 to use the server default.
	 * @param Filter Additional filter expression combined with status == "pending". Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetInviteesAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves invitees of this channel asynchronously (members with "pending" status). Same as GetMembers with a filter for status == "pending".
	 *
	 * @param OnMembershipsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of invitees to return. Pass 0 to use the server default.
	 * @param Filter Additional filter expression combined with status == "pending". Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	void GetInviteesAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Sets or lifts moderation restrictions (ban, mute) for a user on this channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param UserID Unique identifier of the user to restrict or unrestrict.
	 * @param Ban When true, bans the user from this channel; when false, lifts ban if present.
	 * @param Mute When true, mutes the user on this channel; when false, lifts mute if present.
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult SetRestrictions(const FString UserID, bool Ban, bool Mute, FString Reason = "");
	
	/**
	 * Sets or lifts moderation restrictions (ban, mute) asynchronously for a user on this channel.
	 *
	 * @param UserID Unique identifier of the user to restrict or unrestrict.
	 * @param Ban When true, bans the user from this channel; when false, lifts ban if present.
	 * @param Mute When true, mutes the user on this channel; when false, lifts mute if present.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetRestrictionsAsync(const FString UserID, bool Ban, bool Mute, FOnPubnubChatOperationResponse OnOperationResponse, FString Reason = "");
	/**
	 * Sets or lifts moderation restrictions (ban, mute) asynchronously for a user on this channel.
	 *
	 * @param UserID Unique identifier of the user to restrict or unrestrict.
	 * @param Ban When true, bans the user from this channel; when false, lifts ban if present.
	 * @param Mute When true, mutes the user on this channel; when false, lifts mute if present.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 */
	void SetRestrictionsAsync(const FString UserID, bool Ban, bool Mute, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FString Reason = "");
	
	/**
	 * Retrieves moderation restrictions (ban, mute) for a specific user on this channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Returns a restriction with Ban/Mute false if the user has no restrictions on this channel.
	 *
	 * @param User The chat user object to query. Must be valid (non-null).
	 * @return Operation result and the user's restriction (Ban, Mute, Reason) on this channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionResult GetUserRestrictions(UPubnubChatUser* User);
	
	/**
	 * Retrieves moderation restrictions (ban, mute) asynchronously for a specific user on this channel.
	 * Returns a restriction with Ban/Mute false if the user has no restrictions on this channel.
	 *
	 * @param User The chat user object to query. Must be valid (non-null).
	 * @param OnRestrictionResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetUserRestrictionsAsync(UPubnubChatUser* User, FOnPubnubChatGetRestrictionResponse OnRestrictionResponse);
	/**
	 * Retrieves moderation restrictions (ban, mute) asynchronously for a specific user on this channel.
	 * Returns a restriction with Ban/Mute false if the user has no restrictions on this channel.
	 *
	 * @param User The chat user object to query. Must be valid (non-null).
	 * @param OnRestrictionResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetUserRestrictionsAsync(UPubnubChatUser* User, FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponseNative);
	
	/**
	 * Retrieves moderation restrictions for all restricted users on this channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 * @return Operation result, list of restrictions, pagination data, and total count.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionsResult GetUsersRestrictions(const int Limit = 0, FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Retrieves moderation restrictions asynchronously for all restricted users on this channel.
	 *
	 * @param OnRestrictionsResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetUsersRestrictionsAsync(FOnPubnubChatGetRestrictionsResponse OnRestrictionsResponse, const int Limit = 0, FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves moderation restrictions asynchronously for all restricted users on this channel.
	 *
	 * @param OnRestrictionsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	void GetUsersRestrictionsAsync(FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponseNative, const int Limit = 0, FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Fetches message history for this channel within a timetoken range from the PubNub server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * StartTimetoken must be higher (newer) than EndTimetoken. Result.IsMore is true when more messages exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param Count Maximum number of messages to return (default 25).
	 * @return Operation result, list of messages in the range, and IsMore flag when more messages exist.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetHistoryResult GetHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 25);

	/**
	 * Fetches message history asynchronously for this channel within a timetoken range from the PubNub server.
	 * StartTimetoken must be higher (newer) than EndTimetoken. Result.IsMore is true when more messages exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnHistoryResponse Callback executed when the operation completes.
	 * @param Count Maximum number of messages to return (default 25).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetHistoryResponse OnHistoryResponse, const int Count = 25);
	/**
	 * Fetches message history asynchronously for this channel within a timetoken range from the PubNub server.
	 * StartTimetoken must be higher (newer) than EndTimetoken. Result.IsMore is true when more messages exist in the range.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnHistoryResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Count Maximum number of messages to return (default 25).
	 */
	void GetHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatGetHistoryResponseNative OnHistoryResponseNative, const int Count = 25);
	
	/**
	 * Fetches a single message by timetoken from this channel's history on the PubNub server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Timetoken The timetoken of the message to fetch. Must be non-empty.
	 * @return Operation result and the message object, or empty result if not found or on error.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMessageResult GetMessage(const FString Timetoken);
	
	/**
	 * Fetches a single message asynchronously by timetoken from this channel's history on the PubNub server.
	 *
	 * @param Timetoken The timetoken of the message to fetch. Must be non-empty.
	 * @param OnMessageResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetMessageAsync(const FString Timetoken, FOnPubnubChatMessageResponse OnMessageResponse);
	/**
	 * Fetches a single message asynchronously by timetoken from this channel's history on the PubNub server.
	 *
	 * @param Timetoken The timetoken of the message to fetch. Must be non-empty.
	 * @param OnMessageResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetMessageAsync(const FString Timetoken, FOnPubnubChatMessageResponseNative OnMessageResponseNative);
	
	/**
	 * Forwards a message to this channel. Publishes the message to this channel with forwarding metadata (original channel, timetoken).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * The message must not already be from this channel (target must differ from source).
	 *
	 * @param Message The message to forward. Must be valid (non-null) and from a different channel.
	 * @return Operation result. Success if the message was published to this channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult ForwardMessage(UPubnubChatMessage* Message);
	
	/**
	 * Forwards a message asynchronously to this channel. Publishes the message to this channel with forwarding metadata (original channel, timetoken).
	 * The message must not already be from this channel (target must differ from source).
	 *
	 * @param Message The message to forward. Must be valid (non-null) and from a different channel.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ForwardMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Forwards a message asynchronously to this channel. Publishes the message to this channel with forwarding metadata (original channel, timetoken).
	 * The message must not already be from this channel (target must differ from source).
	 *
	 * @param Message The message to forward. Must be valid (non-null) and from a different channel.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void ForwardMessageAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Publishes a custom event payload to this channel.
	 * Blocking: performs network request on the calling thread. Blocks for the duration of the operation.
	 * Always uses Publish (never Signal).
	 *
	 * @param Payload Raw payload to publish.
	 * @param Type Optional custom message type written to PublishSettings.CustomMessageType.
	 * @param StoreInHistory Whether the message should be stored in history.
	 * @return Operation result. Success if publish succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult EmitCustomEvent(FString Payload, FString Type = "", bool StoreInHistory = true);
	/**
	 * Publishes a custom event payload to this channel asynchronously.
	 * Always uses Publish (never Signal).
	 *
	 * @param Payload Raw payload to publish.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Type Optional custom message type written to PublishSettings.CustomMessageType.
	 * @param StoreInHistory Whether the message should be stored in history.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void EmitCustomEventAsync(FString Payload, FOnPubnubChatOperationResponse OnOperationResponse, FString Type = "", bool StoreInHistory = true);
	/**
	 * Publishes a custom event payload to this channel asynchronously.
	 * Always uses Publish (never Signal).
	 *
	 * @param Payload Raw payload to publish.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Type Optional custom message type written to PublishSettings.CustomMessageType.
	 * @param StoreInHistory Whether the message should be stored in history.
	 */
	void EmitCustomEventAsync(FString Payload, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FString Type = "", bool StoreInHistory = true);
	
	/**
	 * Emits a mention event for a user on a message in this channel (e.g. for @mention notifications).
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param UserID Unique identifier of the user being mentioned.
	 * @param Timetoken The timetoken of the message that contains the mention.
	 * @param Text The mention text (e.g. the displayed snippet).
	 * @return Operation result. Success if the mention event was emitted.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult EmitUserMention(const FString UserID, const FString Timetoken, const FString Text);
	
	/**
	 * Emits a mention event asynchronously for a user on a message in this channel (e.g. for @mention notifications).
	 *
	 * @param UserID Unique identifier of the user being mentioned.
	 * @param Timetoken The timetoken of the message that contains the mention.
	 * @param Text The mention text (e.g. the displayed snippet).
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void EmitUserMentionAsync(const FString UserID, const FString Timetoken, const FString Text, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Emits a mention event asynchronously for a user on a message in this channel (e.g. for @mention notifications).
	 *
	 * @param UserID Unique identifier of the user being mentioned.
	 * @param Timetoken The timetoken of the message that contains the mention.
	 * @param Text The mention text (e.g. the displayed snippet).
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void EmitUserMentionAsync(const FString UserID, const FString Timetoken, const FString Text, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for channel metadata updates (and delete events) on this channel. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * Blocking: subscribes on the calling thread. Blocks until the subscription is established.
	 * No-op if already streaming updates.
	 *
	 * @return Operation result. Success if subscribe succeeded or already streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamUpdates();
	
	/**
	 * Starts listening asynchronously for channel metadata updates (and delete events) on this channel. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for channel metadata updates (and delete events) on this channel. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for channel metadata updates on each of the given channels. Calls StreamUpdates() on each channel.
	 * Blocking: performs StreamUpdates on each channel on the calling thread. Blocks for the duration of all operations.
	 *
	 * @param Channels Array of channel objects on which to start streaming updates.
	 * @return Combined operation result from all channels.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatChannel*>& Channels);
	
	/**
	 * Stops listening for channel metadata updates on this channel. OnUpdated and OnDeleted will no longer fire.
	 * Blocking: unsubscribes on the calling thread. Blocks for the duration of the operation.
	 * No-op if not streaming updates.
	 *
	 * @return Operation result. Success if unsubscribe succeeded or was not streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	/**
	 * Stops listening asynchronously for channel metadata updates on this channel. OnUpdated and OnDeleted will no longer fire.
	 * No-op if not streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for channel metadata updates on this channel. OnUpdated and OnDeleted will no longer fire.
	 * No-op if not streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Starts listening for presence updates on this channel.
	 * Local: sets up client-side listener and subscription dedicated to presence events. No-op if already streaming presence.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamPresence();

	/**
	 * Starts listening asynchronously for presence updates on this channel. No-op if already streaming presence.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamPresenceAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for presence updates on this channel. No-op if already streaming presence.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamPresenceAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Stops listening for presence updates on this channel.
	 * Local: stops the listener and unsubscribes presence stream. No-op if not streaming presence.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingPresence();

	/**
	 * Stops listening asynchronously for presence updates on this channel. No-op if not streaming presence.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingPresenceAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for presence updates on this channel. No-op if not streaming presence.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingPresenceAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Emits a typing-start event for the current user on this channel. Not supported on public channels; use for group or direct channels.
	 * Blocking: performs network request on the calling thread. Blocks for the duration of the operation.
	 * Rate-limited: repeated calls within a short window may be ignored to avoid spamming.
	 *
	 * @return Operation result. Success if the typing event was emitted.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StartTyping();
	
	/**
	 * Emits a typing-start event asynchronously for the current user on this channel. Not supported on public channels; use for group or direct channels.
	 * Rate-limited: repeated calls within a short window may be ignored to avoid spamming.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StartTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Emits a typing-start event asynchronously for the current user on this channel. Not supported on public channels; use for group or direct channels.
	 * Rate-limited: repeated calls within a short window may be ignored to avoid spamming.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StartTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Emits a typing-stop event for the current user on this channel. Not supported on public channels.
	 * Blocking: performs network request on the calling thread. Blocks for the duration of the operation.
	 * No-op if StartTyping was not recently called or the typing timeout has already elapsed.
	 *
	 * @return Operation result. Success if the typing-stop event was emitted.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopTyping();
	
	/**
	 * Emits a typing-stop event asynchronously for the current user on this channel. Not supported on public channels.
	 * No-op if StartTyping was not recently called or the typing timeout has already elapsed.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Emits a typing-stop event asynchronously for the current user on this channel. Not supported on public channels.
	 * No-op if StartTyping was not recently called or the typing timeout has already elapsed.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for typing events on this channel. Typing indicators are delivered via OnTypingReceived / OnTypingReceivedNative.
	 * Not supported on public channels; use for group or direct channels.
	 * Local: sets up client-side listener and subscription. No-op if already streaming typing.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamTyping();
	
	/**
	 * Starts listening asynchronously for typing events on this channel. Typing indicators are delivered via OnTypingReceived / OnTypingReceivedNative.
	 * Not supported on public channels; use for group or direct channels. No-op if already streaming typing.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for typing events on this channel. Typing indicators are delivered via OnTypingReceived / OnTypingReceivedNative.
	 * Not supported on public channels; use for group or direct channels. No-op if already streaming typing.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Stops listening for typing events on this channel. OnTypingReceived will no longer fire.
	 * Local: stops the listener and clears the callback. No-op if not streaming typing.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingTyping();
	
	/**
	 * Stops listening asynchronously for typing events on this channel. OnTypingReceived will no longer fire.
	 * No-op if not streaming typing.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingTypingAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for typing events on this channel. OnTypingReceived will no longer fire.
	 * No-op if not streaming typing.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingTypingAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for read receipt events on this channel. Read receipts are delivered via OnReadReceiptReceived / OnReadReceiptReceivedNative.
	 * Not supported on public channels; use for group or direct channels.
	 * Local: sets up client-side listener and subscription.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamReadReceipts();
	
	/**
	 * Starts listening asynchronously for read receipt events on this channel. Read receipts are delivered via OnReadReceiptReceived / OnReadReceiptReceivedNative.
	 * Not supported on public channels; use for group or direct channels.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamReadReceiptsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for read receipt events on this channel. Read receipts are delivered via OnReadReceiptReceived / OnReadReceiptReceivedNative.
	 * Not supported on public channels; use for group or direct channels.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamReadReceiptsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Stops listening for read receipt events on this channel. OnReadReceiptReceived will no longer fire.
	 * Local: stops the listener. No-op if not streaming read receipts.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingReadReceipts();
	
	/**
	 * Stops listening asynchronously for read receipt events on this channel. OnReadReceiptReceived will no longer fire.
	 * No-op if not streaming read receipts.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingReadReceiptsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for read receipt events on this channel. OnReadReceiptReceived will no longer fire.
	 * No-op if not streaming read receipts.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingReadReceiptsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for message report events for this channel. Report events are delivered via OnMessageReportReceived / OnMessageReportReceivedNative.
	 * Local: sets up client-side listener on the channel's moderation/restrictions stream. No-op if already streaming.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamMessageReports();
	
	/**
	 * Starts listening asynchronously for message report events for this channel. Report events are delivered via OnMessageReportReceived / OnMessageReportReceivedNative.
	 * Sets up client-side listener on the channel's moderation/restrictions stream. No-op if already streaming.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamMessageReportsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for message report events for this channel. Report events are delivered via OnMessageReportReceived / OnMessageReportReceivedNative.
	 * Sets up client-side listener on the channel's moderation/restrictions stream. No-op if already streaming.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamMessageReportsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Stops listening for message report events for this channel. OnMessageReportReceived will no longer fire.
	 * Local: stops the listener. No-op if not streaming message reports.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingMessageReports();
	
	/**
	 * Stops listening asynchronously for message report events on this channel. OnMessageReportReceived will no longer fire.
	 * No-op if not streaming message reports.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingMessageReportsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for message report events on this channel. OnMessageReportReceived will no longer fire.
	 * No-op if not streaming message reports.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingMessageReportsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Fetches message report events history for this channel within a timetoken range from the channel's moderation/restrictions stream.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param Count Maximum number of events to return (default 100).
	 * @return Operation result and list of report events in the range.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatEventsResult GetMessageReportsHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 100);

	/**
	 * Fetches message report events history asynchronously for this channel within a timetoken range from the channel's moderation/restrictions stream.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnEventsResponse Callback executed when the operation completes.
	 * @param Count Maximum number of events to return (default 100).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetMessageReportsHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponse OnEventsResponse, const int Count = 100);
	/**
	 * Fetches message report events history asynchronously for this channel within a timetoken range from the channel's moderation/restrictions stream.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnEventsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Count Maximum number of events to return (default 100).
	 */
	void GetMessageReportsHistoryAsync(const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponseNative OnEventsResponseNative, const int Count = 100);

	/**
	 * Starts listening for custom events on this channel. Custom events are delivered via OnCustomEventReceived / OnCustomEventReceivedNative.
	 * Local: sets up client-side listener using Chat::ListenForEvents. No-op if already streaming custom events.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamCustomEvents();

	/**
	 * Starts listening asynchronously for custom events on this channel. No-op if already streaming custom events.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamCustomEventsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for custom events on this channel. No-op if already streaming custom events.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamCustomEventsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Stops listening for custom events on this channel. OnCustomEventReceived will no longer fire.
	 * Local: stops the listener. No-op if not streaming custom events.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingCustomEvents();

	/**
	 * Stops listening asynchronously for custom events on this channel. OnCustomEventReceived will no longer fire.
	 * No-op if not streaming custom events.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingCustomEventsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for custom events on this channel. OnCustomEventReceived will no longer fire.
	 * No-op if not streaming custom events.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingCustomEventsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Creates a new message draft object for this channel. Use the draft to build a message (text, mentions, etc.) and send it via the draft's Send().
	 * Local: does not perform any network requests. The returned object is owned by this channel.
	 *
	 * @param MessageDraftConfig Optional configuration: UserSuggestionSource (where to get user suggestions), IsTypingIndicatorTriggered (whether typing is sent while editing),
	 *        UserLimit (max users in suggestions), ChannelLimit (max channels in suggestions).
	 * @return New message draft object, or nullptr if the channel is not initialized.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	UPubnubChatMessageDraft* CreateMessageDraft(FPubnubChatMessageDraftConfig MessageDraftConfig = FPubnubChatMessageDraftConfig());
	
protected:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString ChannelID = "";
	UPROPERTY()
	UPubnubSubscription* ConnectSubscription = nullptr;
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;
	UPROPERTY()
	UPubnubSubscription* PresenceSubscription = nullptr;
	UPROPERTY()
	UPubnubSubscription* CustomEventsSubscription = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* TypingCallbackStop = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* ReadReceiptsCallbackStop = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* MessageReportsCallbackStop = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;
	bool IsConnected = false;
	bool IsStreamingTyping = false;
	bool IsStreamingPresence = false;
	bool IsStreamingReadReceipts = false;
	bool IsStreamingMessageReports = false;
	bool IsStreamingCustomEvents = false;
	
	TArray<FString> StreamPresenceUserIDs;
	
	FDateTime LastTypingEventTime = FDateTime::MinValue();
	TMap<FString, FTypingIndicatorData> TypingIndicators;
	
	/** Critical section for thread-safe access to typing indicators */
	mutable FCriticalSection TypingIndicatorsCriticalSection;

	/** Rate limiting state for SendText operations */
	FDateTime LastSendTextTime = FDateTime::MinValue();
	int32 SendTextRateLimitPenalty = 0;
	mutable FCriticalSection SendTextRateLimitCriticalSection;

	/**
	 * Calculates delay needed before sending text based on rate limiting with exponential backoff.
	 * @return Delay in seconds (0.0 = can send immediately)
	 */
	float CalculateSendTextRateLimiterDelay();

	void InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID);
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	//This function is for ThreadChannel which does additional logic during SentText (SendText as UFUNCTION can't be directly overriden)
	virtual FPubnubChatOperationResult OnSendText();
	
	//Function to override because ThreadChannel creates different payload for MentionEvent
	virtual FString CreateMentionEventPayload(FString Timetoken, FString Text);
	
	//Add calling OnMessageReceived to ConnectSubscription. Virtual as Thread Channel will override it to use OnThreadMessageReceived
	virtual void AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak);
	
	UFUNCTION()
	void OnChatDestroyed(FString UserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

