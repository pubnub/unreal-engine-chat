// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include <pubnub_chat/message_draft.hpp>
#include "PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubMessageDraft.generated.h"

class UPubnubMessage;
class UPubnubThreadChannel;
class UPubnubCallbackStop;


PN_CHAT_EXPORT struct SuggestedMention {
	std::size_t offset;
	Pubnub::String replace_from;
	Pubnub::String replace_to;
	Pubnub::MentionTarget target;
};

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMentionTarget : public UObject
{
	GENERATED_BODY()
public:
	static UPubnubMentionTarget* Create(Pubnub::MentionTarget MentionTarget);
	~UPubnubMentionTarget();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* User(const FString UserID);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* Channel(const FString Channel);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* Url(const FString Url);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Mention Target")
	FString GetTarget();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Mention Target")
	EPubnubMentionTargetType GetType();
	
	//Internal usage only
	Pubnub::MentionTarget* GetInternalMentionTarget(){return InternalMentionTarget;};

protected:
	Pubnub::MentionTarget* InternalMentionTarget;

	bool IsInternalMentionTargetValid();
};

USTRUCT(BlueprintType)
struct FPubnubSuggestedMention
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Offset = 0;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceFrom = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceTo = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubMentionTarget* Target = nullptr;

	FPubnubSuggestedMention() = default;
	FPubnubSuggestedMention(Pubnub::SuggestedMention SuggestedMention) :
	Offset(SuggestedMention.offset),
	ReplaceFrom(UPubnubChatUtilities::PubnubStringToFString(SuggestedMention.replace_from)),
	ReplaceTo(UPubnubChatUtilities::PubnubStringToFString(SuggestedMention.replace_to))
	{
		Target = UPubnubMentionTarget::Create(SuggestedMention.target);
	};

	//Internal use only
	Pubnub::SuggestedMention GetCppSuggestedMention()
	{
		Pubnub::SuggestedMention SuggestedMention;
		SuggestedMention.offset = Offset;
		SuggestedMention.replace_from = UPubnubChatUtilities::FStringToPubnubString(ReplaceFrom);
		SuggestedMention.replace_to = UPubnubChatUtilities::FStringToPubnubString(ReplaceTo);
		if(Target)
		{
			SuggestedMention.target = *Target->GetInternalMentionTarget();
		}
		return SuggestedMention;
	}
	
};


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMessageElement : public UObject
{
	GENERATED_BODY()
public:
	static UPubnubMessageElement* Create(Pubnub::MessageElement MessageElement);
	~UPubnubMessageElement();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Element")
	static UPubnubMessageElement* PlainText(const FString Text);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Element")
	static UPubnubMessageElement* Link(const FString Text, UPubnubMentionTarget* Target);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message Element")
	FString GetText();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message Element")
	UPubnubMentionTarget* GetTarget();
	
	//Internal usage only
	Pubnub::MessageElement* GetInternalMessageElement(){return InternalMessageElement;};

protected:
	Pubnub::MessageElement* InternalMessageElement;

	bool IsInternalMessageElementValid();
};


DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubDraftUpdated, const TArray<UPubnubMessageElement*>&, MessageElements);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnPubnubDraftUpdatedWithSuggestions, const TArray<UPubnubMessageElement*>&, MessageElements, const TArray<FPubnubSuggestedMention>&, SuggestedMentions);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMessageDraft : public UObject
{
	GENERATED_BODY()
public:
	static UPubnubMessageDraft* Create(Pubnub::MessageDraft MessageDraft);
	~UPubnubMessageDraft();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void InsertText(int Position, const FString Text);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void RemoveText(int Position, int Length);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void InsertSuggestedMention(FPubnubSuggestedMention SuggestedMention, const FString Text);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddMention(int Position, int Length, UPubnubMentionTarget* MentionTarget);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void RemoveMention(int Position);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void Update(FString Text);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void Send(FPubnubSendTextParams SendTextParams = FPubnubSendTextParams());

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddChangeListener(FOnPubnubDraftUpdated DraftUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddChangeListenerWithSuggestions(FOnPubnubDraftUpdatedWithSuggestions DraftUpdateCallback);
	
	//Internal usage only
	Pubnub::MessageDraft* GetInternalMessage(){return InternalMessageDraft;};

protected:
	Pubnub::MessageDraft* InternalMessageDraft;

	bool IsInternalMessageDraftValid();
};
