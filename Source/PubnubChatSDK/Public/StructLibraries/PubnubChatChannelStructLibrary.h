// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatChannelStructLibrary.generated.h"

class UPubnubChatChannel;
class UPubnubChatCallbackStop;
class UPubnubChatMembership;
class UPubnubChatThreadChannel;
class UPubnubChatMessage;
class UPubnubChatThreadMessage;


/**
 * Basic channel data structure used when creating a new channel.
 * Contains editable metadata for channel configuration.
 */
USTRUCT(BlueprintType)
struct FPubnubChatChannelData
{
	GENERATED_BODY()
	
	/** Display name for the channel. Used in UI for user-friendly channel identification. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelName = "";
	/** Detailed description of the channel's purpose or topic. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Description = "";
	/** JSON object providing custom channel data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** Channel status indicator. Max. 50 characters. Examples: "active", "archived", "readonly". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** Channel classification type. Max. 50 characters. Examples: "public", "group", "direct". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";

	FPubnubChannelInputData ToPubnubChannelInputData() const;
	static FPubnubChatChannelData FromPubnubChannelData(const FPubnubChannelData &PubnubChannelData);
};

/**
 * Channel data structure for partial updates with ForceSet flags.
 * Use ForceSet flags to explicitly set fields to empty values when needed.
 * Without ForceSet, empty strings are treated as "no change" during updates.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUpdateChannelInputData
{
	GENERATED_BODY()
	
	/** Display name for the channel. Used in UI for user-friendly channel identification. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelName = "";
	/** Detailed description of the channel's purpose or topic. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Description = "";
	/** JSON object providing custom channel data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** Channel status indicator. Max. 50 characters. Examples: "active", "archived", "readonly". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** Channel classification type. Max. 50 characters. Examples: "public", "group", "direct". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";
	
	/** When true, ChannelName will be updated even if empty. Use to explicitly clear the channel name. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetChannelName = false;
	/** When true, Description will be updated even if empty. Use to explicitly clear the description. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetDescription = false;
	/** When true, Custom will be updated even if empty. Use to explicitly clear custom data. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetCustom = false;
	/** When true, Status will be updated even if empty. Use to explicitly clear the status. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetStatus = false;
	/** When true, Type will be updated even if empty. Use to explicitly clear the type. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetType = false;
	
	/**
	 * Sets all ForceSet flags to true. Useful for full replacement of Channel Data.
	 */
	void ForceSetAllFields()
	{
		ForceSetChannelName = true;
		ForceSetDescription = true;
		ForceSetCustom = true;
		ForceSetStatus = true;
		ForceSetType = true;
	}

	FPubnubChannelInputData ToPubnubChannelInputData() const;
	
	static FPubnubChatUpdateChannelInputData FromChatChannelData(const FPubnubChatChannelData &PubnubChannelData);
};


/**
 * Result of a single channel retrieval operation.
 * Contains the operation status and the retrieved channel object.
 */
USTRUCT(BlueprintType)
struct FPubnubChatChannelResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The retrieved channel object. Null if the operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatChannel* Channel = nullptr;
};

/**
 * Result of a paginated channel list query.
 * Contains multiple channels with pagination information for fetching more results.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetChannelsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of retrieved channel objects matching the query criteria. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatChannel*> Channels;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of channels matching the query (may exceed returned count due to pagination). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Result of a channel suggestion query for mentions.
 * Used by message drafts to suggest channels when typing @channel references.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetChannelSuggestionsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of suggested channels matching the search text. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatChannel*> Channels;
};

/**
 * Result of joining a channel.
 * Returns the created membership object representing the user's connection to the channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatJoinResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The membership created when joining the channel. Null if the operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* Membership = nullptr;
};

/**
 * Result of inviting a single user to a channel.
 * Returns the membership object created for the invited user.
 */
USTRUCT(BlueprintType)
struct FPubnubChatInviteResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The membership created for the invited user. Null if the operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* Membership = nullptr;
};

/**
 * Result of inviting multiple users to a channel.
 * Returns all membership objects created for the invited users.
 */
USTRUCT(BlueprintType)
struct FPubnubChatInviteMultipleResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of memberships created for all invited users. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatMembership*> Memberships;
};

/**
 * Result of creating a group conversation.
 * Contains the new channel and all created memberships (host + invitees).
 */
USTRUCT(BlueprintType)
struct FPubnubChatCreateGroupConversationResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The newly created group conversation channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatChannel* Channel = nullptr;
	/** The membership of the user who created the group (host). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* HostMembership = nullptr;
	/** Array of memberships for all invited users in the group. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatMembership*> InviteesMemberships;
};

/**
 * Result of creating a direct (1:1) conversation.
 * Contains the new channel and memberships for both participants.
 */
USTRUCT(BlueprintType)
struct FPubnubChatCreateDirectConversationResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The newly created direct conversation channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatChannel* Channel = nullptr;
	/** The membership of the user who initiated the conversation. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* HostMembership = nullptr;
	/** The membership of the other user in the direct conversation. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* InviteeMembership;
};

/**
 * Result of fetching message history from a channel.
 * Contains the messages and pagination flag for loading more.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetHistoryResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of messages from the channel history. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatMessage*> Messages;
	/** True if more messages are available beyond this result set. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMore = false;
};

/**
 * Result of fetching message history from a thread channel.
 * Contains thread messages and pagination flag for loading more.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetThreadHistoryResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of thread messages from the thread history. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatThreadMessage*> ThreadMessages;
	/** True if more thread messages are available beyond this result set. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMore = false;
};

/**
 * Represents a single read receipt indicating when a user last read messages in a channel.
 * Used to track message read status across channel members.
 */
USTRUCT(BlueprintType)
struct FPubnubChatReadReceipt
{
	GENERATED_BODY()
	
	/** The unique identifier of the user who generated this read receipt. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	/** Timetoken of the last message read by this user. Messages before this are considered read. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString LastReadTimetoken = "";
};

/**
 * Result of fetching read receipts for a channel.
 * Contains all users' read positions with pagination support.
 */
USTRUCT(BlueprintType)
struct FPubnubChatFetchReadReceiptsResult
{
	GENERATED_BODY()
	
	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of read receipts showing each user's last read position. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatReadReceipt> ReadReceipts;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of read receipts available (may exceed returned count due to pagination). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Result of retrieving a thread channel for a parent message.
 * Thread channels are used for threaded replies to a specific message.
 */
USTRUCT(BlueprintType)
struct FPubnubChatThreadChannelResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The thread channel object. Null if the operation failed or no thread exists. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatThreadChannel* ThreadChannel = nullptr;
};

/**
 * Result of checking whether a message has a thread.
 * Used to determine if threaded replies exist before attempting to load them.
 */
USTRUCT(BlueprintType)
struct FPubnubChatHasThreadResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the message has a thread with at least one reply. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool HasThread = false;
};
