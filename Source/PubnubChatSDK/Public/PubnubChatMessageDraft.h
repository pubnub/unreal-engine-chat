// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChat.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"

#include "PubnubChatMessageDraft.generated.h"

class UPubnubChatChannel;
class UPubnubChatMessage;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageDraftUpdated, const TArray<FPubnubChatMessageElement>&, MessageElements);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageDraftUpdatedNative, const TArray<FPubnubChatMessageElement>& MessageElements);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageDraftUpdatedWithSuggestions, const TArray<FPubnubChatMessageElement>&, MessageElements, const TArray<FPubnubChatSuggestedMention>&, SuggestedMentions);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageDraftUpdatedWithSuggestionsNative, const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions);


/**
 * Editable draft for a channel message with structured content (text and mentions). Belongs to a channel; edits are in-memory until Send.
 * Supports insert/remove text, add/remove mentions, replace-all via Update, and insert-from-suggestion. Fires delegates on content change; Send publishes the draft as a message (with mentions as markdown links).
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMessageDraft : public UObject
{
	GENERATED_BODY()
	friend class UPubnubChatChannel;

public:
	
	/**
	 * Broadcast when the draft content changes (after InsertText, AppendText, RemoveText, AddMention, RemoveMention, Update, or InsertSuggestedMention).
	 * @param MessageElements Current list of message elements (text and mentions) in the draft.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageDraftUpdated OnMessageDraftUpdated;
	FOnPubnubChatMessageDraftUpdatedNative OnMessageDraftUpdatedNative;
	
	/**
	 * Broadcast when the draft content changes, and also provides suggested mentions for @user / #channel patterns in the text (e.g. for autocomplete).
	 * SuggestedMentions may be populated from API when there are listeners; otherwise empty.
	 * @param MessageElements Current list of message elements in the draft.
	 * @param SuggestedMentions Suggestions for unresolved @user / #channel spans in the draft.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageDraftUpdatedWithSuggestions OnMessageDraftUpdatedWithSuggestions;
	FOnPubnubChatMessageDraftUpdatedWithSuggestionsNative OnMessageDraftUpdatedWithSuggestionsNative;
	
	/* PUBLIC FUNCTIONS */
	
	/**
	 * Returns the current draft text by concatenating all message elements (plain text and mention display text).
	 * Local: does not perform any network requests.
	 *
	 * @return Full draft text string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	FString GetCurrentText() const;
	
	/**
	 * Returns a copy of the current message elements (text segments and mentions with position and target).
	 * Local: does not perform any network requests.
	 *
	 * @return Array of message elements.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	TArray<FPubnubChatMessageElement> GetMessageElements() const { return MessageElements; }
	
	/**
	 * Returns the text that would be sent by Send() (mentions serialized as markdown links).
	 * Use this to obtain the serialized form for parsing tests or preview. Local: does not perform network requests.
	 *
	 * @return Serialized draft text (plain segments and [text](url) links). Empty if draft is empty.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	FString GetTextToSend() const;
	
	/**
	 * Inserts text at the given position in the draft. Fires OnMessageDraftUpdated (and suggestion delegates); may trigger typing indicator on the channel.
	 * Local: edits the in-memory draft only. If Position is strictly inside an existing mention (between its start and end), that mention is removed (its text is preserved) and the new text is inserted; all other mentions are preserved and their Start positions are adjusted. Inserting at the start or end of a mention does not remove it (insert before/after behaviour).
	 *
	 * @param Position Index at which to insert (0-based). Must be within current draft bounds. Only when strictly inside a mention is the mention converted to plain text; at mention boundaries the mention is preserved.
	 * @param Text Text to insert. Must be non-empty.
	 * @return Operation result. Success if the text was inserted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult InsertText(int Position, const FString Text);
	
	/**
	 * Appends text at the end of the draft. Equivalent to InsertText(GetCurrentText().Len(), Text). Fires OnMessageDraftUpdated; may trigger typing indicator.
	 * Local: edits the in-memory draft only. Fails if Text is empty.
	 *
	 * @param Text Text to append. Must be non-empty.
	 * @return Operation result. Success if the text was appended.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult AppendText(const FString Text);

	/**
	 * Removes Length characters starting at Position from the draft. Fires OnMessageDraftUpdated; may trigger typing indicator on the channel.
	 * Local: edits the in-memory draft only. Fails if Position or range is invalid, or if the range overlaps an existing mention.
	 *
	 * @param Position Start index (0-based). Must be within draft bounds.
	 * @param Length Number of characters to remove. Must be positive; range must not extend past the end or overlap a mention.
	 * @return Operation result. Success if the text was removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult RemoveText(int Position, int Length);

	/**
	 * Marks the span [Position, Position + Length) as a mention with the given target (user, channel, or URL). Fires OnMessageDraftUpdated; may trigger typing indicator.
	 * Local: edits the in-memory draft only. Fails if the draft is empty, the range is invalid, or overlaps an existing mention.
	 *
	 * @param Position Start index (0-based) of the span to turn into a mention.
	 * @param Length Length of the span. Must be positive; range must be within draft bounds and not overlap another mention.
	 * @param MentionTarget The mention target (user ID, channel ID, or URL).
	 * @return Operation result. Success if the mention was added.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult AddMention(int Position, int Length, const FPubnubChatMentionTarget MentionTarget);
	
	/**
	 * Removes the mention at the given position (converts that element back to plain text). Fires OnMessageDraftUpdated; may trigger typing indicator.
	 * Local: edits the in-memory draft only. Fails if Position is not within a mention element.
	 *
	 * @param Position Any index (0-based) inside the mention element to remove.
	 * @return Operation result. Success if the mention was removed.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult RemoveMention(int Position);
	
	/**
	 * Replaces the entire draft content with NewText. Mentions that overlap changed regions are removed; unchanged regions preserve structure where possible. Fires OnMessageDraftUpdated; may trigger typing indicator.
	 * Local: edits the in-memory draft only. No-op if current text equals NewText.
	 *
	 * @param NewText The new full draft text.
	 * @return Operation result. Success if the draft was updated (or unchanged).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult Update(const FString& NewText);
	
	/**
	 * Replaces the text at the suggestion's offset (ReplaceFrom) with ReplaceTo and adds a mention for the suggestion's target. Used to apply an autocomplete selection (e.g. from OnMessageDraftUpdatedWithSuggestions).
	 * Local: edits the in-memory draft only. Fails if ReplaceFrom does not match the current text at Offset, or if target is invalid.
	 *
	 * @param SuggestedMention Suggestion with Offset, ReplaceFrom, ReplaceTo, and Target. ReplaceFrom must match draft text at Offset.
	 * @return Operation result. Success if the mention was inserted.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult InsertSuggestedMention(const FPubnubChatSuggestedMention SuggestedMention);
	
	/**
	 * Returns the quoted message attached to this draft, if any.
	 * Local: does not perform any network requests.
	 *
	 * @return The quoted message, or nullptr if none is set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	UPubnubChatMessage* GetQuotedMessage() const { return QuotedMessage; }

	/**
	 * Sets (or clears) the quoted message attached to this draft. Pass nullptr to remove any existing quoted message.
	 * The quoted message is sent alongside the draft text when Send() is called.
	 * Local: does not perform any network requests.
	 *
	 * @param InQuotedMessage The message to quote, or nullptr to clear.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	void SetQuotedMessage(UPubnubChatMessage* InQuotedMessage) { QuotedMessage = InQuotedMessage; }

	/**
	 * Sends the draft as a message on the associated channel. Draft text is serialized (mentions as markdown links); the channel's SendText is used.
	 * Blocking: performs network requests on the calling thread. Blocks for the duration of the operation.
	 * Fails if the channel is invalid or the draft text is empty.
	 *
	 * @param SendTextParams Optional parameters passed to the channel send (e.g. metadata, quoted message).
	 * @return Operation result. Success if the message was sent.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult Send(FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
	/**
	 * Sends the draft asynchronously as a message on the associated channel. Fails if the channel is invalid or the draft text is empty.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param SendTextParams Optional parameters passed to the channel send (e.g. metadata, quoted message).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SendAsync(FOnPubnubChatOperationResponse OnOperationResponse, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	/**
	 * Sends the draft asynchronously as a message on the associated channel. Fails if the channel is invalid or the draft text is empty.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param SendTextParams Optional parameters passed to the channel send (e.g. metadata, quoted message).
	 */
	void SendAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
