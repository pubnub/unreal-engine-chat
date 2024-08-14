#pragma once

#include "CoreMinimal.h"
#include "ChatSDK.h"
#include "PubnubChatEnumLibrary.h"
#include "FunctionLibraries/PubnubChatUtilities.h"
#include "PubnubChatStructLibrary.generated.h"

class UPubnubChannel;
class UPubnubMembership;


USTRUCT(BlueprintType)
struct FPubnubStringArrayWrapper
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) TArray<FString> Strings;
	
};

USTRUCT(BlueprintType)
struct FPubnubReadReceiptsWrapper
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) TMap<FString, FPubnubStringArrayWrapper> Receipts;
	
};

USTRUCT(BlueprintType)
struct FPubnubChatChannelData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ChannelName = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Description = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString CustomDataJson = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Updated = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Status = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Type = "";

	FPubnubChatChannelData() = default;
	FPubnubChatChannelData(Pubnub::ChatChannelData ChatChannelData) :
	ChannelName(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.channel_name)),
	Description(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.description)),
	CustomDataJson(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.custom_data_json)),
	Updated(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.updated)),
	Status(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.status)),
	Type(UPubnubChatUtilities::PubnubStringToFString(ChatChannelData.type))
	{};

	//Internal use only
	Pubnub::ChatChannelData GetCppChatChannelData()
	{
		return Pubnub::ChatChannelData(
			{
				UPubnubChatUtilities::FStringToPubnubString(ChannelName),
				UPubnubChatUtilities::FStringToPubnubString(Description),
				UPubnubChatUtilities::FStringToPubnubString(CustomDataJson),
				UPubnubChatUtilities::FStringToPubnubString(Updated),
				UPubnubChatUtilities::FStringToPubnubString(Status),
				UPubnubChatUtilities::FStringToPubnubString(Type)
		});
	}
};

USTRUCT(BlueprintType)
struct FPubnubChatUserData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString UserName = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ExternalID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ProfileUrl = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Email = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString CustomDataJson = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Status = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Type = "";

	FPubnubChatUserData() = default;
	FPubnubChatUserData(Pubnub::ChatUserData ChatUserData) :
	UserName(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.user_name)),
	ExternalID(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.external_id)),
	ProfileUrl(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.profile_url)),
	Email(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.email)),
	CustomDataJson(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.custom_data_json)),
	Status(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.status)),
	Type(UPubnubChatUtilities::PubnubStringToFString(ChatUserData.type))
	{};

	//Internal use only
	Pubnub::ChatUserData GetCppChatUserData()
	{
		return Pubnub::ChatUserData(
			{
				UPubnubChatUtilities::FStringToPubnubString(UserName),
				UPubnubChatUtilities::FStringToPubnubString(ExternalID),
				UPubnubChatUtilities::FStringToPubnubString(ProfileUrl),
				UPubnubChatUtilities::FStringToPubnubString(Email),
				UPubnubChatUtilities::FStringToPubnubString(CustomDataJson),
				UPubnubChatUtilities::FStringToPubnubString(Status),
				UPubnubChatUtilities::FStringToPubnubString(Type)
		});
	}
};

USTRUCT(BlueprintType)
struct FPubnubMessageAction
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) EPubnubMessageActionType Type = EPubnubMessageActionType::PMAT_Reaction;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Value = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Timetoken = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString UserID = "";

	FPubnubMessageAction() = default;
	FPubnubMessageAction(Pubnub::MessageAction& MessageAction) :
	Type((EPubnubMessageActionType)(uint8)MessageAction.type),
	Value(UPubnubChatUtilities::PubnubStringToFString(MessageAction.value)),
	Timetoken(UPubnubChatUtilities::PubnubStringToFString(MessageAction.timetoken)),
	UserID(UPubnubChatUtilities::PubnubStringToFString(MessageAction.user_id))
	{};

	//Internal use only
	Pubnub::MessageAction GetCppMessageAction()
	{
		return Pubnub::MessageAction(
			{
				(Pubnub::pubnub_message_action_type)(uint8)Type,
				UPubnubChatUtilities::FStringToPubnubString(Value),
				UPubnubChatUtilities::FStringToPubnubString(Timetoken),
				UPubnubChatUtilities::FStringToPubnubString(UserID)
		});
	}
};

