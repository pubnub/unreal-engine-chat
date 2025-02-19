// Copyright 2024 PubNub Inc. All Rights Reserved.


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

FString UPubnubAccessManager::ParseToken(FString Token)
{
	return UPubnubChatUtilities::PubnubStringToFString(InternalAccessManager->parse_token(UPubnubChatUtilities::FStringToPubnubString(Token)));
}

void UPubnubAccessManager::SetAuthToken(FString Token)
{
	InternalAccessManager->set_auth_token(UPubnubChatUtilities::FStringToPubnubString(Token));
}

int UPubnubAccessManager::SetPubnubOrigin(FString Origin)
{
	return InternalAccessManager->set_pubnub_origin(UPubnubChatUtilities::FStringToPubnubString(Origin));
}