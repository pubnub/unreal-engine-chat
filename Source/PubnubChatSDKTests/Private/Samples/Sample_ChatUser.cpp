// Copyright 2025 PubNub Inc. All Rights Reserved.

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

	// Bind to receive user metadata updates and delete events
	User->OnUserUpdateReceivedNative.AddUObject(this, &ASample_ChatUser::OnUserUpdateReceived);

	// Start streaming updates (no result callback needed)
	User->StreamUpdatesAsync(nullptr);

	// When updates are no longer needed, stop streaming
	User->StopStreamingUpdatesAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatUser with name of your Actor class
void ASample_ChatUser::OnUserUpdateReceived(EPubnubChatStreamedUpdateType UpdateType, FString UserID, const FPubnubChatUserData& UserData)
{
	/* e.g. refresh user UI when UpdateType is PCSUT_Updated; remove user when PCSUT_Deleted */
}

// snippet.end