USTRUCT(BlueprintType)
struct FPubnubChatMessageData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) EPubnubChatMessageType Type = EPubnubChatMessageType::PCMT_TEXT;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Text = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ChannelID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString UserID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Meta = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) TArray<FPubnubMessageAction> MessageActions;

	FPubnubChatMessageData() = default;
	FPubnubChatMessageData(Pubnub::ChatMessageData& ChatMessageData) :
	Type((EPubnubChatMessageType)(uint8)ChatMessageData.type),
	Text(UPubnubChatUtilities::PubnubStringToFString(ChatMessageData.text)),
	ChannelID(UPubnubChatUtilities::PubnubStringToFString(ChatMessageData.channel_id)),
	UserID(UPubnubChatUtilities::PubnubStringToFString(ChatMessageData.user_id)),
	Meta(UPubnubChatUtilities::PubnubStringToFString(ChatMessageData.meta))
	{
		auto CppMessageActions = ChatMessageData.message_actions.into_std_vector();
		for(auto MessageAction : CppMessageActions)
		{
			MessageActions.Add(MessageAction);
		}
	};

	//Internal use only
	std::vector<Pubnub::MessageAction> GetCppMessageActions()
	{
		std::vector<Pubnub::MessageAction> FinalMessageActions;
		for(auto MessageAction : MessageActions)
		{
			FinalMessageActions.push_back(MessageAction.GetCppMessageAction());
		}
		return FinalMessageActions;
	};
	
	//Internal use only
	Pubnub::ChatMessageData GetCppChatMessageData()
	{
		return Pubnub::ChatMessageData(
			{
				(Pubnub::pubnub_chat_message_type)(uint8)Type,
				UPubnubChatUtilities::FStringToPubnubString(Text),
				UPubnubChatUtilities::FStringToPubnubString(ChannelID),
				UPubnubChatUtilities::FStringToPubnubString(UserID),
				UPubnubChatUtilities::FStringToPubnubString(Meta),
				GetCppMessageActions()
		});
	}
};

USTRUCT(BlueprintType)
struct FPubnubRestriction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Ban = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Mute = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Reason = "";

	FPubnubRestriction() = default;
	FPubnubRestriction(Pubnub::Restriction Restriction) :
	Ban(Restriction.ban),
	Mute(Restriction.mute),
	Reason(UPubnubChatUtilities::PubnubStringToFString(Restriction.reason))
	{};

	//Internal use only
	Pubnub::Restriction GetCppRestriction()
	{
		return Pubnub::Restriction(
			{
				Ban,
				Mute,
				UPubnubChatUtilities::FStringToPubnubString(Reason)
			});
	}
	
};

USTRUCT(BlueprintType)
struct FPubnubChannelRestriction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Ban = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Mute = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Reason = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ChannelID = "";

	FPubnubChannelRestriction() = default;
	FPubnubChannelRestriction(Pubnub::ChannelRestriction Restriction) :
	Ban(Restriction.ban),
	Mute(Restriction.mute),
	Reason(UPubnubChatUtilities::PubnubStringToFString(Restriction.reason)),
	ChannelID(UPubnubChatUtilities::PubnubStringToFString(Restriction.channel_id))
	
	{};

	//Internal use only
	Pubnub::ChannelRestriction GetCppRestriction()
	{
		return Pubnub::ChannelRestriction(
			{
				Ban,
				Mute,
				UPubnubChatUtilities::FStringToPubnubString(Reason),
				UPubnubChatUtilities::FStringToPubnubString(ChannelID)
			});
	}
	
};

