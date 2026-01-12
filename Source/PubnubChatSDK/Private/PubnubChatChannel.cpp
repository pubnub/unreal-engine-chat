// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatChannel.h"
#include "PubnubClient.h"
#include "PubnubChat.h"
#include "PubnubChatCallbackStop.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalMacros.h"
#include "PubnubChatMembership.h"
#include "PubnubChatSubsystem.h"
#include "PubnubChatObjectsRepository.h"
#include "PubnubChatMessage.h"
#include "PubnubChatUser.h"
#include "Entities/PubnubChannelEntity.h"
#include "Entities/PubnubSubscription.h"
#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubChatLogUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "FunctionLibraries/PubnubTimetokenUtilities.h" 


void UPubnubChatChannel::BeginDestroy()
{
	CleanUp();
	
	Super::BeginDestroy();
}

FPubnubChatChannelData UPubnubChatChannel::GetChannelData() const
{
	PUBNUB_CHAT_OBJECT_RETURN_IF_NOT_INITIALIZED(FPubnubChatChannelData());

	//Get channel data from repository
	if (FPubnubChatInternalChannel* InternalChannel = Chat->ObjectsRepository->GetChannelData(ChannelID))
	{
		return InternalChannel->ChannelData;
	}

	UE_LOG(PubnubChatLog, Error, TEXT("Channel data not found in repository for ChannelID: %s"), *ChannelID);
	return FPubnubChatChannelData();
}

FPubnubChatOperationResult UPubnubChatChannel::Update(FPubnubChatUpdateChannelInputData UpdateChannelData)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//SetChannelMetadata by PubnubClient - include all fields in response
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, UpdateChannelData.ToPubnubChannelInputData(), FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	//Update repository with updated channel data
	Chat->ObjectsRepository->UpdateChannelData(ChannelID, FPubnubChatChannelData::FromPubnubChannelData(SetChannelResult.ChannelData));

	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::Connect()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already connected
	if (IsConnected)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add lister to subscription with provided callback
	ConnectSubscription->OnPubnubMessageNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();

		if(!ThisChannel->Chat)
		{return;}
		
		ThisChannel->OnMessageReceived.Broadcast(ThisChannel->Chat->CreateMessageObject(MessageData.Timetoken, MessageData));
		ThisChannel->OnMessageReceivedNative.Broadcast(ThisChannel->Chat->CreateMessageObject(MessageData.Timetoken, MessageData));
	});
	
	//Subscribe with this channel Subscription
	FPubnubOperationResult SubscribeResult = ConnectSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	return FinalResult;
}

