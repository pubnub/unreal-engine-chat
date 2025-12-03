// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatStructLibrary.generated.h"

class UPubnubChat;
class UPubnubChatUser;



USTRUCT(BlueprintType)
struct FPubnubStringArrayWrapper
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FString> Strings;
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
 */
USTRUCT(BlueprintType)
struct FPubnubChatOperationResult
{
	GENERATED_BODY()

	/** Overall status code. 200 if all steps succeeded, otherwise the status of the first failed step */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Status = 0;

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
	 * Sets Status to 200 and Error to false, without affecting steps
	 */
	FPubnubChatOperationResult& MarkSuccess();

	/**
	 * Creates an error result
	 */
	static FPubnubChatOperationResult CreateError(int InStatus, FString InErrorMessage = "");

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
