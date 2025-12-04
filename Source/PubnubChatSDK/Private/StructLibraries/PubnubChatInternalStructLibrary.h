// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatInternalStructLibrary.generated.h"

/**
 * Internal Pubnub Chat User structure. Do not use this directly.
 * Contains all shared data for a user, including data and future variables like timers.
 */
USTRUCT()
struct FPubnubChatInternalUser
{
	GENERATED_BODY()

	/** User's data (name, email, etc.) */
	UPROPERTY()
	FPubnubChatUserData UserData;

	/** User's unique identifier */
	UPROPERTY()
	FString UserID = "";

	/** Timestamp of last update */
	UPROPERTY()
	FDateTime LastUpdated;
	
	
	FPubnubChatInternalUser()
		: LastUpdated(FDateTime::Now())
	{
	}
};

/**
 * Internal Pubnub Chat Channel structure. Do not use this directly.
 * Contains all shared data for a channel, including data and future variables like timers.
 */
USTRUCT()
struct FPubnubChatInternalChannel
{
	GENERATED_BODY()

	/** Channel's data (name, description, etc.) */
	UPROPERTY()
	FPubnubChatChannelData ChannelData;

	/** Channel's unique identifier */
	UPROPERTY()
	FString ChannelID = "";

	/** Timestamp of last update */
	UPROPERTY()
	FDateTime LastUpdated;
	
	
	FPubnubChatInternalChannel()
		: LastUpdated(FDateTime::Now())
	{
	}
};