FPubnubChatJoinResult UPubnubChatChannel::Join(FPubnubChatMembershipData MembershipData)
{
	FPubnubChatJoinResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//SetMemberships by PubnubClient
	FPubnubMembershipInputData MembershipInputData = MembershipData.ToPubnubMembershipInputData(ChannelID);
	//This forces to reset status if not provided by User. Otherwise, Status could stay as "pending" for previously invited user.
	MembershipInputData.ForceSetStatus = true;
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(Chat->CurrentUserID, {MembershipInputData}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");

	//Create membership objects
	UPubnubChatMembership* CreatedMembership = Chat->CreateMembershipObject(Chat->CurrentUser, this, MembershipData);

	//Connect
	FPubnubChatOperationResult ConnectResult = Connect();
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, ConnectResult);

	//SetLastReadMessageTimetoken for created membership
	FPubnubChatOperationResult SetLRMTResult =  CreatedMembership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetLRMTResult);

	//Fill required data to the result
	FinalResult.Membership = CreatedMembership;
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::Disconnect()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Remove message related delegates
	ConnectSubscription->OnPubnubMessageNative.Clear();
	
	//Skip if it's not connected
	if (!IsConnected)
	{ return FinalResult; }

	//Unsubscribe and return result
	FPubnubOperationResult UnsubscribeResult = ConnectSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsConnected = false;
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::Leave()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatOperationResult FinalResult = Disconnect();

	//RemoveMemberships by PubnubClient
	FPubnubMembershipsResult RemoveMembershipsResult = PubnubClient->RemoveMemberships(Chat->CurrentUserID, {ChannelID}, FPubnubMembershipInclude::FromValue(false), 1);
	FinalResult.AddStep("RemoveMemberships", RemoveMembershipsResult.Result);

	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::SendText(const FString Message, FPubnubChatSendTextParams SendTextParams)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Message);

	//Validate quoted message if it was added to the params
	if(SendTextParams.QuotedMessage)
	{
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((SendTextParams.QuotedMessage->GetMessageData().ChannelID == ChannelID), TEXT("You cannot quote messages from other channels"));
		PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((!SendTextParams.QuotedMessage->GetMessageTimetoken().IsEmpty()), TEXT("Quoted message has empty invalid timetoken"));
	}

	//TODO:: apply rate limiter here

	//Configure settings specified in the params
	FPubnubPublishSettings PublishSettings;
	PublishSettings.MetaData = UPubnubChatInternalUtilities::SendTextMetaFromParams(SendTextParams);
	PublishSettings.StoreInHistory = SendTextParams.StoreInHistory;
	if(SendTextParams.SendByPost)
	{
		PublishSettings.PublishMethod = EPubnubPublishMethod::PPM_SendViaPOST;
	}

	//PublishMessage by PubnubClient
	FPubnubPublishMessageResult PublishResult =  PubnubClient->PublishMessage(ChannelID, UPubnubChatInternalUtilities::ChatMessageToPublishString(Message), PublishSettings);

	FPubnubChatOperationResult FinalResult;
	FinalResult.AddStep("PublishMessage", PublishResult.Result);
	
	return FinalResult;
}

FPubnubChatInviteResult UPubnubChatChannel::Invite(UPubnubChatUser* User)
{
	FPubnubChatInviteResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);

	//GetChannelMembers from PubnubClient to check if the user is not already member of that channel
	FString Filter = UPubnubChatInternalUtilities::GetFilterForUserID(User->GetUserID());
	FPubnubMemberInclude Include = FPubnubMemberInclude({.IncludeCustom=true, .IncludeStatus=true, .IncludeType=true});
	FPubnubChannelMembersResult GetChannelMembersResult = PubnubClient->GetChannelMembers(ChannelID, Include, 1, Filter);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelMembersResult.Result, "GetChannelMembers");
	
	//If User is already member of that channel, we just return the membership
	if(!GetChannelMembersResult.MembersData.IsEmpty())
	{
		FinalResult.Membership = Chat->CreateMembershipObject(User, this, GetChannelMembersResult.MembersData[0]);
		return FinalResult;
	}

	//Create Membership with "pending" status
	FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData{.Status = Pubnub_Chat_Invited_User_Membership_status};
	
	//SetMemberships by PubnubClient
	FPubnubMembershipsResult SetMembershipResult = PubnubClient->SetMemberships(User->GetUserID(), {MembershipData.ToPubnubMembershipInputData(ChannelID)}, FPubnubMembershipInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembershipResult.Result, "SetMemberships");

	//Emit Invite event
	FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Invite, User->GetUserID(), UPubnubChatInternalUtilities::GetInviteEventPayload(GetChannelData().Type, ChannelID));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, EmitEventResult);

	//Create Membership Object
	FinalResult.Membership = Chat->CreateMembershipObject(User, this, MembershipData);

	//Set Last Read Timetoken on created Membership
	FPubnubChatOperationResult SetLRMTResult = FinalResult.Membership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
	FinalResult.Result.Merge(SetLRMTResult);

	return FinalResult;
}

