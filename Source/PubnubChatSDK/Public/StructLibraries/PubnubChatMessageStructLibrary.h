// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatMessageStructLibrary.generated.h"

class UPubnubChatMessage;


/**
 * Represents a message action (reaction, receipt, etc.) attached to a message.
 * Message actions allow users to interact with messages without modifying the message content.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageAction
{
	GENERATED_BODY()

	/** The type of message action (reaction, receipt, custom, etc.). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") EPubnubChatMessageActionType Type = EPubnubChatMessageActionType::PCMAT_Reaction;
	/** The action value (e.g., emoji for reactions, custom data for other types). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Value = "";
	/** Timetoken when this action was added to the message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	/** User ID of the user who added this action. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";

	FPubnubMessageActionData ToPubnubMessageActionData() const;
	static FPubnubChatMessageAction FromPubnubMessageActionData(const FPubnubMessageActionData& PubnubMessageActionData);
};

/**
 * Aggregated reaction data for a message.
 * Groups all users who added the same reaction value.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageReaction
{
	GENERATED_BODY()

	/** The reaction value (e.g., emoji or custom reaction identifier). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Value = "";
	/** True if the current user has added this reaction. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMine = false;
	/** Array of user IDs who added this reaction. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FString> UserIDs;
	/** Total count of users who added this reaction. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Count = 0;
};

/**
 * Core message data structure containing all message content and metadata.
 * Represents the complete information about a chat message.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageData
{
	GENERATED_BODY()

	/** Message type identifier for custom handling (e.g., "text", "image", "system"). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";
	/** The actual text content of the message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Text = "";
	/** ID of the channel where this message was sent. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	/** User ID of the message sender. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	/** JSON string containing additional metadata (mentions, quoted messages, custom data). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Meta = "";
	/** Array of all message actions (reactions, receipts, etc.) attached to this message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatMessageAction> MessageActions;
	
	static FPubnubChatMessageData FromPubnubMessageData(const FPubnubMessageData& PubnubMessageData);
	static FPubnubChatMessageData FromPubnubHistoryMessageData(const FPubnubHistoryMessageData& FPubnubHistoryMessageData);
};

/**
 * Data for a quoted message stored in message metadata (Meta.quotedMessage).
 * When a message is sent as a reply quoting another message, the quoted message's
 * timetoken, text, and user ID are stored in the publishing message's metadata.
 */
USTRUCT(BlueprintType)
struct PUBNUBCHATSDK_API FPubnubChatQuotedMessageData
{
	GENERATED_BODY()

	/** Timetoken of the quoted message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	/** Text of the quoted message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Text = "";
	/** User ID of the author of the quoted message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
};

/**
 * Result of a single message retrieval or modification operation.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The message object. Null if the operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMessage* Message = nullptr;
};

/**
 * Result of fetching all reactions for a message.
 * Contains aggregated reaction data grouped by reaction value.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetReactionsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of aggregated reactions, each containing the value, count, and user IDs. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatMessageReaction> Reactions;
};

/**
 * Result of checking if a message has a specific reaction from the current user.
 */
USTRUCT(BlueprintType)
struct FPubnubChatHasReactionResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the current user has added the specified reaction to the message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool HasReaction = false;
};

/**
 * Target information for a mention in a message.
 * Identifies what entity (user, channel, or URL) is being referenced.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMentionTarget
{
	GENERATED_BODY()

	/** Type of mention target (user, channel, or URL). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") EPubnubChatMentionTargetType MentionTargetType = EPubnubChatMentionTargetType::PCMTT_None;
	/** The target identifier (user ID, channel ID, or URL string). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Target = "";
};

/**
 * Represents a parsed element within a message, including any mention information.
 * Used for rendering messages with highlighted mentions and clickable links.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageElement
{
	GENERATED_BODY()

	/** Mention target if this element is a mention (user, channel, or URL). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatMentionTarget MentionTarget;
	/** The text content of this element. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Text = "";
	/** Starting character position in the original message text. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Start = 0;
	/** Character length of this element in the original message text. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Length = 0;
	
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

/**
 * Represents a suggested mention for message drafts.
 * Provides autocomplete suggestions when users type @ or # symbols.
 */
USTRUCT(BlueprintType)
struct FPubnubChatSuggestedMention
{
	GENERATED_BODY()

	/** Character offset where the mention trigger starts in the draft text. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Offset = 0;
	/** The original text that triggered the suggestion (e.g., "@john"). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceFrom = "";
	/** The suggested replacement text (e.g., "@John Smith"). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReplaceTo = "";
	/** The target of the mention (user, channel, or URL reference). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatMentionTarget Target;
};
