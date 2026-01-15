// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatChannelStructLibrary.generated.h"

class UPubnubChatChannel;
class UPubnubChatCallbackStop;
class UPubnubChatMembership;
class UPubnubChatThreadChannel;


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

	FPubnubChannelInputData ToPubnubChannelInputData() const;
	static FPubnubChatChannelData FromPubnubChannelData(const FPubnubChannelData &PubnubChannelData);
};

USTRUCT(BlueprintType)
struct FPubnubChatUpdateChannelInputData
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
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetChannelName = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetDescription = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetCustom = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
	bool ForceSetStatus = false;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "Pubnub") 
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


USTRUCT(BlueprintType)
struct FPubnubChatChannelResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatChannel* Channel = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetChannelsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatChannel*> Channels;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubPage Page;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	int Total = 0;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetChannelSuggestionsResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatChannel*> Channels;
};

USTRUCT(BlueprintType)
struct FPubnubChatJoinResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* Membership = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatInviteResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* Membership = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatInviteMultipleResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatMembership*> Memberships;
};

USTRUCT(BlueprintType)
struct FPubnubChatCreateGroupConversationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatChannel* Channel = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* HostMembership = nullptr;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatMembership*> InviteesMemberships;
};

USTRUCT(BlueprintType)
struct FPubnubChatCreateDirectConversationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatChannel* Channel = nullptr;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* HostMembership = nullptr;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatMembership* InviteeMembership;
};

USTRUCT(BlueprintType)
struct FPubnubChatGetHistoryResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<UPubnubChatMessage*> Messages;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool IsMore = false;
};

USTRUCT(BlueprintType)
struct FPubnubChatUserIDsArray
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TArray<FString> UserIDs;
	
};

USTRUCT(BlueprintType)
struct FPubnubChatReadReceipts
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	TMap<FString, FPubnubChatUserIDsArray> ReadReceipts;
	
};

USTRUCT(BlueprintType)
struct FPubnubChatThreadChannelResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	UPubnubChatThreadChannel* ThreadChannel = nullptr;
};

USTRUCT(BlueprintType)
struct FPubnubChatHasThreadResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	FPubnubChatOperationResult Result;
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat")
	bool HasThread = false;
};