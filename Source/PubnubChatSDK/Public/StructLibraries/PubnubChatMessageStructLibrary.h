// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatMessageStructLibrary.generated.h"

class UPubnubChatMessage;


USTRUCT(BlueprintType)
struct FPubnubChatMessageAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	EPubnubChatMessageActionType Type = EPubnubChatMessageActionType::PCMAT_Reaction;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Value = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Timetoken = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString UserID = "";

	FPubnubMessageActionData ToPubnubMessageActionData() const;
	static FPubnubChatMessageAction FromPubnubMessageActionData(const FPubnubMessageActionData& PubnubMessageActionData);
};

USTRUCT(BlueprintType)
struct FPubnubChatMessageReaction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Value = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool IsMine = false;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FString> UserIDs;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Count = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatMessageData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Type = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Text = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString ChannelID = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString UserID = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Meta = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatMessageAction> MessageActions;
	
	static FPubnubChatMessageData FromPubnubMessageData(const FPubnubMessageData& PubnubMessageData);
	static FPubnubChatMessageData FromPubnubHistoryMessageData(const FPubnubHistoryMessageData& FPubnubHistoryMessageData);
};

USTRUCT(BlueprintType)
struct FPubnubChatMessageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMessage* Message = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetReactionsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatMessageReaction> Reactions;
};

USTRUCT(BlueprintType)
struct FPubnubChatHasReactionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool HasReaction = false;
};

USTRUCT(BlueprintType)
struct FPubnubChatMentionTarget
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	EPubnubChatMentionTargetType MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Target = "";
};

USTRUCT(BlueprintType)
struct FPubnubChatMessageElement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatMentionTarget MentionTarget;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Text = "";
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Start = 0;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Length = 0;
	
	int GetEndPosition() { return Start + Length; }
	void InsertText(int Position, const FString& InText)
	{
		Text.InsertAt(Position, InText);
		Length += InText.Len();
	};
	void InsertTextAtTheEnd(const FString& InText)
	{
		Text.Append(InText);
		Length += InText.Len();
	};
	void RemoveText(int Position, int InLength)
	{
		Text.RemoveAt(Position, InLength);
		Length -= InLength;
	};
};

USTRUCT(BlueprintType)
struct FPubnubChatSuggestedMention
{
	GENERATED_BODY()

	//The offset where the mention starts
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Offset = 0;
	//The original text at the [offset] in the message draft text
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceFrom = "";
	//The suggested replacement for the [replaceFrom] text, e.g. the user's full name
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceTo = "";
	//The target of the mention, such as a user, channel or URL
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatMentionTarget Target;
};
