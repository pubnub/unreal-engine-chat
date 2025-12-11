// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"

#include "PubnubChatChannel.generated.h"

class UPubnubClient;
class UPubnubSubscription;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceived, UPubnubChatMessage*, PubnubMessage);
DECLARE_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceivedNative, UPubnubChatMessage* PubnubMessage);

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatChannel : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChannel;
public:

	virtual void BeginDestroy() override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatChannelData GetChannelData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FString GetChannelID() const { return ChannelID; }

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatConnectResult Connect(FOnPubnubChatChannelMessageReceived MessageCallback);

	FPubnubChatConnectResult Connect(FOnPubnubChatChannelMessageReceivedNative MessageCallbackNative);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Disconnect();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult SendText(const FString Message, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatInviteResult Invite(UPubnubChatUser* User);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
    FPubnubChatInviteMultipleResult InviteMultiple(TArray<UPubnubChatUser*> Users);

	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString ChannelID = "";
	UPROPERTY()
	UPubnubSubscription* ConnectSubscription = nullptr;
	

	bool IsInitialized = false;

	void InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID);
};

