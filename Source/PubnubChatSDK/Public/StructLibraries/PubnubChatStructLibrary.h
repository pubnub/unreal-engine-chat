// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatStructLibrary.generated.h"

class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;
class UPubnubChatChannel;
class UPubnubChatMembership;
class UPubnubChatCallbackStop;


/**
 * Internal structure to track typing indicator state for a user.
 * Stores the timer handle and last typing timestamp for debouncing.
 */
struct FTypingIndicatorData
{
	/** Timer handle for the typing indicator timeout. */
	FTimerHandle TimerHandle;
	/** Timestamp of when the last typing signal was received. */
	FDateTime LastTypingTime;
	
	FTypingIndicatorData() : LastTypingTime(FDateTime::MinValue()) {}
	FTypingIndicatorData(const FTimerHandle& InTimerHandle, const FDateTime& InLastTypingTime)
		: TimerHandle(InTimerHandle), LastTypingTime(InLastTypingTime) {}
};

/**
 * Rate limiter configuration for controlling message send frequency.
 * Prevents spam and ensures fair usage across different channel types.
 */
USTRUCT(BlueprintType)
struct FPubnubChatRateLimiterConfig
{
	GENERATED_BODY()

	/** Rate limits per channel type (channel type -> milliseconds between sends). Empty map = no rate limiting. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config|RateLimiter")
	TMap<FString, int> RateLimitPerChannel;

	/** Exponential factor for rate limit backoff. Each rate limit hit multiplies delay by this factor. Should be between 1.0 and 10.0. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config|RateLimiter")
	float RateLimitFactor = 1.2f;
};

/**
 * Main configuration structure for initializing PubNub Chat.
 * Controls typing indicators, user activity tracking, rate limiting, and read receipts.
 */
USTRUCT(BlueprintType)
struct FPubnubChatConfig
{
	GENERATED_BODY()

	/** Authentication key for access management. Leave empty if not using token-based auth. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") FString AuthKey = "";
	/** Timeout in milliseconds before a typing indicator expires. Default: 5000ms. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int TypingTimeout = 5000;
	/** Minimum time difference in milliseconds between typing signal updates. Default: 1000ms. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int TypingTimeoutDifference = 1000;
	/** Interval in milliseconds for storing user activity timestamps. Default: 600000ms (10 minutes). */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int StoreUserActivityInterval = 600000;
	/** When true, automatically stores user activity timestamps on server. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") bool StoreUserActivityTimestamps = false;
	/** Rate limiter configuration for controlling message frequency. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") FPubnubChatRateLimiterConfig RateLimiter;
	/** Per-channel-type toggle for read receipt events. Keys: "public", "group", "direct". */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") TMap<FString, bool> EmitReadReceiptEvents;

	/** Default: public=false, group=true, direct=true for read receipt events. */
	FPubnubChatConfig()
	{
		EmitReadReceiptEvents.Add(TEXT("public"), false);
		EmitReadReceiptEvents.Add(TEXT("group"), true);
		EmitReadReceiptEvents.Add(TEXT("direct"), true);
	}

	void ValidateConfig();
};

/**
 * Configuration options for creating a message draft.
 * Controls mention suggestions, typing indicators, and suggestion limits.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMessageDraftConfig
{
	GENERATED_BODY()

	/** Source for user mention suggestions (global users or channel members only). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") EPubnubChatMessageDraftSuggestionSource UserSuggestionSource = EPubnubChatMessageDraftSuggestionSource::PCMDSS_Global;
	/** When true, typing indicators are sent while editing this draft. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsTypingIndicatorTriggered = false;
	/** Maximum number of user suggestions to return. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int UserLimit = 10;
	/** Maximum number of channel suggestions to return. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int ChannelLimit = 10;
};


/**
 * Connection status data containing error details when connection fails.
 */
USTRUCT(BlueprintType)
struct FPubnubChatConnectionStatusData
{
	GENERATED_BODY()
	
	/** Error details in case of connection failure. Empty on successful connection. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Reason = "";
};

/**
 * Represents the result of a single operation step within a multi-step Chat operation.
 * Used to track which specific PubNub SDK function failed during a Chat operation.
 */
USTRUCT(BlueprintType)
struct FPubnubChatOperationStepResult
{
	GENERATED_BODY()

	/** Name/description of the operation step (e.g., "Subscribe", "SetMemberships"). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString StepName = "";
	/** The result from the underlying PubNub SDK operation. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubOperationResult OperationResult;
};

/**
 * Common result struct for Chat SDK operations that may involve multiple PubNub SDK calls.
 * Aggregates results from all steps and provides comprehensive error information.
 * 
 * Example usage:
 * - Join channel operation calls Subscribe and SetMemberships
 * - If Subscribe fails, StepResults will contain the error details
 * - Overall Error will be true and ErrorMessage will describe which step failed
 * 
 * Note: For detailed HTTP status codes from server operations, check StepResults[].OperationResult.Status
 */
USTRUCT(BlueprintType)
struct FPubnubChatOperationResult
{
	GENERATED_BODY()

