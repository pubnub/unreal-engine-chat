// Copyright 2026 PubNub Inc. All Rights Reserved.

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPubnubChatObjectDeleted);
DECLARE_MULTICAST_DELEGATE(FOnPubnubChatObjectDeletedNative);
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
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatMembershipResponse, FPubnubChatMembershipResult, MembershipResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatMembershipResponseNative, const FPubnubChatMembershipResult& MembershipResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatMembershipsResponse, FPubnubChatMembershipsResult, MembershipsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatMembershipsResponseNative, const FPubnubChatMembershipsResult& MembershipsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetRestrictionResponse, FPubnubChatGetRestrictionResult, RestrictionResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetRestrictionResponseNative, const FPubnubChatGetRestrictionResult& RestrictionResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetRestrictionsResponse, FPubnubChatGetRestrictionsResult, RestrictionsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetRestrictionsResponseNative, const FPubnubChatGetRestrictionsResult& RestrictionsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetHistoryResponse, FPubnubChatGetHistoryResult, HistoryResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetHistoryResponseNative, const FPubnubChatGetHistoryResult& HistoryResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatFetchReadReceiptsResponse, FPubnubChatFetchReadReceiptsResult, FetchReadReceiptsResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatFetchReadReceiptsResponseNative, const FPubnubChatFetchReadReceiptsResult& FetchReadReceiptsResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetThreadHistoryResponse, FPubnubChatGetThreadHistoryResult, ThreadHistoryResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetThreadHistoryResponseNative, const FPubnubChatGetThreadHistoryResult& ThreadHistoryResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountResponse, FPubnubChatGetUnreadMessagesCountResult, UnreadMessagesCountResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatGetUnreadMessagesCountResponseNative, const FPubnubChatGetUnreadMessagesCountResult& UnreadMessagesCountResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatHasThreadResponse, FPubnubChatHasThreadResult, HasThreadResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatHasThreadResponseNative, const FPubnubChatHasThreadResult& HasThreadResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatHasMemberResponse, FPubnubChatHasMemberResult, HasMemberResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatHasMemberResponseNative, const FPubnubChatHasMemberResult& HasMemberResult);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnPubnubChatIsMemberOnResponse, FPubnubChatIsMemberOnResult, IsMemberOnResult);
DECLARE_DELEGATE_OneParam(FOnPubnubChatIsMemberOnResponseNative, const FPubnubChatIsMemberOnResult& IsMemberOnResult);


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
	
	/**
	 * Broadcast when this chat instance is destroyed (e.g. after DestroyChat is called).
	 * Use to clean up UI or references tied to this chat.
	 * @param UserID The user ID of the chat that was destroyed.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Pubnub Chat|Delegates")
	FOnPubnubChatDestroyed OnChatDestroyed;

	/**
	 * Same as OnChatDestroyed; native multicast delegate that accepts lambdas.
	 * @param UserID The user ID of the chat that was destroyed.
	 */
	FOnPubnubChatDestroyedNative OnChatDestroyedNative;

	/**
	 * Broadcast when the connection status of active subscriptions changes (e.g. connected, disconnected, error).
	 * Use to show connection state in the UI or to trigger ReconnectSubscriptions on disconnect/error.
	 * @param Status New connection status.
	 * @param StatusData Additional status details (e.g. reason, category).
	 */
	UPROPERTY(BlueprintAssignable, Category = "PubnubChat|Delegates")
	FOnPubnubChatConnectionStatusChanged OnConnectionStatusChanged;

	/**
	 * Same as OnConnectionStatusChanged; native multicast delegate that accepts lambdas.
	 * @param Status New connection status.
	 * @param StatusData Additional status details (e.g. reason, category).
	 */
	FOnPubnubChatConnectionStatusChangedNative OnConnectionStatusChangedNative;


	/*  GENERAL  */

	/**
	 * Tears down this chat instance: stops async operations, clears timers and repository,
	 * unsubscribes from subscription status, then broadcasts OnChatDestroyed with the current user ID.
	 * The chat must not be used after this call.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat")
	void DestroyChat();

	/**
	 * Returns the underlying PubNub client used by this chat.
	 * This is useful for advanced use cases where you need to access the PubNub UE Core SDK directly.
	 * Local: does not perform any network requests.
	 *
	 * @return PubNub client or nullptr if chat is not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat")
	UPubnubClient* GetPubnubClient() const { return PubnubClient; }

	
	/*  USER  */

	/**
	 * Returns the current chat user for this chat instance.
	 * Local: does not perform any network requests.
	 *
	 * @return Current user object or nullptr if chat is not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|User")
	UPubnubChatUser* GetCurrentUser() const { return CurrentUser; }
	
	/**
	 * Creates a new user on the PubNub server and returns a chat user object.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if a user with the same ID already exists (use GetUser instead).
	 * The returned user is created from the provided UserData and cached locally.
	 * 
	 * @param UserID Unique identifier for the user to create.
	 * @param UserData Initial user metadata (name, external ID, profile URL, email, custom JSON, status, type).
	 * @return Operation result and created user object (if successful).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult CreateUser(const FString UserID, FPubnubChatUserData UserData = FPubnubChatUserData());

	/**
	 * Creates a new user asynchronously on the PubNub server and returns a chat user object.
	 * Fails if a user with the same ID already exists (use GetUser instead).
	 * The returned user is created from the provided UserData and cached locally.
	 * 
	 * @param UserID Unique identifier for the user to create.
	 * @param OnUserResponse Callback executed when the operation completes.
	 * @param UserData Initial user metadata (name, external ID, profile URL, email, custom JSON, status, type).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void CreateUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse, FPubnubChatUserData UserData = FPubnubChatUserData());
	/**
	 * Creates a new user asynchronously on the PubNub server and returns a chat user object.
	 * Fails if a user with the same ID already exists (use GetUser instead).
	 * The returned user is created from the provided UserData and cached locally.
	 * 
	 * @param UserID Unique identifier for the user to create.
	 * @param OnUserResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param UserData Initial user metadata (name, external ID, profile URL, email, custom JSON, status, type).
	 */
	void CreateUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative, FPubnubChatUserData UserData = FPubnubChatUserData());
	
	
	/**
	 * Retrieves a user from the PubNub server and returns a chat user object.
	 * Blocking: performs network requests on the calling thread.
	 * 
	 * @param UserID Unique identifier of the user to fetch.
	 * @return Operation result and user object (if found).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult GetUser(const FString UserID);

	/**
	 * Retrieves a user asynchronously from the PubNub server and returns a chat user object.
	 * 
	 * @param UserID Unique identifier of the user to fetch.
	 * @param OnUserResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUserAsync(const FString UserID, FOnPubnubChatUserResponse OnUserResponse);
	/**
	 * Retrieves a user asynchronously from the PubNub server and returns a chat user object.
	 * 
	 * @param UserID Unique identifier of the user to fetch.
	 * @param OnUserResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetUserAsync(const FString UserID, FOnPubnubChatUserResponseNative OnUserResponseNative);

	/**
	 * Retrieves users from the PubNub server.
	 * Blocking: performs network requests on the calling thread.
	 * 
	 * @param Limit Max number of users to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 * @return Operation result, list of users, and pagination data.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUsersResult GetUsers(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Retrieves users asynchronously from the PubNub server.
	 * 
	 * @param OnUsersResponse Callback executed when the operation completes.
	 * @param Limit Max number of users to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUsersAsync(FOnPubnubChatGetUsersResponse OnUsersResponse, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves users asynchronously from the PubNub server.
	 * 
	 * @param OnUsersResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of users to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	void GetUsersAsync(FOnPubnubChatGetUsersResponseNative OnUsersResponseNative, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Updates a user's metadata on the PubNub server and returns the updated user.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if the user does not exist.
	 * 
	 * @param UserID Unique identifier of the user to update.
	 * @param UpdateUserData Fields to update and ForceSet flags to control replacements.
	 * @return Operation result and updated user object.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatUserResult UpdateUser(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData);

	/**
	 * Updates a user's metadata asynchronously on the PubNub server and returns the updated user.
	 * Fails if the user does not exist.
	 * 
	 * @param UserID Unique identifier of the user to update.
	 * @param UpdateUserData Fields to update and ForceSet flags to control replacements.
	 * @param OnUserResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void UpdateUserAsync(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatUserResponse OnUserResponse);
	/**
	 * Updates a user's metadata asynchronously on the PubNub server and returns the updated user.
	 * Fails if the user does not exist.
	 * 
	 * @param UserID Unique identifier of the user to update.
	 * @param UpdateUserData Fields to update and ForceSet flags to control replacements.
	 * @param OnUserResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UpdateUserAsync(const FString UserID, FPubnubChatUpdateUserInputData UpdateUserData, FOnPubnubChatUserResponseNative OnUserResponseNative = nullptr);

	/**
	 * Deletes a user on the server.
	 * Blocking: performs network requests on the calling thread.
	 * 
	 * @param UserID Unique identifier of the user to delete.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatOperationResult DeleteUser(const FString UserID);

	/**
	 * Deletes a user asynchronously on the server.
	 * 
	 * @param UserID Unique identifier of the user to delete.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteUserAsync(const FString UserID, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Deletes a user asynchronously on the server.
	 * 
	 * @param UserID Unique identifier of the user to delete.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DeleteUserAsync(const FString UserID, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Returns users whose name starts with the provided text.
	 * Blocking: performs network requests on the calling thread.
	 * 
	 * @param Text Prefix text to match against the user name field (uses a name LIKE "Text*" filter).
	 * @param Limit Max number of users to return.
	 * @return Operation result and matching users.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	FPubnubChatGetUserSuggestionsResult GetUserSuggestions(const FString Text, int Limit = 10);

	/**
	 * Returns users asynchronously whose name starts with the provided text.
	 * 
	 * @param Text Prefix text to match against the user name field (uses a name LIKE "Text*" filter).
	 * @param OnSuggestionsResponse Callback executed when the operation completes.
	 * @param Limit Max number of users to return.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|User")
	void GetUserSuggestionsAsync(const FString Text, FOnPubnubChatGetUserSuggestionsResponse OnSuggestionsResponse, int Limit = 10);
	/**
	 * Returns users asynchronously whose name starts with the provided text.
	 * 
	 * @param Text Prefix text to match against the user name field (uses a name LIKE "Text*" filter).
	 * @param OnSuggestionsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of users to return.
	 */
	void GetUserSuggestionsAsync(const FString Text, FOnPubnubChatGetUserSuggestionsResponseNative OnSuggestionsResponseNative, int Limit = 10);

	
	/*  CHANNEL  */

	/**
	 * Creates a public channel on the PubNub server and returns a chat channel object.
	 * Blocking: performs network requests on the calling thread.
	 * Always sets the channel type to "public" regardless of ChannelData.Type.
	 *
	 * @param ChannelID Unique identifier for the channel to create.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @return Operation result and created channel object (if successful).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult CreatePublicConversation(const FString ChannelID, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	/**
	 * Creates a public channel asynchronously on the PubNub server and returns a chat channel object.
	 * Always sets the channel type to "public" regardless of ChannelData.Type.
	 *
	 * @param ChannelID Unique identifier for the channel to create.
	 * @param OnChannelResponse Callback executed when the operation completes.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreatePublicConversationAsync(const FString ChannelID, FOnPubnubChatChannelResponse OnChannelResponse, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());
	/**
	 * Creates a public channel asynchronously on the PubNub server and returns a chat channel object.
	 * Always sets the channel type to "public" regardless of ChannelData.Type.
	 *
	 * @param ChannelID Unique identifier for the channel to create.
	 * @param OnChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 */
	void CreatePublicConversationAsync(const FString ChannelID, FOnPubnubChatChannelResponseNative OnChannelResponseNative, FPubnubChatChannelData ChannelData = FPubnubChatChannelData());

	/**
	 * Creates a group channel, adds the current user as host, and invites provided users.
	 * Blocking: performs network requests on the calling thread.
	 * Always sets the channel type to "group" regardless of ChannelData.Type.
	 *
	 * @param Users Users to invite. At least one valid user is required.
	 * @param ChannelID Optional channel ID. If empty, a GUID is generated.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 * @return Operation result, created channel, host membership, and invitees' memberships.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateGroupConversationResult CreateGroupConversation(TArray<UPubnubChatUser*> Users, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	/**
	 * Creates a group channel asynchronously, adds the current user as host, and invites provided users.
	 * Always sets the channel type to "group" regardless of ChannelData.Type.
	 *
	 * @param Users Users to invite. At least one valid user is required.
	 * @param OnGroupConversationResponse Callback executed when the operation completes.
	 * @param ChannelID Optional channel ID. If empty, a GUID is generated.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreateGroupConversationAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatCreateGroupConversationResponse OnGroupConversationResponse, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	/**
	 * Creates a group channel asynchronously, adds the current user as host, and invites provided users.
	 * Always sets the channel type to "group" regardless of ChannelData.Type.
	 *
	 * @param Users Users to invite. At least one valid user is required.
	 * @param OnGroupConversationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param ChannelID Optional channel ID. If empty, a GUID is generated.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 */
	void CreateGroupConversationAsync(TArray<UPubnubChatUser*> Users, FOnPubnubChatCreateGroupConversationResponseNative OnGroupConversationResponseNative, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	/**
	 * Creates a direct channel with a single user, adds the current user as host, and invites the provided user.
	 * Blocking: performs network requests on the calling thread.
	 * Always sets the channel type to "direct" regardless of ChannelData.Type.
	 *
	 * @param User User to invite. Must be valid.
	 * @param ChannelID Optional channel ID. If empty, a deterministic ID is generated from both user IDs.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 * @return Operation result, created channel, host membership, and invitee membership.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatCreateDirectConversationResult CreateDirectConversation(UPubnubChatUser* User, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());

	/**
	 * Creates a direct channel asynchronously with a single user, adds the current user as host, and invites the provided user.
	 * Always sets the channel type to "direct" regardless of ChannelData.Type.
	 *
	 * @param User User to invite. Must be valid.
	 * @param OnDirectConversationResponse Callback executed when the operation completes.
	 * @param ChannelID Optional channel ID. If empty, a deterministic ID is generated from both user IDs.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void CreateDirectConversationAsync(UPubnubChatUser* User, FOnPubnubChatCreateDirectConversationResponse OnDirectConversationResponse, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	/**
	 * Creates a direct channel asynchronously with a single user, adds the current user as host, and invites the provided user.
	 * Always sets the channel type to "direct" regardless of ChannelData.Type.
	 *
	 * @param User User to invite. Must be valid.
	 * @param OnDirectConversationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param ChannelID Optional channel ID. If empty, a deterministic ID is generated from both user IDs.
	 * @param ChannelData Initial channel metadata (name, description, custom JSON, status, type, etc.).
	 * @param HostMembershipData Membership metadata for the current user in the new channel.
	 */
	void CreateDirectConversationAsync(UPubnubChatUser* User, FOnPubnubChatCreateDirectConversationResponseNative OnDirectConversationResponseNative, const FString ChannelID = "", FPubnubChatChannelData ChannelData = FPubnubChatChannelData(), FPubnubChatMembershipData HostMembershipData = FPubnubChatMembershipData());
	
	/**
	 * Retrieves a channel from the PubNub server and returns a chat channel object.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param ChannelID Unique identifier of the channel to fetch.
	 * @return Operation result and channel object (if found).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult GetChannel(const FString ChannelID);

	/**
	 * Retrieves a channel asynchronously from the PubNub server and returns a chat channel object.
	 *
	 * @param ChannelID Unique identifier of the channel to fetch.
	 * @param OnChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelAsync(const FString ChannelID, FOnPubnubChatChannelResponse OnChannelResponse);
	/**
	 * Retrieves a channel asynchronously from the PubNub server and returns a chat channel object.
	 *
	 * @param ChannelID Unique identifier of the channel to fetch.
	 * @param OnChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetChannelAsync(const FString ChannelID, FOnPubnubChatChannelResponseNative OnChannelResponseNative);

	/**
	 * Retrieves channels from the PubNub server.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param Limit Max number of channels to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 * @return Operation result, list of channels, and pagination data.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatGetChannelsResult GetChannels(const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Retrieves channels asynchronously from the PubNub server.
	 *
	 * @param OnChannelsResponse Callback executed when the operation completes.
	 * @param Limit Max number of channels to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelsAsync(FOnPubnubChatGetChannelsResponse OnChannelsResponse, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Retrieves channels asynchronously from the PubNub server.
	 *
	 * @param OnChannelsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of channels to return. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	void GetChannelsAsync(FOnPubnubChatGetChannelsResponseNative OnChannelsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubGetAllSort Sort = FPubnubGetAllSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Updates a channel's metadata on the PubNub server and returns the updated channel.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if the channel does not exist.
	 *
	 * @param ChannelID Unique identifier of the channel to update.
	 * @param UpdateChannelData Fields to update and ForceSet flags to control replacements.
	 * @return Operation result and updated channel object.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatChannelResult UpdateChannel(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData);

	/**
	 * Updates a channel's metadata asynchronously on the PubNub server and returns the updated channel.
	 * Fails if the channel does not exist.
	 *
	 * @param ChannelID Unique identifier of the channel to update.
	 * @param UpdateChannelData Fields to update and ForceSet flags to control replacements.
	 * @param OnChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void UpdateChannelAsync(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatChannelResponse OnChannelResponse);
	/**
	 * Updates a channel's metadata asynchronously on the PubNub server and returns the updated channel.
	 * Fails if the channel does not exist.
	 *
	 * @param ChannelID Unique identifier of the channel to update.
	 * @param UpdateChannelData Fields to update and ForceSet flags to control replacements.
	 * @param OnChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UpdateChannelAsync(const FString ChannelID, FPubnubChatUpdateChannelInputData UpdateChannelData, FOnPubnubChatChannelResponseNative OnChannelResponseNative);

	/**
	 * Deletes a channel on the server.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param ChannelID Unique identifier of the channel to delete.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult DeleteChannel(const FString ChannelID);

	/**
	 * Deletes a channel asynchronously on the server.
	 *
	 * @param ChannelID Unique identifier of the channel to delete.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DeleteChannelAsync(const FString ChannelID, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Deletes a channel asynchronously on the server.
	 *
	 * @param ChannelID Unique identifier of the channel to delete.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DeleteChannelAsync(const FString ChannelID, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Pins a message to a channel.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param Message Message to pin.
	 * @param Channel Channel to pin the message to.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult PinMessageToChannel(UPubnubChatMessage* Message, UPubnubChatChannel* Channel);

	/**
	 * Pins a message to a channel asynchronously.
	 *
	 * @param Message Message to pin.
	 * @param Channel Channel to pin the message to.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void PinMessageToChannelAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pins a message to a channel asynchronously.
	 *
	 * @param Message Message to pin.
	 * @param Channel Channel to pin the message to.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void PinMessageToChannelAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Unpins the currently pinned message from a channel.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param Channel Channel to unpin the message from.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	FPubnubChatOperationResult UnpinMessageFromChannel(UPubnubChatChannel* Channel);

	/**
	 * Unpins the currently pinned message from a channel asynchronously.
	 *
	 * @param Channel Channel to unpin the message from.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void UnpinMessageFromChannelAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Unpins the currently pinned message from a channel asynchronously.
	 *
	 * @param Channel Channel to unpin the message from.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void UnpinMessageFromChannelAsync(UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Returns channels whose name starts with the provided text.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param Text Prefix text to match against the channel name field (uses a name LIKE "Text*" filter).
	 * @param Limit Max number of channels to return.
	 * @return Operation result and matching channels.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Chanenl")
	FPubnubChatGetChannelSuggestionsResult GetChannelSuggestions(const FString Text, int Limit = 10);

	/**
	 * Returns channels asynchronously whose name starts with the provided text.
	 *
	 * @param Text Prefix text to match against the channel name field (uses a name LIKE "Text*" filter).
	 * @param OnSuggestionsResponse Callback executed when the operation completes.
	 * @param Limit Max number of channels to return.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Channel")
	void GetChannelSuggestionsAsync(const FString Text, FOnPubnubChatGetChannelSuggestionsResponse OnSuggestionsResponse, int Limit = 10);
	/**
	 * Returns channels asynchronously whose name starts with the provided text.
	 *
	 * @param Text Prefix text to match against the channel name field (uses a name LIKE "Text*" filter).
	 * @param OnSuggestionsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of channels to return.
	 */
	void GetChannelSuggestionsAsync(const FString Text, FOnPubnubChatGetChannelSuggestionsResponseNative OnSuggestionsResponseNative, int Limit = 10);

	
	/* PRESENCE */
	
	/**
	 * Lists channels the specified user is actively subscribed to.
	 * Presence is based on active subscriptions (messages, events, or updates), not memberships.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param UserID Unique identifier of the user to query.
	 * @return Operation result and list of channels where the user is present.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWherePresentResult WherePresent(const FString UserID);

	/**
	 * Lists channels asynchronously the specified user is actively subscribed to.
	 * Presence is based on active subscriptions (messages, events, or updates), not memberships.
	 *
	 * @param UserID Unique identifier of the user to query.
	 * @param OnWherePresentResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void WherePresentAsync(const FString UserID, FOnPubnubChatWherePresentResponse OnWherePresentResponse);
	/**
	 * Lists channels asynchronously the specified user is actively subscribed to.
	 * Presence is based on active subscriptions (messages, events, or updates), not memberships.
	 *
	 * @param UserID Unique identifier of the user to query.
	 * @param OnWherePresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void WherePresentAsync(const FString UserID, FOnPubnubChatWherePresentResponseNative OnWherePresentResponseNative);
	
	/**
	 * Lists users currently present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param Limit Max number of users to return.
	 * @param Offset Number of users to skip for pagination.
	 * @return Operation result and list of user IDs present on the channel.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatWhoIsPresentResult WhoIsPresent(const FString ChannelID, int Limit = 1000, int Offset = 0);

	/**
	 * Lists users asynchronously currently present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param OnWhoIsPresentResponse Callback executed when the operation completes.
	 * @param Limit Max number of users to return.
	 * @param Offset Number of users to skip for pagination.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void WhoIsPresentAsync(const FString ChannelID, FOnPubnubChatWhoIsPresentResponse OnWhoIsPresentResponse, int Limit = 1000, int Offset = 0);
	/**
	 * Lists users asynchronously currently present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param OnWhoIsPresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of users to return.
	 * @param Offset Number of users to skip for pagination.
	 */
	void WhoIsPresentAsync(const FString ChannelID, FOnPubnubChatWhoIsPresentResponseNative OnWhoIsPresentResponseNative, int Limit = 1000, int Offset = 0);
	
	/**
	 * Checks if a user is present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @param ChannelID Unique identifier of the channel to check.
	 * @return Operation result and presence flag.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	FPubnubChatIsPresentResult IsPresent(const FString UserID, const FString ChannelID);

	/**
	 * Checks asynchronously if a user is present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsPresentResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Presence")
	void IsPresentAsync(const FString UserID, const FString ChannelID, FOnPubnubChatIsPresentResponse OnIsPresentResponse);
	/**
	 * Checks asynchronously if a user is present on a channel.
	 * Presence reflects active subscriptions (messages, events, or updates), not channel membership.
	 *
	 * @param UserID Unique identifier of the user to check.
	 * @param ChannelID Unique identifier of the channel to check.
	 * @param OnIsPresentResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void IsPresentAsync(const FString UserID, const FString ChannelID, FOnPubnubChatIsPresentResponseNative OnIsPresentResponseNative);


	/* MODERATION */
	
	/**
	 * Sets or lifts moderation restrictions for a user on a channel.
	 * Blocking: performs network requests on the calling thread.
	 *
	 * @param Restriction Restriction settings (UserID, ChannelID, Ban/Mute flags, Reason).
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatOperationResult SetRestrictions(FPubnubChatRestriction Restriction);

	/**
	 * Sets or lifts moderation restrictions asynchronously for a user on a channel.
	 *
	 * @param Restriction Restriction settings (UserID, ChannelID, Ban/Mute flags, Reason).
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void SetRestrictionsAsync(FPubnubChatRestriction Restriction, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Sets or lifts moderation restrictions asynchronously for a user on a channel.
	 *
	 * @param Restriction Restriction settings (UserID, ChannelID, Ban/Mute flags, Reason).
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void SetRestrictionsAsync(FPubnubChatRestriction Restriction, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	/**
	 * Fetches chat events history for a channel within a timetoken range.
	 * Blocking: performs network requests on the calling thread.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param Count Maximum number of events to return.
	 * @return Operation result, events list, and "IsMore" flag when more events exist in the range.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	FPubnubChatEventsResult GetEventsHistory(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, const int Count = 100);

	/**
	 * Fetches chat events history asynchronously for a channel within a timetoken range.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnEventsResponse Callback executed when the operation completes.
	 * @param Count Maximum number of events to return.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Moderation")
	void GetEventsHistoryAsync(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponse OnEventsResponse, int Count = 100);
	/**
	 * Fetches chat events history asynchronously for a channel within a timetoken range.
	 * StartTimetoken must be higher (newer) than EndTimetoken.
	 *
	 * @param ChannelID Unique identifier of the channel to query.
	 * @param StartTimetoken Start timetoken (inclusive) for the history range. Must be higher (newer) than EndTimetoken.
	 * @param EndTimetoken End timetoken (inclusive) for the history range. Must be lower (older) than StartTimetoken.
	 * @param OnEventsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Count Maximum number of events to return.
	 */
	void GetEventsHistoryAsync(const FString ChannelID, const FString StartTimetoken, const FString EndTimetoken, FOnPubnubChatEventsResponseNative OnEventsResponseNative, int Count = 100);
	
	
	/* MESSAGES */
	
	/**
	 * Forwards a message to another channel by publishing it to the target channel with forwarding metadata.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if the target channel is the same as the message's channel.
	 *
	 * @param Message Message to forward.
	 * @param Channel Channel to forward the message to (must be different from the message's channel).
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatOperationResult ForwardMessage(UPubnubChatMessage* Message, UPubnubChatChannel* Channel);

	/**
	 * Forwards a message asynchronously to another channel by publishing it to the target channel with forwarding metadata.
	 * Fails if the target channel is the same as the message's channel.
	 *
	 * @param Message Message to forward.
	 * @param Channel Channel to forward the message to (must be different from the message's channel).
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ForwardMessageAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Forwards a message asynchronously to another channel by publishing it to the target channel with forwarding metadata.
	 * Fails if the target channel is the same as the message's channel.
	 *
	 * @param Message Message to forward.
	 * @param Channel Channel to forward the message to (must be different from the message's channel).
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void ForwardMessageAsync(UPubnubChatMessage* Message, UPubnubChatChannel* Channel, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);

	/**
	 * Returns unread message counts per channel for the current user's memberships.
	 * Blocking: performs network requests on the calling thread.
	 * Counts are based on last-read-message timetokens stored on each membership.
	 *
	 * @param Limit Max number of memberships to consider. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 * @return Operation result, unread counts per channel (with channel and membership), and pagination data.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatGetUnreadMessagesCountsResult GetUnreadMessagesCounts(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Returns unread message counts asynchronously per channel for the current user's memberships.
	 * Counts are based on last-read-message timetokens stored on each membership.
	 *
	 * @param OnUnreadMessagesCountsResponse Callback executed when the operation completes.
	 * @param Limit Max number of memberships to consider. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	void GetUnreadMessagesCountsAsync(FOnPubnubChatGetUnreadMessagesCountsResponse OnUnreadMessagesCountsResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Returns unread message counts asynchronously per channel for the current user's memberships.
	 * Counts are based on last-read-message timetokens stored on each membership.
	 *
	 * @param OnUnreadMessagesCountsResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of memberships to consider. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	void GetUnreadMessagesCountsAsync(FOnPubnubChatGetUnreadMessagesCountsResponseNative OnUnreadMessagesCountsResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Marks all messages as read for the current user's memberships in the given range.
	 * Blocking: performs network requests on the calling thread.
	 * Updates last-read-message timetoken on each membership and emits receipt events on non-public channels.
	 *
	 * @param Limit Max number of memberships to update. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 * @return Operation result, updated memberships, and pagination data.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	FPubnubChatMarkAllMessagesAsReadResult MarkAllMessagesAsRead(const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());

	/**
	 * Marks all messages as read asynchronously for the current user's memberships in the given range.
	 * Updates last-read-message timetoken on each membership and emits receipt events on non-public channels.
	 *
	 * @param OnMarkAllMessagesAsReadResponse Callback executed when the operation completes.
	 * @param Limit Max number of memberships to update. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Messages")
	void MarkAllMessagesAsReadAsync(FOnPubnubChatMarkAllMessagesAsReadResponse OnMarkAllMessagesAsReadResponse, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	/**
	 * Marks all messages as read asynchronously for the current user's memberships in the given range.
	 * Updates last-read-message timetoken on each membership and emits receipt events on non-public channels.
	 *
	 * @param OnMarkAllMessagesAsReadResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Limit Max number of memberships to update. Pass 0 to use the server default.
	 * @param Filter Expression used to filter the results. Check online documentation to see exact filter formulas.
	 * @param Sort Key-value pair of a property to sort by, and a sort direction.
	 * @param Page Pagination information. Use Page.Next to get the next page or Page.Prev to get the previous page.
	 *             If both are provided, Next takes precedence.
	 */
	void MarkAllMessagesAsReadAsync(FOnPubnubChatMarkAllMessagesAsReadResponseNative OnMarkAllMessagesAsReadResponseNative, const int Limit = 0, const FString Filter = "", FPubnubMembershipSort Sort = FPubnubMembershipSort(), FPubnubPage Page = FPubnubPage());
	
	
	/* THREADS */
	
	/**
	 * Creates a thread channel for replying to a message (local only until first message is sent on the thread).
	 * Blocking: performs a network check to ensure the thread does not already exist.
	 * Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 *
	 * @param Message Parent message to create the thread on.
	 * @return Operation result and thread channel object (thread is created on the server when the first reply is sent).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatThreadChannelResult CreateThreadChannel(UPubnubChatMessage* Message);

	/**
	 * Creates a thread channel asynchronously for replying to a message (local only until first message is sent on the thread).
	 * Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 *
	 * @param Message Parent message to create the thread on.
	 * @param OnThreadChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	void CreateThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	/**
	 * Creates a thread channel asynchronously for replying to a message (local only until first message is sent on the thread).
	 * Fails if the message is already in a thread, if the message is deleted, or if a thread for this message already exists.
	 *
	 * @param Message Parent message to create the thread on.
	 * @param OnThreadChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void CreateThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);

	/**
	 * Retrieves the thread channel for a message if it already exists on the server.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if no thread exists for this message.
	 *
	 * @param Message Parent message whose thread channel to fetch.
	 * @return Operation result and thread channel object (if the thread exists).
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatThreadChannelResult GetThreadChannel(UPubnubChatMessage* Message);

	/**
	 * Retrieves asynchronously the thread channel for a message if it already exists on the server.
	 * Fails if no thread exists for this message.
	 *
	 * @param Message Parent message whose thread channel to fetch.
	 * @param OnThreadChannelResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	void GetThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponse OnThreadChannelResponse);
	/**
	 * Retrieves asynchronously the thread channel for a message if it already exists on the server.
	 * Fails if no thread exists for this message.
	 *
	 * @param Message Parent message whose thread channel to fetch.
	 * @param OnThreadChannelResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void GetThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatThreadChannelResponseNative OnThreadChannelResponseNative);

	/**
	 * Removes the thread from a message: deletes the thread root message action and the thread channel.
	 * Blocking: performs network requests on the calling thread.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @param Message Parent message whose thread to remove.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads")
	FPubnubChatOperationResult RemoveThreadChannel(UPubnubChatMessage* Message);

	/**
	 * Removes the thread from a message asynchronously: deletes the thread root message action and the thread channel.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @param Message Parent message whose thread to remove.
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category="Pubnub Chat|Threads", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void RemoveThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Removes the thread from a message asynchronously: deletes the thread root message action and the thread channel.
	 * Fails if the message has no thread or has an invalid thread root action.
	 *
	 * @param Message Parent message whose thread to remove.
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void RemoveThreadChannelAsync(UPubnubChatMessage* Message, FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	
	/* ACCESS MANAGER */
	
	/**
	 * Returns the access manager for this chat instance.
	 * Use it to set or parse auth tokens, check permissions (CanI), and configure PubNub origin.
	 * Local: does not perform any network requests.
	 *
	 * @return Access manager object or nullptr if chat is not initialized.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Pubnub Chat|Access Manager")
	UPubnubChatAccessManager* GetAccessManager() const { return AccessManager; }
	
	
	/* CONNECTION STATUS */
	
	/**
	 * Reconnects all active subscriptions (e.g. after unexpected disconnect or connection error).
	 * Blocking: performs the reconnect on the calling thread.
	 * Use after receiving a disconnected or connection-error status from OnConnectionStatusChanged, or after DisconnectSubscriptions.
	 *
	 * @param Timetoken Optional timetoken to resume from; empty to use default.
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status")
	FPubnubChatOperationResult ReconnectSubscriptions(const FString Timetoken = "");

	/**
	 * Reconnects all active subscriptions asynchronously (e.g. after unexpected disconnect or connection error).
	 * Use after receiving a disconnected or connection-error status from OnConnectionStatusChanged, or after DisconnectSubscriptions.
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 * @param Timetoken Optional timetoken to resume from; empty to use default.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void ReconnectSubscriptionsAsync(FOnPubnubChatOperationResponse OnOperationResponse, const FString Timetoken = "");
	/**
	 * Reconnects all active subscriptions asynchronously (e.g. after unexpected disconnect or connection error).
	 * Use after receiving a disconnected or connection-error status from OnConnectionStatusChanged, or after DisconnectSubscriptions.
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 * @param Timetoken Optional timetoken to resume from; empty to use default.
	 */
	void ReconnectSubscriptionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr, const FString Timetoken = "");

	/**
	 * Pauses all active subscriptions (stops receiving messages and events until ReconnectSubscriptions is called).
	 * Blocking: performs the disconnect on the calling thread.
	 *
	 * @return Operation result.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status")
	FPubnubChatOperationResult DisconnectSubscriptions();

	/**
	 * Pauses all active subscriptions asynchronously (stops receiving messages and events until ReconnectSubscriptions is called).
	 *
	 * @param OnOperationResponse Callback executed when the operation completes.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|Connection Status", meta = (AutoCreateRefTerm = "OnOperationResponse"))
	void DisconnectSubscriptionsAsync(FOnPubnubChatOperationResponse OnOperationResponse);
	/**
	 * Pauses all active subscriptions asynchronously (stops receiving messages and events until ReconnectSubscriptions is called).
	 *
	 * @param OnOperationResponseNative Native callback executed when the operation completes (accepts lambdas).
	 */
	void DisconnectSubscriptionsAsync(FOnPubnubChatOperationResponseNative OnOperationResponseNative = nullptr);
	
	
private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	UPROPERTY()
	bool bOwnsPubnubClient = false;
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

	FPubnubChatInitChatResult InitChat(const FString InUserID, const FPubnubChatConfig& InChatConfig, UPubnubClient* InPubnubClient, bool bInOwnsPubnubClient);
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
	
	/* EVENTS */
	
	FPubnubChatOperationResult EmitChatEvent(EPubnubChatEventType EventType, const FString ChannelID, const FString Payload, EPubnubChatEventMethod EventMethod = EPubnubChatEventMethod::PCEM_Default);
	FPubnubChatListenForEventsResult ListenForEvents(const FString ChannelID, EPubnubChatEventType EventType, FOnPubnubChatEventReceivedNative EventCallbackNative);

};

