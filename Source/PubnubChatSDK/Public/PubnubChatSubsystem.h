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
	UPubnubChat* GetChat(FString UserID);

	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	void DestroyChat(FString UserID);
	
	UFUNCTION(BlueprintCallable, Category = "PubnubChat")
	void DestroyAllChats();

private:
	UPROPERTY()
	TMap<FString, UPubnubChat*> Chats;
	
	UFUNCTION()
	void OnChatDestroyed(FString UserID);

	UPubnubClient* CreatePubnubClient(FString PublishKey, FString SubscribeKey, FString UserID);

};