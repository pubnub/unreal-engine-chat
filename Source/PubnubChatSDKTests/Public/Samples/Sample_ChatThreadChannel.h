// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatThreadChannel.h"
#include "PubnubChatThreadMessage.h"
#include "PubnubChatMessage.h"

// snippet.end

#include "Sample_ChatThreadChannel.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatThreadChannel : public AActor
{
	GENERATED_BODY()

public:

	// snippet.thread_channel_get_parent_id
	// blueprint.uvu7fzlh
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetParentChannelIDSample();

	// snippet.get_parent_message
	// blueprint.q7d34byc
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetParentMessageSample();

	// snippet.get_thread_history
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetThreadHistorySample();

	UFUNCTION()
	void OnGetThreadHistoryResponse(const FPubnubChatGetThreadHistoryResult& Result);

	// snippet.thread_channel_pin_to_parent
	// blueprint.9j_ejg0c
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void PinMessageToParentChannelSample();

	// snippet.thread_channel_unpin_from_parent
	// blueprint.qf5ev_rc
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void UnpinMessageFromParentChannelSample();

	// snippet.listen_thread_messages
	// blueprint.c_8uhs0k
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void ListenThreadMessagesSample();

	void OnThreadMessageReceived(UPubnubChatThreadMessage* ThreadMessage);

	// snippet.end
};
