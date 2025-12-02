// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "PubnubChatUser.h"


#include "PubnubChat.generated.h"

class UPubnubClient;
enum class EPubnubSubscriptionStatus  : uint8;
struct FPubnubSubscriptionStatusData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPubnubChatDestroyed);
DECLARE_MULTICAST_DELEGATE(FOnPubnubChatDestroyedNative);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChanged, EPubnubChatConnectionStatus, Status, const FPubnubChatConnectionStatusData&, StatusData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChangedNative, EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData);

/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChat : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChatSubsystem;
	
public:

	UPROPERTY(BlueprintAssignable, Category = "PubnubChat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "PubnubChat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyedNative;

	//TODO:: decide if delegate names should have "PubnubChat" or "Chat" or without
	/**Listener to react for connection status changed */
	UPROPERTY(BlueprintAssignable, Category = "PubnubChat|Delegates")
	FOnPubnubChatConnectionStatusChanged OnConnectionStatusChanged;

	/**Listener to react for connection status changed , equivalent that accepts lambdas*/
	FOnPubnubChatConnectionStatusChangedNative OnConnectionStatusChangedNative;

	UFUNCTION(BlueprintCallable, Category="PubnubChat")
	void DestroyChat();

	UFUNCTION(BlueprintCallable)
	UPubnubChatUser* GetUserForInit(const FString UserID);


private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;

	UPROPERTY()
	FPubnubChatConfig ChatConfig;

	bool IsInitialized = false;

	void InitChat(const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);
	UFUNCTION()
	void OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData);
	
	

};
