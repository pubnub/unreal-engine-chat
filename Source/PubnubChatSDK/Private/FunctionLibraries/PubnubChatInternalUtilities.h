// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatInternalUtilities.generated.h"

class FJsonObject;


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatInternalUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	

	/* SOFT DELETE */
	
	UFUNCTION()
	static FString GetSoftDeletedObjectPropertyKey();

	UFUNCTION()
	static FString AddDeletedPropertyToCustom(const FString CurrentCustom);
	
	UFUNCTION()
	static FString RemoveDeletedPropertyFromCustom(const FString CurrentCustom);
	
	UFUNCTION()
	static bool HasDeletedPropertyInCustom(const FString CurrentCustom);

	/* PUBLISH MESSAGE */

	UFUNCTION()
	static FString ChatMessageToPublishString(const FString ChatMessage);

	UFUNCTION()
	static FString PublishedStringToChatMessage(const FString PublishedMessage);

	UFUNCTION()
	static FString SendTextMetaFromParams(const FPubnubChatSendTextParams& SendTextParams);

	
	/* EVENTS */
	
	UFUNCTION()
	static EPubnubChatEventMethod GetDefaultChatEventMethodForEventType(EPubnubChatEventType EventType);

	UFUNCTION()
	static FPubnubChatEvent GetEventFromPubnubMessageData(const FPubnubMessageData& MessageData);

	static FString GetReceiptEventPayload(const FString& Timetoken);

	static FString GetInviteEventPayload(const FString ChannelID, const FString ChannelType);
	
	
	/* MEMBERSHIP */
	
	static FString GetLastReadMessageTimetokenPropertyKey();

	static void AddLastReadMessageTimetokenToMembershipData(FPubnubChatMembershipData& MembershipData, const FString Timetoken);

	static FString GetFilterForMultipleUsersID(const TArray<UPubnubChatUser*>& Users);

	
	/* CHANNEL */
	
	static FString GetPinnedMessageTimetokenPropertyKey();
	
	static FString GetPinnedMessageChannelIDPropertyKey();
	
	//Message should be validated before using this function
	static void AddPinnedMessageToChannelData(FPubnubChatChannelData& ChannelData, UPubnubChatMessage* Message);
	
	static bool RemovePinnedMessageFromChannelData(FPubnubChatChannelData& ChannelData);
	
	/* ACCESS MANAGER */

	/**
	 * Checks if a permission exists and is true for a given resource in Resources (exact match).
	 */
	static bool CheckResourcePermission(const TSharedPtr<FJsonObject>& ResourcesObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr);

	/**
	 * Checks if a permission exists and is true for a given resource in Patterns (regex match).
	 */
	static bool CheckPatternPermission(const TSharedPtr<FJsonObject>& PatternsObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr);

	/* HASHING */

	/**
	 * Hashes a string using the cyrb53a algorithm.
	 * @param Str The string to hash
	 * @param Seed Optional seed value (defaults to 0)
	 * @return 64-bit unsigned hash value
	 */
	static uint64 HashString(const FString& Str, int32 Seed = 0);


	/* TEMPLATES */
	
	template<typename ObjectType>
	static TArray<ObjectType> RemoveInvalidObjects(const TArray<ObjectType>& ObjectsArray)
	{
		TArray<ObjectType> ObjectsArrayCopy = ObjectsArray;
		for (int i = ObjectsArray.Num() - 1; i >= 0; i--)
		{
			if(!ObjectsArray[i])
			{
				ObjectsArrayCopy.RemoveAt(i);
			}
		}

		return ObjectsArrayCopy;
	}
};
