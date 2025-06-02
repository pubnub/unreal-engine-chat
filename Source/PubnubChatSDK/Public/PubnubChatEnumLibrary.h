// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatEnumLibrary.generated.h"

UENUM(BlueprintType)
enum class EPubnubMessageActionType : uint8
{
	PMAT_Reaction			 UMETA(DisplayName="Reaction"),
	PMAT_Receipt			 UMETA(DisplayName="Receipt"),
	PMAT_Custom				 UMETA(DisplayName="Custom"),
	PMAT_Edited				 UMETA(DisplayName="Edited"),
	PMAT_Deleted			 UMETA(DisplayName="Deleted"),
	PMAT_ThreadRootId		 UMETA(DisplayName="ThreadRootId")
};

UENUM(BlueprintType)
enum class EPubnubChatEventType : uint8
{
	PCET_TYPING				 UMETA(DisplayName="Typing"),
	PCET_REPORT				 UMETA(DisplayName="Report"),
	PCET_RECEPIT			 UMETA(DisplayName="Receipt"),
	PCET_MENTION			 UMETA(DisplayName="Mention"),
	PCET_INVITE				 UMETA(DisplayName="Invite"),
	PCET_CUSTOM				 UMETA(DisplayName="Custom"),
	PCET_MODERATION			 UMETA(DisplayName="Moderation")
};

UENUM(BlueprintType)
enum class EPubnubChatMessageType : uint8
{
	PCMT_TEXT				UMETA(DisplayName="Text")
};

UENUM(BlueprintType)
enum class EPubnubAccessManagerResourceType : uint8
{
	PAMRT_UUIDS				UMETA(DisplayName="Uuids"),
	PAMRT_CHANNELS			UMETA(DisplayName="Channels"),
};

UENUM(BlueprintType)
enum class EPubnubAccessManagerPermission : uint8
{
	PAMP_READ				UMETA(DisplayName="Read"),
	PAMP_WRITE				UMETA(DisplayName="Write"),
	PAMP_MANAGE				UMETA(DisplayName="Manage"),
	PAMP_DELETE				UMETA(DisplayName="Delete"),
	PAMP_GET				UMETA(DisplayName="Get"),
	PAMP_JOIN				UMETA(DisplayName="Join"),
	PAMP_UPDATE				UMETA(DisplayName="Update"),
};

UENUM(BlueprintType)
enum class EPubnubMentionTargetType : uint8
{
	PMTT_User				UMETA(DisplayName="User"),
	PMTT_Channel			UMETA(DisplayName="Channel"),
	PMTT_Url				UMETA(DisplayName="Url")
};

UENUM(BlueprintType)
enum class EPubnubMessageDraftSuggestionSource : uint8
{
	PMDSS_Channel			UMETA(DisplayName="Channel"),
	PMDSS_Global			UMETA(DisplayName="Global"),
};

UENUM(BlueprintType)
enum class EPubnubConnectionStatus : uint8
{
	PCS_CONNECTION_ONLINE		UMETA(DisplayName="ConnectionOnline"),
	PCS_CONNECTION_OFFLINE		UMETA(DisplayName="ConnectionOffline"),
	PCS_CONNECTION_ERROR		UMETA(DisplayName="ConnectionError")
};