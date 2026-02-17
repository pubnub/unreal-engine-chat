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

	// snippet.thread_message_get_parent_id
	// blueprint._3iklcva
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void GetParentChannelIDSample();

	// snippet.thread_message_pin_to_parent
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void PinMessageToParentChannelSample();

	// snippet.thread_message_unpin_from_parent
	UFUNCTION(BlueprintCallable, Category = "PubnubChat|Samples|ChatThreadMessage")
	void UnpinMessageFromParentChannelSample();

	// snippet.end
};
