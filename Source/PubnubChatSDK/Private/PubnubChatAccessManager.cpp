// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatAccessManager.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubChatInternalConverters.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"

bool UPubnubChatAccessManager::CanI(EPubnubChatAccessManagerPermission Permission, EPubnubChatAccessManagerResourceType ResourceType, const FString ResourceName)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(IsInitialized, TEXT("This object was already destroyed or was not initialized correctly"), false);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PubnubClient, TEXT("This object was already destroyed or was not initialized correctly"), false);
	
	if(ResourceName.IsEmpty())
	{return false;}

	//If token is empty, no any permissions are applied, assuming no PAM and return true
	if(CurrentAuthToken.IsEmpty())
	{return true;}

	//Parse Current Auth Token into string
	FString ParsedToken = ParseToken(CurrentAuthToken);

	//Check if parsed token has required fields
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if(!UPubnubJsonUtilities::StringToJsonObject(ParsedToken, JsonObject))
	{return true;}

	//Convert enums to strings
	FString PermissionStr = UPubnubChatInternalConverters::AccessManagerPermissionToString(Permission);
	FString ResourceTypeStr = UPubnubChatInternalConverters::AccessManagerResourceTypeToString(ResourceType);

	if(PermissionStr.IsEmpty() || ResourceTypeStr.IsEmpty() || ResourceName.IsEmpty())
	{
		return false;
	}

	//Get Resources object (optional - may not exist)
	const TSharedPtr<FJsonObject>* ResourcesObjectPtr = nullptr;
	bool HasResources = JsonObject->TryGetObjectField(ANSI_TO_TCHAR("Resources"), ResourcesObjectPtr) && ResourcesObjectPtr && (*ResourcesObjectPtr).IsValid();

	//Get Patterns object (optional - may not exist)
	const TSharedPtr<FJsonObject>* PatternsObjectPtr = nullptr;
	bool HasPatterns = JsonObject->TryGetObjectField(ANSI_TO_TCHAR("Patterns"), PatternsObjectPtr) && PatternsObjectPtr && (*PatternsObjectPtr).IsValid();

	//If both Resources and Patterns are missing/invalid
	if(!HasResources && !HasPatterns)
	{
		return false;
	}

	//First, check Resources for exact match (if it exists)
	if(HasResources)
	{
		const TSharedPtr<FJsonObject>& ResourcesObject = *ResourcesObjectPtr;
		bool HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(ResourcesObject, ResourceTypeStr, ResourceName, PermissionStr);
		if(HasPermission)
		{
			return HasPermission;
		}
	}

	//If not found in Resources (or Resources doesn't exist), check Patterns for regex match (if it exists)
	if(HasPatterns)
	{
		const TSharedPtr<FJsonObject>& PatternsObject = *PatternsObjectPtr;
		bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(PatternsObject, ResourceTypeStr, ResourceName, PermissionStr);
		return HasPermission;
	}

	//Neither Resources nor Patterns granted permission (Resources exists but denied, Patterns doesn't exist)
	return false;
}

FString UPubnubChatAccessManager::ParseToken(const FString Token)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(IsInitialized, TEXT("This object was already destroyed or was not initialized correctly"), "");
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PubnubClient, TEXT("This object was already destroyed or was not initialized correctly"), "");
	
	return PubnubClient->ParseToken(Token);
}

void UPubnubChatAccessManager::SetAuthToken(const FString Token)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(IsInitialized, TEXT("This object was already destroyed or was not initialized correctly"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PubnubClient, TEXT("This object was already destroyed or was not initialized correctly"));
	
	CurrentAuthToken = Token;
	PubnubClient->SetAuthToken(Token);
}

int UPubnubChatAccessManager::SetPubnubOrigin(const FString Origin)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(IsInitialized, TEXT("This object was already destroyed or was not initialized correctly"), -1);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PubnubClient, TEXT("This object was already destroyed or was not initialized correctly"), -1);
	
	return PubnubClient->SetOrigin(Origin);
}

FString UPubnubChatAccessManager::GetPubnubOrigin() const
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(IsInitialized, TEXT("This object was already destroyed or was not initialized correctly"), "");
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(PubnubClient, TEXT("This object was already destroyed or was not initialized correctly"), "");

	return PubnubClient->GetOrigin();
}

void UPubnubChatAccessManager::InitAccessManager(UPubnubClient* InPubnubClient)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init AccessManager, PubnubClient is invalid"));

	PubnubClient = InPubnubClient;
	IsInitialized = true;
}
