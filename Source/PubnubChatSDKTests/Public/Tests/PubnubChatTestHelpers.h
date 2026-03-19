// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "PubnubChat.h"
#include "PubnubChatSubsystem.h"
#include "StructLibraries/PubnubChatStructLibrary.h"

// Forward declarations
class UPubnubClient;
class UPubnubChatObjectsRepository;

/**
 * Test helper utilities for accessing private members via Unreal Engine's reflection system.
 * This allows tests to access UPROPERTY members without requiring friend classes or
 * creating dependencies from the main module to the test module.
 */
namespace PubnubChatTestHelpers
{
	/**
	 * Gets the Chat object from UPubnubChatSubsystem using reflection.
	 * @param Subsystem The subsystem to get the Chat from
	 * @param UserID The UserID to look up the chat for
	 * @return The Chat object, or nullptr if not found or not accessible
	 */
	UPubnubChat* GetChatFromSubsystem(UPubnubChatSubsystem* Subsystem, const FString& UserID);
	
	/**
	 * Gets the PubnubClient from UPubnubChat using reflection.
	 * @param Chat The Chat object to get the client from
	 * @return The PubnubClient object, or nullptr if not found or not accessible
	 */
	UPubnubClient* GetPubnubClientFromChat(UPubnubChat* Chat);
	
	/**
	 * Gets the CurrentUser from UPubnubChat using reflection.
	 * @param Chat The Chat object to get the user from
	 * @return The CurrentUser object, or nullptr if not found or not accessible
	 */
	UPubnubChatUser* GetCurrentUserFromChat(UPubnubChat* Chat);
	
	/**
	 * Gets the ChatConfig from UPubnubChat using reflection.
	 * @param Chat The Chat object to get the config from
	 * @return The ChatConfig, or default constructed config if not found
	 */
	FPubnubChatConfig GetChatConfigFromChat(UPubnubChat* Chat);
	
	/**
	 * Gets the IsInitialized flag from UPubnubChat using reflection.
	 * Note: This only works if IsInitialized is a UPROPERTY. If it's not,
	 * this will return false.
	 * @param Chat The Chat object to get the flag from
	 * @return The IsInitialized flag value, or false if not accessible
	 */
	bool GetIsInitializedFromChat(UPubnubChat* Chat);
	
	/**
	 * Gets the ObjectsRepository from UPubnubChat using reflection.
	 * @param Chat The Chat object to get the repository from
	 * @return The ObjectsRepository object, or nullptr if not found or not accessible
	 */
	UPubnubChatObjectsRepository* GetObjectsRepositoryFromChat(UPubnubChat* Chat);
}

#endif // WITH_DEV_AUTOMATION_TESTS

