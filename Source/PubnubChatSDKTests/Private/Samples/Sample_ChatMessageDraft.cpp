// Copyright 2026 PubNub Inc. All Rights Reserved.

#include "Samples/Sample_ChatMessageDraft.h"

#include "PubnubChatMessageDraft.h"
#include "FunctionLibraries/PubnubChatMessageDraftUtilities.h"

// snippet.quote_message

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::QuoteMessageSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	UPubnubChatMessage* Message = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)
	// Assumes Message is a valid UPubnubChatMessage to quote (e.g. from channel history)

	// Send a message that quotes another message
	UPubnubChatMessageDraft* MessageDraft = Channel->CreateMessageDraft();
	MessageDraft->SetQuotedMessage(Message);
	MessageDraft->InsertText(0, "Quoting this: ");
	MessageDraft->Send();
}

// snippet.add_user_mention

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::AddUserMentionSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	MyMessageDraft->InsertText(0, "Hello Alex! I have sent you this link on the #offtopic channel.");

	FString UserName = "Alex";
	FPubnubChatMentionTarget UserMentionTarget = UPubnubChatMessageDraftUtilities::CreateUserMentionTarget(UserName);

	MyMessageDraft->AddMention(6, UserName.Len(), UserMentionTarget);

	MyMessageDraft->Send();
}

// snippet.add_channel_reference

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::AddChannelReferenceSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	MyMessageDraft->InsertText(0, "Hello Alex! I have sent you this link on the #offtopic channel.");

	FString ChannelName = "offtopic";
	FPubnubChatMentionTarget ChannelMentionTarget = UPubnubChatMessageDraftUtilities::CreateChannelMentionTarget(ChannelName);

	MyMessageDraft->AddMention(45, ChannelName.Len() + 1, ChannelMentionTarget);

	MyMessageDraft->Send();
}

// snippet.add_url_link

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::AddUrlLinkSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	MyMessageDraft->InsertText(0, "Hello Alex! I have sent you this link on the #offtopic channel.");

	FString Url = "https://www.example.com";
	FPubnubChatMentionTarget UrlMentionTarget = UPubnubChatMessageDraftUtilities::CreateUrlMentionTarget(Url);

	MyMessageDraft->AddMention(33, 4, UrlMentionTarget);

	MyMessageDraft->Send();
}

// snippet.message_draft_add_mention

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftAddMentionSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	MyMessageDraft->InsertText(0, "Hello Alex! I have sent you this link on the #offtopic channel.");

	// Add a user mention for "Alex"
	FString UserName = "Alex";
	FPubnubChatMentionTarget UserMentionTarget = UPubnubChatMessageDraftUtilities::CreateUserMentionTarget(UserName);
	MyMessageDraft->AddMention(6, UserName.Len(), UserMentionTarget);

	// Add a URL link for "this"
	FString Url = "https://www.example.com";
	FPubnubChatMentionTarget UrlMentionTarget = UPubnubChatMessageDraftUtilities::CreateUrlMentionTarget(Url);
	int LinkStartPosition = 33;
	MyMessageDraft->AddMention(LinkStartPosition, 4, UrlMentionTarget);

	// Add a channel reference for "#offtopic"
	FString ChannelName = "offtopic";
	FPubnubChatMentionTarget ChannelMentionTarget = UPubnubChatMessageDraftUtilities::CreateChannelMentionTarget(ChannelName);
	int ChannelStartPosition = LinkStartPosition + 4 + 8;
	MyMessageDraft->AddMention(ChannelStartPosition, ChannelName.Len() + 1, ChannelMentionTarget);

	MyMessageDraft->Send();
}

// snippet.message_draft_remove_mention

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftRemoveMentionSample()
{
	// snippet.hide
	UPubnubChatMessageDraft* MessageDraft = nullptr;
	// snippet.show

	// Assumes MessageDraft is a valid UPubnubChatMessageDraft with existing mentions
	// e.g. the message reads: "Hello Alex! I have sent you this link on the #offtopic channel."

	// Remove the link mention at position 33
	MessageDraft->RemoveMention(33);
}

// snippet.message_draft_update

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftUpdateSample()
{
	// snippet.hide
	UPubnubChatMessageDraft* MessageDraft = nullptr;
	// snippet.show

	// Assumes MessageDraft is a valid UPubnubChatMessageDraft
	// e.g. the message reads: "I sent [Alex] this picture." where [Alex] is a user mention

	MessageDraft->Update("I did not send Alex this picture.");
	// The message now reads: "I did not send [Alex] this picture."
	// The mention is preserved because its text wasn't changed
}

