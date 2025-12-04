// Copyright 2025 PubNub Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestHelpers.h"
#include "UObject/UnrealType.h"
#include "UObject/Class.h"
#include "PubnubClient.h"

namespace PubnubChatTestHelpers
{
	UPubnubChat* GetChatFromSubsystem(UPubnubChatSubsystem* Subsystem)
	{
		if (!Subsystem)
		{
			return nullptr;
		}
		
		// Use reflection to access the private "Chat" UPROPERTY
		FProperty* ChatProperty = Subsystem->GetClass()->FindPropertyByName(TEXT("Chat"));
		if (!ChatProperty)
		{
			return nullptr;
		}
		
		// Get the value of the property
		UPubnubChat** ChatPtr = ChatProperty->ContainerPtrToValuePtr<UPubnubChat*>(Subsystem);
		if (!ChatPtr)
		{
			return nullptr;
		}
		
		return *ChatPtr;
	}
	
	UPubnubClient* GetPubnubClientFromChat(UPubnubChat* Chat)
	{
		if (!Chat)
		{
			return nullptr;
		}
		
		// Use reflection to access the private "PubnubClient" UPROPERTY
		FProperty* ClientProperty = Chat->GetClass()->FindPropertyByName(TEXT("PubnubClient"));
		if (!ClientProperty)
		{
			return nullptr;
		}
		
		// Get the value of the property
		TObjectPtr<UPubnubClient>* ClientPtr = ClientProperty->ContainerPtrToValuePtr<TObjectPtr<UPubnubClient>>(Chat);
		if (!ClientPtr)
		{
			return nullptr;
		}
		
		return ClientPtr->Get();
	}
	
	UPubnubChatUser* GetCurrentUserFromChat(UPubnubChat* Chat)
	{
		if (!Chat)
		{
			return nullptr;
		}
		
		// Use reflection to access the private "CurrentUser" UPROPERTY
		FProperty* UserProperty = Chat->GetClass()->FindPropertyByName(TEXT("CurrentUser"));
		if (!UserProperty)
		{
			return nullptr;
		}
		
		// Get the value of the property
		TObjectPtr<UPubnubChatUser>* UserPtr = UserProperty->ContainerPtrToValuePtr<TObjectPtr<UPubnubChatUser>>(Chat);
		if (!UserPtr)
		{
			return nullptr;
		}
		
		return UserPtr->Get();
	}
	
	FPubnubChatConfig GetChatConfigFromChat(UPubnubChat* Chat)
	{
		if (!Chat)
		{
			return FPubnubChatConfig();
		}
		
		// Use reflection to access the private "ChatConfig" UPROPERTY
		FProperty* ConfigProperty = Chat->GetClass()->FindPropertyByName(TEXT("ChatConfig"));
		if (!ConfigProperty)
		{
			return FPubnubChatConfig();
		}
		
		// Get the value of the property
		FPubnubChatConfig* ConfigPtr = ConfigProperty->ContainerPtrToValuePtr<FPubnubChatConfig>(Chat);
		if (!ConfigPtr)
		{
			return FPubnubChatConfig();
		}
		
		return *ConfigPtr;
	}
	
	bool GetIsInitializedFromChat(UPubnubChat* Chat)
	{
		if (!Chat)
		{
			return false;
		}
		
		// Use reflection to access the private "IsInitialized" UPROPERTY
		FProperty* InitProperty = Chat->GetClass()->FindPropertyByName(TEXT("IsInitialized"));
		if (!InitProperty)
		{
			return false;
		}
		
		// Get the value of the property
		bool* InitPtr = InitProperty->ContainerPtrToValuePtr<bool>(Chat);
		if (!InitPtr)
		{
			return false;
		}
		
		return *InitPtr;
	}

	UPubnubChatObjectsRepository* GetObjectsRepositoryFromChat(UPubnubChat* Chat)
	{
		if (!Chat)
		{
			return nullptr;
		}
		
		// Use reflection to access the private "ObjectsRepository" UPROPERTY
		FProperty* RepositoryProperty = Chat->GetClass()->FindPropertyByName(TEXT("ObjectsRepository"));
		if (!RepositoryProperty)
		{
			return nullptr;
		}
		
		// Get the value of the property
		TObjectPtr<UPubnubChatObjectsRepository>* RepositoryPtr = RepositoryProperty->ContainerPtrToValuePtr<TObjectPtr<UPubnubChatObjectsRepository>>(Chat);
		if (!RepositoryPtr)
		{
			return nullptr;
		}
		
		return RepositoryPtr->Get();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS

