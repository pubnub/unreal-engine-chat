// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "PubnubAccessManager.h"
#include "PubnubChatSubsystem.h"
#include "PubnubMacroUtilities.h"
#include "FunctionLibraries/PubnubChatUtilities.h"
#include "FunctionLibraries/PubnubLogUtilities.h"

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
	if(!IsInternalAccessManagerValid()) {return false;}
	
	return InternalAccessManager->can_i((Pubnub::AccessManager::Permission)(uint8)Permission, (Pubnub::AccessManager::ResourceType)(uint8)ResourceType, UPubnubChatUtilities::FStringToPubnubString(ResourceName));
}

FString UPubnubAccessManager::ParseToken(FString Token)
{
	if(!IsInternalAccessManagerValid()) {return "";}
	
	PUBNUB_RETURN_IF_EMPTY(Token, "");
	
	return UPubnubChatUtilities::PubnubStringToFString(InternalAccessManager->parse_token(UPubnubChatUtilities::FStringToPubnubString(Token)));
}

void UPubnubAccessManager::SetAuthToken(FString Token)
{
	if(!IsInternalAccessManagerValid()) {return;}
	
	PUBNUB_RETURN_IF_EMPTY(Token);
	
	InternalAccessManager->set_auth_token(UPubnubChatUtilities::FStringToPubnubString(Token));
}

int UPubnubAccessManager::SetPubnubOrigin(FString Origin)
{
	if(!IsInternalAccessManagerValid()) {return -1;}
	
	PUBNUB_RETURN_IF_EMPTY(Origin, -1);
	
	return InternalAccessManager->set_pubnub_origin(UPubnubChatUtilities::FStringToPubnubString(Origin));
}

bool UPubnubAccessManager::IsInternalAccessManagerValid()
{
	if(InternalAccessManager == nullptr)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("This Access Manager is invalid"));
		return false;
	}
	return true;
}