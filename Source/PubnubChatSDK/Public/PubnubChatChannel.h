// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"

#include "PubnubChatChannel.generated.h"

class UPubnubClient;
class UPubnubSubscription;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceived, UPubnubChatMessage*, PubnubMessage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceivedNative, UPubnubChatMessage* PubnubMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatChannelUpdateReceived, EPubnubChatStreamedUpdateType, UpdateType, FString, ChannelID, FPubnubChatChannelData, ChannelData);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatChannelUpdateReceivedNative, EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, const FPubnubChatChannelData& ChannelData);

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
	
	/* DELEGATES */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatChannelMessageReceived OnMessageReceived;
	FOnPubnubChatChannelMessageReceivedNative OnMessageReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatChannelUpdateReceived OnChannelUpdateReceived;
	FOnPubnubChatChannelUpdateReceivedNative OnChannelUpdateReceivedNative;
	
	/* PUBLIC FUNCTIONS */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatChannelData GetChannelData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FString GetChannelID() const { return ChannelID; }
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Update(FPubnubChatUpdateChannelInputData UpdateChannelData);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatOperationResult Connect();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
	FPubnubChatJoinResult Join(FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData());
	
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
	FPubnubChatMessageResult GetPinnedMessage();
	
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
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetHistoryResult GetHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 25);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatMessageResult GetMessage(const FString Timetoken);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult ForwardMessage(UPubnubChatMessage* Message);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult EmitUserMention(const FString UserID, const FString Timetoken, const FString Text);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingUpdates();
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChat> Chat = nullptr;
	UPROPERTY()
	FString ChannelID = "";
	UPROPERTY()
	UPubnubSubscription* ConnectSubscription = nullptr;
	UPROPERTY()
	UPubnubSubscription* UpdatesSubscription = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;
	bool IsConnected = false;

	void InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID);
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	void ClearAllSubscriptions();
};

