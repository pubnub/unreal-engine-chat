// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChat.generated.h"

class UPubnubClient;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPubnubChatDestroyed);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChat : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChatSubsystem;
	
public:

	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat")
	void DestroyChat();


private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;

	UPROPERTY()
	FPubnubChatConfig ChatConfig;

	bool IsInitialized = false;

	void InitChat(FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);

};
