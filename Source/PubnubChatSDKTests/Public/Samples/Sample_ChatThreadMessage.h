// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "GameFramework/Actor.h"
// snippet.includes
#include "PubnubChat.h"
#include "PubnubChatThreadMessage.h"

// snippet.end

#include "Sample_ChatThreadMessage.generated.h"

UCLASS()
class PUBNUBCHATSDKTESTS_API ASample_ChatThreadMessage : public AActor
{
	GENERATED_BODY()

public:

	// snippet.get_parent_channel_id
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void GetParentChannelIDSample();

	// snippet.pin_message_to_parent_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void PinMessageToParentChannelSample();

	// snippet.unpin_message_from_parent_channel
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void UnpinMessageFromParentChannelSample();

	// snippet.end
};
