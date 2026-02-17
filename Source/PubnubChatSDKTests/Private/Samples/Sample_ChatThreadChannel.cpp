// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatThreadChannel.h"

// snippet.thread_channel_get_parent_id

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::GetParentChannelIDSample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Get parent channel ID (local, no network call)
	FString ParentChannelID = ThreadChannel->GetParentChannelID();
}

// snippet.get_parent_message

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::GetParentMessageSample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Get the parent message this thread replies to (local, no network call)
	UPubnubChatMessage* ParentMessage = ThreadChannel->GetParentMessage();
}

// snippet.get_thread_history

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::GetThreadHistorySample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Start timetoken (inclusive); must be higher (newer) than EndTimetoken (17-digit PubNub timetoken)
	FString StartTimetoken = TEXT("17800000000000000");
	// End timetoken (inclusive); must be lower (older) than StartTimetoken
	FString EndTimetoken = TEXT("17200000000000000");

	// Callback for when the operation completes (returns thread messages and IsMore)
	FOnPubnubChatGetThreadHistoryResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatThreadChannel::OnGetThreadHistoryResponse);
	ThreadChannel->GetThreadHistoryAsync(StartTimetoken, EndTimetoken, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::OnGetThreadHistoryResponse(const FPubnubChatGetThreadHistoryResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<UPubnubChatThreadMessage*> ThreadMessages = Result.ThreadMessages;
	bool bIsMore = Result.IsMore;
}

// snippet.thread_channel_pin_to_parent

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::PinMessageToParentChannelSample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Thread message to pin to the parent channel
	UPubnubChatThreadMessage* ThreadMessage = nullptr;

	// Pin the thread message on the parent channel
	ThreadChannel->PinMessageToParentChannelAsync(ThreadMessage, nullptr);
}

// snippet.thread_channel_unpin_from_parent

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::UnpinMessageFromParentChannelSample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Unpin the currently pinned message from the parent channel
	ThreadChannel->UnpinMessageFromParentChannelAsync(nullptr);
}

// snippet.listen_thread_messages

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::ListenThreadMessagesSample()
{
	// snippet.hide
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
	// snippet.show

	// Assumes ThreadChannel is a valid UPubnubChatThreadChannel

	// Bind to receive new thread messages (replies)
	ThreadChannel->OnThreadMessageReceivedNative.AddUObject(this, &ASample_ChatThreadChannel::OnThreadMessageReceived);

	// Start listening for thread messages
	ThreadChannel->ConnectAsync();

	// After some time when receiving messages is not needed anymore - disconnect
	ThreadChannel->Disconnect();
}

// ACTION REQUIRED: Replace ASample_ChatThreadChannel with name of your Actor class
void ASample_ChatThreadChannel::OnThreadMessageReceived(UPubnubChatThreadMessage* ThreadMessage)
{
	/* e.g. display thread reply in UI */
}

// snippet.end
