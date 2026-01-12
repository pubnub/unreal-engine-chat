// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
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
	
	/* FILTERS */
	
	static FString GetFilterForUserID(const FString& UserID);
	static FString GetFilterForMultipleUsersID(const TArray<UPubnubChatUser*>& Users);
	static FString GetFilterForChannelID(const FString& ChannelID);
	static FString GetFilterForChannelsRestrictions();
	
	/* SOFT DELETE */
	
	static FString GetSoftDeletedObjectPropertyKey();
	static FString AddDeletedPropertyToCustom(const FString CurrentCustom);
	static FString RemoveDeletedPropertyFromCustom(const FString CurrentCustom);
	static bool HasDeletedPropertyInCustom(const FString CurrentCustom);

	
	/* PUBLISH MESSAGE */
	
	static FString ChatMessageToPublishString(const FString ChatMessage);
	static FString PublishedStringToChatMessage(const FString PublishedMessage);
	static FString SendTextMetaFromParams(const FPubnubChatSendTextParams& SendTextParams);
	static FString GetForwardedMessageMeta(const FString& OriginalMessageMeta, const FString& UserID, const FString& ChannelID);
	
	
	/* RESTRICTIONS */
	
	static FString GetRestrictionsChannelForChannelID(const FString ChannelID);
	static FString GetModerationEventChannelForUserID(const FString UserID);
	static FString GetChannelIDFromModerationChannel(const FString ModerationChannelID);
	static FString GetChannelMemberCustomForRestriction(const FPubnubChatRestriction& Restriction);
	static FPubnubChatRestriction GetRestrictionFromChannelMemberCustom(const FString& Custom);

	
	/* EVENTS */
	
	static EPubnubChatEventMethod GetDefaultChatEventMethodForEventType(EPubnubChatEventType EventType);
	static FPubnubChatEvent GetEventFromPubnubMessageData(const FPubnubMessageData& MessageData);
	static FPubnubChatEvent GetEventFromPubnubHistoryMessageData(const FPubnubHistoryMessageData& MessageData);
	static FString GetReceiptEventPayload(const FString& Timetoken);
	static FString GetInviteEventPayload(const FString ChannelID, const FString ChannelType);
	static FString GetModerationEventPayload(const FString ModerationChannel, const FString RestrictionType, const FString Reason);
	static bool IsThisEventMessage(const FString& MessageContent);
	static FString GetMentionEventPayload(const FString& ChannelID, const FString& Timetoken, const FString& Text, const FString& ParentChannel = "");
	static FString GetReportMessageEventPayload(const FString& Text, const FString& Reason, const FString& ChannelID, const FString& UserID, const FString& Timetoken);
	
	/* MEMBERSHIP */
	
	static FString GetLastReadMessageTimetokenPropertyKey();
	static void AddLastReadMessageTimetokenToMembershipData(FPubnubChatMembershipData& MembershipData, const FString Timetoken);
	static FString GetLastReadMessageTimetokenFromMembershipData(const FPubnubChatMembershipData& MembershipData);
	
	
	/* CHANNEL */
	
	static bool IsPubnubInternalChannel(const FString& ChannelID);
	static FString GetPinnedMessageTimetokenPropertyKey();
	static FString GetPinnedMessageChannelIDPropertyKey();
	static void AddPinnedMessageToChannelData(FPubnubChatChannelData& ChannelData, UPubnubChatMessage* Message);
	static bool RemovePinnedMessageFromChannelData(FPubnubChatChannelData& ChannelData);
	
	
	/* MESSAGE ACTIONS */
	
	static TArray<FPubnubChatMessageAction> FilterMessageActionsOfType(const TArray<FPubnubChatMessageAction>& MessageActions, const EPubnubChatMessageActionType& MessageActionType);
	static FPubnubChatMessageAction GetMessageReactionForUserID(const TArray<FPubnubChatMessageAction>& MessageReactions, const FString& Reaction, const FString& UserID);
	static bool RemoveReactionFromReactionsArray(TArray<FPubnubChatMessageAction>& MessageReactions, const FPubnubChatMessageAction& Reaction);
	static bool IsChatMessageActionEqualPubnubAction(const FPubnubChatMessageAction& ChatAction, const FPubnubMessageActionData& PubnubAction);
	
	
	/* STREAM UPDATES */
	
	static bool IsPubnubMessageChannelUpdate(const FString& MessageContent);
	static bool IsPubnubMessageUserUpdate(const FString& MessageContent);
	static bool IsPubnubMessageMembershipUpdate(const FString& MessageContent);
	static bool IsPubnubMessageChatMessageUpdate(const FString& MessageContent);
	static bool IsPubnubMessageDeleteEvent(const FString& MessageContent);
	static void UpdateChatChannelFromPubnubChannelUpdateData(const FPubnubChannelUpdateData& PubnubChannelUpdateData, FPubnubChatChannelData& ChannelData);
	static void UpdateChatUserFromPubnubUserUpdateData(const FPubnubUserUpdateData& PubnubUserUpdateData, FPubnubChatUserData& UserData);
	static void UpdateChatMembershipFromPubnubMembershipUpdateData(const FPubnubMembershipUpdateData& PubnubMembershipUpdateData, FPubnubChatMembershipData& MembershipData);
	//Returns true if data was updated
	static bool UpdateChatMessageDataFromPubnubMessage(const FPubnubMessageData& MessageData, const FString& ChatMessageTimetoken, FPubnubChatMessageData& ChatMessageData);
	
	
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

	/* MESSAGE ACTIONS */

	/**
	 * Sorts message actions by timetoken in ascending order (oldest first, most recent last).
	 * Timetokens are compared numerically after conversion to int64.
	 * @param MessageActions Array of message actions to sort (modified in place)
	 */
	static void SortMessageActionsByTimetoken(TArray<FPubnubChatMessageAction>& MessageActions);


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
