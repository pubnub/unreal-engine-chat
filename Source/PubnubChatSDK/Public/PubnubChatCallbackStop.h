// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatCallbackStop.generated.h"


/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatCallbackStop : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChatUser;
	friend class UPubnubChatChannel;
	friend class UPubnubChatMessage;

public:
	virtual void BeginDestroy() override;
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|CallbackStop")
	FPubnubChatOperationResult Stop();

private:
	void InitCallbackStop(TFunction<FPubnubChatOperationResult()> InStopCallback);

	bool IsStopped = false;
	
	TFunction<FPubnubChatOperationResult()> StopCallback = nullptr;
};

