// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "PubnubChatEnumLibrary.h"

#include "PubnubChatUser.generated.h"

class UPubnubClient;
class UPubnubChat;
class UPubnubSubscription;
class UPubnubChatCallbackStop;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatUserUpdated, FString, UserID, FPubnubChatUserData, UserData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatUserUpdatedNative, FString UserID, const FPubnubChatUserData& UserData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserMentioned, FPubnubChatUserMention, UserMention);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserMentionedNative, const FPubnubChatUserMention& UserMention);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserInvited, FPubnubChatInviteEvent, InviteEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserInvitedNative, const FPubnubChatInviteEvent& InviteEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserRestrictionChanged, FPubnubChatRestriction, Restriction);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatUserRestrictionChangedNative, const FPubnubChatRestriction& Restriction);

/**
 * Represents a chat user in the PubNub Chat SDK. Provides access to user metadata, memberships, presence, restrictions, and streaming updates.
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatUser : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	/**
	 * Broadcast when this user's metadata is updated (after StreamUpdates is active).
	 * For user deletion, use OnDeleted instead.
	 * @param UserID The user ID (this user).
	 * @param UserData Updated user metadata.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatUserUpdated OnUpdated;
	FOnPubnubChatUserUpdatedNative OnUpdatedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatUserMentioned OnMentioned;
	FOnPubnubChatUserMentionedNative OnMentionedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatUserInvited OnInvited;
	FOnPubnubChatUserInvitedNative OnInvitedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatUserRestrictionChanged OnRestrictionChanged;
	FOnPubnubChatUserRestrictionChangedNative OnRestrictionChangedNative;
	
	/**
	 * Broadcast when this user is deleted on the server (after StreamUpdates is active).
	 * Subscribe to remove the user from local UI or lists when they are deleted elsewhere.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatObjectDeleted OnDeleted;
	FOnPubnubChatObjectDeletedNative OnDeletedNative;
	
	/* PUBLIC FUNCTIONS */
	
	/**
	 * Returns the current user metadata (UserName, ExternalID, ProfileUrl, Email, Custom, Status, Type) from the local cache.
	 * Local: does not perform any network requests. Data may be stale if the user was updated elsewhere.
	 *
	 * @return User metadata struct, or empty struct if user is not initialized or not in cache.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatUserData GetUserData() const;
	
	/**
	 * Returns the unique identifier of this user.
	 * Local: does not perform any network requests.
	 *
	 * @return User ID string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetUserID() const { return UserID; }

	/**
	 * Updates this user's metadata on the PubNub server and updates the local cache.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the user is not initialized or the server request fails.
	 *
	 * @param UpdateUserData Fields to update (UserName, ExternalID, ProfileUrl, Email, Custom, Status, Type) and ForceSet flags
	 *        (ForceSetUserName, ForceSetExternalID, ForceSetProfileUrl, ForceSetEmail, ForceSetCustom, ForceSetStatus, ForceSetType) to control
	 *        whether each field is merged or fully replaced.
	 * @return Operation result with success or error details.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Update(FPubnubChatUpdateUserInputData UpdateUserData);
	
	/**
	 * Updates this user's metadata asynchronously on the PubNub server and updates the local cache.
	 * Fails if the user is not initialized or the server request fails.
	 *
	 * @param UpdateUserData Fields to update (UserName, ExternalID, ProfileUrl, Email, Custom, Status, Type) and ForceSet flags to control merge or full replacement.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Updates this user's metadata asynchronously on the PubNub server and updates the local cache.
	 * Fails if the user is not initialized or the server request fails.
	 *
	 * @param UpdateUserData Fields to update (UserName, ExternalID, ProfileUrl, Email, Custom, Status, Type) and ForceSet flags
	 *        (ForceSetUserName, ForceSetExternalID, ForceSetProfileUrl, ForceSetEmail, ForceSetCustom, ForceSetStatus, ForceSetType) to control
	 *        whether each field is merged or fully replaced.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UpdateAsync(FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Deletes this user on the PubNub server.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User")
	FPubnubChatOperationResult Delete();

	/**
	 * Deletes this user asynchronously on the PubNub server.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Deletes this user asynchronously on the PubNub server.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DeleteAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Returns whether this user is considered active based on the last-active timestamp in the local cache.
	 * Local: does not perform network requests. Compares lastActiveTimestamp from user Custom metadata to the chat's StoreUserActivityInterval.
	 *
	 * @return true if the user's last activity is within the activity interval, false otherwise or if no timestamp or not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	bool IsActive() const;
	
	/**
	 * Returns the last-active timestamp string from this user's custom metadata in the local cache.
	 * Local: does not perform any network requests.
	 *
	 * @return Last-active timestamp (timetoken format), or empty string if not set or not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetLastActiveTimestamp() const;
	
	/**
	 * Lists channel IDs where this user is currently present (subscribed). Presence reflects active subscriptions, not channel membership.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @return Operation result and list of channel IDs where this user is present.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatWherePresentResult WherePresent();
	
	/**
	 * Lists channel IDs asynchronously where this user is currently present (subscribed). Presence reflects active subscriptions, not channel membership.
	 *
	 * @param OnWherePresentResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void WherePresentAsync(FOnPubnubChatWherePresentResponse OnWherePresentResponse);
	/**
	 * Lists channel IDs asynchronously where this user is currently present (subscribed). Presence reflects active subscriptions, not channel membership.
	 *
	 * @param OnWherePresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void WherePresentAsync(FOnPubnubChatWherePresentResponseNative OnWherePresentResponseNative);
	
	/**
	 * Checks whether this user is currently present (subscribed) on the given channel. Presence reflects active subscriptions, not channel membership.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @return Operation result and whether this user is present on the channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatIsPresentResult IsPresentOn(const FString ChannelID);
	
	/**
	 * Checks asynchronously whether this user is currently present (subscribed) on the given channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsPresentResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponse OnIsPresentResponse);
	/**
	 * Checks asynchronously whether this user is currently present (subscribed) on the given channel. Presence reflects active subscriptions, not channel membership.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsPresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void IsPresentOnAsync(const FString ChannelID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative);
	
	/**
	 * Retrieves this user's channel memberships from the PubNub server. Returns channel and membership data for each membership.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of memberships to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 * @return Operation result, list of memberships (channel + membership data), pagination data, and total count.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatMembershipsResult GetMemberships(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Retrieves this user's channel memberships asynchronously from the PubNub server. Returns channel and membership data for each membership.
	 *
	 * @param OnMembershipsResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of memberships to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetMembershipsAsync(FOnPubnubChatMembershipsResponse OnMembershipsResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves this user's channel memberships asynchronously from the PubNub server. Returns channel and membership data for each membership.
	 *
	 * @param OnMembershipsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of memberships to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation for filter formulas.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	void GetMembershipsAsync(FOnPubnubChatMembershipsResponseNative OnMembershipsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Retrieves this user's membership on a specific channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @return Operation result and membership object if found; Membership is nullptr if the user is not a member of that channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatMembershipResult GetMembership(const FString ChannelID);

	/**
	 * Retrieves this user's membership on a specific channel asynchronously.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param OnMembershipResponse Callback executed when the operation completes. Returned Membership is nullptr if the user is not a member of that channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetMembershipAsync(const FString ChannelID, FOnPubnubChatMembershipResponse OnMembershipResponse);
	/**
	 * Retrieves this user's membership on a specific channel asynchronously.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param OnMembershipResponseNative Native callback executed when the operation completes (accepts lambdas). Returned Membership is nullptr if the user is not a member of that channel.
	 */
	void GetMembershipAsync(const FString ChannelID, FOnPubnubChatMembershipResponseNative OnMembershipResponseNative);

	/**
	 * Checks whether this user is a member of a specific channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @return Operation result and a boolean indicating whether membership exists.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatIsMemberOnResult IsMemberOn(const FString ChannelID);

	/**
	 * Checks asynchronously whether this user is a member of a specific channel.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsMemberOnResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void IsMemberOnAsync(const FString ChannelID, FOnPubnubChatIsMemberOnResponse OnIsMemberOnResponse);
	/**
	 * Checks asynchronously whether this user is a member of a specific channel.
	 *
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsMemberOnResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void IsMemberOnAsync(const FString ChannelID, FOnPubnubChatIsMemberOnResponseNative OnIsMemberOnResponseNative);

	/**
	 * Sets or lifts moderation restrictions (ban, mute) for this user on the given channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param ChannelID Unique identifier of the channel on which to restrict or unrestrict this user.
	 * @param Ban When true, bans this user from the channel; when false, lifts ban if present.
	 * @param Mute When true, mutes this user on the channel; when false, lifts mute if present.
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult SetRestrictions(const FString ChannelID, bool Ban, bool Mute, FString Reason = "");
	
	/**
	 * Sets or lifts moderation restrictions (ban, mute) asynchronously for this user on the given channel.
	 *
	 * @param ChannelID Unique identifier of the channel on which to restrict or unrestrict this user.
	 * @param Ban When true, bans this user from the channel; when false, lifts ban if present.
	 * @param Mute When true, mutes this user on the channel; when false, lifts mute if present.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponse OnOperationResponse, FString Reason = "");
	/**
	 * Sets or lifts moderation restrictions (ban, mute) asynchronously for this user on the given channel.
	 *
	 * @param ChannelID Unique identifier of the channel on which to restrict or unrestrict this user.
	 * @param Ban When true, bans this user from the channel; when false, lifts ban if present.
	 * @param Mute When true, mutes this user on the channel; when false, lifts mute if present.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Reason Optional reason for the restriction (e.g. for audit).
	 */
	void SetRestrictionsAsync(const FString ChannelID, bool Ban, bool Mute, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FString Reason = "");
	
	/**
	 * Retrieves moderation restrictions (ban, mute) for this user on the given channel.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Returns a restriction with Ban/Mute false if this user has no restrictions on the channel.
	 *
	 * @param Channel The channel object to query. Must be valid (non-null).
	 * @return Operation result and this user's restriction (Ban, Mute, Reason) on the channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionResult GetChannelRestrictions(UPubnubChatChannel* Channel);
	
	/**
	 * Retrieves moderation restrictions (ban, mute) asynchronously for this user on the given channel.
	 * Returns a restriction with Ban/Mute false if this user has no restrictions on the channel.
	 *
	 * @param Channel The channel object to query. Must be valid (non-null).
	 * @param OnRestrictionResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponse OnRestrictionResponse);
	/**
	 * Retrieves moderation restrictions (ban, mute) asynchronously for this user on the given channel.
	 * Returns a restriction with Ban/Mute false if this user has no restrictions on the channel.
	 *
	 * @param Channel The channel object to query. Must be valid (non-null).
	 * @param OnRestrictionResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetChannelRestrictionsAsync(UPubnubChatChannel* Channel, FOnPubnubChatGetRestrictionResponseNative OnRestrictionResponseNative);
	
	/**
	 * Retrieves moderation restrictions for this user on all channels where they are restricted.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 *
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 * @return Operation result, list of restrictions, pagination data, and total count.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionsResult GetChannelsRestrictions(const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Retrieves moderation restrictions asynchronously for this user on all channels where they are restricted.
	 *
	 * @param OnRestrictionsResponse Callback executed when the operation completes.
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponse OnRestrictionsResponse, const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves moderation restrictions asynchronously for this user on all channels where they are restricted.
	 *
	 * @param OnRestrictionsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Maximum number of restrictions to return. Pass 0 to use the server default.
	 * @param Sort Key-value pair of a property to sort by and sort direction.
	 * @param Page Pagination information. Use Page.Next or Page.Prev for next/previous page; Next takes precedence if both set.
	 */
	void GetChannelsRestrictionsAsync(FOnPubnubChatGetRestrictionsResponseNative OnRestrictionsResponseNative, const int Limit = 0, FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	/**
	 * Starts listening for mention events targeted to this user. Mentions are delivered via OnMentioned / OnMentionedNative.
	 * Local: sets up client-side listener using Chat::ListenForEvents on this user's channel. No-op if already streaming mentions.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamMentions();

	/**
	 * Starts listening asynchronously for mention events targeted to this user. No-op if already streaming mentions.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamMentionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for mention events targeted to this user. No-op if already streaming mentions.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamMentionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Stops listening for mention events targeted to this user. OnMentioned and OnMentionedNative will no longer fire.
	 * Local: stops the listener. No-op if not streaming mentions.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingMentions();

	/**
	 * Stops listening asynchronously for mention events targeted to this user.
	 * No-op if not streaming mentions.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingMentionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for mention events targeted to this user.
	 * No-op if not streaming mentions.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingMentionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Starts listening for invite events targeted to this user. Invitations are delivered via OnInvited / OnInvitedNative.
	 * Local: sets up client-side listener using Chat::ListenForEvents on this user's channel. No-op if already streaming invitations.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamInvitations();

	/**
	 * Starts listening asynchronously for invite events targeted to this user. No-op if already streaming invitations.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamInvitationsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for invite events targeted to this user. No-op if already streaming invitations.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamInvitationsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Stops listening for invite events targeted to this user. OnInvited and OnInvitedNative will no longer fire.
	 * Local: stops the listener. No-op if not streaming invitations.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingInvitations();

	/**
	 * Stops listening asynchronously for invite events targeted to this user.
	 * No-op if not streaming invitations.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingInvitationsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for invite events targeted to this user.
	 * No-op if not streaming invitations.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingInvitationsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Starts listening for moderation restriction events targeted to this user. Restriction changes are delivered via
	 * OnRestrictionChanged / OnRestrictionChangedNative.
	 * Local: sets up client-side listener using Chat::ListenForEvents on this user's moderation event channel.
	 * No-op if already streaming restrictions.
	 *
	 * @return Operation result. Success if the listener was started.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamRestrictions();

	/**
	 * Starts listening asynchronously for moderation restriction events targeted to this user.
	 * No-op if already streaming restrictions.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamRestrictionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for moderation restriction events targeted to this user.
	 * No-op if already streaming restrictions.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamRestrictionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Stops listening for moderation restriction events targeted to this user.
	 * OnRestrictionChanged and OnRestrictionChangedNative will no longer fire.
	 * Local: stops the listener. No-op if not streaming restrictions.
	 *
	 * @return Operation result. Success if the listener was stopped.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingRestrictions();

	/**
	 * Stops listening asynchronously for moderation restriction events targeted to this user.
	 * No-op if not streaming restrictions.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingRestrictionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for moderation restriction events targeted to this user.
	 * No-op if not streaming restrictions.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingRestrictionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for user metadata updates (and delete events) for this user. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * Blocking: subscribes on the calling thread. Blocks until the subscription is established.
	 * No-op if already streaming updates.
	 *
	 * @return Operation result. Success if subscribe succeeded or already streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StreamUpdates();
	
	/**
	 * Starts listening asynchronously for user metadata updates (and delete events) for this user. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StreamUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Starts listening asynchronously for user metadata updates (and delete events) for this user. Updates are delivered via OnUpdated; deletions via OnDeleted.
	 * No-op if already streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StreamUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Starts listening for user metadata updates on each of the given users. Calls StreamUpdates() on each user.
	 * Blocking: performs StreamUpdates on each user on the calling thread. Blocks for the duration of all operations.
	 *
	 * @param Users Array of user objects on which to start streaming updates.
	 * @return Combined operation result from all users.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatUser*>& Users);
	
	/**
	 * Stops listening for user metadata updates for this user. OnUpdated and OnDeleted will no longer fire.
	 * Blocking: unsubscribes on the calling thread. Blocks for the duration of the operation.
	 * No-op if not streaming updates.
	 *
	 * @return Operation result. Success if unsubscribe succeeded or was not streaming.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	/**
	 * Stops listening asynchronously for user metadata updates for this user. OnUpdated and OnDeleted will no longer fire.
	 * No-op if not streaming updates.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Stops listening asynchronously for user metadata updates for this user. OnUpdated and OnDeleted will no longer fire.
	 * No-op if not streaming updates.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void StopStreamingUpdatesAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString UserID = "";
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* MentionedCallbackStop = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* InvitedCallbackStop = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* RestrictionCallbackStop = nullptr;

	bool IsInitialized = false;
	bool IsStreamingMentions = false;
	bool IsStreamingInvitations = false;
	bool IsStreamingRestrictions = false;
	bool IsStreamingUpdates = false;

	void InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID);
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION()
	void OnChatDestroyed(FString InUserID);
	void ClearAllSubscriptions();
	void CleanUp();
};
