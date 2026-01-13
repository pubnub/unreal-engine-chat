// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatStructLibrary.generated.h"

class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;
class UPubnubChatChannel;
class UPubnubChatMembership;
class UPubnubChatCallbackStop;


/** Structure to store typing indicator data (timer handle and timestamp) */
struct FTypingIndicatorData
{
	FTimerHandle TimerHandle;
	FDateTime LastTypingTime;
	
	FTypingIndicatorData() : LastTypingTime(FDateTime::MinValue()) {}
	FTypingIndicatorData(const FTimerHandle& InTimerHandle, const FDateTime& InLastTypingTime)
		: TimerHandle(InTimerHandle), LastTypingTime(InLastTypingTime) {}
};


USTRUCT(BlueprintType)
struct FPubnubChatConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") FString AuthKey = "";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int TypingTimeout = 5000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int TypingTimeoutDifference = 1000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") int StoreUserActivityInterval = 600000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PubnubChat|Config") bool StoreUserActivityTimestamps = false;
	
	void ValidateConfig();
};

USTRUCT(BlueprintType)
struct FPubnubChatConnectionStatusData
{
	GENERATED_BODY()
	
	/**Error details in case of ConnectionError.*/
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Reason = "";
};

/**
 * Represents the result of a single operation step within a multi-step Chat operation.
 * Used to track which specific Pubnub SDK function failed during a Chat operation.
 */
USTRUCT(BlueprintType)
struct FPubnubChatOperationStepResult
{
	GENERATED_BODY()

	/** Name/description of the operation step (e.g., "Subscribe", "SetMemberships") */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString StepName = "";

	/** The result from the underlying Pubnub SDK operation */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubOperationResult OperationResult;
};

/**
 * Common result struct for Chat SDK operations that may involve multiple Pubnub SDK calls.
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

	/** Overall check whether the operation succeeded (all steps must succeed) */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool Error = false;

	/** Aggregated error message describing which step(s) failed and why */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString ErrorMessage = "";

	/** Array of results from each individual operation step */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatOperationStepResult> StepResults;

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

USTRUCT(BlueprintType)
struct FPubnubChatInitChatResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChat* Chat = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatSendTextParams
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool StoreInHistory = true;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool SendByPost = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Meta = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMessage* QuotedMessage = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatMembershipData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Custom = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Status = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Type = "";

	FPubnubMembershipInputData ToPubnubMembershipInputData(const FString ChannelID) const;
	FPubnubChannelMemberInputData ToPubnubChannelMemberInputData(const FString UserID) const;

	static FPubnubChatMembershipData FromPubnubMembershipData(const FPubnubMembershipData& PubnubMembershipData);
	static FPubnubChatMembershipData FromPubnubChannelMemberData(const FPubnubChannelMemberData& PubnubChannelMemberData);
};

USTRUCT(BlueprintType)
struct FPubnubChatUpdateMembershipInputData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Custom = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Status = "";

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Type = "";
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetCustom = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetStatus = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
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

USTRUCT(BlueprintType)
struct FPubnubChatListenForEventsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatCallbackStop* CallbackStop = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Timetoken = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") EPubnubChatEventType Type = EPubnubChatEventType::PCET_Custom;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Payload = "";
};

USTRUCT(BlueprintType)
struct FPubnubChatEventsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatEvent> Events;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool IsMore = false;
};

USTRUCT(BlueprintType)
struct FPubnubChatWherePresentResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FString> Channels;
};

USTRUCT(BlueprintType)
struct FPubnubChatWhoIsPresentResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FString> Users;
};

USTRUCT(BlueprintType)
struct FPubnubChatIsPresentResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool IsPresent = false;
};

USTRUCT(BlueprintType)
struct FPubnubChatIsDeletedResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool IsDeleted = false;
};

USTRUCT(BlueprintType)
struct FPubnubChatMembershipsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatMembership*> Memberships;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatRestriction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString UserID = "";
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString ChannelID = "";
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool Ban = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool Mute = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FString Reason = "";
};

USTRUCT(BlueprintType)
struct FPubnubChatGetRestrictionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatRestriction Restriction;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetRestrictionsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatRestriction> Restrictions;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatUnreadMessagesCountsWrapper
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatChannel* Channel;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* Membership;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Count = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetUnreadMessagesCountsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FPubnubChatUnreadMessagesCountsWrapper> UnreadMessagesCounts;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatMarkAllMessagesAsReadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatMembership*> Memberships;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};