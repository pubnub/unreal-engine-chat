// Copyright 2024 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <pubnub_chat/channel.hpp>
#include "UObject/NoExportTypes.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChannel.generated.h"

class UPubnubMessage;
class UPubnubChannel;
class UPubnubMembership;
class UPubnubUser;
class UPubnubCallbackStop;
class UPubnubMessageDraft;


DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelMessageReceived, UPubnubMessage*, PubnubMessage);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelStreamUpdateReceived, UPubnubChannel*, PubnubChannel);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelsStreamUpdateOnReceived, const TArray<UPubnubChannel*>&, PubnubChannels);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelStreamPresenceReceived, const TArray<FString>&, PresentUsersIDs);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelTypingReceived, const TArray<FString>&, TypingUsersIDs);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChannelStreamReadReceiptsReceived, FPubnubReadReceiptsWrapper, Receipts);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubStreamMessageReportsReceived, FPubnubEvent, Event);



USTRUCT(BlueprintType)
struct FPubnubMembersResponseWrapper
{
	GENERATED_BODY();
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubMembership*> Memberships;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status;

	FPubnubMembersResponseWrapper() = default;
	FPubnubMembersResponseWrapper(Pubnub::MembersResponseWrapper& Wrapper);
	
};

USTRUCT(BlueprintType)
struct FPubnubUsersRestrictionsWrapper
{
	GENERATED_BODY();
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubUserRestriction> Restrictions;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status;

	FPubnubUsersRestrictionsWrapper() = default;
	FPubnubUsersRestrictionsWrapper(Pubnub::UsersRestrictionsWrapper& Wrapper);
	
};

USTRUCT(BlueprintType)
struct FPubnubMessageReportsHistoryWrapper
{
	GENERATED_BODY();

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<FPubnubEvent> Events;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") bool IsMore;

	FPubnubMessageReportsHistoryWrapper() = default;
	FPubnubMessageReportsHistoryWrapper(Pubnub::EventsHistoryWrapper& Wrapper) :
	IsMore(Wrapper.is_more)
	{
		auto CppEvents = Wrapper.events.into_std_vector();
		for(auto Event : CppEvents)
		{
			Events.Add(Event);
		}
	}
};


/**
 * 
 */
UCLASS(BlueprintType)
class PUBNUBCHATSDK_API UPubnubChannel : public UObject
{
	GENERATED_BODY()
	
public:
	virtual ~UPubnubChannel();
	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Channel")
	FString GetChannelID();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Channel")
	FPubnubChatChannelData GetChannelData();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubChannel* Update(FPubnubChatChannelData ChannelData); 

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void Connect(FOnPubnubChannelMessageReceived MessageCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void Disconnect();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void Join(FOnPubnubChannelMessageReceived MessageCallback, FString CustomData = "");

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void Leave();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void DeleteChannel();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void SendText(FString Message, FPubnubSendTextParams SendTextParams = FPubnubSendTextParams());

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* StreamUpdates(FOnPubnubChannelStreamUpdateReceived ChannelUpdateCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* StreamUpdatesOn(TArray<UPubnubChannel*> Channels, FOnPubnubChannelsStreamUpdateOnReceived ChannelUpdateCallback);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* StreamPresence(FOnPubnubChannelStreamPresenceReceived PresenceCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	TArray<FString> WhoIsPresent();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	bool IsPresent(FString UserID);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void SetRestrictions(FString UserID, FPubnubRestriction Restrictions);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	FPubnubRestriction GetUserRestrictions(UPubnubUser* User);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	FPubnubUsersRestrictionsWrapper GetUsersRestrictions(FString Sort = "", int Limit = 0, FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubMessage* GetMessage(FString Timetoken);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	TArray<UPubnubMessage*> GetHistory(FString StartTimetoken, FString EndTimetoken, int Count = 25);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	FPubnubMembersResponseWrapper GetMembers(FString Filter = "", FString Sort = "", int Limit = 0, FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubMembership* Invite(UPubnubUser* User);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	TArray<UPubnubMembership*> InviteMultiple(TArray<UPubnubUser*> Users);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void StartTyping();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void StopTyping();
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* GetTyping(FOnPubnubChannelTypingReceived TypingCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubChannel* PinMessage(UPubnubMessage* Message);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubChannel* UnpinMessage();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubMessage* GetPinnedMessage();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void ForwardMessage(UPubnubMessage* Message);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	void EmitUserMention(FString UserID, FString Timetoken, FString Text);
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* StreamReadReceipts(FOnPubnubChannelStreamReadReceiptsReceived ReadReceiptsCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubCallbackStop* StreamMessageReports(FOnPubnubStreamMessageReportsReceived MessageReportsCallback);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	FPubnubMessageReportsHistoryWrapper GetMessageReportsHistory(FString StartTimetoken, FString EndTimetoken, int Count = 100);

	UFUNCTION(BlueprintCallable, Category = "Pubnub Channel")
	UPubnubMessageDraft* CreateMessageDraft(FPubnubMessageDraftConfig MessageDraftConfig = FPubnubMessageDraftConfig());
	
	//Internal usage only
	static UPubnubChannel* Create(Pubnub::Channel Channel);
	
	//Internal usage only
	Pubnub::Channel* GetInternalChannel(){return InternalChannel;};

	
protected:
	Pubnub::Channel* InternalChannel;

	bool IsInternalChannelValid();
	bool IsThreadChannel = false;
};
