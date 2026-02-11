// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatMessage.h"

// snippet.get_message_data

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetMessageDataSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Get message data from local cache (local, no network call)
	FPubnubChatMessageData MessageData = Message->GetMessageData();
	FString Text = MessageData.Text;
}

// snippet.get_message_timetoken

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetMessageTimetokenSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Get the message timetoken (local, no network call)
	FString Timetoken = Message->GetMessageTimetoken();
}

// snippet.get_current_text

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetCurrentTextSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Get the current text (includes latest edit if any)
	FString CurrentText = Message->GetCurrentText();
}

// snippet.get_type

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetTypeSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Get the message type (e.g. "text")
	FString Type = Message->GetType();
}

// snippet.edit_text

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::EditTextSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// New text content for the message
	FString NewText = TEXT("Updated message text");

	// Edit the message (no result callback needed)
	Message->EditTextAsync(NewText, nullptr);
}

// snippet.delete_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::DeleteMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Delete the message (use Soft = true to soft-delete)
	Message->DeleteAsync(nullptr);
}

// snippet.restore_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::RestoreMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Restore a soft-deleted message
	Message->RestoreAsync(nullptr);
}

// snippet.is_deleted

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::IsDeletedSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Check whether this message is marked as deleted (local check)
	FPubnubChatIsDeletedResult Result = Message->IsDeleted();
	bool bIsDeleted = Result.IsDeleted;
}

// snippet.pin_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::PinMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Pin this message to its channel
	Message->PinAsync(nullptr);
}

// snippet.unpin_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::UnpinMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Unpin this message from its channel
	Message->UnpinAsync(nullptr);
}

// snippet.toggle_reaction

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::ToggleReactionSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Reaction value to toggle for the current user
	FString Reaction = TEXT("thumbs_up");

	// Toggle the reaction (add or remove)
	Message->ToggleReactionAsync(Reaction, nullptr);
}

// snippet.get_reactions

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetReactionsSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Callback for when the operation completes (returns reactions)
	FOnPubnubChatGetReactionsResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMessage::OnGetReactionsResponse);
	Message->GetReactionsAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnGetReactionsResponse(const FPubnubChatGetReactionsResult& Result)
{
	if (Result.Result.Error) { return; }
	TArray<FPubnubChatMessageAction> Reactions = Result.Reactions;
}

// snippet.has_user_reaction

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::HasUserReactionSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Reaction value to check for the current user
	FString Reaction = TEXT("thumbs_up");

	// Callback for when the operation completes (returns whether user has reaction)
	FOnPubnubChatHasReactionResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMessage::OnHasUserReactionResponse);
	Message->HasUserReactionAsync(Reaction, Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnHasUserReactionResponse(const FPubnubChatHasReactionResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bHasReaction = Result.HasReaction;
}

// snippet.forward_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::ForwardMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Target channel to forward this message to
	UPubnubChatChannel* TargetChannel = nullptr;

	// Forward the message to the target channel (publishes with forwarding metadata)
	Message->ForwardAsync(TargetChannel, nullptr);
}

// snippet.report_message

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::ReportMessageSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Reason or context for reporting this message
	FString Reason = TEXT("Inappropriate content");

	// Report this message to the moderation stream
	Message->ReportAsync(nullptr, Reason);
}

// snippet.stream_updates

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::StreamUpdatesSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Bind to receive message action updates (edits, reactions, deletes)
	Message->OnMessageUpdateReceivedNative.AddUObject(this, &ASample_ChatMessage::OnMessageUpdateReceived);

	// Start streaming updates (no result callback needed)
	Message->StreamUpdatesAsync(nullptr);

	// When updates are no longer needed, stop streaming
	Message->StopStreamingUpdatesAsync(nullptr);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnMessageUpdateReceived(FString Timetoken, const FPubnubChatMessageData& MessageData)
{
	/* e.g. update message UI when edits or reactions arrive */
}

// snippet.create_thread

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::CreateThreadSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Callback for when the operation completes (returns thread channel)
	FOnPubnubChatThreadChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMessage::OnCreateThreadResponse);
	Message->CreateThreadAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnCreateThreadResponse(const FPubnubChatThreadChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	// At this point the thread channel exists locally; it is created on the server when the first reply is sent
	UPubnubChatThreadChannel* ThreadChannel = Result.ThreadChannel;
	
	// Send the first reply to create the thread on the server
	ThreadChannel->SendText(TEXT("First thread message"));
}

// snippet.get_thread

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::GetThreadSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Callback for when the operation completes (returns thread channel if it exists)
	FOnPubnubChatThreadChannelResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMessage::OnGetThreadResponse);
	Message->GetThreadAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnGetThreadResponse(const FPubnubChatThreadChannelResult& Result)
{
	if (Result.Result.Error) { return; }
	UPubnubChatThreadChannel* ThreadChannel = Result.ThreadChannel;
}

// snippet.has_thread

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::HasThreadSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Callback for when the operation completes (returns HasThread)
	FOnPubnubChatHasThreadResponseNative Callback;
	// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
	Callback.BindUObject(this, &ASample_ChatMessage::OnHasThreadResponse);
	Message->HasThreadAsync(Callback);
}

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::OnHasThreadResponse(const FPubnubChatHasThreadResult& Result)
{
	if (Result.Result.Error) { return; }
	bool bHasThread = Result.HasThread;
}

// snippet.remove_thread

// ACTION REQUIRED: Replace ASample_ChatMessage with name of your Actor class
void ASample_ChatMessage::RemoveThreadSample()
{
	// snippet.hide
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Message is a valid UPubnubChatMessage

	// Remove the thread from this message
	Message->RemoveThreadAsync(nullptr);
}

// snippet.end
