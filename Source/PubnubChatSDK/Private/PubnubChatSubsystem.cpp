// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "PubnubChatSubsystem.h"
#include "PubnubChatVersion.h"

#include "PubnubClient.h"
#include "PubnubChat.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubSubsystem.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "Kismet/GameplayStatics.h"


void UPubnubChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPubnubChatSubsystem::Deinitialize()
{
	// Ensure all Chats are destroyed before subsystem deinitialization
	DestroyAllChats();
	
	Super::Deinitialize();
}

FPubnubChatInitChatResult UPubnubChatSubsystem::InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config)
{
	FPubnubChatInitChatResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, PublishKey);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, SubscribeKey);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);
	
	// Create PubnubClient
	UPubnubClient* PubnubClient = CreatePubnubClient(PublishKey, SubscribeKey, UserID);
	if(!PubnubClient)
	{
		FinalResult.Result = FPubnubChatOperationResult::CreateError(TEXT("Failed to create PubnubClient"));
		return FinalResult;
	}

	return InitChatInternal(UserID, Config, PubnubClient);
}

FPubnubChatInitChatResult UPubnubChatSubsystem::InitChatWithPubnubClient(FString UserID, UPubnubClient* PubnubClient, FPubnubChatConfig Config)
{
	FPubnubChatInitChatResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, PubnubClient);
	
	//Make sure PubnubClient has correct UserID
	PubnubClient->SetUserID(UserID);

	return InitChatInternal(UserID, Config, PubnubClient);
}

UPubnubChat* UPubnubChatSubsystem::GetChat(FString UserID)
{
	if(UserID.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("GetChat Error - UserID is empty"));
		return nullptr;
	}

	UPubnubChat** FoundChat = Chats.Find(UserID);
	if(!FoundChat || !*FoundChat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Chat with UserID '%s' doesn't exist. Call 'Init Chat' instead"), *UserID);
		return nullptr;
	}

	return *FoundChat;
}

void UPubnubChatSubsystem::DestroyChat(FString UserID)
{
	if(UserID.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("DestroyChat Error - UserID is empty"));
		return;
	}

	UPubnubChat** FoundChat = Chats.Find(UserID);
	if(!FoundChat || !*FoundChat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Can't destroy chat with UserID '%s' as it doesn't exist"), *UserID);
		return;
	}
	
	UPubnubChat* ChatToDestroy = *FoundChat;
	ChatToDestroy->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
	ChatToDestroy->DestroyChat();
	
	Chats.Remove(UserID);
}

void UPubnubChatSubsystem::DestroyAllChats()
{
	for(auto& ChatPair : Chats)
	{
		if(ChatPair.Value)
		{
			ChatPair.Value->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
			ChatPair.Value->DestroyChat();
		}
	}
	Chats.Empty();
}

void UPubnubChatSubsystem::OnChatDestroyed(FString UserID)
{
	// Remove the destroyed chat from the map using the provided UserID
	Chats.Remove(UserID);
}

UPubnubClient* UPubnubChatSubsystem::CreatePubnubClient(FString PublishKey, FString SubscribeKey, FString UserID)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if(!GameInstance)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("CreatePubnubClient Error - GameInstance is invalid"));
	}
	
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();
	
	if(!PubnubSubsystem)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("CreatePubnubClient Error - PubnubSubsystem is invalid"));
		return nullptr;
	}

	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = PublishKey;
	ClientConfig.SubscribeKey = SubscribeKey;
	ClientConfig.UserID = UserID;
	
	UPubnubClient* PubnubClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);

	if(!PubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("CreatePubnubClient Error - Created PubnubClient is invalid"));
		return nullptr;
	}

	return PubnubClient;
}

FPubnubChatConfig UPubnubChatSubsystem::GetDefaultChatConfig()
{
	return FPubnubChatConfig();
}


FPubnubChatInitChatResult UPubnubChatSubsystem::InitChatInternal(FString UserID, FPubnubChatConfig Config, UPubnubClient* PubnubClient)
{
	FPubnubChatInitChatResult FinalResult;
	
	// Check if chat with this UserID already exists
	if(UPubnubChat** ExistingChat = Chats.Find(UserID))
	{
		if(*ExistingChat)
		{
			UE_LOG(PubnubChatLog, Warning, TEXT("Chat with UserID '%s' already exists. Returning existing Chat"), *UserID);
			FinalResult.Result = FPubnubChatOperationResult::CreateError(FString::Printf(TEXT("Chat with UserID '%s' already exists. Returning existing Chat"), *UserID));
			FinalResult.Chat = *ExistingChat;
			return FinalResult;
		}
	}
	
	//Adjust some Config values that might be not in range
	Config.ValidateConfig();

	UPubnubChat* NewChat = NewObject<UPubnubChat>(this);
	NewChat->OnChatDestroyed.AddDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
	FPubnubChatInitChatResult InitChatResult = NewChat->InitChat(UserID, Config, PubnubClient);

	PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(FinalResult, InitChatResult);

	FinalResult.Result.Merge(InitChatResult.Result);
	FinalResult.Result.MarkSuccess();
	FinalResult.Chat = NewChat;

	// Store the chat in the map
	Chats.Add(UserID, NewChat);

	return FinalResult;
}