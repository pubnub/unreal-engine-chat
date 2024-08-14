// Copyright Epic Games, Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"

#include "ChatSDK.h"
#include "PubnubChat.h"
#include "FunctionLibraries/PubnubChatUtilities.h"


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
	if(Chat != nullptr)
	{
		UE_LOG(PubnubLog, Warning, TEXT("Chat already exists. (Only one chat object can be created). Returning existing Char"));
		return Chat;
	}
	
	Pubnub::ChatConfig CppConfig;
	CppConfig.auth_key = UPubnubChatUtilities::FStringToPubnubString(Config.AuthKey);
	CppConfig.typing_timeout = Config.TypingTimeout;
	CppConfig.typing_timeout_difference = Config.TypingTimeoutDifference;
	try
	{
		Chat = UPubnubChat::Create(Pubnub::Chat::init(UPubnubChatUtilities::FStringToPubnubString(PublishKey),
	UPubnubChatUtilities::FStringToPubnubString(SubscribeKey),UPubnubChatUtilities::FStringToPubnubString(UserID), CppConfig));
		return Chat;
	}
	catch (std::exception& Exception)
	{
		UE_LOG(PubnubLog, Error, TEXT("Can't create chat. Error: %s"), UTF8_TO_TCHAR(Exception.what()));
	}
	return nullptr;
	
}

UPubnubChat* UPubnubChatSubsystem::GetChat()
{
	if(Chat == nullptr)
	{
		UE_LOG(PubnubLog, Warning, TEXT("Chat doesn't exist. Call 'Create Chat' instead"));
		return nullptr;
	}

	return Chat;
}
