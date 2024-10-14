// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PubnubChatSubsystem.generated.h"

class UPubnubChat;

DECLARE_LOG_CATEGORY_EXTERN(PubnubChatLog, Log, All);

USTRUCT(BlueprintType)
struct FPubnubChatConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") FString AuthKey = "";
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int TypingTimeout = 5000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int TypingTimeoutDifference = 1000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") int StoreUserActivityInterval = 600000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Config") bool StoreUserActivityTimestamps = false;
	
};



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

};