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
class UPubnubChatThreadChannel;
class UPubnubChatThreadMessage;
enum class EPubnubSubscriptionStatus  : uint8;
struct FPubnubSubscriptionStatusData;
class FPubnubFunctionThread;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPubnubChatDestroyed, FString, UserID);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPubnubChatDestroyedNative, FString UserID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChanged, EPubnubChatConnectionStatus, Status, const FPubnubChatConnectionStatusData&, StatusData);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPubnubChatConnectionStatusChangedNative, EPubnubChatConnectionStatus Status, const FPubnubChatConnectionStatusData& StatusData);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatEventReceived, FPubnubChatEvent, Event);
DECLARE_DELEGATE_OneParam(FOnPubnubChatEventReceivedNative, const FPubnubChatEvent& Event);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatUserResponse, FPubnubChatUserResult, UserResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatUserResponseNative, const FPubnubChatUserResult& UserResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetUsersResponse, FPubnubChatGetUsersResult, UsersResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetUsersResponseNative, const FPubnubChatGetUsersResult& UsersResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatOperationResponse, FPubnubChatOperationResult, OperationResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatOperationResponseNative, const FPubnubChatOperationResult& OperationResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetUserSuggestionsResponse, FPubnubChatGetUserSuggestionsResult, SuggestionsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetUserSuggestionsResponseNative, const FPubnubChatGetUserSuggestionsResult& SuggestionsResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatChannelResponse, FPubnubChatChannelResult, ChannelResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatChannelResponseNative, const FPubnubChatChannelResult& ChannelResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatCreateGroupConversationResponse, FPubnubChatCreateGroupConversationResult, GroupConversationResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatCreateGroupConversationResponseNative, const FPubnubChatCreateGroupConversationResult& GroupConversationResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatCreateDirectConversationResponse, FPubnubChatCreateDirectConversationResult, DirectConversationResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatCreateDirectConversationResponseNative, const FPubnubChatCreateDirectConversationResult& DirectConversationResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetChannelsResponse, FPubnubChatGetChannelsResult, ChannelsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetChannelsResponseNative, const FPubnubChatGetChannelsResult& ChannelsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetChannelSuggestionsResponse, FPubnubChatGetChannelSuggestionsResult, SuggestionsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetChannelSuggestionsResponseNative, const FPubnubChatGetChannelSuggestionsResult& SuggestionsResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatWherePresentResponse, FPubnubChatWherePresentResult, WherePresentResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatWherePresentResponseNative, const FPubnubChatWherePresentResult& WherePresentResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatWhoIsPresentResponse, FPubnubChatWhoIsPresentResult, WhoIsPresentResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatWhoIsPresentResponseNative, const FPubnubChatWhoIsPresentResult& WhoIsPresentResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatIsPresentResponse, FPubnubChatIsPresentResult, IsPresentResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatIsPresentResponseNative, const FPubnubChatIsPresentResult& IsPresentResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatEventsResponse, FPubnubChatEventsResult, EventsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatEventsResponseNative, const FPubnubChatEventsResult& EventsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountsResponse, FPubnubChatGetUnreadMessagesCountsResult, UnreadMessagesCountsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountsResponseNative, const FPubnubChatGetUnreadMessagesCountsResult& UnreadMessagesCountsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatMarkAllMessagesAsReadResponse, FPubnubChatMarkAllMessagesAsReadResult, MarkAllMessagesAsReadResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatMarkAllMessagesAsReadResponseNative, const FPubnubChatMarkAllMessagesAsReadResult& MarkAllMessagesAsReadResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatThreadChannelResponse, FPubnubChatThreadChannelResult, ThreadChannelResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatThreadChannelResponseNative, const FPubnubChatThreadChannelResult& ThreadChannelResult);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatJoinResponse, FPubnubChatJoinResult, JoinResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatJoinResponseNative, const FPubnubChatJoinResult& JoinResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatInviteResponse, FPubnubChatInviteResult, InviteResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatInviteResponseNative, const FPubnubChatInviteResult& InviteResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatInviteMultipleResponse, FPubnubChatInviteMultipleResult, InviteMultipleResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatInviteMultipleResponseNative, const FPubnubChatInviteMultipleResult& InviteMultipleResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatMessageResponse, FPubnubChatMessageResult, MessageResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatMessageResponseNative, const FPubnubChatMessageResult& MessageResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatMembershipsResponse, FPubnubChatMembershipsResult, MembershipsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatMembershipsResponseNative, const FPubnubChatMembershipsResult& MembershipsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatIsDeletedResponse, FPubnubChatIsDeletedResult, IsDeletedResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatIsDeletedResponseNative, const FPubnubChatIsDeletedResult& IsDeletedResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetRestrictionResponse, FPubnubChatGetRestrictionResult, RestrictionResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetRestrictionResponseNative, const FPubnubChatGetRestrictionResult& RestrictionResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetRestrictionsResponse, FPubnubChatGetRestrictionsResult, RestrictionsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetRestrictionsResponseNative, const FPubnubChatGetRestrictionsResult& RestrictionsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetHistoryResponse, FPubnubChatGetHistoryResult, HistoryResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetHistoryResponseNative, const FPubnubChatGetHistoryResult& HistoryResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetThreadHistoryResponse, FPubnubChatGetThreadHistoryResult, ThreadHistoryResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetThreadHistoryResponseNative, const FPubnubChatGetThreadHistoryResult& ThreadHistoryResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountResponse, FPubnubChatGetUnreadMessagesCountResult, UnreadMessagesCountResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountResponseNative, const FPubnubChatGetUnreadMessagesCountResult& UnreadMessagesCountResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetReactionsResponse, FPubnubChatGetReactionsResult, ReactionsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetReactionsResponseNative, const FPubnubChatGetReactionsResult& ReactionsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatHasReactionResponse, FPubnubChatHasReactionResult, HasReactionResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatHasReactionResponseNative, const FPubnubChatHasReactionResult& HasReactionResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatHasThreadResponse, FPubnubChatHasThreadResult, HasThreadResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatHasThreadResponseNative, const FPubnubChatHasThreadResult& HasThreadResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatIsActiveResponse, bool, bIsActive);
DECLARE_DELEGATE_OneParam(FOnPubnubChatIsActiveResponseNative, bool bIsActive);


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
	friend class UPubnubChatThreadChannel;
	friend class UPubnubChatThreadMessage;
	
