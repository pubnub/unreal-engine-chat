// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"

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

	virtual void BeginDestroy() override;

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	FPubnubChatUserData UserData;
	UPROPERTY()
	FString UserID = "";
	

	bool IsInitialized = false;

	void InitUser(UPubnubClient* InPubnubClient, const FString InUserID, const FPubnubChatUserData& InUserData);
};
