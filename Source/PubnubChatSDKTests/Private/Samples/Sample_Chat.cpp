// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_Chat.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatMessage.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

// snippet.get_current_user

#include "PubnubChatUser.h"

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetCurrentUserSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show
	
	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Get the current chat user (local, no network call — the player who owns this chat session)
	UPubnubChatUser* CurrentUser = Chat->GetCurrentUser();
	if (CurrentUser)
	{
		// Use CurrentUser for display name, avatar URL, or to fetch memberships
		FString DisplayName = CurrentUser->GetUserData().UserName;
	}
}

// snippet.create_user

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreateUserSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Prepare initial metadata for a new user (e.g. when registering a new player profile)
	FPubnubChatUserData UserData;
	UserData.UserName = TEXT("WinningPlayer");
	UserData.ProfileUrl = TEXT("https://example.com/avatars/player.png");

	// Bind a callback with signature matching FOnPubnubChatUserResponseNative, then call Async
	FOnPubnubChatUserResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnCreateUserResponse);
	Chat->CreateUserAsync(TEXT("Player_002"), Callback, UserData);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnCreateUserResponse(const FPubnubChatUserResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatUser* NewUser = Result.User;
	// Use NewUser; if user already exists, Result.Result.Error will be set and you may call GetUser instead
}

// snippet.chat_get_user

#include "PubnubChatUser.h"

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUserSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Bind a callback with signature matching FOnPubnubChatUserResponseNative, then call Async
	FOnPubnubChatUserResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetUserResponse);
	Chat->GetUserAsync(TEXT("Player_002"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetUserResponse(const FPubnubChatUserResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatUser* OtherUser = Result.User;
	FString OtherDisplayName = OtherUser->GetUserData().UserName;
}

// snippet.get_users

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUsersSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Bind a callback with signature matching FOnPubnubChatGetUsersResponseNative, then call Async
	FOnPubnubChatGetUsersResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetUsersResponse);
	Chat->GetUsersAsync(Callback, 20);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetUsersResponse(const FPubnubChatGetUsersResult& Result)
{
	if (Result.Result.Error) { return; }
	for (UPubnubChatUser* User : Result.Users)
	{
		/* show in UI, etc. */
	}
	// Use Result.Page for pagination (Result.Page.Next / Result.Page.Prev)
}

// snippet.chat_update_user

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::UpdateUserSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Update a user's metadata asynchronously (e.g. player changed display name or status in settings)
	FPubnubChatUpdateUserInputData UpdateData;
	UpdateData.UserName = TEXT("NewDisplayName");
	UpdateData.Status = TEXT("InMatch");

	FOnPubnubChatUserResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnUpdateUserResponse);
	Chat->UpdateUserAsync(TEXT("Player_001"), UpdateData, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnUpdateUserResponse(const FPubnubChatUserResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatUser* UpdatedUser = Result.User;
}

// snippet.chat_delete_user

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::DeleteUserSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Bind a callback with signature matching FOnPubnubChatOperationResponseNative, then call Async
	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnDeleteUserResponse);
	Chat->DeleteUserAsync(TEXT("Player_002"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnDeleteUserResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Deletion completed
}

// snippet.get_user_suggestions

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUserSuggestionsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Bind a callback with signature matching FOnPubnubChatGetUserSuggestionsResponseNative, then call Async
	FOnPubnubChatGetUserSuggestionsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetUserSuggestionsResponse);
	Chat->GetUserSuggestionsAsync(TEXT("Win"), Callback, 10);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetUserSuggestionsResponse(const FPubnubChatGetUserSuggestionsResult& Result)
{
	if (Result.Result.Error) { return; }
	for (UPubnubChatUser* User : Result.Users)
	{
		/* add to suggestion list in UI */
	}
}

// snippet.create_public_conversation

