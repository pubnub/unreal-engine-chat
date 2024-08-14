#pragma once

#include "CoreMinimal.h"

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
