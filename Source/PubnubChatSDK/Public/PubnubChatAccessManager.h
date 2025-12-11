// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatEnumLibrary.h"
#include "PubnubChatAccessManager.generated.h"

class UPubnubClient;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatAccessManager : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;

public:
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	bool CanI(EPubnubChatAccessManagerPermission Permission, EPubnubChatAccessManagerResourceType ResourceType, const FString ResourceName);

	/**
	 * Decodes an existing token and returns the object containing permissions embedded in that token.
	 * The client can use this method for debugging to check the permissions to the resources or find out the token's ttl details.
	 * 
	 * @param Token Current token with embedded permissions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	FString ParseToken(const FString Token);

	/**
	 * Update client with authentication token granted by the server.
	 * 
	 * @param Token Current token with embedded permissions
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	void SetAuthToken(const FString Token);

	/**
	 * Update Pubnub origin used by the Chat sdk.
	 * 
	 * @return 0 if origin set, +1 if origin set will be applied with new connection, -1 if setting origin not enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	int SetPubnubOrigin(const FString Origin);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Access Manager")
	FString GetPubnubOrigin() const;

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	
	UPROPERTY()
	bool IsInitialized = false;

	FString CurrentAuthToken = "";

	void InitAccessManager(UPubnubClient* InPubnubClient);
};

