// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HAL/CriticalSection.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
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
	 * Gets user data from the repository. Creates entry if it doesn't exist.
	 * @param UserID The unique identifier of the user
	 * @return Reference to the internal user data
	 */
	FPubnubChatInternalUser& GetOrCreateUserData(const FString& UserID);

	/**
	 * Gets user data from the repository. Returns nullptr if not found.
	 * @param UserID The unique identifier of the user
	 * @return Pointer to the internal user data, or nullptr if not found
	 */
	FPubnubChatInternalUser* GetUserData(const FString& UserID);

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
	 * Gets channel data from the repository. Creates entry if it doesn't exist.
	 * @param ChannelID The unique identifier of the channel
	 * @return Reference to the internal channel data
	 */
	FPubnubChatInternalChannel& GetOrCreateChannelData(const FString& ChannelID);

	/**
	 * Gets channel data from the repository. Returns nullptr if not found.
	 * @param ChannelID The unique identifier of the channel
	 * @return Pointer to the internal channel data, or nullptr if not found
	 */
	FPubnubChatInternalChannel* GetChannelData(const FString& ChannelID);

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
	 * Gets message data from the repository. Creates entry if it doesn't exist.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 * @return Reference to the internal message data
	 */
	FPubnubChatInternalMessage& GetOrCreateMessageData(const FString& MessageID);

	/**
	 * Gets message data from the repository. Returns nullptr if not found.
	 * @param MessageID The composite unique identifier of the message in format "[ChannelID].[Timetoken]"
	 * @return Pointer to the internal message data, or nullptr if not found
	 */
	FPubnubChatInternalMessage* GetMessageData(const FString& MessageID);

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
	 * Clears all user, channel, and message data from the repository.
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

	/** Reference counts for User objects - tracks how many User objects exist for each UserID */
	TMap<FString, int32> UserReferenceCounts;

	/** Reference counts for Channel objects - tracks how many Channel objects exist for each ChannelID */
	TMap<FString, int32> ChannelReferenceCounts;

	/** Reference counts for Message objects - tracks how many Message objects exist for each composite MessageID (format: "[ChannelID].[Timetoken]") */
	TMap<FString, int32> MessageReferenceCounts;

	/** Critical section for thread-safe access to user data */
	mutable FCriticalSection UsersCriticalSection;

	/** Critical section for thread-safe access to channel data */
	mutable FCriticalSection ChannelsCriticalSection;

	/** Critical section for thread-safe access to message data */
	mutable FCriticalSection MessagesCriticalSection;
};