FPubnubChatInviteMultipleResult UPubnubChatChannel::InviteMultiple(TArray<UPubnubChatUser*> Users)
{
	FPubnubChatInviteMultipleResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	TArray<UPubnubChatUser*> ValidUsers = UPubnubChatInternalUtilities::RemoveInvalidObjects(Users);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_CONDITION_FAILED(FinalResult, !ValidUsers.IsEmpty(), TEXT("At least one valid user has to be provided"));

	FString Filter = UPubnubChatInternalUtilities::GetFilterForMultipleUsersID(ValidUsers);
	TArray<FPubnubChannelMemberInputData> MembersInput;
	//Create Membership with "pending" status
	FPubnubChatMembershipData MembershipData = FPubnubChatMembershipData{.Status = Pubnub_Chat_Invited_User_Membership_status};

	for(auto& User : ValidUsers)
	{
		MembersInput.Add(MembershipData.ToPubnubChannelMemberInputData(User->GetUserID()));
	}

	//SetChannelMembers by PubnubClient
	FPubnubChannelMembersResult SetMembersResult = PubnubClient->SetChannelMembers(ChannelID, MembersInput, FPubnubMemberInclude::FromValue(false), 1);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, SetMembersResult.Result, "SetChannelMembers");

	//For every invited user create a Membership and send invitation events
	for(auto& User : ValidUsers)
	{
		UPubnubChatMembership* Membership = Chat->CreateMembershipObject(User, this, MembershipData);
		FinalResult.Memberships.Add(Membership);

		//Emit Invite event
		FPubnubChatOperationResult EmitEventResult = Chat->EmitChatEvent(EPubnubChatEventType::PCET_Invite, User->GetUserID(), UPubnubChatInternalUtilities::GetInviteEventPayload(GetChannelData().Type, ChannelID));
		FinalResult.Result.Merge(EmitEventResult);

		//Set Last Read Timetoken on created Membership
		FPubnubChatOperationResult SetLRMTResult = Membership->SetLastReadMessageTimetoken(UPubnubTimetokenUtilities::GetCurrentUnixTimetoken());
		FinalResult.Result.Merge(SetLRMTResult);
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::PinMessage(UPubnubChatMessage* Message)
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_CONDITION_FAILED((Message->GetMessageData().ChannelID == ChannelID), TEXT("Can't pin Message from another Channel"));
	
	//Add pinned message to ChannelData
	FPubnubChatChannelData ChannelData = GetChannelData();
	UPubnubChatInternalUtilities::AddPinnedMessageToChannelData(ChannelData, Message);
	
	//Update Channel data
	FPubnubChatOperationResult UpdateResult = Update(FPubnubChatUpdateChannelInputData::FromChatChannelData(ChannelData));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::UnpinMessage()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Remove pinned message from ChannelData
	FPubnubChatChannelData ChannelData = GetChannelData();
	if (UPubnubChatInternalUtilities::RemovePinnedMessageFromChannelData(ChannelData))
	{
		//Update Channel data - only if there was pinned message removed
		FPubnubChatUpdateChannelInputData UpdateChannelInputData = FPubnubChatUpdateChannelInputData::FromChatChannelData(ChannelData);
		UpdateChannelInputData.ForceSetCustom = true; //After removing pinned message Custom field might be empty, so we force to set it
		FPubnubChatOperationResult UpdateResult = Update(UpdateChannelInputData);
		PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, UpdateResult);
	}
	
	return FinalResult;
}

FPubnubChatMessageResult UPubnubChatChannel::GetPinnedMessage()
{
	FPubnubChatMessageResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	FPubnubChatChannelData ChannelData = GetChannelData();
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	FString PinnedMessageTimetoken;
	FString PinnedMessageChannelID;
	JsonObject->TryGetStringField(UPubnubChatInternalUtilities::GetPinnedMessageTimetokenPropertyKey(), PinnedMessageTimetoken);
	JsonObject->TryGetStringField(UPubnubChatInternalUtilities::GetPinnedMessageChannelIDPropertyKey(), PinnedMessageChannelID);
	
	if (PinnedMessageTimetoken.IsEmpty() || PinnedMessageChannelID.IsEmpty())
	{ return FinalResult; }
	
	if (PinnedMessageChannelID == ChannelID)
	{
		return GetMessage(PinnedMessageTimetoken);
	}
	
	//TODO:: Get message from thread here
	
	return FinalResult;
}