// snippet.message_draft_insert_text

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftInsertTextSample()
{
	// snippet.hide
	UPubnubChatMessageDraft* MyMessageDraft = nullptr;
	// snippet.show

	// Assumes MyMessageDraft is a valid UPubnubChatMessageDraft

	// Set initial text in the message draft
	MyMessageDraft->Update(TEXT("Check this support article https://www.support-article.com/."));

	// Insert "out " at position 6 ("Check " is 6 characters long)
	int InsertPosition = 6;
	MyMessageDraft->InsertText(InsertPosition, TEXT("out "));

	// The draft now reads: "Check out this support article https://www.support-article.com/."
}

// snippet.message_draft_remove_text

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftRemoveTextSample()
{
	// snippet.hide
	UPubnubChatMessageDraft* MyMessageDraft = nullptr;
	// snippet.show

	// Assumes MyMessageDraft is a valid UPubnubChatMessageDraft

	// Set initial text in the message draft
	MyMessageDraft->Update(TEXT("Check out this support article https://www.support-article.com/."));

	// Remove "out " starting at position 6 (4 characters including the trailing space)
	int RemovePosition = 6;
	int RemoveLength = 4;
	MyMessageDraft->RemoveText(RemovePosition, RemoveLength);

	// The draft now reads: "Check this support article https://www.support-article.com/."
}

// snippet.message_draft_send

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftSendSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();
	MyMessageDraft->Update("Hello Alex!");

	FPubnubChatOperationResult Result = MyMessageDraft->Send();
}

// snippet.message_draft_updated

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftUpdatedSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	// Bind to receive draft content changes
	MyMessageDraft->OnMessageDraftUpdatedNative.AddUObject(this, &ASample_ChatMessageDraft::OnMessageDraftUpdate);

	// Trigger the delegate by updating the draft
	MyMessageDraft->Update("Review the message draft contents.");
}

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::OnMessageDraftUpdate(const TArray<FPubnubChatMessageElement>& MessageElements)
{
	for (const FPubnubChatMessageElement& Element : MessageElements)
	{
		UE_LOG(LogTemp, Log, TEXT("Draft Element Text: %s"), *Element.Text);
	}
}

// snippet.message_draft_updated_with_suggestions

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftUpdatedWithSuggestionsSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	// Bind to receive draft content changes with suggestions for @user and #channel patterns
	MyMessageDraft->OnMessageDraftUpdatedWithSuggestionsNative.AddUObject(this, &ASample_ChatMessageDraft::OnMessageDraftUpdateWithSuggestions);

	// Trigger the delegate by updating the draft with mentions
	MyMessageDraft->Update("Discuss this with @Alex and review in #general.");
}

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::OnMessageDraftUpdateWithSuggestions(const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions)
{
	for (const FPubnubChatMessageElement& Element : MessageElements)
	{
		UE_LOG(LogTemp, Log, TEXT("Draft Element Text: %s"), *Element.Text);
	}

	for (const FPubnubChatSuggestedMention& Suggestion : SuggestedMentions)
	{
		UE_LOG(LogTemp, Log, TEXT("Suggested Mention: Replace '%s' with '%s'"), *Suggestion.ReplaceFrom, *Suggestion.ReplaceTo);
	}
}

// snippet.message_draft_insert_suggested_mention

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::MessageDraftInsertSuggestedMentionSample()
{
	// snippet.hide
	UPubnubChatChannel* Channel = nullptr;
	// snippet.show

	// Assumes Channel is a valid UPubnubChatChannel (e.g. from GetChannel)

	UPubnubChatMessageDraft* MyMessageDraft = Channel->CreateMessageDraft();

	// Bind to receive suggestions and automatically insert the first one
	MyMessageDraft->OnMessageDraftUpdatedWithSuggestionsNative.AddUObject(this, &ASample_ChatMessageDraft::OnMessageDraftUpdateWithSuggestions_InsertSample);

	// Trigger the delegate and suggestions
	MyMessageDraft->Update("Please coordinate with @Al for the meeting.");
}

// ACTION REQUIRED: Replace ASample_ChatMessageDraft with name of your Actor class
void ASample_ChatMessageDraft::OnMessageDraftUpdateWithSuggestions_InsertSample(const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions)
{
	// snippet.hide
	UPubnubChatMessageDraft* MyMessageDraft = nullptr;
	// snippet.show

	if (SuggestedMentions.Num() > 0)
	{
		const FPubnubChatSuggestedMention& FirstSuggestion = SuggestedMentions[0];
		MyMessageDraft->InsertSuggestedMention(FirstSuggestion);
		UE_LOG(LogTemp, Log, TEXT("Inserted suggested mention: %s"), *FirstSuggestion.ReplaceTo);
	}
}

// snippet.end
