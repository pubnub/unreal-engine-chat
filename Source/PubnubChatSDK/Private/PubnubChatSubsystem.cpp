// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatInternalMacros.h"
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

FPubnubChatInitChatResult UPubnubChatSubsystem::InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config)
{
	//TODO:: Add checks for empty keys and userID
	//TODO:: Maybe add support for multiple chats??

	FPubnubChatInitChatResult FinalResult;
	
	if(Chat)
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("Chat already exists. (Only one chat object can be created). Returning existing Chat"));
		FinalResult.Result = FPubnubChatOperationResult(0, true, TEXT("Chat already exists. (Only one chat object can be created). Returning existing Chat"));
		FinalResult.Chat = Chat;
		FinalResult.Result.MarkSuccess();
		return FinalResult;
	}

	Chat = NewObject<UPubnubChat>(this);
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
	FPubnubChatInitChatResult InitChatResult = Chat->InitChat(UserID, Config, CreatePubnubClient(PublishKey, SubscribeKey, UserID));

	PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(FinalResult, InitChatResult);

	FinalResult.Result.Merge(InitChatResult.Result);
	FinalResult.Result.MarkSuccess();

	return FinalResult;
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
