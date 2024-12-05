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

	//Internal usage only
	static UPubnubAccessManager* Create(Pubnub::AccessManager AccessManager);
private:
	Pubnub::AccessManager* InternalAccessManager;
};
