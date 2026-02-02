// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"

#include "PubnubChatMessageDraft.generated.h"

class UPubnubChatChannel;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageDraftUpdated, const TArray<FPubnubChatMessageElement>&, MessageElements);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageDraftUpdatedNative, const TArray<FPubnubChatMessageElement>& MessageElements);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageDraftUpdatedWithSuggestions, const TArray<FPubnubChatMessageElement>&, MessageElements, const TArray<FPubnubChatSuggestedMention>&, SuggestedMentions);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatMessageDraftUpdatedWithSuggestionsNative, const TArray<FPubnubChatMessageElement>& MessageElements, const TArray<FPubnubChatSuggestedMention>& SuggestedMentions);


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatMessageDraft : public UObject
{
	GENERATED_BODY()
	friend class UPubnubChatChannel;

public:
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageDraftUpdated OnMessageDraftUpdated;
	FOnPubnubChatMessageDraftUpdatedNative OnMessageDraftUpdatedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageDraftUpdatedWithSuggestions OnMessageDraftUpdatedWithSuggestions;
	FOnPubnubChatMessageDraftUpdatedWithSuggestionsNative OnMessageDraftUpdatedWithSuggestionsNative;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	FString GetCurrentText() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message Draft")
	TArray<FPubnubChatMessageElement> GetMessageElements() const { return MessageElements; }
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult InsertText(int Position, const FString Text);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult RemoveText(int Position, int Length);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult AddMention(int Position, int Length, const FPubnubChatMentionTarget MentionTarget);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult RemoveMention(int Position);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult Update(const FString& NewText);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult InsertSuggestedMention(const FPubnubChatSuggestedMention SuggestedMention);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Message Draft")
	FPubnubChatOperationResult Send(FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
private:
	UPROPERTY()
	UPubnubChatChannel* Channel = nullptr;
	UPROPERTY()
	FPubnubChatMessageDraftConfig MessageDraftConfig;
	
	UPROPERTY()
	TArray<FPubnubChatMessageElement> MessageElements;
	
	//Cache for suggestion results to avoid repeated API calls
	TMap<FString, TArray<FPubnubChatSuggestedMention>> SuggestionsCache;
	
	//Flag to suppress delegate and typing indicator calls during batch operations
	bool bSuppressDelegateAndTyping = false;
	
	FString GetDraftTextToSend();
	
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

