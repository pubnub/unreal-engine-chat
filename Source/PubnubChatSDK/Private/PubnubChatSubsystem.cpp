// Copyright 2026 PubNub Inc. All Rights Reserved.


#include "PubnubChatSubsystem.h"
#include "PubnubChatVersion.h"

#include "PubnubClient.h"
#include "PubnubChat.h"
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubSubsystem.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubUtilities.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeLock.h"


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

	return InitChatInternal(UserID, Config, PubnubClient, true);
}

FPubnubChatInitChatResult UPubnubChatSubsystem::InitChatWithPubnubClient(FString UserID, UPubnubClient* PubnubClient, FPubnubChatConfig Config)
{
	FPubnubChatInitChatResult FinalResult;
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, UserID);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, PubnubClient);
	
	//Make sure PubnubClient has correct UserID
	PubnubClient->SetUserID(UserID);
	PubnubClient->SetRuntimeSdkVersionSuffix(UPubnubChatInternalUtilities::GetChatSdkVersionSuffix());

	return InitChatInternal(UserID, Config, PubnubClient, false);
}

void UPubnubChatSubsystem::InitChatAsync(FString PublishKey, FString SubscribeKey, FString UserID, FOnPubnubChatInitChatResponse OnInitChatResponse, FPubnubChatConfig Config)
{
	FOnPubnubChatInitChatResponseNative NativeCallback;
	NativeCallback.BindLambda([OnInitChatResponse](const FPubnubChatInitChatResult& InitChatResult)
	{
		OnInitChatResponse.ExecuteIfBound(InitChatResult);
	});

	InitChatAsync(PublishKey, SubscribeKey, UserID, NativeCallback, MoveTemp(Config));
}

void UPubnubChatSubsystem::InitChatAsync(FString PublishKey, FString SubscribeKey, FString UserID, FOnPubnubChatInitChatResponseNative OnInitChatResponseNative, FPubnubChatConfig Config)
{
	FPubnubChatInitChatResult ErrorResult;
	if (PublishKey.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: PublishKey field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		ErrorResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, ErrorResult);
		return;
	}
	if (SubscribeKey.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: SubscribeKey field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		ErrorResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, ErrorResult);
		return;
	}
	if (UserID.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: UserID field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		ErrorResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, ErrorResult);
		return;
	}

	TWeakObjectPtr<UPubnubChatSubsystem> WeakThis = MakeWeakObjectPtr(this);

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, PublishKey = MoveTemp(PublishKey), SubscribeKey = MoveTemp(SubscribeKey), UserID = MoveTemp(UserID), Config = MoveTemp(Config), OnInitChatResponseNative]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		FPubnubChatInitChatResult Result = WeakThis->InitChat(PublishKey, SubscribeKey, UserID, Config);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, Result);
	});
}

void UPubnubChatSubsystem::InitChatWithPubnubClientAsync(FString UserID, UPubnubClient* PubnubClient, FOnPubnubChatInitChatResponse OnInitChatResponse, FPubnubChatConfig Config)
{
	FOnPubnubChatInitChatResponseNative NativeCallback;
	NativeCallback.BindLambda([OnInitChatResponse](const FPubnubChatInitChatResult& InitChatResult)
	{
		OnInitChatResponse.ExecuteIfBound(InitChatResult);
	});

	InitChatWithPubnubClientAsync(UserID, PubnubClient, NativeCallback, MoveTemp(Config));
}

void UPubnubChatSubsystem::InitChatWithPubnubClientAsync(FString UserID, UPubnubClient* PubnubClient, FOnPubnubChatInitChatResponseNative OnInitChatResponseNative, FPubnubChatConfig Config)
{
	FPubnubChatInitChatResult ErrorResult;
	if (UserID.IsEmpty())
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: UserID field can't be empty. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		ErrorResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, ErrorResult);
		return;
	}
	if (!PubnubClient)
	{
		FString ErrorMessage = FString::Printf(TEXT("[%s]: PubnubClient has to be a valid object. Aborting operation."), *UPubnubChatLogUtilities::ConvertFunctionNameMacroToLog(ANSI_TO_TCHAR(__FUNCTION__)));
		UE_LOG(PubnubChatLog, Error, TEXT("%s"), *ErrorMessage);
		ErrorResult.Result = FPubnubChatOperationResult::CreateError(ErrorMessage);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, ErrorResult);
		return;
	}

	TWeakObjectPtr<UPubnubChatSubsystem> WeakThis = MakeWeakObjectPtr(this);
	TWeakObjectPtr<UPubnubClient> WeakClient = MakeWeakObjectPtr(PubnubClient);

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, WeakClient, UserID = MoveTemp(UserID), Config = MoveTemp(Config), OnInitChatResponseNative]()
	{
		if (!WeakThis.IsValid() || !WeakClient.IsValid())
		{
			return;
		}

		FPubnubChatInitChatResult Result = WeakThis->InitChatWithPubnubClient(UserID, WeakClient.Get(), Config);
		UPubnubUtilities::CallPubnubDelegate(OnInitChatResponseNative, Result);
	});
}

