// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatMessage.h"
#include "PubnubChatChannel.h"
#include "PubnubChatThreadChannel.h"

// snippet.end

#include "Sample_ChatMessage.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatMessage : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_message_data
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetMessageDataSample();

	// snippet.get_message_timetoken
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetMessageTimetokenSample();

	// snippet.get_current_text
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetCurrentTextSample();

	// snippet.get_type
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetTypeSample();

	// snippet.edit_text
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void EditTextSample();

	// snippet.delete_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void DeleteMessageSample();

	// snippet.restore_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void RestoreMessageSample();

	// snippet.is_deleted
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void IsDeletedSample();

	// snippet.pin_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void PinMessageSample();

	// snippet.unpin_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void UnpinMessageSample();

	// snippet.toggle_reaction
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ToggleReactionSample();

	// snippet.get_reactions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetReactionsSample();

	// snippet.has_user_reaction
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void HasUserReactionSample();

	// snippet.forward_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ForwardMessageSample();

	// snippet.report_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ReportMessageSample();

	// snippet.stream_message_updates
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void StreamUpdatesSample();

	void OnMessageUpdateReceived(FString Timetoken, const FPubnubChatMessageData& MessageData);

	// snippet.create_thread
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void CreateThreadSample();

	UFUNCTION()
	void OnCreateThreadResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.get_thread
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetThreadSample();

	UFUNCTION()
	void OnGetThreadResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.has_thread
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void HasThreadSample();

	// snippet.remove_thread
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void RemoveThreadSample();

	// snippet.end
};
