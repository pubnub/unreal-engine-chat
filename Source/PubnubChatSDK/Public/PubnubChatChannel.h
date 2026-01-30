// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"
#include "HAL/CriticalSection.h"

#include "PubnubChatChannel.generated.h"

class UPubnubClient;
class UPubnubSubscription;
class UPubnubChat;
class UPubnubChatUser;
class UPubnubChatMessage;
class UPubnubChatCallbackStop;
class UPubnubChatMessageDraft;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceived, UPubnubChatMessage*, PubnubMessage);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatChannelMessageReceivedNative, UPubnubChatMessage* PubnubMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatChannelUpdateReceived, EPubnubChatStreamedUpdateType, UpdateType, FString, ChannelID, FPubnubChatChannelData, ChannelData);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnPubnubChatChannelUpdateReceivedNative, EPubnubChatStreamedUpdateType UpdateType, FString ChannelID, const FPubnubChatChannelData& ChannelData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatTypingReceived, const TArray<FString>&, TypingUserIDs);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatTypingReceivedNative, const TArray<FString>& TypingUserIDs);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatReadReceiptReceived, FPubnubChatReadReceipts, ReadReceipts);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatReadReceiptReceivedNative, const FPubnubChatReadReceipts& ReadReceipts);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReportReceived, FPubnubChatEvent, ReportEvent);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatMessageReportReceivedNative, const FPubnubChatEvent& ReportEvent);


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChatChannel : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
public:

	virtual void BeginDestroy() override;
	
	/* DELEGATES */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatChannelMessageReceived OnMessageReceived;
	FOnPubnubChatChannelMessageReceivedNative OnMessageReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatChannelUpdateReceived OnChannelUpdateReceived;
	FOnPubnubChatChannelUpdateReceivedNative OnChannelUpdateReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatTypingReceived OnTypingReceived;
	FOnPubnubChatTypingReceivedNative OnTypingReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatReadReceiptReceived OnReadReceiptReceived;
	FOnPubnubChatReadReceiptReceivedNative OnReadReceiptReceivedNative;
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatMessageReportReceived OnMessageReportReceived;
	FOnPubnubChatMessageReportReceivedNative OnMessageReportReceivedNative;
	
	
	/* PUBLIC FUNCTIONS */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FPubnubChatChannelData GetChannelData() const;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Chat|Channel")
	FString GetChannelID() const { return ChannelID; }
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Channel")
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
	static FPubnubChatOperationResult StreamUpdatesOn(const TArray<UPubnubChatChannel*>& Channels);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingUpdates();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StartTyping();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopTyping();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamTyping();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingTyping();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamReadReceipts();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingReadReceipts();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StreamMessageReports();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult StopStreamingMessageReports();
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatEventsResult GetMessageReportsHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count = 100);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	UPubnubChatMessageDraft* CreateMessageDraft(FPubnubChatMessageDraftConfig MessageDraftConfig = FPubnubChatMessageDraftConfig());
	
protected:
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
	UPROPERTY()
	UPubnubChatCallbackStop* TypingCallbackStop = nullptr;
	UPROPERTY()
	UPubnubChatCallbackStop* MessageReportsCallbackStop = nullptr;

	bool IsInitialized = false;
	bool IsStreamingUpdates = false;
	bool IsConnected = false;
	bool IsStreamingTyping = false;
	bool IsStreamingReadReceipts = false;
	bool IsStreamingMessageReports = false;
	
	FDateTime LastTypingEventTime = FDateTime::MinValue();
	TMap<FString, FTypingIndicatorData> TypingIndicators;
	
	/** Critical section for thread-safe access to typing indicators */
	mutable FCriticalSection TypingIndicatorsCriticalSection;

	/** Rate limiting state for SendText operations */
	FDateTime LastSendTextTime = FDateTime::MinValue();
	int32 SendTextRateLimitPenalty = 0;
	mutable FCriticalSection SendTextRateLimitCriticalSection;

	/**
	 * Calculates delay needed before sending text based on rate limiting with exponential backoff.
	 * @return Delay in seconds (0.0 = can send immediately)
	 */
	float CalculateSendTextRateLimiterDelay();

	void InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID);
	
	FPubnubChatGetRestrictionsResult GetRestrictions(const int Limit = 0, const FString Filter = "", FPubnubMemberSort Sort = FPubnubMemberSort(), FPubnubPage Page = FPubnubPage());
	
	//This function is for ThreadChannel which does additional logic during SentText (SendText as UFUNCTION can't be directly overriden)
	virtual FPubnubChatOperationResult OnSendText();
	
	//Add calling OnMessageReceived to ConnectSubscription. Virtual as Thread Channel will override it to use OnThreadMessageReceived
	virtual void AddOnMessageReceivedLambdaToSubscription(TWeakObjectPtr<UPubnubChatChannel> ThisChannelWeak);
	
	UFUNCTION()
	void OnChatDestroyed(FString UserID);
	void ClearAllSubscriptions();
	void CleanUp();
};