UPubnubChat* UPubnubChatSubsystem::GetChat(FString UserID)
{
	if(UserID.IsEmpty())
	{
		UE_LOG(PubnubChatLog, Warning, TEXT("GetChat Error - UserID is empty"));
		return nullptr;
	}

	FScopeLock Lock(&ChatsMutex);

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

	UPubnubChat* ChatToDestroy = nullptr;
	{
		FScopeLock Lock(&ChatsMutex);
		UPubnubChat** FoundChat = Chats.Find(UserID);
		if(!FoundChat || !*FoundChat)
		{
			UE_LOG(PubnubChatLog, Warning, TEXT("Can't destroy chat with UserID '%s' as it doesn't exist"), *UserID);
			return;
		}

		ChatToDestroy = *FoundChat;
		ChatToDestroy->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
		Chats.Remove(UserID);
	}

	ChatToDestroy->DestroyChat();
}

void UPubnubChatSubsystem::DestroyAllChats()
{
	TArray<UPubnubChat*> ChatsToDestroy;
	{
		FScopeLock Lock(&ChatsMutex);
		ChatsToDestroy.Reserve(Chats.Num());
		for(auto& ChatPair : Chats)
		{
			if(ChatPair.Value)
			{
				ChatPair.Value->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatSubsystem::OnChatDestroyed);
				ChatsToDestroy.Add(ChatPair.Value);
			}
		}
		Chats.Empty();
	}

	for(UPubnubChat* Chat : ChatsToDestroy)
	{
		if(Chat)
		{
			Chat->DestroyChat();
		}
	}
}

void UPubnubChatSubsystem::OnChatDestroyed(FString UserID)
{
	FScopeLock Lock(&ChatsMutex);
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
	ClientConfig.LoggerConfig.DefaultLoggerMinLevel = EPubnubLogLevel::PLL_Debug;
	ClientConfig.LoggerConfig.DefaultLoggerMinCCoreLevel = EPubnubLogLevel::PLL_Debug;
	
	UPubnubClient* PubnubClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);

	if(!PubnubClient)
	{
		UE_LOG(PubnubChatLog, Error, TEXT("CreatePubnubClient Error - Created PubnubClient is invalid"));
		return nullptr;
	}

	PubnubClient->SetRuntimeSdkVersionSuffix(UPubnubChatInternalUtilities::GetChatSdkVersionSuffix());

	return PubnubClient;
}

FPubnubChatConfig UPubnubChatSubsystem::GetDefaultChatConfig()
{
	return FPubnubChatConfig();
}


FPubnubChatInitChatResult UPubnubChatSubsystem::InitChatInternal(FString UserID, FPubnubChatConfig Config, UPubnubClient* PubnubClient, bool bChatOwnsPubnubClient)
{
	FScopeLock Lock(&ChatsMutex);

	FPubnubChatInitChatResult FinalResult;
	
	// Check if chat with this UserID already exists
	if(UPubnubChat** ExistingChat = Chats.Find(UserID))
	{
		if(*ExistingChat)
		{
			if(bChatOwnsPubnubClient && PubnubClient)
			{
				PubnubClient->DestroyClient();
			}
			
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
	FPubnubChatInitChatResult InitChatResult = NewChat->InitChat(UserID, Config, PubnubClient, bChatOwnsPubnubClient);

	if(InitChatResult.Result.Error && bChatOwnsPubnubClient && PubnubClient)
	{
		PubnubClient->DestroyClient();
	}

	PUBNUB_CHAT_RETURN_WRAPPER_IF_RESULT_FAILED(FinalResult, InitChatResult);

	FinalResult.Result.Merge(InitChatResult.Result);
	FinalResult.Result.MarkSuccess();
	FinalResult.Chat = NewChat;

	// Store the chat in the map
	Chats.Add(UserID, NewChat);

	return FinalResult;
}