#include "PubnubChatChannel.h"

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreatePublicConversationSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Create a public channel (e.g. lobby or global chat); optional metadata
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("Game Lobby");
	ChannelData.Description = TEXT("Main lobby channel");

	FOnPubnubChatChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnCreatePublicConversationResponse);
	Chat->CreatePublicConversationAsync(TEXT("Lobby_001"), Callback, ChannelData);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnCreatePublicConversationResponse(const FPubnubChatChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatChannel* Channel = Result.Channel;
}

// snippet.create_group_conversation

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreateGroupConversationSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// UsersToInvite: populate with users to invite (e.g. from GetUser/GetUsers); at least one required
	TArray<UPubnubChatUser*> UsersToInvite;

	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = TEXT("Squad Chat");

	FOnPubnubChatCreateGroupConversationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnCreateGroupConversationResponse);
	Chat->CreateGroupConversationAsync(UsersToInvite, Callback, TEXT(""), ChannelData);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnCreateGroupConversationResponse(const FPubnubChatCreateGroupConversationResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatChannel* Channel = Result.Channel;
}

// snippet.create_direct_conversation

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreateDirectConversationSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// OtherUser: the user to start a direct chat with (e.g. from GetUser)
	UPubnubChatUser* OtherUser = nullptr;

	FOnPubnubChatCreateDirectConversationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnCreateDirectConversationResponse);
	Chat->CreateDirectConversationAsync(OtherUser, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnCreateDirectConversationResponse(const FPubnubChatCreateDirectConversationResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatChannel* Channel = Result.Channel;
}

// snippet.chat_get_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Fetch a channel by ID asynchronously (e.g. to show details or join)
	FOnPubnubChatChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetChannelResponse);
	Chat->GetChannelAsync(TEXT("Lobby_001"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetChannelResponse(const FPubnubChatChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatChannel* Channel = Result.Channel;
}

// snippet.get_channels

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetChannelsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Retrieve a list of channels asynchronously (e.g. for channel browser or server list)
	FOnPubnubChatGetChannelsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetChannelsResponse);
	Chat->GetChannelsAsync(Callback, 20);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetChannelsResponse(const FPubnubChatGetChannelsResult& Result)
{
	if (Result.Result.Error) { return; }
	for (UPubnubChatChannel* Channel : Result.Channels)
	{
		/* show in UI, etc. */
	}
	// Use Result.Page for pagination (Result.Page.Next / Result.Page.Prev)
}

// snippet.chat_update_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::UpdateChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Update channel metadata asynchronously (e.g. rename or change description)
	FPubnubChatUpdateChannelInputData UpdateData;
	UpdateData.ChannelName = TEXT("Updated Lobby Name");
	UpdateData.Description = TEXT("New description");

	FOnPubnubChatChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnUpdateChannelResponse);
	Chat->UpdateChannelAsync(TEXT("Lobby_001"), UpdateData, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnUpdateChannelResponse(const FPubnubChatChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatChannel* UpdatedChannel = Result.Channel;
}

// snippet.delete_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::DeleteChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Hard delete asynchronously: remove channel from the server
	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnDeleteChannelResponse);
	Chat->DeleteChannelAsync(TEXT("Lobby_001"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnDeleteChannelResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Deletion completed
}

// snippet.pin_message_to_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::PinMessageToChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// Message: the message to pin (e.g. received from channel listeners or from history)
	UPubnubChatMessage* Message = nullptr;
	// Channel: the channel to pin the message to (e.g. from GetChannel or channel list)
	UPubnubChatChannel* Channel = nullptr;

	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnPinMessageToChannelResponse);
	Chat->PinMessageToChannelAsync(Message, Channel, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnPinMessageToChannelResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Pin completed
}

// snippet.unpin_message_from_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::UnpinMessageFromChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// Channel: the channel to unpin the current pinned message from (e.g. from GetChannel or channel list)
	UPubnubChatChannel* Channel = nullptr;

	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnUnpinMessageFromChannelResponse);
	Chat->UnpinMessageFromChannelAsync(Channel, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnUnpinMessageFromChannelResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Unpin completed
}

