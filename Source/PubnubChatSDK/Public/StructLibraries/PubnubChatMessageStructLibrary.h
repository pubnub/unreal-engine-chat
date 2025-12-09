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

	FPubnubMessageActionData ToPubnubMessageActionData();
	static FPubnubChatMessageAction FromPubnubMessageActionData(const FPubnubMessageActionData& PubnubMessageActionData);
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

	FPubnubMessageData ToPubnubMessageData();
	static FPubnubChatMessageData FromPubnubMessageData(const FPubnubMessageData& PubnubMessageData);
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

