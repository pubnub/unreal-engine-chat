// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
#include "Sample_ChatSubsystem.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatSubsystem : public AActor
{
	GENERATED_BODY()

public:
	
	// snippet.init_chat
	// blueprint.imo3pku9
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatSample();
	
	// snippet.init_chat_with_config
	// blueprint.80b0pb_w
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatWithConfigSample();

	// snippet.init_chat_with_pubnub_client
	// blueprint.2r01674r
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void InitChatWithPubnubClientSample();

	// snippet.get_chat
	// blueprint.9jzkm2a2
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void GetChatSample();

	// snippet.destroy_chat
	// blueprint.twfc2-7q
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void DestroyChatSample();

	// snippet.destroy_all_chats
	// blueprint.t2u8e17d
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatSubsystem")
	void DestroyAllChatsSample();
	
	// snippet.end
	
};
