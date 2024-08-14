// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChannel.h"
#include "pubnub_chat/thread_channel.hpp"
#include "PubnubThreadChannel.generated.h"


class UPubnubMessage;
class UPubnubThreadMessage;

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubThreadChannel : public UPubnubChannel
{
	GENERATED_BODY()
	
public:
	static UPubnubThreadChannel* Create(Pubnub::ThreadChannel ThreadChannel);
	~UPubnubThreadChannel() override {delete InternalChannel;}

	UFUNCTION(BlueprintCallable, BlueprintPure,  Category = "Pubnub Thread Channel")
	UPubnubMessage* GetParentMessage();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Thread Channel")
	FString GetParentChannelID();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Channel")
	TArray<UPubnubThreadMessage*> GetThreadHistory(int Limit, FString Start, FString End);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Channel")
	UPubnubThreadChannel* PinMessageToThread(UPubnubThreadMessage* ThreadMessage);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Channel")
	UPubnubThreadChannel* UnpinMessageFromThread();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Channel")
	UPubnubChannel* PinMessageToParentChannel(UPubnubThreadMessage* ThreadMessage);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Channel")
	UPubnubChannel* UnpinMessageFromParentChannel();

	//Internal usage only
	Pubnub::ThreadChannel* GetInternalThreadChannel();

	bool IsInternalThreadChannelValid();
};
