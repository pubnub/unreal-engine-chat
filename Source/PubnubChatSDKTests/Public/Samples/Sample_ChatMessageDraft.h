// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatChannel.h"
#include "PubnubChatMessageDraft.h"

// snippet.end

#include "Sample_ChatMessageDraft.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatMessageDraft : public AActor
{
	GENERATED_BODY()

public:

	// snippet.quote_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void QuoteMessageSample();

	// snippet.add_user_mention
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void AddUserMentionSample();

	// snippet.add_channel_reference
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void AddChannelReferenceSample();

	// snippet.add_url_link
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void AddUrlLinkSample();

	// snippet.message_draft_add_mention
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftAddMentionSample();

	// snippet.message_draft_remove_mention
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftRemoveMentionSample();

	// snippet.message_draft_update
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftUpdateSample();

	// snippet.message_draft_insert_text
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftInsertTextSample();

	// snippet.message_draft_remove_text
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftRemoveTextSample();

	// snippet.message_draft_send
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftSendSample();

	// snippet.message_draft_updated
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftUpdatedSample();

	void OnMessageDraftUpdate(const TArray<FPubnubChatMessageElement>& MessageElements);

	// snippet.message_draft_updated_with_suggestions
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftUpdatedWithSuggestionsSample();

	void OnMessageDraftUpdateWithSuggestions(const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions);

	// snippet.message_draft_insert_suggested_mention
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatMessageDraft")
	void MessageDraftInsertSuggestedMentionSample();

	void OnMessageDraftUpdateWithSuggestions_InsertSample(const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions);

	// snippet.end
};
