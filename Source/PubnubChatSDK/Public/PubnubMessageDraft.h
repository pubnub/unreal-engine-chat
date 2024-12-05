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


/**
 * Defines the target of the mention attached to a [MessageDraft].
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMentionTarget : public UObject
{
	GENERATED_BODY()
public:
	~UPubnubMentionTarget();

	/**
	 * Create MentionTarget to an user identified by [UserID].
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* CreateUserMentionTarget(const FString UserID);

	/**
	 * Create MentionTarget to reference a channel identified by [ChannelID].
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* CreateChannelMentionTarget(const FString Channel);

	/**
	 * Create MentionTarget with a link to [Url].
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Mention Target")
	static UPubnubMentionTarget* CreateUrlMentionTarget(const FString Url);

	/**
	 * Get the "Target" of this MentionTarget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Mention Target")
	FString GetTarget();

	/**
	 * Get the type of this MentionTarget.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Mention Target")
	EPubnubMentionTargetType GetType();

	//Internal usage only
	static UPubnubMentionTarget* Create(Pubnub::MentionTarget MentionTarget);
	
	//Internal usage only
	Pubnub::MentionTarget* GetInternalMentionTarget(){return InternalMentionTarget;};

protected:
	Pubnub::MentionTarget* InternalMentionTarget;

	bool IsInternalMentionTargetValid();
};

/**
 * A potential mention suggestion received from [AddChangeListenerWithSuggestions].
 *
 * It can be used with [PubnubMessageDraft->InsertSuggestedMention] to accept the suggestion and attach a mention to a message draft.
 *
 */
USTRUCT(BlueprintType)
struct FPubnubSuggestedMention
{
	GENERATED_BODY()

	//The offset where the mention starts
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Offset = 0;
	//The original text at the [offset] in the message draft text
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceFrom = "";
	//The suggested replacement for the [replaceFrom] text, e.g. the user's full name
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceTo = "";
	//The target of the mention, such as a user, channel or URL
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
 * Part of a [PubnubMessageDraft] content.
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMessageElement : public UObject
{
	GENERATED_BODY()
public:
	~UPubnubMessageElement();
	
	/**
	 * Get the literal text contained in this [PubnubMessageElement]. This is what the user should see when reading or composing the message.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message Element")
	FString GetText();

	/**
	 * Get the mention contained in this element. Can be nullptr if this message element is just a plain text without [MentionTarget]
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Message Element")
	UPubnubMentionTarget* GetTarget();

	//Internal usage only
	static UPubnubMessageElement* Create(Pubnub::MessageElement MessageElement);
	
	//Internal usage only
	Pubnub::MessageElement* GetInternalMessageElement(){return InternalMessageElement;};

protected:
	Pubnub::MessageElement* InternalMessageElement;

	bool IsInternalMessageElementValid();
};


DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubDraftUpdated, const TArray<UPubnubMessageElement*>&, MessageElements);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnPubnubDraftUpdatedWithSuggestions, const TArray<UPubnubMessageElement*>&, MessageElements, const TArray<FPubnubSuggestedMention>&, SuggestedMentions);

/**
 * MessageDraft is an object that refers to a single message that has not been published yet.
 *
 * It helps with editing tasks such as adding user mentions, channel references and links.
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubMessageDraft : public UObject
{
	GENERATED_BODY()
public:
	~UPubnubMessageDraft();

	/**
     * Insert some text into the [PubnubMessageDraft] text at the given offset.
     *
     * @param Position the position from the start of the message draft where insertion will occur
     * @param Text the text to insert at the given offset
     */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void InsertText(int Position, const FString Text);

	/**
     * Remove a number of characters from the [PubnubMessageDraft] text at the given offset.
     *
     * @param Position the position from the start of the message draft where removal will occur
     * @param Length the number of characters to remove, starting at the given offset
     */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void RemoveText(int Position, int Length);

	/**
	 * Insert mention into the [PubnubMessageDraft] according to [SuggestedMention.Offset], [SuggestedMention.ReplaceFrom] and
	 * [SuggestedMention.Target].
	 *
	 * The [SuggestedMention] must be up to date with the message text, that is: [SuggestedMention.ReplaceFrom] must
	 * match the message draft at position [SuggestedMention.ReplaceFrom], otherwise an error will be thrown.
	 *
	 * @param SuggestedMention a [SuggestedMention] that can be obtained from [MessageDraftChangeListener]
	 * @param Text the text to replace [SuggestedMention.ReplaceFrom] with. [SuggestedMention.ReplaceTo] can be used for example.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void InsertSuggestedMention(FPubnubSuggestedMention SuggestedMention, const FString Text);

	/**
     * Add a mention to a user, channel or link specified by [MentionTarget] at the given offset.
     *
     * @param Position the start of the mention
     * @param Length the number of characters (length) of the mention
     * @param MentionTarget the target of the mention, e.g. use [CreateUserMentionTarget], [CreateChannelMentionTarget], [CreateUrlMentionTarget]
     */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddMention(int Position, int Length, UPubnubMentionTarget* MentionTarget);

	/**
	 * Remove a mention starting at the given offset, if any.
	 *
	 * @param Position the start of the mention to remove
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void RemoveMention(int Position);

	/**
     * Update the whole message draft text with a new value.
     *
     * Internally [PubnubMessageDraft] will try to calculate the optimal set of insertions and removals that will convert the
     * current text to the provided [Text], in order to preserve any mentions. This is a best effort operation, and if
     * any mention text is found to be modified, the mention will be invalidated and removed.
     */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void Update(FString Text);

	/**
	 * Send the [PubnubMessageDraft], along with all mentions.
	 *
	 * @param SendTextParams Additional Parameters that can be added to the message
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void Send(FPubnubSendTextParams SendTextParams = FPubnubSendTextParams());

	/**
	 * Add a callback to listen for changes to the contents of this [PubnubMessageDraft].
	 * This callback will not contain any suggestions for user and channels.
	 * To get suggestions use [AddChangeListenerWithSuggestions] instead
	 *
	 * @param DraftUpdateCallback the [FOnPubnubDraftUpdatedWithSuggestions] callback that will receive the most current
	 * message elements list.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddChangeListener(FOnPubnubDraftUpdated DraftUpdateCallback);

	/**
	 * Add a callback to listen for changes to the contents of this [PubnubMessageDraft], as well as
	 * to retrieve the current mention suggestions for users and channels (e.g. when the message draft contains
	 * "... @name ..." or "... #chann ...")
	 *
	 * @param DraftUpdateCallback the [FOnPubnubDraftUpdatedWithSuggestions] callback that will receive the most current
	 * message elements list and suggestions list.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Message Draft")
	void AddChangeListenerWithSuggestions(FOnPubnubDraftUpdatedWithSuggestions DraftUpdateCallback);

	//Internal usage only
	static UPubnubMessageDraft* Create(Pubnub::MessageDraft MessageDraft);
	//Internal usage only
	Pubnub::MessageDraft* GetInternalMessage(){return InternalMessageDraft;};

protected:
	Pubnub::MessageDraft* InternalMessageDraft;

	bool IsInternalMessageDraftValid();
};
