// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"

#include "PubnubChatUser.generated.h"

class UPubnubClient;
class UPubnubChat;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatUser : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FPubnubChatUserData GetUserData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|User")
	FString GetUserID() const { return UserID; }

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString UserID = "";
	

	bool IsInitialized = false;

	void InitUser(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InUserID);
};