private:
	UPROPERTY()
	UPubnubChatChannel* Channel = nullptr;
	UPROPERTY()
	FPubnubChatMessageDraftConfig MessageDraftConfig;
	
	UPROPERTY()
	TArray<FPubnubChatMessageElement> MessageElements;
	
	UPROPERTY()
	UPubnubChatMessage* QuotedMessage = nullptr;
	
	//Cache for suggestion results to avoid repeated API calls
	TMap<FString, TArray<FPubnubChatSuggestedMention>> SuggestionsCache;
	
	//Flag to suppress delegate and typing indicator calls during batch operations
	bool bSuppressDelegateAndTyping = false;
	
	FString GetDraftTextToSend() const;
	
	void InitMessageDraft(UPubnubChatChannel* InChannel, const FPubnubChatMessageDraftConfig& InMessageDraftConfig);
	void FireMessageDraftChangedDelegate();
	void TriggerTypingIndicator();
	void MoveMessageElementsAfterPosition(int Position, int Length, bool IncludeEqualPosition);
	
	bool IsPositionWithinMentionTarget(int Position, int Length);
	
	void MergeAdjacentTextElements();
	
	// Suggested mentions functionality
	bool HasListenersForSuggestions() const;
	TArray<FPubnubChatSuggestedMention> GetSuggestedMentions();
	
	// Helper struct for regex matches
	struct FMentionMatch
	{
		int32 Offset;
		FString Text;
		bool bIsUserMention; // true for @username, false for #channelname
	};
	
	TArray<FMentionMatch> FindUserMentionMatches(const FString& Text) const;
	TArray<FMentionMatch> FindChannelMentionMatches(const FString& Text) const;
	TArray<FPubnubChatSuggestedMention> ResolveUserSuggestions(const TArray<FMentionMatch>& Matches);
	TArray<FPubnubChatSuggestedMention> ResolveChannelSuggestions(const TArray<FMentionMatch>& Matches);
	
	// Helper functions for markdown link escaping
	static FString EscapeLinkText(const FString& Text);
	static FString EscapeLinkUrl(const FString& Url);
};

