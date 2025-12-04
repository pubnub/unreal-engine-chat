// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
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


	/*  DELEGATES  */
	
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


	/*  GENERAL  */

	UFUNCTION(BlueprintCallable, Category="PubnubChat")
	void DestroyChat();

	
	/*  USER  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	UPubnubChatUser* GetCurrentUser();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult CreateUser(const FString UserID, FPubnubChatUserData UserData = FPubnubChatUserData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult GetUser(const FString UserID);

	//TODO:: GetUsers

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult UpdateUser(const FString UserID, FPubnubChatUserData UserData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult DeleteUser(const FString UserID, bool Soft = false);


private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;

	UPROPERTY()
	TObjectPtr<UPubnubChatUser> CurrentUser = nullptr;

	UPROPERTY()
	FPubnubChatConfig ChatConfig;
	
	UPROPERTY()
	bool IsInitialized = false;
	
	UFUNCTION()
	void OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData);

	FPubnubChatInitChatResult InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);
	FPubnubChatUserResult GetUserForInit(const FString InUserID);


	
	

};
