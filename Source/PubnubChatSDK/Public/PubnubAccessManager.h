// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <pubnub_chat/access_manager.hpp>
#include "PubnubChatEnumLibrary.h"
#include "PubnubAccessManager.generated.h"




/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubAccessManager : public UObject
{
	GENERATED_BODY()
	
public:
	
	~UPubnubAccessManager();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	bool CanI(EPubnubAccessManagerPermission Permission, EPubnubAccessManagerResourceType ResourceType, FString ResourceName);

	/**
	 * Decodes an existing token and returns the object containing permissions embedded in that token.
	 * The client can use this method for debugging to check the permissions to the resources or find out the token's ttl details.
	 * 
	 * @param Token Current token with embedded permissions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	FString ParseToken(FString Token);

	/**
	 * Update client with authentication token granted by the server.
	 * 
	 * @param Token Current token with embedded permissions
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	void SetAuthToken(FString Token);

	/**
	 * Update Pubnub origin used by the Chat sdk.
	 * 
	 * @return 0 if origin set, +1 if origin set will be applied with new connection, -1 if setting origin not enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	int SetPubnubOrigin(FString Origin);

	//Internal usage only
	static UPubnubAccessManager* Create(Pubnub::AccessManager AccessManager);
private:
	Pubnub::AccessManager* InternalAccessManager;
};
