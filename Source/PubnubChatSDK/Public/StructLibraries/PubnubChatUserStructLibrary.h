// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatUserStructLibrary.generated.h"

class UPubnubChatUser;


USTRUCT(BlueprintType)
struct FPubnubChatUserData
{
	GENERATED_BODY()
	
	//Display name for the user.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString UserName = "";
	//User's identifier in an external system.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString ExternalID = "";
	//The URL of the user's profile picture.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString ProfileUrl = "";
	//The user's email address.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Email = "";
	//JSON object providing custom user data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Custom = "";
	//User status. Max. 50 characters.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Status = "";
	//User type. Max. 50 characters.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Type = "";

	FPubnubUserInputData ToPubnubUserInputData() const;
	static FPubnubChatUserData FromPubnubUserData(const FPubnubUserData &PubnubUserData);
};

USTRUCT(BlueprintType)
struct FPubnubChatUserResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatUser* User = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetUsersResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatUser*> Users;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetUserSuggestionsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatUser*> Users;
};