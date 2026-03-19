// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChat.h"
#include "PubnubChatSubsystem.generated.h"

class UPubnubClient;

DECLARE_LOG_CATEGORY_EXTERN(PubnubChatLog, Log, All);


/**
 * Game instance subsystem that owns and provides access to UPubnubChat instances per user.
 * Use InitChat or InitChatWithPubnubClient to create a chat, GetChat to retrieve it, and DestroyChat / DestroyAllChats to tear down.
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	/**
	 * Creates a PubNub client with the given keys and user ID, then initializes a chat for that user. The chat is stored in the subsystem and can be retrieved with GetChat(UserID).
	 * Blocking: creates the client and initializes the chat on the calling thread. If a chat for this UserID already exists, returns an error result but with the existing Chat in the result (no new chat created).
	 *
	 * @param PublishKey PubNub publish key. Must a valid key from the PubNub Admin Portal.
	 * @param SubscribeKey PubNub subscribe key. Must a valid key from the PubNub Admin Portal.
	 * @param UserID User ID for this chat session. Must be non-empty.
	 * @param Config Optional chat configuration (typing timeout, rate limiter, etc.). Default is valid.
	 * @return Result and the UPubnubChat instance. Error if keys/UserID empty, client creation failed, or chat init failed; also error (with existing Chat) if chat for UserID already exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	FPubnubChatInitChatResult InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config = FPubnubChatConfig());

	/**
	 * Initializes a chat for the given user using an existing UPubnubClient. The client's UserID is set to the provided UserID. The chat is stored in the subsystem and can be retrieved with GetChat(UserID).
	 * Blocking: initializes the chat on the calling thread. If a chat for this UserID already exists, returns an error result but with the existing Chat in the result (no new chat created).
	 *
	 * @param UserID User ID for this chat session. Must be non-empty.
	 * @param PubnubClient Existing PubNub client to use. Must be non-null.
	 * @param Config Optional chat configuration. Default is valid.
	 * @return Result and the UPubnubChat instance. Error if UserID empty, PubnubClient null, or chat init failed; also error (with existing Chat) if chat for UserID already exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	FPubnubChatInitChatResult InitChatWithPubnubClient(FString UserID, UPubnubClient* PubnubClient, FPubnubChatConfig Config = FPubnubChatConfig());
	
	/**
	 * Returns the chat instance for the given user ID, if one was created by InitChat or InitChatWithPubnubClient.
	 * Local: does not perform any network requests. Returns null if UserID is empty or no chat exists for that UserID.
	 *
	 * @param UserID User ID used when the chat was initialized.
	 * @return The UPubnubChat for that user, or null.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PubnubChat")
	UPubnubChat* GetChat(FString UserID);

	/**
	 * Destroys the chat for the given user ID (disconnects, cleans up, removes from the subsystem). No-op if UserID is empty or no chat exists for that UserID.
	 *
	 * @param UserID User ID of the chat to destroy.
	 */
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	void DestroyChat(FString UserID);
	
	/**
	 * Destroys all chats owned by this subsystem (disconnects and cleans up each, then clears the internal map). Called automatically on Deinitialize.
	 */
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	void DestroyAllChats();

	/**
	 * Returns a default chat config with all default values filled (e.g. EmitReadReceiptEvents: public=false, group=true, direct=true).
	 * Use this in Blueprint instead of "Make FPubnubChatConfig" when you want the standard defaults; "Make Struct" does not run C++ constructors, so the map would otherwise be empty.
	 *
	 * @return Default FPubnubChatConfig with EmitReadReceiptEvents and other defaults set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PubnubChat")
	static FPubnubChatConfig GetDefaultChatConfig();

private:
	UPROPERTY()
	TMap<FString, UPubnubChat*> Chats;
	
	UFUNCTION()
	void OnChatDestroyed(FString UserID);

	UPubnubClient* CreatePubnubClient(FString PublishKey, FString SubscribeKey, FString UserID);

	FPubnubChatInitChatResult InitChatInternal(FString UserID, FPubnubChatConfig Config, UPubnubClient* PubnubClient);

};