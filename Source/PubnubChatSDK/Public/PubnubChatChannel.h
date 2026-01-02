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
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Update(FPubnubChatChannelData ChannelData);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatConnectResult Connect(FOnPubnubChatChannelMessageReceived MessageCallback);

	FPubnubChatConnectResult Connect(FOnPubnubChatChannelMessageReceivedNative MessageCallbackNative);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatJoinResult Join(FOnPubnubChatChannelMessageReceived MessageCallback, FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());

	FPubnubChatJoinResult Join(FOnPubnubChatChannelMessageReceivedNative MessageCallbackNative, FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Disconnect();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Leave();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult SendText(const FString Message, FPubnubChatSendTextParams SendTextParams = FPubnubChatSendTextParams());
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatInviteResult Invite(UPubnubChatUser* User);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
    FPubnubChatInviteMultipleResult InviteMultiple(TArray<UPubnubChatUser*> Users);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult PinMessage(UPubnubChatMessage* Message);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult UnpinMessage();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatWhoIsPresentResult WhoIsPresent(int Limit = 1000, int Offset = 0);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatIsPresentResult IsPresent(const FString UserID);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Delete(bool Soft = false);
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Restore();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatIsDeletedResult IsDeleted();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMembershipsResult GetMembers(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMembershipsResult GetInvitees(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult SetRestrictions(const FString UserID, bool Ban, bool Mute, FString Reason = "");
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionResult GetUserRestrictions(UPubnubChatUser* User);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetRestrictionsResult GetUsersRestrictions(const int Limit = 0, FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
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
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
};

