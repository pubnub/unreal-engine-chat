// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HAL/CriticalSection.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatInternalStructLibrary.h"
#include "PubnubChatObjectsRepository.generated.h"

/**
 * Repository class that manages shared data for all Chat objects.
 * All User and Channel objects with the same ID reference the same data in this repository.
 * This ensures data synchronization across all instances.
 * 
 * Objects must register themselves when created and unregister when destroyed.
 * When the reference count for an object ID reaches 0, the data is automatically cleaned up.
 * 
 * This is an internal class and should not be used directly.
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatObjectsRepository : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;

public:
	/**
	 * Registers a User object. Call this when a User object is created.
	 * Increments the reference count for this UserID.
	 * @param UserID The unique identifier of the user
	 */
	void RegisterUser(const FString& UserID);

	/**
	 * Unregisters a User object. Call this when a User object is destroyed.
	 * Decrements the reference count. If count reaches 0, data is automatically cleaned up.
	 * @param UserID The unique identifier of the user
	 */
	void UnregisterUser(const FString& UserID);

	/**
	 * Registers a Channel object. Call this when a Channel object is created.
	 * Increments the reference count for this ChannelID.
	 * @param ChannelID The unique identifier of the channel
	 */
	void RegisterChannel(const FString& ChannelID);

	/**
	 * Unregisters a Channel object. Call this when a Channel object is destroyed.
	 * Decrements the reference count. If count reaches 0, data is automatically cleaned up.
	 * @param ChannelID The unique identifier of the channel
	 */
	void UnregisterChannel(const FString& ChannelID);

	/**
	 * Copies user data from the repository under lock.
	 * @param UserID The unique identifier of the user
	 * @param OutUserData Receives a copy of the stored user data when found
	 * @return True if the user exists in the repository, false otherwise
	 */
	bool TryGetUserData(const FString& UserID, FPubnubChatUserData& OutUserData) const;

	/**
	 * Updates user data in the repository. Creates entry if it doesn't exist.
	 * @param UserID The unique identifier of the user
	 * @param UserData The new user data to store
	 */
	void UpdateUserData(const FString& UserID, const FPubnubChatUserData& UserData);

	/**
	 * Removes user data from the repository.
	 * @param UserID The unique identifier of the user to remove
	 * @return True if user was found and removed, false otherwise
	 */
	bool RemoveUserData(const FString& UserID);
	
	/**
	 * Copies channel data from the repository under lock.
	 * @param ChannelID The unique identifier of the channel
	 * @param OutChannelData Receives a copy of the stored channel data when found
	 * @return True if the channel exists in the repository, false otherwise
	 */
	bool TryGetChannelData(const FString& ChannelID, FPubnubChatChannelData& OutChannelData) const;

	/**
	 * Updates channel data in the repository. Creates entry if it doesn't exist.
	 * @param ChannelID The unique identifier of the channel
	 * @param ChannelData The new channel data to store
	 */
	void UpdateChannelData(const FString& ChannelID, const FPubnubChatChannelData& ChannelData);

	/**
	 * Removes channel data from the repository.
	 * @param ChannelID The unique identifier of the channel to remove
	 * @return True if channel was found and removed, false otherwise
	 */
	bool RemoveChannelData(const FString& ChannelID);

	/**
	 * Registers a Message object. Call this when a Message object is created.
	 * Increments the reference count for this MessageID.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 */
	void RegisterMessage(const FString& MessageID);

	/**
	 * Unregisters a Message object. Call this when a Message object is destroyed.
	 * Decrements the reference count. If count reaches 0, data is automatically cleaned up.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 */
	void UnregisterMessage(const FString& MessageID);
	
	/**
	 * Copies message data from the repository under lock.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 * @param OutMessageData Receives a copy of the stored message data when found
	 * @return True if the message exists in the repository, false otherwise
	 */
	bool TryGetMessageData(const FString& MessageID, FPubnubChatMessageData& OutMessageData) const;

	/**
	 * Updates message data in the repository. Creates entry if it doesn't exist.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 * @param MessageData The new message data to store
	 */
	void UpdateMessageData(const FString& MessageID, const FPubnubChatMessageData& MessageData);

	/**
	 * Removes message data from the repository.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 * @return True if message was found and removed, false otherwise
	 */
	bool RemoveMessageData(const FString& MessageID);

	/**
	 * Registers a Membership object. Call this when a Membership object is created.
	 * Increments the reference count for this MembershipID.
	 * @param MembershipID The composite unique identifier of the membership in format "[UserID].[ChannelID]"
	 */
	void RegisterMembership(const FString& MembershipID);

	/**
	 * Unregisters a Membership object. Call this when a Membership object is destroyed.
	 * Decrements the reference count. If count reaches 0, data is automatically cleaned up.
	 * @param MembershipID The composite unique identifier of the membership in format "[UserID].[ChannelID]"
	 */
	void UnregisterMembership(const FString& MembershipID);

	/**
	 * Copies membership data from the repository under lock.
	 * @param MembershipID The composite unique identifier of the membership in format "[UserID].[ChannelID]"
	 * @param OutMembershipData Receives a copy of the stored membership data when found
	 * @return True if the membership exists in the repository, false otherwise
	 */
	bool TryGetMembershipData(const FString& MembershipID, FPubnubChatMembershipData& OutMembershipData) const;

	/**
	 * Updates membership data in the repository. Creates entry if it doesn't exist.
	 * @param MembershipID The composite unique identifier of the membership in format "[UserID].[ChannelID]"
	 * @param MembershipData The new membership data to store
	 */
	void UpdateMembershipData(const FString& MembershipID, const FPubnubChatMembershipData& MembershipData);

	/**
	 * Removes membership data from the repository.
	 * @param MembershipID The composite unique identifier of the membership in format "[UserID].[ChannelID]"
	 * @return True if membership was found and removed, false otherwise
	 */
	bool RemoveMembershipData(const FString& MembershipID);

	/**
	 * Clears all user, channel, message, and membership data from the repository.
	 */
	void ClearAll();

private:
	/** Map of UserID to internal user data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalUser> Users;

	/** Map of ChannelID to internal channel data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalChannel> Channels;

	/** Map of composite MessageID (format: "[ChannelID].[Timetoken]") to internal message data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalMessage> Messages;

	/** Map of composite MembershipID (format: "[UserID].[ChannelID]") to internal membership data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalMembership> Memberships;

	/** Reference counts for User objects - tracks how many User objects exist for each UserID */
	TMap<FString, int32> UserReferenceCounts;

	/** Reference counts for Channel objects - tracks how many Channel objects exist for each ChannelID */
	TMap<FString, int32> ChannelReferenceCounts;

	/** Reference counts for Message objects - tracks how many Message objects exist for each composite MessageID (format: "[ChannelID].[Timetoken]") */
	TMap<FString, int32> MessageReferenceCounts;

	/** Reference counts for Membership objects - tracks how many Membership objects exist for each composite MembershipID (format: "[UserID].[ChannelID]") */
	TMap<FString, int32> MembershipReferenceCounts;

	/** Critical section for thread-safe access to user data */
	mutable FCriticalSection UsersCriticalSection;

	/** Critical section for thread-safe access to channel data */
	mutable FCriticalSection ChannelsCriticalSection;

	/** Critical section for thread-safe access to message data */
	mutable FCriticalSection MessagesCriticalSection;

	/** Critical section for thread-safe access to membership data */
	mutable FCriticalSection MembershipsCriticalSection;
};

