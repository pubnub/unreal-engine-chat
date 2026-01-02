// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PubnubChatEnumLibrary.generated.h"


UENUM(BlueprintType)
enum class EPubnubChatConnectionStatus : uint8
{
	PCCS_CONNECTION_ONLINE		UMETA(DisplayName="ConnectionOnline"),
	PCCS_CONNECTION_OFFLINE		UMETA(DisplayName="ConnectionOffline"),
	PCCS_CONNECTION_ERROR		UMETA(DisplayName="ConnectionError")
};

UENUM(BlueprintType)
enum class EPubnubChatMessageActionType : uint8
{
	PCMAT_Reaction				UMETA(DisplayName="Reaction"),
	PCMAT_Receipt				UMETA(DisplayName="Custom"),
	PCMAT_Custom				UMETA(DisplayName="Reaction"),
	PCMAT_Edited				UMETA(DisplayName="Custom"),
	PCMAT_Deleted				UMETA(DisplayName="Reaction")
};

UENUM(BlueprintType)
enum class EPubnubChatEventType : uint8
{
	PCET_Typing					UMETA(DisplayName="Typing"),
	PCET_Report					UMETA(DisplayName="Report"),
	PCET_Receipt				UMETA(DisplayName="Receipt"),
	PCET_Mention				UMETA(DisplayName="Mention"),
	PCET_Invite					UMETA(DisplayName="Invite"),
	PCET_Custom					UMETA(DisplayName="Custom"),
	PCET_Moderation				UMETA(DisplayName="Moderation"),
	
	Count
};
ENUM_RANGE_BY_COUNT(EPubnubChatEventType, EPubnubChatEventType::Count);

UENUM(BlueprintType)
enum class EPubnubChatEventMethod : uint8
{
	PCEM_Default				UMETA(DisplayName="Default"),
	PCEM_Publish				UMETA(DisplayName="Publish"),
	PCEM_Signal					UMETA(DisplayName="Signal")
};

UENUM(BlueprintType)
enum class EPubnubChatAccessManagerPermission : uint8
{
	PCAMP_Read					UMETA(DisplayName="Read"),
	PCAMP_Write					UMETA(DisplayName="Write"),
	PCAMP_Manage				UMETA(DisplayName="Manage"),
	PCAMP_Delete				UMETA(DisplayName="Delete"),
	PCAMP_Get					UMETA(DisplayName="Get"),
	PCAMP_Join					UMETA(DisplayName="Join"),
	PCAMP_Update				UMETA(DisplayName="Update"),
};

UENUM(BlueprintType)
enum class EPubnubChatAccessManagerResourceType : uint8
{
	PCAMRT_Users			UMETA(DisplayName="Uuids"),
	PCAMRT_Channels			UMETA(DisplayName="Channels"),
};