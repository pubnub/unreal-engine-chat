// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubSubsystem.h"
#include "Kismet/GameplayStatics.h"


void UPubnubChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UPubnubChatSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

UPubnubChat* UPubnubChatSubsystem::InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config)
{
	//TODO:: Add checks for empty keys and userID
	
	if(Chat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Chat already exists. (Only one chat object can be created). Returning existing Char"));
		return Chat;
	}

	Chat = NewObject<UPubnubChat>(this);
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
	Chat->InitChat(Config, CreatePubnubClient(PublishKey, SubscribeKey, UserID));

	//Make sure Chat was correctly initialized
	if(!Chat->IsInitialized)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("InitChat Error - Chat was not initialized correctly"));
		Chat = nullptr;
		return nullptr;
	}

	return Chat;
}

UPubnubChat* UPubnubChatSubsystem::GetChat()
{
	if(!Chat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Chat doesn't exist. Call 'Create Chat' instead"));
		return nullptr;
	}

	return Chat;
}

void UPubnubChatSubsystem::DestroyChat()
{
	if(!Chat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Can't destroy chat as it doesn't exists"));
		return;
	}
	
	Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
	Chat->DestroyChat();
	Chat = nullptr;
}

void UPubnubChatSubsystem::OnChatDestroyed()
{
	Chat = nullptr;
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
