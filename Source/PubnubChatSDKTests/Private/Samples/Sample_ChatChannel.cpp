// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatChannel.h"

// snippet.get_channel_data

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetChannelDataSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Get channel metadata from local cache (local, no network call)
	FPubnubChatChannelData ChannelData = Channel->GetChannelData();
	FString ChannelName = ChannelData.ChannelName;
}

// snippet.channel_get_id

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetChannelIDSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Get the channel ID (local, no network call)
	FString ChannelID = Channel->GetChannelID();
}

// snippet.channel_update

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::UpdateChannelSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Fields to update (name, description, custom, status, type)
	FPubnubChatUpdateChannelInputData UpdateData;
	UpdateData.ChannelName = TEXT("Updated Channel Name");
	UpdateData.Description = TEXT("New description");

	// Callback for when the operation completes
	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnUpdateChannelResponse);
	Channel->UpdateAsync(UpdateData, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnUpdateChannelResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Update completed
}

// snippet.connect

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::ConnectSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Add listener for received messages before connecting
	Channel->OnMessageReceivedNative.AddUObject(this, &ASample_ChatChannel::OnChannelMessageReceived);

	// Start listening for messages on this channel
	Channel->ConnectAsync(nullptr);

	// After some time when receiving messages is not needed anymore - disconnect
	Channel->Disconnect();
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnChannelMessageReceived(UPubnubChatMessage* Message)
{
	/* e.g. display in chat UI */
}

// snippet.join

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::JoinSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Add listener for received messages before joining
	Channel->OnMessageReceivedNative.AddUObject(this, &ASample_ChatChannel::OnChannelMessageReceived_JoinSample);

	// Callback for when join completes (returns membership)
	FOnPubnubChatJoinResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnJoinResponse);
	Channel->JoinAsync(Callback);

	// After some time when receiving messages is not needed anymore - disconnect
	Channel->Disconnect();
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnChannelMessageReceived_JoinSample(UPubnubChatMessage* Message)
{
	/* e.g. display in chat UI */
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnJoinResponse(const FPubnubChatJoinResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatMembership* Membership = Result.Membership;
}

// snippet.disconnect

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::DisconnectSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Unsubscribe from this channel; stop receiving new messages
	Channel->DisconnectAsync(nullptr);
}

// snippet.leave

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::LeaveSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Remove current user's membership and unsubscribe from this channel
	Channel->LeaveAsync(nullptr);
}

// snippet.send_text

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::SendTextSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Publish a text message to this channel
	Channel->SendTextAsync(TEXT("Hello"), nullptr);
}

// snippet.invite

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::InviteSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User to invite (e.g. from GetUser or user lookup)
	UPubnubChatUser* User = nullptr;

	// Callback for when the invite completes (returns created/existing membership)
	FOnPubnubChatInviteResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnInviteResponse);
	Channel->InviteAsync(User, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnInviteResponse(const FPubnubChatInviteResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatMembership* Membership = Result.Membership;
}

// snippet.invite_multiple

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::InviteMultipleSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Users to invite (e.g. from GetUser or user lookup; at least one valid required)
	TArray<UPubnubChatUser*> Users;

	// Callback for when the operation completes (returns created memberships)
	FOnPubnubChatInviteMultipleResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnInviteMultipleResponse);
	Channel->InviteMultipleAsync(Users, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnInviteMultipleResponse(const FPubnubChatInviteMultipleResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatMembership*> Memberships = Result.Memberships;
}

// snippet.channel_pin_message

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::PinMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Message to pin; must be from this channel or from a thread on this channel
	UPubnubChatMessage* Message = nullptr;

	// Pin a message to this channel (stores in channel metadata)
	Channel->PinMessageAsync(Message, nullptr);
}

// snippet.channel_unpin_message

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::UnpinMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Remove the currently pinned message from this channel
	Channel->UnpinMessageAsync(nullptr);
}

// snippet.get_pinned_message

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetPinnedMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Callback for when the operation completes (returns pinned message or empty)
	FOnPubnubChatMessageResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetPinnedMessageResponse);
	Channel->GetPinnedMessageAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetPinnedMessageResponse(const FPubnubChatMessageResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatMessage* PinnedMessage = Result.Message;
}

