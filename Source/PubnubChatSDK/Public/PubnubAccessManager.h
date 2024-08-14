// Fill out your copyright notice in the Description page of Project Settings.

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

	static UPubnubAccessManager* Create(Pubnub::AccessManager AccessManager);
	~UPubnubAccessManager();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	bool CanI(EPubnubAccessManagerPermission Permission, EPubnubAccessManagerResourceType ResourceType, FString ResourceName);

private:
	Pubnub::AccessManager* InternalAccessManager;
};
