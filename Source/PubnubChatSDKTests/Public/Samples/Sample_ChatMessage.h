// Copyright 2026 PubNub Inc. All Rights Reserved.

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
	// blueprint.gwhceuu_
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetMessageDataSample();

	// snippet.get_message_timetoken
	// blueprint.hvsjvlcv
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetMessageTimetokenSample();

	// snippet.get_current_text
	// blueprint.ebrex1ki
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetCurrentTextSample();

	// snippet.get_type
	// blueprint.vjpqzt0g
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetTypeSample();

	// snippet.edit_text
	// blueprint.0kdeymv2
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void EditTextSample();

	// snippet.delete_message
	// blueprint.vo11j8oa
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void DeleteMessageSample();

	// snippet.restore_message
	// blueprint.p_1lyeon
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void RestoreMessageSample();

	// snippet.is_deleted
	// blueprint.vz96cbw1
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void IsDeletedSample();

	// snippet.message_pin
	// blueprint.csuliofh
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void PinMessageSample();

	// snippet.message_unpin
	// blueprint._3zojcp3
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void UnpinMessageSample();

	// snippet.toggle_reaction
	// blueprint.t6cuxad-
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ToggleReactionSample();

	// snippet.get_reactions
	// blueprint.wf-6eaxp
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetReactionsSample();

	// snippet.has_user_reaction
	// blueprint.p42t-oh2
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void HasUserReactionSample();

	// snippet.message_forward
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ForwardMessageSample();

	// snippet.report_message
	// blueprint.z3rdh_21
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void ReportMessageSample();

	// snippet.stream_message_updates
	// blueprint._e51uod5
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void StreamUpdatesSample();

	void OnMessageUpdateReceived(FString Timetoken, const FPubnubChatMessageData& MessageData);

	// snippet.create_thread
	// blueprint.mtz1c0z9
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void CreateThreadSample();

	UFUNCTION()
	void OnCreateThreadResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.get_thread
	// blueprint._pan734k
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void GetThreadSample();

	UFUNCTION()
	void OnGetThreadResponse(const FPubnubChatThreadChannelResult& Result);

	// snippet.has_thread
	// blueprint.sba8zta_
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void HasThreadSample();

	// snippet.remove_thread
	// blueprint.cus9v70w
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessage")
	void RemoveThreadSample();

	// snippet.end
};
