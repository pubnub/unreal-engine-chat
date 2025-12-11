// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
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

/**
 * Internal Pubnub Chat Message structure. Do not use this directly.
 * Contains all shared data for a message, including data and future variables like timers.
 */
USTRUCT()
struct FPubnubChatInternalMessage
{
	GENERATED_BODY()

	/** Message's data (text, type, etc.) */
	UPROPERTY()
	FPubnubChatMessageData MessageData;

	/** Message's unique identifier */
	UPROPERTY()
	FString MessageID = "";

	/** Timestamp of last update */
	UPROPERTY()
	FDateTime LastUpdated;
	
	
	FPubnubChatInternalMessage()
		: LastUpdated(FDateTime::Now())
	{
	}
};

/**
 * Internal Pubnub Chat Membership structure. Do not use this directly.
 * Contains all shared data for a membership, including data and future variables like timers.
 */
USTRUCT()
struct FPubnubChatInternalMembership
{
	GENERATED_BODY()

	/** Membership's data (custom data, status, type, etc.) */
	UPROPERTY()
	FPubnubChatMembershipData MembershipData;

	/** Membership's unique identifier (format: "[UserID].[ChannelID]") */
	UPROPERTY()
	FString MembershipID = "";

	/** Timestamp of last update */
	UPROPERTY()
	FDateTime LastUpdated;
	
	
	FPubnubChatInternalMembership()
		: LastUpdated(FDateTime::Now())
	{
	}
};