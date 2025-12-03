// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChat.h"
#include "PubnubChatSubsystem.generated.h"

class UPubnubClient;

DECLARE_LOG_CATEGORY_EXTERN(PubnubChatLog, Log, All);


UCLASS()
class PUBNUBCHATSDK_API UPubnubChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	FPubnubChatInitChatResult InitChat(FString PublishKey, FString SubscribeKey, FString UserID, FPubnubChatConfig Config = FPubnubChatConfig());

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PubnubChat")
	UPubnubChat* GetChat();

	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	void DestroyChat();

private:
	UPROPERTY()
	UPubnubChat* Chat = nullptr;
	
	UFUNCTION()
	void OnChatDestroyed();

	UPubnubClient* CreatePubnubClient(FString PublishKey, FString SubscribeKey, FString UserID);

};