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
	
private:
	UPROPERTY()
	UPubnubChatChannel* Channel = nullptr;
	UPROPERTY()
	FPubnubChatMessageDraftConfig MessageDraftConfig;
	
	UPROPERTY()
	TArray<FPubnubChatMessageElement> MessageElements;
	
	void InitMessageDraft(UPubnubChatChannel* InChannel, const FPubnubChatMessageDraftConfig& InMessageDraftConfig);
	void FireMessageDraftChangedDelegate();
	void TriggerTypingIndicator();
	void MoveMessageElementsAfterPosition(int Position, int Length, bool IncludeEqualPosition);
	
	bool IsPositionWithinMentionTarget(int Position, int Length);
	
	void MergeAdjacentTextElements();
};

