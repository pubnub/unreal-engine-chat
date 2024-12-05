// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <pubnub_chat/callback_stop.hpp>
#include "PubnubCallbackStop.generated.h"


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubCallbackStop : public UObject
{
	GENERATED_BODY()
public:
	
	~UPubnubCallbackStop();

	UFUNCTION(BlueprintCallable, Category = "CallbackStop")
	void Stop();

	//Internal usage only
	static UPubnubCallbackStop* Create(Pubnub::CallbackStop CallbackStop);

protected:
	Pubnub::CallbackStop* InternalCallbackStop;
};