FPubnubChatWhoIsPresentResult UPubnubChatChannel::WhoIsPresent(int Limit, int Offset)
{
	FPubnubChatWhoIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->WhoIsPresent(ChannelID, Limit, Offset);
}

FPubnubChatIsPresentResult UPubnubChatChannel::IsPresent(const FString UserID)
{
	FPubnubChatIsPresentResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return Chat->IsPresent(UserID, ChannelID);
}

FPubnubChatOperationResult UPubnubChatChannel::Delete(bool Soft)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();

	FPubnubChatChannelResult DeleteChannelResult = Chat->DeleteChannel(ChannelID, Soft);
	return DeleteChannelResult.Result;
}

FPubnubChatOperationResult UPubnubChatChannel::Restore()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	FPubnubChatOperationResult FinalResult;
	
	//GetChannelMetadata from PubnubClient to have up to date data
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, GetChannelResult.Result, "GetChannelMetadata");

	//Remove Deleted property from Custom field
	FPubnubChannelInputData NewChannelData = FPubnubChannelInputData::FromPubnubChannelData(GetChannelResult.ChannelData);
	NewChannelData.Custom = UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(NewChannelData.Custom);

	//SetChannelMetadata with updated metadata
	FPubnubChannelMetadataResult SetChannelResult = PubnubClient->SetChannelMetadata(ChannelID, NewChannelData);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SetChannelResult.Result, "SetChannelMetadata");
	
	return FinalResult;
}

FPubnubChatIsDeletedResult UPubnubChatChannel::IsDeleted()
{
	FPubnubChatIsDeletedResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//GetChannelMetadata from PubnubClient to have up to date data
	FPubnubChannelMetadataResult GetChannelResult = PubnubClient->GetChannelMetadata(ChannelID, FPubnubGetMetadataInclude::FromValue(true));
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetChannelResult.Result, "GetChannelMetadata");
	
	FinalResult.IsDeleted = UPubnubChatInternalUtilities::HasDeletedPropertyInCustom(GetChannelResult.ChannelData.Custom);
	
	return FinalResult;
}

FPubnubChatMembershipsResult UPubnubChatChannel::GetMembers(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatMembershipsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	//GetChannelMembers using PubnubClient
	FPubnubChannelMembersResult GetMembersResult = PubnubClient->GetChannelMembers(ChannelID, FPubnubMemberInclude::FromValue(true), Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result, "GetChannelMembers");
	
	//Create corresponding Chat objects for all returned memberships
	for (auto& MembershipData : GetMembersResult.MembersData)
	{
		UPubnubChatUser* User =  Chat->CreateUserObject(MembershipData.User.UserID, MembershipData.User);
		UPubnubChatMembership* Membership = Chat->CreateMembershipObject(User, this, MembershipData);
		FinalResult.Memberships.Add(Membership);
	}
	
	FinalResult.Page = GetMembersResult.Page;
	FinalResult.Total = GetMembersResult.TotalCount;
	return FinalResult;
}

FPubnubChatMembershipsResult UPubnubChatChannel::GetInvitees(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatMembershipsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);

	//Create final Filter so status == "pending" and eventually additional Filter provided by user
	FString FinalFilter = FString::Printf(TEXT(R"(status == "%s")"), *Pubnub_Chat_Invited_User_Membership_status);
	if (!Filter.IsEmpty())
	{
		FinalFilter += " && ";
		FinalFilter += Filter;
	}
	
	return GetMembers(Limit, FinalFilter, Sort, Page);
}

FPubnubChatOperationResult UPubnubChatChannel::SetRestrictions(const FString UserID, bool Ban, bool Mute, FString Reason)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(UserID);
	
	return Chat->SetRestrictions(FPubnubChatRestriction(UserID, ChannelID, Ban, Mute, Reason));
}

