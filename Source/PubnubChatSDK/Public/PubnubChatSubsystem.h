// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatSubsystem.generated.h"

class UPubnubChat;

DECLARE_LOG_CATEGORY_EXTERN(PubnubLog, Log, All);

USTRUCT(BlueprintType)
struct FPubnubChatConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere) FString AuthKey = "";
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int TypingTimeout = 5000;
	UPROPERTY(BlueprintReadWrite, EditAnywhere) int TypingTimeoutDifference = 1000;
	//UPROPERTY(BlueprintReadWrite, EditAnywhere) int StoreUserActivityInterval = 600000;
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

private:
	UPubnubChat* Chat = nullptr;

};