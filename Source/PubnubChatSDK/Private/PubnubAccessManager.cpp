// Fill out your copyright notice in the Description page of Project Settings.


#include "PubnubAccessManager.h"
#include "FunctionLibraries/PubnubChatUtilities.h"

UPubnubAccessManager* UPubnubAccessManager::Create(Pubnub::AccessManager AccessManager)
{
	UPubnubAccessManager* NewAccessManager = NewObject<UPubnubAccessManager>();
	NewAccessManager->InternalAccessManager = new Pubnub::AccessManager(AccessManager);
	return NewAccessManager;
}

UPubnubAccessManager::~UPubnubAccessManager()
{
	delete InternalAccessManager;
}


bool UPubnubAccessManager::CanI(EPubnubAccessManagerPermission Permission, EPubnubAccessManagerResourceType ResourceType, FString ResourceName)
{
	return InternalAccessManager->can_i((Pubnub::AccessManager::Permission)(uint8)Permission, (Pubnub::AccessManager::ResourceType)(uint8)ResourceType, UPubnubChatUtilities::FStringToPubnubString(ResourceName));
}