// snippet.channel_who_is_present

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::WhoIsPresentSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Callback for when the operation completes (returns list of user IDs present)
	FOnPubnubChatWhoIsPresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnWhoIsPresentResponse);
	Channel->WhoIsPresentAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnWhoIsPresentResponse(const FPubnubChatWhoIsPresentResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FString> UserIDs = Result.Users;
}

// snippet.channel_is_present

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::IsPresentSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User ID to check for presence (subscribed) on this channel
	FString UserID = TEXT("user-abc123");

	// Callback for when the operation completes (returns whether user is present)
	FOnPubnubChatIsPresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnIsPresentResponse);
	Channel->IsPresentAsync(UserID, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnIsPresentResponse(const FPubnubChatIsPresentResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bIsPresent = Result.IsPresent;
}

// snippet.delete

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::DeleteSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Delete this channel on the server
	Channel->DeleteAsync(nullptr);
}

// snippet.get_members

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetMembersSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Callback for when the operation completes (returns memberships with user data)
	FOnPubnubChatMembershipsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetMembersResponse);
	Channel->GetMembersAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetMembersResponse(const FPubnubChatMembershipsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatMembership*> Memberships = Result.Memberships;
}

// snippet.get_invitees

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetInviteesSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Callback for when the operation completes (returns invitees = members with pending status)
	FOnPubnubChatMembershipsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetInviteesResponse);
	Channel->GetInviteesAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetInviteesResponse(const FPubnubChatMembershipsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatMembership*> Memberships = Result.Memberships;
}

// snippet.channel_set_restrictions

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::SetRestrictionsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User ID to set restrictions for
	FString UserID = TEXT("user-abc123");

	// Set restriction (ban in this example) 
	Channel->SetRestrictionsAsync(UserID, true, false, nullptr, TEXT("Cheater"));
}

// snippet.lift_restrictions

void ASample_ChatChannel::LiftRestrictionsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User ID to lift restrictions for
	FString UserID = TEXT("user-abc123");

	// Lift restrictions (when ban = false and mute = false - it's lifting any restrictions)
	Channel->SetRestrictionsAsync(UserID, false, false, nullptr);
}

// snippet.get_user_restrictions

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetUserRestrictionsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User to query restrictions for (e.g. from GetUser or members list)
	UPubnubChatUser* User = nullptr;

	// Callback for when the operation completes (returns ban/mute/reason for this user)
	FOnPubnubChatGetRestrictionResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetUserRestrictionsResponse);
	Channel->GetUserRestrictionsAsync(User, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetUserRestrictionsResponse(const FPubnubChatGetRestrictionResult& Result)
{
	if (Result.Result.Error) { return; }
	FPubnubChatRestriction Restriction = Result.Restriction;
}

// snippet.get_users_restrictions

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetUsersRestrictionsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Callback for when the operation completes (returns list of restrictions)
	FOnPubnubChatGetRestrictionsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetUsersRestrictionsResponse);
	Channel->GetUsersRestrictionsAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetUsersRestrictionsResponse(const FPubnubChatGetRestrictionsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FPubnubChatRestriction> Restrictions = Result.Restrictions;
}

// snippet.get_history

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetHistorySample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Start timetoken (inclusive); must be higher (newer) than EndTimetoken (17-digit PubNub timetoken)
	FString StartTimetoken = TEXT("17800000000000000");
	// End timetoken (inclusive); must be lower (older) than StartTimetoken
	FString EndTimetoken = TEXT("17200000000000000");

	// Callback for when the operation completes (returns messages and IsMore)
	FOnPubnubChatGetHistoryResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetHistoryResponse);
	Channel->GetHistoryAsync(StartTimetoken, EndTimetoken, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetHistoryResponse(const FPubnubChatGetHistoryResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatMessage*> Messages = Result.Messages;
	bool bIsMore = Result.IsMore;
}

// snippet.get_message

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Timetoken of the message to fetch from this channel's history (17-digit PubNub timetoken)
	FString Timetoken = TEXT("17388000000000000");

	// Callback for when the operation completes (returns message or empty)
	FOnPubnubChatMessageResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetMessageResponse);
	Channel->GetMessageAsync(Timetoken, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetMessageResponse(const FPubnubChatMessageResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatMessage* Message = Result.Message;
}

// snippet.channel_forward_message

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::ForwardMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Message to forward; must be from a different channel (e.g. from another channel's history)
	UPubnubChatMessage* Message = nullptr;

	// Forward a message to this channel (publishes with forwarding metadata)
	Channel->ForwardMessageAsync(Message, nullptr);
}

// snippet.emit_user_mention

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::EmitUserMentionSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// User ID being mentioned
	FString UserID = TEXT("user-abc123");
	// Timetoken of the message that contains the mention (17-digit PubNub timetoken)
	FString Timetoken = TEXT("17388000000000000");
	// Mention text to display (e.g. @username)
	FString Text = TEXT("@johndoe");

	// Emit a mention event for @mention notifications
	Channel->EmitUserMentionAsync(UserID, Timetoken, Text, nullptr);
}

