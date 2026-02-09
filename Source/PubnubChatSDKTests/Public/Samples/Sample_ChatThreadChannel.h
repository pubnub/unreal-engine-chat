// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

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

	// snippet.get_parent_channel_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetParentChannelIDSample();

	// snippet.get_parent_message
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetParentMessageSample();

	// snippet.get_thread_history
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void GetThreadHistorySample();

	UFUNCTION()
	void OnGetThreadHistoryResponse(const FPubnubChatGetThreadHistoryResult& Result);

	// snippet.pin_message_to_parent_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void PinMessageToParentChannelSample();

	// snippet.unpin_message_from_parent_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void UnpinMessageFromParentChannelSample();

	// snippet.listen_thread_messages
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadChannel")
	void ListenThreadMessagesSample();

	void OnThreadMessageReceived(UPubnubChatThreadMessage* ThreadMessage);

	// snippet.end
};