USTRUCT(BlueprintType)
struct FPubnubUserRestriction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Ban = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) bool Mute = false;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Reason = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString UserID = "";

	FPubnubUserRestriction() = default;
	FPubnubUserRestriction(Pubnub::UserRestriction Restriction) :
	Ban(Restriction.ban),
	Mute(Restriction.mute),
	Reason(UPubnubChatUtilities::PubnubStringToFString(Restriction.reason)),
	UserID(UPubnubChatUtilities::PubnubStringToFString(Restriction.user_id))
	
	{};

	//Internal use only
	Pubnub::UserRestriction GetCppRestriction()
	{
		return Pubnub::UserRestriction(
			{
				Ban,
				Mute,
				UPubnubChatUtilities::FStringToPubnubString(Reason),
				UPubnubChatUtilities::FStringToPubnubString(UserID)
			});
	}
	
};

USTRUCT(BlueprintType)
struct FPubnubPage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Next = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Prev = "";

	FPubnubPage() = default;
	FPubnubPage(Pubnub::Page& Page) :
	Next(UPubnubChatUtilities::PubnubStringToFString(Page.next)),
	Prev(UPubnubChatUtilities::PubnubStringToFString(Page.prev))
	{};

	//Internal use only
	Pubnub::Page GetCppPage()
	{
		return Pubnub::Page(
			{
				UPubnubChatUtilities::FStringToPubnubString(Next),
				UPubnubChatUtilities::FStringToPubnubString(Prev)
			});
	}
	
};

USTRUCT(BlueprintType)
struct FPubnubMentionedUser
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Name = "";

	FPubnubMentionedUser() = default;
	FPubnubMentionedUser(Pubnub::MentionedUser MentionedUser) :
	ID(UPubnubChatUtilities::PubnubStringToFString(MentionedUser.id)),
	Name(UPubnubChatUtilities::PubnubStringToFString(MentionedUser.name))
	{};

	//Internal use only
	Pubnub::MentionedUser GetCppMentionedUser()
	{
		return Pubnub::MentionedUser(
			{
				UPubnubChatUtilities::FStringToPubnubString(ID),
				UPubnubChatUtilities::FStringToPubnubString(Name)
			});
	}
};

USTRUCT(BlueprintType)
struct FPubnubReferencedChannel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Name = "";

	FPubnubReferencedChannel() = default;
	FPubnubReferencedChannel(Pubnub::ReferencedChannel ReferencedChannel) :
	ID(UPubnubChatUtilities::PubnubStringToFString(ReferencedChannel.id)),
	Name(UPubnubChatUtilities::PubnubStringToFString(ReferencedChannel.name))
	{};

	//Internal use only
	Pubnub::ReferencedChannel GetCppReferencedChannel()
	{
		return Pubnub::ReferencedChannel(
			{
				UPubnubChatUtilities::FStringToPubnubString(ID),
				UPubnubChatUtilities::FStringToPubnubString(Name)
			});
	}
};

USTRUCT(BlueprintType)
struct FPubnubTextLink
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) int Start_Index = 0;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) int End_Index = 0;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Link = "";

	FPubnubTextLink() = default;
	FPubnubTextLink(Pubnub::TextLink& TextLink) :
	Start_Index(TextLink.start_index),
	End_Index(TextLink.end_index),
	Link(UPubnubChatUtilities::PubnubStringToFString(TextLink.link))
	{};

	//Internal use only
	Pubnub::TextLink GetCppTextLink()
	{
		return Pubnub::TextLink(
			{
				Start_Index,
				End_Index,
				UPubnubChatUtilities::FStringToPubnubString(Link)
			});
	}
};

USTRUCT(BlueprintType)
struct FPubnubEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Timetoken = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) EPubnubChatEventType Type;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString ChannelID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString UserID = "";
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere) FString Payload = "";

	FPubnubEvent() = default;
	FPubnubEvent(Pubnub::Event event) :
	Timetoken(UPubnubChatUtilities::PubnubStringToFString(event.timetoken)),
	Type((EPubnubChatEventType)(uint8)event.type),
	ChannelID(UPubnubChatUtilities::PubnubStringToFString(event.channel_id)),
	UserID(UPubnubChatUtilities::PubnubStringToFString(event.user_id)),
	Payload(UPubnubChatUtilities::PubnubStringToFString(event.payload))
	{};
};