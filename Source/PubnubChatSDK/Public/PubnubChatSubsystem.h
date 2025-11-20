// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatSubsystem.generated.h"

class UPubnubClient;
class UPubnubChat;

DECLARE_LOG_CATEGORY_EXTERN(PubnubChatLog, Log, All);


UCLASS()
class PUBNUBCHATSDK_API UPubnubChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat")
	UPubnubChat* InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config = FPubnubChatConfig());

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat")
	UPubnubChat* GetChat();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat")
	void DestroyChat();

private:
	UPROPERTY()
	UPubnubChat* Chat = nullptr;
	
	UFUNCTION()
	void OnChatDestroyed();

	UPubnubClient* CreatePubnubClient(FString PublishKey, FString SubscribeKey, FString UserID);

};