FPubnubChatGetRestrictionResult UPubnubChatChannel::GetUserRestrictions(UPubnubChatUser* User)
{
	FPubnubChatGetRestrictionResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_OBJECT_INVALID(FinalResult, User);
	
	FPubnubChatGetRestrictionsResult GetRestrictionsResult = GetRestrictions(1, UPubnubChatInternalUtilities::GetFilterForUserID(User->GetUserID()));
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetRestrictionsResult.Result);

	//If there was any restriction returned, just add it. If not, there is no restriction
	if (!GetRestrictionsResult.Restrictions.IsEmpty())
	{
		FinalResult.Restriction = GetRestrictionsResult.Restrictions[0];
	}
	else
	{
		FinalResult.Restriction = FPubnubChatRestriction({.UserID = User->GetUserID(), .ChannelID = ChannelID});
	}
	
	return FinalResult;
}

FPubnubChatGetRestrictionsResult UPubnubChatChannel::GetUsersRestrictions(const int Limit, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	
	return GetRestrictions(Limit, "", Sort, Page);
}

FPubnubChatGetHistoryResult UPubnubChatChannel::GetHistory(const FString StartTimetoken, const FString EndTimetoken, const int Count)
{
	FPubnubChatGetHistoryResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, StartTimetoken);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, EndTimetoken);
	
	FPubnubFetchHistorySettings FetchHistorySettings;
	FetchHistorySettings.MaxPerChannel = Count;
	FetchHistorySettings.Start = StartTimetoken;
	FetchHistorySettings.End = EndTimetoken;
	FetchHistorySettings.IncludeUserID = true;
	FetchHistorySettings.IncludeMessageActions = true;
	FetchHistorySettings.IncludeMeta = true;
	FPubnubFetchHistoryResult FetchHistoryResult = PubnubClient->FetchHistory(ChannelID, FetchHistorySettings);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, FetchHistoryResult.Result, "FetchHistory");
	
	for (auto& MessageData : FetchHistoryResult.Messages)
	{
		UPubnubChatMessage* Message = Chat->CreateMessageObject(MessageData.Timetoken, MessageData);
		FinalResult.Messages.Add(Message);
	}
	
	//If we got the exact amount of messages as specified count, probably there are more events in a given range
	FinalResult.IsMore = FetchHistoryResult.Messages.Num() == Count;
	
	return FinalResult;
}

FPubnubChatMessageResult UPubnubChatChannel::GetMessage(const FString Timetoken)
{
	FPubnubChatMessageResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_WRAPPER_IF_NOT_INITIALIZED(FinalResult);
	PUBNUB_CHAT_RETURN_WRAPPER_IF_FIELD_EMPTY(FinalResult, Timetoken);
	
	FString StartTimetoken = UPubnubTimetokenUtilities::AddIntToTimetoken(Timetoken, 1);
	FPubnubChatGetHistoryResult GetHistoryResult = GetHistory(StartTimetoken, Timetoken, 1);
	PUBNUB_CHAT_MERGE_CHAT_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetHistoryResult.Result);
	
	if (!GetHistoryResult.Messages.IsEmpty())
	{
		FinalResult.Message = GetHistoryResult.Messages[0];
	}
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::ForwardMessage(UPubnubChatMessage* Message)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_OBJECT_INVALID(Message);
	
	return Chat->ForwardMessage(Message, this);
}

FPubnubChatOperationResult UPubnubChatChannel::EmitUserMention(const FString UserID, const FString Timetoken, const FString Text)
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(UserID);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Timetoken);
	PUBNUB_CHAT_RETURN_OPERATION_RESULT_IF_FIELD_EMPTY(Text);
	
	FString EventPayload = UPubnubChatInternalUtilities::GetMentionEventPayload(ChannelID, Timetoken, Text);
	return Chat->EmitChatEvent(EPubnubChatEventType::PCET_Mention, UserID, EventPayload);
}

