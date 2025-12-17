// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
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
	static FString AddDeletedPropertyToCustom(FString CurrentCustom);

	UFUNCTION()
	static FString RemoveDeletedPropertyFromCustom(FString CurrentCustom);

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
	
	UFUNCTION()
	static FString GetLastReadMessageTimetokenPropertyKey();

	static void AddLastReadMessageTimetokenToMembershipData(const FPubnubChatMembershipData& MembershipData, const FString Timetoken);

	static FString GetFilterForMultipleUsersID(const TArray<UPubnubChatUser*>& Users);

	/* ACCESS MANAGER */

	/**
	 * Checks if a permission exists and is true for a given resource in Resources (exact match).
	 */
	static bool CheckResourcePermission(const TSharedPtr<FJsonObject>& ResourcesObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr);

	/**
	 * Checks if a permission exists and is true for a given resource in Patterns (regex match).
	 */
	static bool CheckPatternPermission(const TSharedPtr<FJsonObject>& PatternsObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr);


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
