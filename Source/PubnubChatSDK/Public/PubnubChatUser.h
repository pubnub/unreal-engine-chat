// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatUser.generated.h"

class UPubnubClient;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatUser : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	FPubnubUserData UserData;

	bool IsInitialized = false;

	void InitUser(UPubnubClient* InPubnubClient, const FPubnubUserData& InUserData);
};
