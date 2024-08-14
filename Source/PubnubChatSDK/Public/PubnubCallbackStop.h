// Fill out your copyright notice in the Description page of Project Settings.

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

	static UPubnubCallbackStop* Create(Pubnub::CallbackStop CallbackStop);
	~UPubnubCallbackStop();

	UFUNCTION(BlueprintCallable)
	void Stop();

protected:
	Pubnub::CallbackStop* InternalCallbackStop;
};
