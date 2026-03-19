// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatUser.h"

// snippet.get_user_data

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetUserDataSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Get user metadata from local cache (local, no network call)
	FPubnubChatUserData UserData = User->GetUserData();
	FString UserName = UserData.UserName;
}

// snippet.user_get_id

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetUserIDSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Get the user ID (local, no network call)
	FString UserID = User->GetUserID();
}

// snippet.user_update

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::UpdateUserSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Fields to update (name, external ID, email, custom, status, type)
	FPubnubChatUpdateUserInputData UpdateData;
	UpdateData.UserName = TEXT("Jane Doe");
	UpdateData.ExternalID = TEXT("ext-001");
	UpdateData.Email = TEXT("jane.doe@example.com");

	// Update user metadata on the server (no result callback needed)
	User->UpdateAsync(UpdateData, nullptr);
}

// snippet.user_delete

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::DeleteUserSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Delete this user on the server
	User->DeleteAsync(nullptr);
}

// snippet.is_active

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::IsActiveSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Check if user is active based on last-activity timestamp
	bool bIsActive = User->IsActive();
}

// snippet.get_last_active_timestamp

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetLastActiveTimestampSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Get last active timestamp (timetoken format) from local cache
	FString LastActiveTimetoken = User->GetLastActiveTimestamp();
}

// snippet.user_where_present

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::WherePresentSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Callback for when the operation completes (returns channel IDs)
	FOnPubnubChatWherePresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnWherePresentResponse);
	User->WherePresentAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnWherePresentResponse(const FPubnubChatWherePresentResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FString> Channels = Result.Channels;
}

// snippet.is_present_on

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::IsPresentOnSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Channel ID to check presence for
	FString ChannelID = TEXT("lobby-01");

	// Callback for when the operation completes (returns whether user is present)
	FOnPubnubChatIsPresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnIsPresentOnResponse);
	User->IsPresentOnAsync(ChannelID, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnIsPresentOnResponse(const FPubnubChatIsPresentResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bIsPresent = Result.IsPresent;
}

// snippet.get_memberships

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetMembershipsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Callback for when the operation completes (returns memberships with channel data)
	FOnPubnubChatMembershipsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnGetMembershipsResponse);
	User->GetMembershipsAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnGetMembershipsResponse(const FPubnubChatMembershipsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatMembership*> Memberships = Result.Memberships;
}

// snippet.get_membership

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetMembershipSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Channel ID to query membership for
	FString ChannelID = TEXT("lobby-01");

	// Callback for when the operation completes (returns one membership or empty)
	FOnPubnubChatMembershipResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnGetMembershipResponse);
	User->GetMembershipAsync(ChannelID, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnGetMembershipResponse(const FPubnubChatMembershipResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatMembership* Membership = Result.Membership;
}

// snippet.is_member_on

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::IsMemberOnSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Channel ID to check membership on
	FString ChannelID = TEXT("lobby-01");

	// Callback for when the operation completes (returns true if user is a member on channel)
	FOnPubnubChatIsMemberOnResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnIsMemberOnResponse);
	User->IsMemberOnAsync(ChannelID, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnIsMemberOnResponse(const FPubnubChatIsMemberOnResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bIsMemberOn = Result.IsMemberOn;
}

// snippet.user_set_restrictions

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::SetRestrictionsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Channel ID on which to set or lift restrictions for this user
	FString ChannelID = TEXT("lobby-01");

	// Set or lift ban/mute for this user on the channel
	User->SetRestrictionsAsync(ChannelID, false, true, nullptr, "inappropriate language");
}

// snippet.get_channel_restrictions

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetChannelRestrictionsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Channel to query restrictions for
	UPubnubChatChannel* Channel = nullptr;

	// Callback for when the operation completes (returns ban/mute/reason)
	FOnPubnubChatGetRestrictionResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnGetChannelRestrictionsResponse);
	User->GetChannelRestrictionsAsync(Channel, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnGetChannelRestrictionsResponse(const FPubnubChatGetRestrictionResult& Result)
{
	if (Result.Result.Error) { return; }
	FPubnubChatRestriction Restriction = Result.Restriction;
}

// snippet.get_channels_restrictions

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::GetChannelsRestrictionsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Callback for when the operation completes (returns restrictions across channels)
	FOnPubnubChatGetRestrictionsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatUser::OnGetChannelsRestrictionsResponse);
	User->GetChannelsRestrictionsAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnGetChannelsRestrictionsResponse(const FPubnubChatGetRestrictionsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FPubnubChatRestriction> Restrictions = Result.Restrictions;
}

// snippet.stream_user_updates

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::StreamUpdatesSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetUser)

	// Bind to receive user metadata updates and deletion events
	User->OnUpdatedNative.AddUObject(this, &ASample_ChatUser::OnUserUpdateReceived);
	User->OnDeletedNative.AddUObject(this, &ASample_ChatUser::OnUserDeleted);

	// Start streaming updates (no result callback needed)
	User->StreamUpdatesAsync(nullptr);

	// When updates are no longer needed, stop streaming
	User->StopStreamingUpdatesAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnUserUpdateReceived(FString UserID, const FPubnubChatUserData& UserData)
{
	/* e.g. refresh user UI with updated metadata */
}

void ASample_ChatUser::OnUserDeleted()
{
	/* e.g. remove user from list */
}

// snippet.stream_mentions

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::StreamMentionsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetCurrentUser)

	// Bind to receive mention notifications for this user
	User->OnMentionedNative.AddUObject(this, &ASample_ChatUser::OnUserMentioned);

	// Start streaming mentions
	User->StreamMentionsAsync(nullptr);

	// When mention notifications are no longer needed, stop streaming
	User->StopStreamingMentionsAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnUserMentioned(const FPubnubChatUserMention& UserMention)
{
	/* e.g. show @mention notification in UI */
}

// snippet.stream_restrictions

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::StreamRestrictionsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetCurrentUser)

	// Bind to receive moderation restriction changes for this user
	User->OnRestrictionChangedNative.AddUObject(this, &ASample_ChatUser::OnRestrictionChanged);

	// Start streaming restriction events
	User->StreamRestrictionsAsync(nullptr);

	// When restriction events are no longer needed, stop streaming
	User->StopStreamingRestrictionsAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnRestrictionChanged(const FPubnubChatRestriction& Restriction)
{
	/* e.g. update UI to reflect ban/mute changes */
}

// snippet.stream_invitations

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::StreamInvitationsSample()
{
	// snippet.hide
	UPubnubChatUser* User = nullptr;
	// snippet.show

	// Assumes User is a valid UPubnubChatUser (e.g. from GetCurrentUser)

	// Bind to receive invite events for this user
	User->OnInvitedNative.AddUObject(this, &ASample_ChatUser::OnUserInvited);

	// Start streaming invitation events
	User->StreamInvitationsAsync(nullptr);

	// When invitation events are no longer needed, stop streaming
	User->StopStreamingInvitationsAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnUserInvited(const FPubnubChatInviteEvent& InviteEvent)
{
	/* e.g. show channel invite notification in UI */
}

// snippet.end
