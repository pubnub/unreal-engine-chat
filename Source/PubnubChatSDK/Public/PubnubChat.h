// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"


#include "PubnubChat.generated.h"

class UPubnubClient;
class UPubnubChatUser;
class UPubnubChatChannel;
class UPubnubChatObjectsRepository;
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
	friend class UPubnubChatUser;
	friend class UPubnubChatChannel;
	friend class UPubnubChatMessage;
	
public:


	/*  DELEGATES  */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyedNative;

	//TODO:: decide if delegate names should have "PubnubChat" or "Chat" or without
	/**Listener to react for connection status changed */
	UPROPERTY(BlueprintAssignable, Category = "PubnubChat|Delegates")
	FOnPubnubChatConnectionStatusChanged OnConnectionStatusChanged;

	/**Listener to react for connection status changed , equivalent that accepts lambdas*/
	FOnPubnubChatConnectionStatusChangedNative OnConnectionStatusChangedNative;


	/*  GENERAL  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat")
	void DestroyChat();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat")
	UPubnubClient* GetPubnubClient() const {return PubnubClient;}
	
	/*  USER  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	UPubnubChatUser* GetCurrentUser();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult CreateUser(const FString UserID, FPubnubChatUserData UserData = FPubnubChatUserData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult GetUser(const FString UserID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUsersResult GetUsers(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult UpdateUser(const FString UserID, FPubnubChatUserData UserData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult DeleteUser(const FString UserID, bool Soft = false);


	/*  CHANNEL  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult GetChannel(const FString ChannelID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetChannelsResult GetChannels(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult UpdateChannel(const FString ChannelID, FPubnubChatChannelData ChannelData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult DeleteChannel(const FString ChannelID, bool Soft = false);

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;

	UPROPERTY()
	TObjectPtr<UPubnubChatUser> CurrentUser = nullptr;

	UPROPERTY()
	FPubnubChatConfig ChatConfig;

	/** Repository that manages shared data for all chat objects */
	UPROPERTY()
	TObjectPtr<UPubnubChatObjectsRepository> ObjectsRepository = nullptr;
	
	UPROPERTY()
	bool IsInitialized = false;
	
	UFUNCTION()
	void OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData);

	FPubnubChatInitChatResult InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);
	FPubnubChatUserResult GetUserForInit(const FString InUserID);

	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubChatUserData& ChatUserData);
	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubUserData& UserData);

	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChatChannelData& ChatChannelData);
	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChannelData& ChannelData);

	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData);
	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData);
	
};