FPubnubChatOperationResult UPubnubChatChannel::StreamUpdates()
{
	FPubnubChatOperationResult FinalResult;
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	
	//Skip if it's already streaming
	if (IsStreamingUpdates)
	{ return FinalResult; }
	
	TWeakObjectPtr<UPubnubChatChannel> ThisWeak = MakeWeakObjectPtr(this);
	
	//Add lister to subscription with provided callback
	UpdatesSubscription->OnPubnubObjectEventNative.AddLambda([ThisWeak](const FPubnubMessageData& MessageData)
	{
		if(!ThisWeak.IsValid())
		{return;}
		
		UPubnubChatChannel* ThisChannel = ThisWeak.Get();

		if(!ThisChannel->Chat)
		{return;}
		
		//If this is not ChannelUpdate, just ignore this message
		if (UPubnubChatInternalUtilities::IsPubnubMessageChannelUpdate(MessageData.Message))
		{
			//Check if channel was deleted or updated
			if (UPubnubChatInternalUtilities::IsPubnubMessageDeleteEvent(MessageData.Message))
			{
				//Remove this channel from repository
				ThisChannel->Chat->ObjectsRepository->RemoveChannelData(ThisChannel->ChannelID);
				
				//Call delegates with Deleted type
				ThisChannel->OnChannelUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisChannel->ChannelID, FPubnubChatChannelData());
				ThisChannel->OnChannelUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Deleted, ThisChannel->ChannelID, FPubnubChatChannelData());
			}
			else
			{
				//Adjust this channel data based on the update message
				FPubnubChatChannelData ChatChannelData = ThisChannel->GetChannelData();
				FPubnubChannelUpdateData ChannelUpdateData = UPubnubJsonUtilities::GetChannelUpdateDataFromMessageContent(MessageData.Message);
				UPubnubChatInternalUtilities::UpdateChatChannelFromPubnubChannelUpdateData(ChannelUpdateData, ChatChannelData);
							
				//Update repository with new channel data
				ThisChannel->Chat->ObjectsRepository->UpdateChannelData(ThisChannel->ChannelID, ChatChannelData);
							
				//Call delegates with new channel data
				ThisChannel->OnChannelUpdateReceived.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisChannel->ChannelID, ChatChannelData);
				ThisChannel->OnChannelUpdateReceivedNative.Broadcast(EPubnubChatStreamedUpdateType::PCSUT_Updated, ThisChannel->ChannelID, ChatChannelData);
			}
		}
	});
	
	//Subscribe with UpdatesSubscription (not ConnectSubscription) to receive channel metadata updates
	FPubnubOperationResult SubscribeResult = UpdatesSubscription->Subscribe();
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_OPR_RESULT_IF_ERROR(FinalResult, SubscribeResult, "Subscribe");
	
	IsStreamingUpdates = true;
	
	return FinalResult;
}

FPubnubChatOperationResult UPubnubChatChannel::StopStreamingUpdates()
{
	PUBNUB_CHAT_OBJECT_RETURN_OPERATION_RESULT_IF_NOT_INITIALIZED();
	FPubnubChatOperationResult FinalResult;

	//Remove message related delegates
	UpdatesSubscription->OnPubnubObjectEventNative.Clear();
	
	//Skip if it's not streaming updates
	if (!IsStreamingUpdates)
	{ return FinalResult; }

	//Unsubscribe and return result
	FPubnubOperationResult UnsubscribeResult = UpdatesSubscription->Unsubscribe();
	FinalResult.AddStep("Unsubscribe", UnsubscribeResult);
	IsStreamingUpdates = false;
	return FinalResult;
}

