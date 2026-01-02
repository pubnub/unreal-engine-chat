// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "PubnubChatEnumLibrary.h"
#include "StructLibraries/PubnubChatMessageStructLibrary.h"


#include "PubnubChat.generated.h"

class UPubnubClient;
class UPubnubSubscription;
class UPubnubChatUser;
class UPubnubChatChannel;
class UPubnubChatMembership;
class UPubnubChatAccessManager;
class UPubnubChatObjectsRepository;
enum class EPubnubSubscriptionStatus  : uint8;
struct FPubnubSubscriptionStatusData;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatDestroyed, FString, UserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatDestroyedNative, FString UserID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChanged, EPubnubChatConnectionStatus, Status, const FPubnubChatConnectionStatusData&, StatusData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChangedNative, EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatEventReceived, FPubnubChatEvent, Event);
DECLARE_DELEGATE_OneParam(FOnPubnubChatEventReceivedNative, const FPubnubChatEvent& Event);



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
	friend class UPubnubChatMembership;
	
public:


	/*  DELEGATES  */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	/**Listener to react for chat destroyed, equivalent that accepts lambdas*/
	FOnPubnubChatDestroyedNative OnChatDestroyedNative;

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
	UPubnubClient* GetPubnubClient() const { return PubnubClient; }

	
	/*  USER  */

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|User")
	UPubnubChatUser* GetCurrentUser() const { return CurrentUser; }
	
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

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUserSuggestionsResult GetUserSuggestions(const FString Text, int Limit = 10);

	
	/*  CHANNEL  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateGroupConversationResult CreateGroupConversation(TArray<UPubnubChatUser*> Users, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateDirectConversationResult CreateDirectConversation(UPubnubChatUser* User, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult GetChannel(const FString ChannelID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetChannelsResult GetChannels(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult UpdateChannel(const FString ChannelID, FPubnubChatChannelData ChannelData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult DeleteChannel(const FString ChannelID, bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult PinMessageToChannel(UPubnubChatMessage* Message, UPubnubChatChannel* Channel);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult UnpinMessageFromChannel(UPubnubChatChannel* Channel);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Chanenl")
	FPubnubChatGetChannelSuggestionsResult GetChannelSuggestions(const FString Text, int Limit = 10);

	
	/* PRESENCE */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWherePresentResult WherePresent(const FString UserID);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWhoIsPresentResult WhoIsPresent(const FString ChannelID, int Limit = 1000, int Offset = 0);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatIsPresentResult IsPresent(const FString UserID, const FString ChannelID);


	/* MODERATION */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatOperationResult SetRestrictions(FPubnubChatRestriction Restriction);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatOperationResult EmitChatEvent(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, EPubnubChatEventMethod EventMethod = EPubnubChatEventMethod::PCEM_Default);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatListenForEventsResult ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceived EventCallback);
	FPubnubChatListenForEventsResult ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceivedNative EventCallbackNative);


	/* ACCESS MANAGER */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|Access Manager")
	UPubnubChatAccessManager* GetAccessManager() const { return AccessManager; }
	
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatUser> CurrentUser = nullptr;
	UPROPERTY()
	TObjectPtr<UPubnubChatAccessManager> AccessManager = nullptr;
	UPROPERTY()
	FString CurrentUserID = "";
	UPROPERTY()
	FPubnubChatConfig ChatConfig;
	/** Repository that manages shared data for all chat objects */
	UPROPERTY()
	TObjectPtr<UPubnubChatObjectsRepository> ObjectsRepository = nullptr;
	UPROPERTY()
	bool IsInitialized = false;
	//Container for subscriptions used during listen for events - we need to keep them alive
	UPROPERTY()
	TArray<UPubnubSubscription*> ListenForEventsSubscriptions;
	
	UFUNCTION()
	void OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData);

	FPubnubChatInitChatResult InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);
	FPubnubChatUserResult GetUserForInit(const FString InUserID);
	
	
	/* CREATORS FOR CHAT OBJECTS */

	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubChatUserData& ChatUserData);
	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubUserData& UserData);

	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChatChannelData& ChatChannelData);
	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChannelData& ChannelData);

	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData);
	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData);

	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChatMembershipData& ChatMembershipData);
	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubMembershipData& MembershipData);
	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChannelMemberData& ChannelMemberData);
	
};