// snippet.create_message_draft

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::CreateMessageDraftSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Create a message draft to build and send a message (text, mentions, etc.)
	UPubnubChatMessageDraft* Draft = Channel->CreateMessageDraft();
}

// snippet.get_message_reports_history

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::GetMessageReportsHistorySample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Start timetoken (inclusive); must be higher (newer) than EndTimetoken (17-digit PubNub timetoken)
	FString StartTimetoken = TEXT("17800000000000000");
	// End timetoken (inclusive); must be lower (older) than StartTimetoken
	FString EndTimetoken = TEXT("17200000000000000");

	// Callback for when the operation completes (returns report events)
	FOnPubnubChatEventsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatChannel::OnGetMessageReportsHistoryResponse);
	Channel->GetMessageReportsHistoryAsync(StartTimetoken, EndTimetoken, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnGetMessageReportsHistoryResponse(const FPubnubChatEventsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FPubnubChatEvent> Events = Result.Events;
}

// snippet.stream_channel_updates

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::StreamUpdatesSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Bind to receive channel metadata updates and deletion events
	Channel->OnUpdatedNative.AddUObject(this, &ASample_ChatChannel::OnChannelUpdateReceived);
	Channel->OnDeletedNative.AddUObject(this, &ASample_ChatChannel::OnChannelDeleted);

	// Start streaming channel updates (no result callback needed)
	Channel->StreamUpdatesAsync(nullptr);

	// When updates are no longer needed, stop streaming
	Channel->StopStreamingUpdatesAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnChannelUpdateReceived(FString ChannelID, const FPubnubChatChannelData& ChannelData)
{
	/* e.g. refresh channel UI with updated metadata */
}

void ASample_ChatChannel::OnChannelDeleted()
{
	/* e.g. remove channel from list */
}

// snippet.stream_typing

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::StreamTypingSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Bind to receive typing indicator events (user IDs currently typing)
	Channel->OnTypingChangedNative.AddUObject(this, &ASample_ChatChannel::OnTypingReceived);

	// Start streaming typing events (not supported on public channels; use for group or direct)
	Channel->StreamTypingAsync(nullptr);

	// When typing indicators are no longer needed, stop streaming
	Channel->StopStreamingTypingAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnTypingReceived(const TArray<FString>& TypingUserIDs)
{
	/* e.g. show typing indicators in chat UI for TypingUserIDs */
}

// snippet.stream_read_receipts

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::StreamReadReceiptsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Bind to receive read receipt events (who read which message)
	Channel->OnReadReceiptReceivedNative.AddUObject(this, &ASample_ChatChannel::OnReadReceiptReceived);

	// Start streaming read receipts (not supported on public channels; use for group or direct)
	Channel->StreamReadReceiptsAsync(nullptr);

	// When read receipts are no longer needed, stop streaming
	Channel->StopStreamingReadReceiptsAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnReadReceiptReceived(const FPubnubChatReadReceipt& ReadReceipts)
{
	/* e.g. update read state per message in chat UI */
}

// snippet.stream_message_reports

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::StreamMessageReportsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	// Bind to receive message report events (moderation stream)
	Channel->OnMessageReportedNative.AddUObject(this, &ASample_ChatChannel::OnMessageReportReceived);

	// Start streaming message reports
	Channel->StreamMessageReportsAsync(nullptr);

	// When report events are no longer needed, stop streaming
	Channel->StopStreamingMessageReportsAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatChannel with name of your Actor class
void ASample_ChatChannel::OnMessageReportReceived(const FPubnubChatReportEvent& ReportEvent)
{
	/* e.g. handle moderation events (report payload, user, etc.) */
}

// snippet.end