public:


	/*  DELEGATES  */
	
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	/**Listener to react for chat destroyed, equivalent that accepts lambdas*/
	FOnPubnubChatDestroyedNative OnChatDestroyedNative;
	
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
	void CreateUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse, FPubnubChatUserData UserData = FPubnubChatUserData());
	void CreateUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative, FPubnubChatUserData UserData = FPubnubChatUserData());
	
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult GetUser(const FString UserID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse);
	void GetUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUsersResult GetUsers(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUsersAsync(FOnPubnubChatGetUsersResponse OnUsersResponse, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());
	void GetUsersAsync(FOnPubnubChatGetUsersResponseNative OnUsersResponseNative, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult UpdateUser(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void UpdateUserAsync(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatUserResponse OnUserResponse);
	void UpdateUserAsync(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatUserResponseNative OnUserResponseNative = nullptr);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult DeleteUser(const FString UserID, bool Soft = false);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteUserAsync(const FString UserID, FOnPubnubChatOperationResponse OnOperationResponse, bool Soft = false);
	void DeleteUserAsync(const FString UserID, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, bool Soft = false);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUserSuggestionsResult GetUserSuggestions(const FString Text, int Limit = 10);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUserSuggestionsAsync(const FString Text, FOnPubnubChatGetUserSuggestionsResponse OnSuggestionsResponse, int Limit = 10);
	void GetUserSuggestionsAsync(const FString Text, FOnPubnubChatGetUserSuggestionsResponseNative OnSuggestionsResponseNative, int Limit = 10);

	
	/*  CHANNEL  */

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreatePublicConversationAsync(const FString ChannelID, FOnPubnubChatChannelResponse OnChannelResponse, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());
	void CreatePublicConversationAsync(const FString ChannelID, FOnPubnubChatChannelResponseNative OnChannelResponseNative, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateGroupConversationResult CreateGroupConversation(TArray<UPubnubChatUser*> Users, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreateGroupConversationAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatCreateGroupConversationResponse OnGroupConversationResponse, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	void CreateGroupConversationAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatCreateGroupConversationResponseNative OnGroupConversationResponseNative, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateDirectConversationResult CreateDirectConversation(UPubnubChatUser* User, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreateDirectConversationAsync(UPubnubChatUser* User, FOnPubnubChatCreateDirectConversationResponse OnDirectConversationResponse, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	void CreateDirectConversationAsync(UPubnubChatUser* User, FOnPubnubChatCreateDirectConversationResponseNative OnDirectConversationResponseNative, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult GetChannel(const FString ChannelID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelAsync(const FString ChannelID, FOnPubnubChatChannelResponse OnChannelResponse);
	void GetChannelAsync(const FString ChannelID, FOnPubnubChatChannelResponseNative OnChannelResponseNative);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetChannelsResult GetChannels(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelsAsync(FOnPubnubChatGetChannelsResponse OnChannelsResponse, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());
	void GetChannelsAsync(FOnPubnubChatGetChannelsResponseNative OnChannelsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult UpdateChannel(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void UpdateChannelAsync(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatChannelResponse OnChannelResponse);
	void UpdateChannelAsync(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatChannelResponseNative OnChannelResponseNative);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult DeleteChannel(const FString ChannelID, bool Soft = false);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteChannelAsync(const FString ChannelID, FOnPubnubChatOperationResponse OnOperationResponse, bool Soft = false);
	void DeleteChannelAsync(const FString ChannelID, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, bool Soft = false);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult PinMessageToChannel(UPubnubChatMessage* Message, UPubnubChatChannel* Channel);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageToChannelAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	void PinMessageToChannelAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult UnpinMessageFromChannel(UPubnubChatChannel* Channel);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageFromChannelAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	void UnpinMessageFromChannelAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Chanenl")
	FPubnubChatGetChannelSuggestionsResult GetChannelSuggestions(const FString Text, int Limit = 10);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelSuggestionsAsync(const FString Text, FOnPubnubChatGetChannelSuggestionsResponse OnSuggestionsResponse, int Limit = 10);
	void GetChannelSuggestionsAsync(const FString Text, FOnPubnubChatGetChannelSuggestionsResponseNative OnSuggestionsResponseNative, int Limit = 10);

	
	/* PRESENCE */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWherePresentResult WherePresent(const FString UserID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void WherePresentAsync(const FString UserID, FOnPubnubChatWherePresentResponse OnWherePresentResponse);
	void WherePresentAsync(const FString UserID, FOnPubnubChatWherePresentResponseNative OnWherePresentResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWhoIsPresentResult WhoIsPresent(const FString ChannelID, int Limit = 1000, int Offset = 0);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void WhoIsPresentAsync(const FString ChannelID, FOnPubnubChatWhoIsPresentResponse OnWhoIsPresentResponse, int Limit = 1000, int Offset = 0);
	void WhoIsPresentAsync(const FString ChannelID, FOnPubnubChatWhoIsPresentResponseNative OnWhoIsPresentResponseNative, int Limit = 1000, int Offset = 0);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatIsPresentResult IsPresent(const FString UserID, const FString ChannelID);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void IsPresentAsync(const FString UserID, const FString ChannelID, FOnPubnubChatIsPresentResponse OnIsPresentResponse);
	void IsPresentAsync(const FString UserID, const FString ChannelID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative);


	/* MODERATION */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatOperationResult SetRestrictions(FPubnubChatRestriction Restriction);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetRestrictionsAsync(FPubnubChatRestriction Restriction, FOnPubnubChatOperationResponse OnOperationResponse);
	void SetRestrictionsAsync(FPubnubChatRestriction Restriction, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatOperationResult EmitChatEvent(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, EPubnubChatEventMethod EventMethod = EPubnubChatEventMethod::PCEM_Default);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void EmitChatEventAsync(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, FOnPubnubChatOperationResponse OnOperationResponse, EPubnubChatEventMethod EventMethod = EPubnubChatEventMethod::PCEM_Default);
	void EmitChatEventAsync(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, EPubnubChatEventMethod EventMethod = EPubnubChatEventMethod::PCEM_Default);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatEventsResult GetEventsHistory(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, const int Count = 100);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	void GetEventsHistoryAsync(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponse OnEventsResponse, int Count = 100);
	void GetEventsHistoryAsync(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponseNative OnEventsResponseNative, int Count = 100);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatListenForEventsResult ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceived EventCallback);
	FPubnubChatListenForEventsResult ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceivedNative EventCallbackNative);

	
	/* MESSAGES */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatOperationResult ForwardMessage(UPubnubChatMessage* Message, UPubnubChatChannel* Channel);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ForwardMessageAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	void ForwardMessageAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatGetUnreadMessagesCountsResult GetUnreadMessagesCounts(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	void GetUnreadMessagesCountsAsync(FOnPubnubChatGetUnreadMessagesCountsResponse OnUnreadMessagesCountsResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	void GetUnreadMessagesCountsAsync(FOnPubnubChatGetUnreadMessagesCountsResponseNative OnUnreadMessagesCountsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatMarkAllMessagesAsReadResult MarkAllMessagesAsRead(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	void MarkAllMessagesAsReadAsync(FOnPubnubChatMarkAllMessagesAsReadResponse OnMarkAllMessagesAsReadResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	void MarkAllMessagesAsReadAsync(FOnPubnubChatMarkAllMessagesAsReadResponseNative OnMarkAllMessagesAsReadResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	
	/* THREADS */
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatThreadChannelResult CreateThreadChannel(UPubnubChatMessage* Message);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	void CreateThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	void CreateThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatThreadChannelResult GetThreadChannel(UPubnubChatMessage* Message);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	void GetThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	void GetThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);
	
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatOperationResult RemoveThreadChannel(UPubnubChatMessage* Message);

	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RemoveThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse);
	void RemoveThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	
	/* ACCESS MANAGER */
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|Access Manager")
	UPubnubChatAccessManager* GetAccessManager() const { return AccessManager; }
	
	
	/* CONNECTION STATUS */
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status")
	FPubnubChatOperationResult ReconnectSubscriptions(const FString Timetoken = "");

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ReconnectSubscriptionsAsync(FOnPubnubChatOperationResponse OnOperationResponse, const FString Timetoken = "");
	void ReconnectSubscriptionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, const FString Timetoken = "");
	
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status")
	FPubnubChatOperationResult DisconnectSubscriptions();

	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DisconnectSubscriptionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	void DisconnectSubscriptionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	
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
	
	
	FPubnubFunctionThread* AsyncFunctionsThread = nullptr;
	
	//Timer handles for user activity timestamp management
	FTimerHandle LastSavedActivityIntervalTimerHandle;
	FTimerHandle RunWithDelayTimerHandle;
	
	UFUNCTION()
	void OnPubnubSubscriptionStatusChanged(EPubnubSubscriptionStatus Status, FPubnubSubscriptionStatusData StatusData);

	FPubnubChatInitChatResult InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient);
	FPubnubChatUserResult GetUserForInit(const FString InUserID);
	
	void StoreUserActivityTimestamp();
	void SaveTimestamp();
	void RunSaveTimestampInterval();
	
	
	/* CREATORS FOR CHAT OBJECTS */

	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubChatUserData& ChatUserData);
	UPubnubChatUser* CreateUserObject(const FString UserID, const FPubnubUserData& UserData);

	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChatChannelData& ChatChannelData);
	UPubnubChatChannel* CreateChannelObject(const FString ChannelID, const FPubnubChannelData& ChannelData);

	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData);
	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData);
	UPubnubChatMessage* CreateMessageObject(const FString Timetoken, const FPubnubHistoryMessageData& HistoryMessageData);

	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChatMembershipData& ChatMembershipData);
	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubMembershipData& MembershipData);
	UPubnubChatMembership* CreateMembershipObject(UPubnubChatUser* User, UPubnubChatChannel* Channel, const FPubnubChannelMemberData& ChannelMemberData);
	
	UPubnubChatThreadChannel* CreateThreadChannelObject(const FString ThreadChannelID, const FPubnubChatChannelData& ThreadChannelData, UPubnubChatMessage* Message, bool IsThreadAlreadyConfirmed);
	UPubnubChatThreadChannel* CreateThreadChannelObject(const FString ThreadChannelID, const FPubnubChannelData& ChannelData, UPubnubChatMessage* Message, bool IsThreadAlreadyConfirmed);

	UPubnubChatThreadMessage* CreateThreadMessageObject(const FString Timetoken, const FPubnubChatMessageData& ChatMessageData, const FString ParentChannelID);
	UPubnubChatThreadMessage* CreateThreadMessageObject(const FString Timetoken, const FPubnubMessageData& MessageData, const FString ParentChannelID);
	UPubnubChatThreadMessage* CreateThreadMessageObject(const FString Timetoken, const FPubnubHistoryMessageData& HistoryMessageData, const FString ParentChannelID);

};

