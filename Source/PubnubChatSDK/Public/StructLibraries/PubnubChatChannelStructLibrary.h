// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatChannelStructLibrary.generated.h"

class UPubnubChatChannel;


USTRUCT(BlueprintType)
struct FPubnubChatChannelData
{
	GENERATED_BODY()
	
	//Display name for the user.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString ChannelName = "";
	//User's identifier in an external system.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Description = "";
	//JSON object providing custom user data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Custom = "";
	//User status. Max. 50 characters.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Status = "";
	//User type. Max. 50 characters.
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Pubnub") FString Type = "";

	FPubnubChannelData ToPubnubChannelData();
	static FPubnubChatChannelData FromPubnubChannelData(const FPubnubChannelData &PubnubChannelData);
};