// snippet.get_channel_suggestions

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetChannelSuggestionsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Get channels whose name starts with the typed text asynchronously (e.g. for channel search autocomplete)
	FOnPubnubChatGetChannelSuggestionsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetChannelSuggestionsResponse);
	Chat->GetChannelSuggestionsAsync(TEXT("Lob"), Callback, 10);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetChannelSuggestionsResponse(const FPubnubChatGetChannelSuggestionsResult& Result)
{
	if (Result.Result.Error) { return; }
	for (UPubnubChatChannel* Channel : Result.Channels)
	{
		/* add to suggestion list in UI */
	}
}

// snippet.chat_where_present

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::WherePresentSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// List channels where a user is present (e.g. which channels they are subscribed to — for "online" or "in channel" indicator)
	FOnPubnubChatWherePresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnWherePresentResponse);
	Chat->WherePresentAsync(TEXT("Player_001"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnWherePresentResponse(const FPubnubChatWherePresentResult& Result)
{
	if (Result.Result.Error) { return; }
	for (const FString& ChannelID : Result.Channels)
	{
		/* e.g. show in "user's active channels" list */
	}
}

// snippet.chat_who_is_present

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::WhoIsPresentSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// List users currently present on a channel (e.g. "who is in this lobby" or online list)
	FOnPubnubChatWhoIsPresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnWhoIsPresentResponse);
	Chat->WhoIsPresentAsync(TEXT("Lobby_001"), Callback, 100, 0);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnWhoIsPresentResponse(const FPubnubChatWhoIsPresentResult& Result)
{
	if (Result.Result.Error) { return; }
	for (const FString& UserID : Result.Users)
	{
		/* e.g. show in "online in channel" list */
	}
}

// snippet.chat_is_present

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::IsPresentSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Check if a user is present on a channel (e.g. "is this player in the lobby?")
	FOnPubnubChatIsPresentResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnIsPresentResponse);
	Chat->IsPresentAsync(TEXT("Player_001"), TEXT("Lobby_001"), Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnIsPresentResponse(const FPubnubChatIsPresentResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bUserIsPresent = Result.IsPresent;
}

// snippet.chat_set_restrictions

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::SetRestrictionsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Set or lift moderation restrictions (e.g. ban or mute a user on a channel)
	FPubnubChatRestriction Restriction;
	Restriction.UserID = TEXT("Player_002");
	Restriction.ChannelID = TEXT("Lobby_001");
	Restriction.Ban = true;
	Restriction.Reason = TEXT("Violation of rules");

	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnSetRestrictionsResponse);
	Chat->SetRestrictionsAsync(Restriction, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnSetRestrictionsResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Restrictions updated
}

// snippet.get_events_history

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetEventsHistorySample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	FString StartTimetoken = TEXT("0");
	FString EndTimetoken = TEXT("0");
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	// StartTimetoken / EndTimetoken: use timetoken range (Start newer than End); e.g. from a previous history or message response

	FOnPubnubChatEventsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetEventsHistoryResponse);
	Chat->GetEventsHistoryAsync(TEXT("Lobby_001"), StartTimetoken, EndTimetoken, Callback, 100);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetEventsHistoryResponse(const FPubnubChatEventsResult& Result)
{
	if (Result.Result.Error) { return; }
	for (const FPubnubChatEvent& Event : Result.Events)
	{
		/* e.g. process moderation/chat events (typing, report, etc.) */
	}
	// Result.IsMore indicates more events exist in the range
}

// snippet.chat_forward_message

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::ForwardMessageSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// Message: the message to forward (e.g. from channel listeners or from history)
	UPubnubChatMessage* Message = nullptr;
	// TargetChannel: the channel to forward to (must be different from the message's channel)
	UPubnubChatChannel* TargetChannel = nullptr;

	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnForwardMessageResponse);
	Chat->ForwardMessageAsync(Message, TargetChannel, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnForwardMessageResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Forward completed
}