void UPubnubChatChannel::InitChannel(UPubnubClient* InPubnubClient, UPubnubChat* InChat, const FString InChannelID)
{
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InPubnubClient, TEXT("Can't init Channel, PubnubClient is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(InChat, TEXT("Can't init Channel, Chat is invalid"));
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(!InChannelID.IsEmpty(), TEXT("Can't init Channel, ChannelID is empty"));

	ChannelID = InChannelID;
	PubnubClient = InPubnubClient;
	Chat = InChat;

	UPubnubChannelEntity* ChannelEntity = PubnubClient->CreateChannelEntity(ChannelID);
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ChannelEntity, TEXT("Can't init Channel, Failed to create ChannelEntity"));

	ConnectSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(ConnectSubscription, TEXT("Can't init Channel, Failed to create Connect Subscription"));
	
	UpdatesSubscription = ChannelEntity->CreateSubscription();
	PUBNUB_CHAT_RETURN_IF_CONDITION_FAILED(UpdatesSubscription, TEXT("Can't init Channel, Failed to create Updates Subscription"));
	
	// Register this channel object with the repository
	if (Chat->ObjectsRepository)
	{
		Chat->ObjectsRepository->RegisterChannel(ChannelID);
	}
	
	//Add delegate to OnChatDestroyed to this object is cleaned up as well
	Chat->OnChatDestroyed.AddDynamic(this, &UPubnubChatChannel::OnChatDestroyed);
	
	IsInitialized = true;
}

FPubnubChatGetRestrictionsResult UPubnubChatChannel::GetRestrictions(const int Limit, const FString Filter, FPubnubMemberSort Sort, FPubnubPage Page)
{
	FPubnubChatGetRestrictionsResult FinalResult;
	
	//Getting restrictions is actually getting members from moderation channel
	FString ModerationChannelID = UPubnubChatInternalUtilities::GetRestrictionsChannelForChannelID(ChannelID);
	FPubnubMemberInclude Include = FPubnubMemberInclude({.IncludeCustom = true, .IncludeTotalCount = true}); 
	FPubnubChannelMembersResult GetMembersResult = PubnubClient->GetChannelMembers(ModerationChannelID, Include, Limit, Filter, Sort, Page);
	PUBNUB_CHAT_ADD_PUBNUB_RESULT_AND_RETURN_WRAPPER_IF_ERROR(FinalResult, GetMembersResult.Result, "GetChannelMembers");
	
	//Convert Custom fields to Restrictions
	for (auto& MemberData : GetMembersResult.MembersData)
	{
		FPubnubChatRestriction Restriction = UPubnubChatInternalUtilities::GetRestrictionFromChannelMemberCustom(MemberData.Custom);
		Restriction.UserID = MemberData.User.UserID;
		Restriction.ChannelID = ChannelID;
		FinalResult.Restrictions.Add(Restriction);
	}
	
	FinalResult.Page = GetMembersResult.Page;
	FinalResult.Total = GetMembersResult.TotalCount;
	
	return FinalResult;
}

void UPubnubChatChannel::OnChatDestroyed(FString UserID)
{
	CleanUp();
}

void UPubnubChatChannel::ClearAllSubscriptions()
{
	if (ConnectSubscription)
	{
		ConnectSubscription->OnPubnubMessageNative.Clear();
		if (IsConnected)
		{
			ConnectSubscription->Unsubscribe();
			IsConnected = false;
		}

		ConnectSubscription = nullptr;
	}
	
	if (UpdatesSubscription)
	{
		UpdatesSubscription->OnPubnubObjectEventNative.Clear();
		if (IsStreamingUpdates)
		{
			UpdatesSubscription->Unsubscribe();
			IsStreamingUpdates = false;
		}

		UpdatesSubscription = nullptr;
	}
}

void UPubnubChatChannel::CleanUp()
{
	//Clean up subscription if channel is being destroyed while connected
	if (IsInitialized)
	{
		ClearAllSubscriptions();
	}
	
	//Unregister from repository before destruction
	if (IsInitialized && Chat && Chat->ObjectsRepository && !ChannelID.IsEmpty())
	{
		Chat->OnChatDestroyed.RemoveDynamic(this, &UPubnubChatChannel::OnChatDestroyed);
		Chat->ObjectsRepository->UnregisterChannel(ChannelID);
	}
	
	IsInitialized = false;
}
