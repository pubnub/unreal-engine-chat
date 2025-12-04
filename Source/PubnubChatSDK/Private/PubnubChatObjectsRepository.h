// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HAL/CriticalSection.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
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
	 * Clears all user and channel data from the repository.
	 */
	void ClearAll();

private:
	/** Map of UserID to internal user data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalUser> Users;

	/** Map of ChannelID to internal channel data */
	UPROPERTY()
	TMap<FString, FPubnubChatInternalChannel> Channels;

	/** Reference counts for User objects - tracks how many User objects exist for each UserID */
	TMap<FString, int32> UserReferenceCounts;

	/** Reference counts for Channel objects - tracks how many Channel objects exist for each ChannelID */
	TMap<FString, int32> ChannelReferenceCounts;

	/** Critical section for thread-safe access to user data */
	mutable FCriticalSection UsersCriticalSection;

	/** Critical section for thread-safe access to channel data */
	mutable FCriticalSection ChannelsCriticalSection;
};

