// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"

#include "PubnubChatMessage.generated.h"

class UPubnubClient;
class UPubnubChat;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatMessage : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FPubnubChatMessageData GetMessageData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Message")
	FString GetMessageTimetoken() const { return Timetoken; }

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString Timetoken = "";
	UPROPERTY()
	FString ChannelID = "";
	

	bool IsInitialized = false;

	void InitMessage(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID, const FString InTimetoken);

	/**
	 * Gets the internal composite message ID used for repository operations.
	 * Format: [ChannelID].[Timetoken]
	 * @return Composite message identifier
	 */
	FString GetInternalMessageID() const;
};