// snippet.get_unread_messages_counts

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUnreadMessagesCountsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Get unread message counts per channel (e.g. for badge counts on channel list)
	FOnPubnubChatGetUnreadMessagesCountsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetUnreadMessagesCountsResponse);
	Chat->GetUnreadMessagesCountsAsync(Callback, 0);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetUnreadMessagesCountsResponse(const FPubnubChatGetUnreadMessagesCountsResult& Result)
{
	if (Result.Result.Error) { return; }
	for (const FPubnubChatUnreadMessagesCountsWrapper& Wrapper : Result.UnreadMessagesCounts)
	{
		/* Wrapper.Channel has Wrapper.Count unread messages */
	}
}

// snippet.mark_all_messages_as_read

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::MarkAllMessagesAsReadSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Mark all messages as read for the current user's memberships
	FOnPubnubChatMarkAllMessagesAsReadResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnMarkAllMessagesAsReadResponse);
	Chat->MarkAllMessagesAsReadAsync(Callback, 0);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnMarkAllMessagesAsReadResponse(const FPubnubChatMarkAllMessagesAsReadResult& Result)
{
	if (Result.Result.Error) { return; }
	// Mark as read completed; Result.Memberships contains updated memberships
}

// snippet.create_thread_channel

#include "PubnubChatThreadChannel.h"

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreateThreadChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// ParentMessage: the message to create a thread on (e.g. from channel listeners or from history)
	UPubnubChatMessage* ParentMessage = nullptr;

	FOnPubnubChatThreadChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnCreateThreadChannelResponse);
	Chat->CreateThreadChannelAsync(ParentMessage, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnCreateThreadChannelResponse(const FPubnubChatThreadChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	// At this point the thread channel exists locally; it is created on the server when the first reply is sent
	UPubnubChatThreadChannel* ThreadChannel = Result.ThreadChannel;
	
	// Send the first reply to create the thread on the server
	ThreadChannel->SendText(TEXT("First thread message"));
}

// snippet.get_thread_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetThreadChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// ParentMessage: the message whose thread channel to fetch (e.g. from channel listeners or history)
	UPubnubChatMessage* ParentMessage = nullptr;

	FOnPubnubChatThreadChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnGetThreadChannelResponse);
	Chat->GetThreadChannelAsync(ParentMessage, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnGetThreadChannelResponse(const FPubnubChatThreadChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatThreadChannel* ThreadChannel = Result.ThreadChannel;
}

// snippet.remove_thread_channel

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::RemoveThreadChannelSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat
	
	// ParentMessage: the message whose thread to remove (e.g. from channel listeners or history)
	UPubnubChatMessage* ParentMessage = nullptr;

	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnRemoveThreadChannelResponse);
	Chat->RemoveThreadChannelAsync(ParentMessage, Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnRemoveThreadChannelResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Thread removed
}

// snippet.reconnect_subscriptions

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::ReconnectSubscriptionsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Reconnect all active subscriptions (e.g. after disconnect or connection error from OnConnectionStatusChanged)
	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnReconnectSubscriptionsResponse);
	Chat->ReconnectSubscriptionsAsync(Callback);

	// Optional: pass a timetoken to resume from a specific point
	// Chat->ReconnectSubscriptionsAsync(Callback, Timetoken);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnReconnectSubscriptionsResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Reconnect completed
}

// snippet.disconnect_subscriptions

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::DisconnectSubscriptionsSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Pause all active subscriptions (e.g. when going to background or before ReconnectSubscriptions)
	FOnPubnubChatOperationResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
	Callback.BindUObject(this, &ASample_Chat::OnDisconnectSubscriptionsResponse);
	Chat->DisconnectSubscriptionsAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnDisconnectSubscriptionsResponse(const FPubnubChatOperationResult& Result)
{
	if (Result.Error) { return; }
	// Disconnect completed; call ReconnectSubscriptions when ready to resume
}

// snippet.connection_status_changed

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::ConnectionStatusChangedSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Bind to connection status changes (e.g. to update UI or log state)
	Chat->OnConnectionStatusChangedNative.AddUObject(this, &ASample_Chat::OnConnectionStatusChanged);
}

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::OnConnectionStatusChanged(EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData)
{
	//React to connection statuses, for example check for errors
	if (Status == EPubnubChatConnectionStatus::PCCS_ConnectionError)
	{
		UE_LOG(LogTemp, Error, TEXT("Connection Error. Reason: %s"), *StatusData.Reason);
	}
}