	/** Overall check whether the operation succeeded (all steps must succeed). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool Error = false;
	/** Aggregated error message describing which step(s) failed and why. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ErrorMessage = "";
	/** Array of results from each individual operation step. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatOperationStepResult> StepResults;

	/**
	 * Sets Error to false, without affecting steps
	 */
	FPubnubChatOperationResult& MarkSuccess();

	/**
	 * Creates an error result
	 */
	static FPubnubChatOperationResult CreateError(FString InErrorMessage = "");

	/**
	 * Creates a result from a single Pubnub operation result
	 * @param StepName Name of the operation step
	 * @param OperationResult Result from the Pubnub SDK operation
	 */
	static FPubnubChatOperationResult FromSingleStep(const FString& StepName, const FPubnubOperationResult& OperationResult);

	/**
	 * Merges a new step result into this result.
	 * If the step failed, the overall result becomes an error.
	 * @param StepName Name of the operation step
	 * @param OperationResult Result from the Pubnub SDK operation
	 * @return Reference to this result for chaining
	 */
	FPubnubChatOperationResult& AddStep(const FString& StepName, const FPubnubOperationResult& OperationResult);
	
	/**
	 * Merges another Chat operation result into this one.
	 * Useful when combining results from parallel operations or sub-operations.
	 * @param OtherResult Another Chat operation result to merge
	 * @return Reference to this result for chaining
	 */
	FPubnubChatOperationResult& Merge(const FPubnubChatOperationResult& OtherResult);
};

/**
 * Result of initializing the PubNub Chat SDK.
 * Contains the main Chat object used for all subsequent operations.
 */
USTRUCT(BlueprintType)
struct FPubnubChatInitChatResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The initialized Chat object. Null if initialization failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChat* Chat = nullptr;
};

/**
 * Parameters for sending text messages.
 * Controls message persistence and delivery options.
 */
USTRUCT(BlueprintType)
struct FPubnubChatSendTextParams
{
	GENERATED_BODY()

	/** When true, message is stored in channel history for later retrieval. Default: true. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool StoreInHistory = true;
	/** When true, uses POST method for larger payloads. Default: false (uses GET). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool SendByPost = false;
	/** JSON string with additional metadata to attach to the message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Meta = "";
};

/**
 * Membership data structure containing relationship metadata between a user and a channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMembershipData
{
	GENERATED_BODY()

	/** JSON string with custom membership data (e.g., role, permissions, preferences). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** Membership status. Max. 50 characters. Examples: "active", "invited", "banned". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** Membership type. Max. 50 characters. Examples: "admin", "member", "guest". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";

	FPubnubMembershipInputData ToPubnubMembershipInputData(const FString ChannelID) const;
	FPubnubChannelMemberInputData ToPubnubChannelMemberInputData(const FString UserID) const;

	static FPubnubChatMembershipData FromPubnubMembershipData(const FPubnubMembershipData& PubnubMembershipData);
	static FPubnubChatMembershipData FromPubnubChannelMemberData(const FPubnubChannelMemberData& PubnubChannelMemberData);
};

/**
 * Membership data for partial updates with ForceSet flags.
 * Use ForceSet flags to explicitly set fields to empty values when needed.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUpdateMembershipInputData
{
	GENERATED_BODY()

	/** JSON string with custom membership data (e.g., role, permissions, preferences). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** Membership status. Max. 50 characters. Examples: "active", "invited", "banned". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** Membership type. Max. 50 characters. Examples: "admin", "member", "guest". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";
	
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
	 * Sets all ForceSet flags to true. Useful for full replacement of Membership Data.
	 */
	void ForceSetAllFields()
	{
		ForceSetCustom = true;
		ForceSetStatus = true;
		ForceSetType = true;
	}
	
	FPubnubMembershipInputData ToPubnubMembershipInputData(const FString ChannelID) const;
	FPubnubChannelMemberInputData ToPubnubChannelMemberInputData(const FString UserID) const;
	
	static FPubnubChatUpdateMembershipInputData FromChatMembershipData(const FPubnubChatMembershipData &PubnubMembershipData);
};

/**
 * Result of starting an event listener.
 * Contains the callback stop handle for unsubscribing from events.
 */
USTRUCT(BlueprintType)
struct FPubnubChatListenForEventsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Handle to stop receiving events. Call Stop() to unsubscribe. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatCallbackStop* CallbackStop = nullptr;
};

/**
 * Generic chat event structure for all event types.
 * Contains common event data applicable to all PubNub Chat events.
 */
USTRUCT(BlueprintType)
struct FPubnubChatEvent
{
	GENERATED_BODY()

