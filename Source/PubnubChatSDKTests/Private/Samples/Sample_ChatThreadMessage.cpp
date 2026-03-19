// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatThreadMessage.h"

// snippet.thread_message_get_parent_id

// ACTION REQUIRED: Replace ASample_ChatThreadMessage with name of your Actor class
void ASample_ChatThreadMessage::GetParentChannelIDSample()
{
	// snippet.hide
	UPubnubChatThreadMessage* ThreadMessage = nullptr;
	// snippet.show

	// Assumes ThreadMessage is a valid UPubnubChatThreadMessage

	// Get parent channel ID (local, no network call)
	FString ParentChannelID = ThreadMessage->GetParentChannelID();
}

// snippet.thread_message_pin_to_parent

// ACTION REQUIRED: Replace ASample_ChatThreadMessage with name of your Actor class
void ASample_ChatThreadMessage::PinMessageToParentChannelSample()
{
	// snippet.hide
	UPubnubChatThreadMessage* ThreadMessage = nullptr;
	// snippet.show

	// Assumes ThreadMessage is a valid UPubnubChatThreadMessage

	// Pin this thread message to the parent channel
	ThreadMessage->PinMessageToParentChannelAsync(nullptr);
}

// snippet.thread_message_unpin_from_parent

// ACTION REQUIRED: Replace ASample_ChatThreadMessage with name of your Actor class
void ASample_ChatThreadMessage::UnpinMessageFromParentChannelSample()
{
	// snippet.hide
	UPubnubChatThreadMessage* ThreadMessage = nullptr;
	// snippet.show

	// Assumes ThreadMessage is a valid UPubnubChatThreadMessage

	// Unpin this thread message from the parent channel
	ThreadMessage->UnpinMessageFromParentChannelAsync(nullptr);
}

// snippet.end
