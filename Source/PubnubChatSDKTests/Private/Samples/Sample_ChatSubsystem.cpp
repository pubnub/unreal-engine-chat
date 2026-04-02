#include "Samples/Sample_ChatSubsystem.h"


// snippet.init_chat

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::InitChatSample()
{
	// Get PubnubChatSubsystem from GameInstance
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();
	
	// Initialize Chat - InitChat may fail under some conditions so make sure to check the Result for errors before using the Chat
	FPubnubChatInitChatResult InitChatResult = PubnubChatSubsystem->InitChat(TEXT("demo"), TEXT("demo"), TEXT("Player_001"));
	UPubnubChat* PubnubChat = InitChatResult.Chat;
}

// snippet.init_chat_with_config

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::InitChatWithConfigSample()
{
	// Get PubnubChatSubsystem from GameInstance
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();
	
	// Create config with  AuthKey and enable Chat to store user activity
	FPubnubChatConfig Config;
	Config.AuthKey = TEXT("p0F2AkF0GmheUpNDdHRsGDxDcmVzpURjaGFuoWtnbG9iYWxfY2hhdANDZ3JwoENzcGOgQ3VzcqBEdXVpZKBDcGF0pURjaGFuoENncnCgQ3NwY6BDdXNyoER1dWlkoERtZXRhoENzaWdYILa9OLrP_dhe31sW_seO2r9KhD6mp9Yi9vZxcX9QY04R");
	Config.StoreUserActivityInterval = true;
	
	// Initialize Chat - InitChat may fail under some conditions so make sure to check the Result for errors before using the Chat
	FPubnubChatInitChatResult InitChatResult = PubnubChatSubsystem->InitChat(TEXT("demo"), TEXT("demo"), TEXT("Player_001"), Config);
	UPubnubChat* PubnubChat = InitChatResult.Chat;
}

// snippet.init_chat_with_pubnub_client

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"
#include "PubnubSubsystem.h"
#include "PubnubClient.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::InitChatWithPubnubClientSample()
{
	// Get GameInstance and both subsystems (Chat and base Pubnub)
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();
	UPubnubSubsystem* PubnubSubsystem = GameInstance->GetSubsystem<UPubnubSubsystem>();

	// Create a Pubnub client (e.g. shared with presence or pub/sub elsewhere in your game)
	FPubnubConfig ClientConfig;
	ClientConfig.PublishKey = TEXT("demo");
	ClientConfig.SubscribeKey = TEXT("demo");
	ClientConfig.UserID = TEXT("Player_001");
	UPubnubClient* PubnubClient = PubnubSubsystem->CreatePubnubClient(ClientConfig);

	// Initialize Chat using that client; check Result for errors before using Chat
	FPubnubChatInitChatResult InitChatResult = PubnubChatSubsystem->InitChatWithPubnubClient(TEXT("Player_001"), PubnubClient);
	UPubnubChat* PubnubChat = InitChatResult.Chat;
}

// snippet.get_chat

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::GetChatSample()
{
	// Get PubnubChatSubsystem from GameInstance (e.g. from your player controller or HUD)
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();

	// Retrieve the chat for this player (must have been initialized earlier, e.g. on login)
	UPubnubChat* Chat = PubnubChatSubsystem->GetChat(TEXT("Player_001"));

	// Use the chat only if it exists
	if (Chat)
	{
		// Chat is ready for sending messages, joining channels, etc.
	}
}

// snippet.destroy_chat

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::DestroyChatSample()
{
	// Get PubnubChatSubsystem from GameInstance
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();

	// Destroy chat for this user (e.g. when player logs out or leaves the match)
	PubnubChatSubsystem->DestroyChat(TEXT("Player_001"));
}

// snippet.destroy_all_chats

#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "PubnubChatSubsystem.h"

// ACTION REQUIRED: Replace ASample_ChatSubsystem with name of your Actor class
void ASample_ChatSubsystem::DestroyAllChatsSample()
{
	// Get PubnubChatSubsystem from GameInstance
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	UPubnubChatSubsystem* PubnubChatSubsystem = GameInstance->GetSubsystem<UPubnubChatSubsystem>();

	// Tear down all chats (e.g. when returning to main menu or shutting down)
	PubnubChatSubsystem->DestroyAllChats();
}

// snippet.end
