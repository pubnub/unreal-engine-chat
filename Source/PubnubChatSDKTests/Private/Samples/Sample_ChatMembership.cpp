// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatMembership.h"

// snippet.get_membership_data

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetMembershipDataSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get membership metadata from local cache (local, no network call)
	FPubnubChatMembershipData MembershipData = Membership->GetMembershipData();
	FString Status = MembershipData.Status;
}

// snippet.get_user

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetUserSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get the user object for this membership (local, no network call)
	UPubnubChatUser* User = Membership->GetUser();
}

// snippet.get_channel

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetChannelSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get the channel object for this membership (local, no network call)
	UPubnubChatChannel* Channel = Membership->GetChannel();
}

// snippet.get_user_id

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetUserIDSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get the user ID for this membership (local, no network call)
	FString UserID = Membership->GetUserID();
}

// snippet.get_channel_id

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetChannelIDSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get the channel ID for this membership (local, no network call)
	FString ChannelID = Membership->GetChannelID();
}

// snippet.get_last_read_message_timetoken

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetLastReadMessageTimetokenSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Get last-read message timetoken from local cache
	FString LastReadTimetoken = Membership->GetLastReadMessageTimetoken();
}

// snippet.delete_membership

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::DeleteMembershipSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Remove this membership from the channel on the server
	Membership->DeleteAsync(nullptr);
}

// snippet.update_membership

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::UpdateMembershipSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Fields to update for this membership (custom, status, type)
	FPubnubChatUpdateMembershipInputData UpdateData;
	UpdateData.Status = TEXT("active");

	// Update membership on the server (no result callback needed)
	Membership->UpdateAsync(UpdateData, nullptr);
}

// snippet.set_last_read_message_timetoken

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::SetLastReadMessageTimetokenSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Timetoken of the message to mark as last read (17-digit PubNub timetoken)
	FString Timetoken = TEXT("17388000000000000");

	// Set last-read message timetoken (no result callback needed)
	Membership->SetLastReadMessageTimetokenAsync(Timetoken, nullptr);
}

// snippet.set_last_read_message

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::SetLastReadMessageSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Message to mark as last read (e.g. from channel history)
	UPubnubChatMessage* Message = nullptr;

	// Set last-read message using a message object (no result callback needed)
	Membership->SetLastReadMessageAsync(Message, nullptr);
}

// snippet.stream_updates

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::StreamUpdatesSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Bind to receive membership updates (update/delete)
	Membership->OnMembershipUpdateReceivedNative.AddUObject(this, &ASample_ChatMembership::OnMembershipUpdateReceived);

	// Start streaming updates (no result callback needed)
	Membership->StreamUpdatesAsync(nullptr);

	// When updates are no longer needed, stop streaming
	Membership->StopStreamingUpdatesAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::OnMembershipUpdateReceived(EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, FString UserID, const FPubnubChatMembershipData& MembershipData)
{
	/* e.g. refresh membership UI when UpdateType is PCSUT_Updated; remove membership when PCSUT_Deleted */
}

// snippet.get_unread_messages_count

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::GetUnreadMessagesCountSample()
{
	// snippet.hide
	UPubnubChatMembership* Membership = nullptr;
	// snippet.show

	// Assumes Membership is a valid UPubnubChatMembership

	// Callback for when the operation completes (returns unread count)
	FOnPubnubChatGetUnreadMessagesCountResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMembership::OnGetUnreadMessagesCountResponse);
	Membership->GetUnreadMessagesCountAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMembership with name of your Actor class
void ASample_ChatMembership::OnGetUnreadMessagesCountResponse(const FPubnubChatGetUnreadMessagesCountResult& Result)
{
	if (Result.Result.Error) { return; }
	int Count = Result.Count;
}

// snippet.end