// snippet.connection_status_changed_reconnect

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::ConnectionStatusChangedReconnectSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	Chat->OnConnectionStatusChangedNative.AddLambda([this, Chat](EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData)
	{
		if (!Chat)
		{ return; }
		
		// Try to reconnect subscriptions when there is a connection error
		if (Status == EPubnubChatConnectionStatus::PCCS_ConnectionError)
		{
			Chat->ReconnectSubscriptions();
		}
	});
}

// snippet.access_manager_set_origin

#include "PubnubChatAccessManager.h"

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::AccessManagerSetOriginSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();

	FString CustomOrigin = "abc.pubnubapi.com";
	int Result = AccessManager->SetPubnubOrigin(CustomOrigin);

	if (Result == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Custom origin set successfully."));
	}
	else if (Result == 1)
	{
		UE_LOG(LogTemp, Log, TEXT("Custom origin will be applied on reconnect."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Setting custom origin is not enabled."));
	}
}

// snippet.access_manager_can_i

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::AccessManagerCanISample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();

	FString ChannelName = "customer_XYZ";
	EPubnubChatAccessManagerResourceType ResourceType = EPubnubChatAccessManagerResourceType::PCAMRT_Channels;
	EPubnubChatAccessManagerPermission Permission = EPubnubChatAccessManagerPermission::PCAMP_Write;

	bool bCanSendMessage = AccessManager->CanI(Permission, ResourceType, ChannelName);
}

// snippet.access_manager_set_auth_token

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::AccessManagerSetAuthTokenSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();

	FString AuthToken = "p0thisAkFl043rhDdHRsCkNyZXisRGNoYW6hanNlY3JldAFDZ3Jwsample3KgQ3NwY6BDcGF0pERjaGFuoENnctokenVzcqBDc3BjoERtZXRhoENzaWdYIGOAeTyWGJI";
	AccessManager->SetAuthToken(AuthToken);
}

// snippet.access_manager_parse_token

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::AccessManagerParseTokenSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();

	FString Token = "p0thisAkFl043rhDdHRsCkNyZXisRGNoYW6hanNlY3JldAFDZ3Jwsample3KgQ3NwY6BDcGF0pERjaGFuoENnctokenVzcqBDc3BjoERtZXRhoENzaWdYIGOAeTyWGJI";
	FString TokenDetails = AccessManager->ParseToken(Token);

	UE_LOG(LogTemp, Log, TEXT("Token Details: %s"), *TokenDetails);
}

// snippet.get_channels_pagination

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetChannelsPaginationSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Fetch first 25 channels
	FPubnubChatGetChannelsResult Channels = Chat->GetChannels(25);

	for (auto& Channel : Channels.Channels)
	{
		FString ChannelID = Channel->GetChannelID();
	}

	// Fetch next 25 channels using the 'next' page token from the previous response
	if (!Channels.Page.Next.IsEmpty())
	{
		FPubnubPage NextPage;
		NextPage.Next = Channels.Page.Next;

		FPubnubChatGetChannelsResult NextChannels = Chat->GetChannels(25, "", FPubnubGetAllSort(), NextPage);

		for (auto& Channel : NextChannels.Channels)
		{
			FString ChannelID = Channel->GetChannelID();
		}
	}
}

// snippet.get_channels_archived

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetChannelsArchivedSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Fetch all archived (soft-deleted) channels
	FPubnubChatGetChannelsResult Channels = Chat->GetChannels(100, "status=='deleted'");

	for (auto& Channel : Channels.Channels)
	{
		FString ChannelID = Channel->GetChannelID();
	}

	// Handle pagination if more results exist
	while (!Channels.Page.Next.IsEmpty())
	{
		Channels = Chat->GetChannels(100, "status=='deleted'", FPubnubGetAllSort(), FPubnubPage{Channels.Page.Next, ""});
		for (auto& Channel : Channels.Channels)
		{
			FString ChannelID = Channel->GetChannelID();
		}
	}
}