	/** Timetoken when the event occurred. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	/** Type of the event (typing, receipt, mention, invite, custom, etc.). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") EPubnubChatEventType Type = EPubnubChatEventType::PCET_Custom;
	/** ID of the channel where the event occurred. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	/** User ID of the user who triggered the event. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	/** JSON payload with event-specific data. Parse based on event Type. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Payload = "";
};


/**
 * Report event data when a user reports a message or another user.
 * Used for content moderation workflows.
 */
USTRUCT(BlueprintType)
struct FPubnubChatReportEvent
{
	GENERATED_BODY()

	/** Description or reason text for the report. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Text = "";
	/** Timetoken of the reported message (if reporting a message). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString MessageTimetoken = "";
	/** Category or reason code for the report. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Reason = "";
	/** ID of the channel containing the reported message. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReportedMessageChannelID = "";
	/** User ID of the reported user. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ReportedUserID = "";
};

/**
 * Invite event data when a user is invited to join a channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatInviteEvent
{
	GENERATED_BODY()
	
	/** Timetoken when the invite was sent. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	/** ID of the channel the user is invited to. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	/** Type of channel (public, group, direct). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelType = "";
	/** User ID of the user who sent the invitation. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString InvitedByUserID = "";
};

/**
 * Custom event data for application-specific events.
 * Allows sending arbitrary payloads between users.
 */
USTRUCT(BlueprintType)
struct FPubnubChatCustomEvent
{
	GENERATED_BODY()
	
	/** Timetoken when the custom event was sent. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	/** User ID of the user who sent the event. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	/** JSON payload with custom event data. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Payload = "";
	/** Custom event type identifier for routing/handling. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";
};

/**
 * Result of fetching historical events from a channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatEventsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of events retrieved from history. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatEvent> Events;
	/** True if more events are available beyond this result set. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMore = false;
};

/**
 * Result of querying which channels a user is currently present in.
 */
USTRUCT(BlueprintType)
struct FPubnubChatWherePresentResult
{
	GENERATED_BODY()
	
	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of channel IDs where the user is currently present. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FString> Channels;
};

/**
 * Result of querying which users are currently present in a channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatWhoIsPresentResult
{
	GENERATED_BODY()
	
	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of user IDs currently present in the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FString> Users;
};

/**
 * Result of checking if a specific user is present in a specific channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatIsPresentResult
{
	GENERATED_BODY()
	
	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the user is currently present in the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsPresent = false;
};

/**
 * Result of checking if a message or entity has been deleted.
 */
USTRUCT(BlueprintType)
struct FPubnubChatIsDeletedResult
{
	GENERATED_BODY()
	
	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the message/entity has been soft-deleted. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsDeleted = false;
};

/**
 * Result of retrieving a single membership.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMembershipResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The membership object. Null if not found or operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* Membership = nullptr;
};


/**
 * Result of a paginated membership list query.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMembershipsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of membership objects. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatMembership*> Memberships;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of memberships matching the query. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Result of checking if a channel has a specific user as a member.
 */
USTRUCT(BlueprintType)
struct FPubnubChatHasMemberResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the user is a member of the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool HasMember = false;
};

/**
 * Result of checking if the current user is a member of a specific channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatIsMemberOnResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** True if the current user is a member of the specified channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMemberOn = false;
};

/**
 * Restriction data for a user on a specific channel (ban/mute status).
 * Used for moderation and access control.
 */
USTRUCT(BlueprintType)
struct FPubnubChatRestriction
{
	GENERATED_BODY()

	/** User ID of the restricted user. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	/** Channel ID where the restriction applies. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	/** True if the user is banned from the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool Ban = false;
	/** True if the user is muted in the channel (can read but not send). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool Mute = false;
	/** Optional reason for the restriction. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Reason = "";
};

/**
 * Result of retrieving a single restriction entry.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetRestrictionResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The restriction data for the user/channel combination. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatRestriction Restriction;
};

/**
 * Result of a paginated restrictions list query.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetRestrictionsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of restriction entries. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatRestriction> Restrictions;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of restrictions matching the query. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Wrapper containing unread message count for a specific channel/membership.
 * Used in batch unread count queries.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUnreadMessagesCountsWrapper
{
	GENERATED_BODY()
	
	/** The channel with unread messages. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatChannel* Channel = nullptr;
	/** The user's membership for this channel (contains last read position). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatMembership* Membership = nullptr;
	/** Number of unread messages in the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Count = 0;
};

/**
 * Result of fetching unread message counts across multiple channels.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetUnreadMessagesCountsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of unread count wrappers for each channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubChatUnreadMessagesCountsWrapper> UnreadMessagesCounts;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of channels with unread messages. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Result of fetching unread message count for a single channel.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetUnreadMessagesCountResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Number of unread messages in the channel. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Count = 0;
};


/**
 * Result of marking all messages as read across multiple channels.
 * Returns updated memberships with new last-read positions.
 */
USTRUCT(BlueprintType)
struct FPubnubChatMarkAllMessagesAsReadResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of updated memberships with new last-read timetokens. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatMembership*> Memberships;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of channels where messages were marked as read. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};
