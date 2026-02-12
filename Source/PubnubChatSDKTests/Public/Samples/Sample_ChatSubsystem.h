// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Sample_ChatSubsystem.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatSubsystem : public AActor
{
	GENERATED_BODY()

public:
	
	// snippet.init_chat
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatSample();
	
	// snippet.init_chat_with_config
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatWithConfigSample();

	// snippet.init_chat_with_pubnub_client
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatWithPubnubClientSample();

	// snippet.get_chat
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void GetChatSample();

	// snippet.destroy_chat
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void DestroyChatSample();

	// snippet.destroy_all_chats
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void DestroyAllChatsSample();
	
	// snippet.end
	
};