// snippet.get_users_pagination

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUsersPaginationSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Fetch first 25 users
	FPubnubChatGetUsersResult UsersResponsePage1 = Chat->GetUsers(25);

	for (UPubnubChatUser* User : UsersResponsePage1.Users)
	{
		FString UserID = User->GetUserID();
		UE_LOG(LogTemp, Log, TEXT("User ID: %s"), *UserID);
	}

	// Fetch next 25 users using the 'next' page token from the previous response
	if (!UsersResponsePage1.Page.Next.IsEmpty())
	{
		FPubnubPage NextPage;
		NextPage.Next = UsersResponsePage1.Page.Next;

		FPubnubChatGetUsersResult UsersResponsePage2 = Chat->GetUsers(25, "", FPubnubGetAllSort(), NextPage);

		for (UPubnubChatUser* User : UsersResponsePage2.Users)
		{
			FString UserID = User->GetUserID();
			UE_LOG(LogTemp, Log, TEXT("User ID: %s"), *UserID);
		}
	}
}

// snippet.get_users_archived

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::GetUsersArchivedSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Fetch all archived (soft-deleted) users
	FString Filter = "status=='deleted'";
	FPubnubChatGetUsersResult ArchivedUsersResponse = Chat->GetUsers(0, Filter);

	for (UPubnubChatUser* User : ArchivedUsersResponse.Users)
	{
		FString UserId = User->GetUserID();
		UE_LOG(LogTemp, Log, TEXT("Archived User ID: %s"), *UserId);
	}
}

// snippet.create_group_conversation_example

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreateGroupConversationExampleSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Create user objects for the agents to invite
	FPubnubChatUserData UserData_007;
	UserData_007.UserName = "Agent 007";
	FPubnubChatUserResult UserResult_007 = Chat->CreateUser("agent-007", UserData_007);
	if (UserResult_007.Result.Error) { return; }

	FPubnubChatUserData UserData_008;
	UserData_008.UserName = "Agent 008";
	FPubnubChatUserResult UserResult_008 = Chat->CreateUser("agent-008", UserData_008);
	if (UserResult_008.Result.Error) { return; }

	// Define the conversation/channel ID and metadata
	FString ChannelID = "group.agent-007&agent-008&my_user";

	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = "Weekly Sync on Customer XYZ";
	ChannelData.Description = "Weekly discussion regarding customer XYZ";
	ChannelData.Custom = "{\"topic\": \"customer XYZ\", \"frequency\": \"weekly\"}";
	ChannelData.Status = "active";
	ChannelData.Type = "group";

	FPubnubChatMembershipData MembershipData;
	MembershipData.Custom = "{\"role\": \"premium-support\"}";
	MembershipData.Status = "active";
	MembershipData.Type = "player";

	// Create the group conversation with invited users and custom membership data
	TArray<UPubnubChatUser*> Users = { UserResult_007.User, UserResult_008.User };
	FPubnubChatCreateGroupConversationResult CreatedResult = Chat->CreateGroupConversation(Users, ChannelID, ChannelData, MembershipData);
}

// snippet.create_public_conversation_example

// ACTION REQUIRED: Replace ASample_Chat with name of your Actor class
void ASample_Chat::CreatePublicConversationExampleSample()
{
	// snippet.hide
	UPubnubChat* Chat = nullptr;
	// snippet.show

	// Assumes Chat is a valid and initialized instance of UPubnubChat

	// Define the conversation/channel ID
	FString ChannelID = "ask-support";

	// Define the channel data
	FPubnubChatChannelData ChannelData;
	ChannelData.ChannelName = "ask-support";
	ChannelData.Description = "Space dedicated to answering all support-related questions";

	// Create the public conversation
	FPubnubChatChannelResult ChannelResult = Chat->CreatePublicConversation(ChannelID, ChannelData);
	if (ChannelResult.Result.Error) { return; }
	UPubnubChatChannel* Channel = ChannelResult.Channel;
}

// snippet.end