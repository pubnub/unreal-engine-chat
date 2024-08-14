// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PubnubMessage.h"
#include "pubnub_chat/thread_message.hpp"
#include "PubnubThreadMessage.generated.h"


class UPubnubThreadMessage;
class UPubnubChannel;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubThreadMessagesStreamUpdateOnReceived, const TArray<UPubnubThreadMessage*>&, PubnubThreadMessages);


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubThreadMessage : public UPubnubMessage
{
	GENERATED_BODY()
	
public:
	static UPubnubThreadMessage* Create(Pubnub::ThreadMessage ThreadMessage);
	~UPubnubThreadMessage() override {delete InternalMessage;}

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Thread Message")
	FString GetParentChannelID();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Message")
	UPubnubChannel* PinToParentChannel();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Thread Message")
	UPubnubChannel* UnpinFromParentChannel();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Message")
	UPubnubCallbackStop* StreamThreadMessageUpdatesOn(TArray<UPubnubThreadMessage*> Messages, FOnPubnubThreadMessagesStreamUpdateOnReceived MessageUpdateCallback);

	
	//Internal usage only
	Pubnub::ThreadMessage* GetInternalThreadMessage();

	bool IsInternalThreadMessageValid();
};
