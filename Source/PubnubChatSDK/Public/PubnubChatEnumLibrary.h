// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/EnumRange.h"
#include "PubnubChatEnumLibrary.generated.h"


UENUM(BlueprintType)
enum class EPubnubChatConnectionStatus : uint8
{
	PCCS_ConnectionOnline		UMETA(DisplayName="ConnectionOnline"),
	PCCS_ConnectionOffline		UMETA(DisplayName="ConnectionOffline"),
	PCCS_ConnectionError		UMETA(DisplayName="ConnectionError")
};

UENUM(BlueprintType)
enum class EPubnubChatMessageActionType : uint8
{
	PCMAT_Reaction				UMETA(DisplayName="Reaction"),
	PCMAT_Receipt				UMETA(DisplayName="Receipt"),
	PCMAT_Custom				UMETA(DisplayName="Custom"),
	PCMAT_Edited				UMETA(DisplayName="Edited"),
	PCMAT_Deleted				UMETA(DisplayName="Deleted"),
	PCMAT_ThreadRootId			UMETA(DisplayName="ThreadRootId")
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

UENUM(BlueprintType)
enum class EPubnubChatStreamedUpdateType : uint8
{
	PCSUT_Updated			UMETA(DisplayName="Updated"),
	PCSUT_Deleted			UMETA(DisplayName="Deleted"),
};

UENUM(BlueprintType)
enum class EPubnubChatMessageDraftSuggestionSource : uint8
{
	PCMDSS_Channel			UMETA(DisplayName="Channel"),
	PCMDSS_Global			UMETA(DisplayName="Global"),
};

UENUM(BlueprintType)
enum class EPubnubChatMentionTargetType : uint8
{
	PCMTT_None				UMETA(DisplayName="None"),
	PCMTT_User				UMETA(DisplayName="User"),
	PCMTT_Channel			UMETA(DisplayName="Channel"),
	PCMTT_Url				UMETA(DisplayName="Url")
};