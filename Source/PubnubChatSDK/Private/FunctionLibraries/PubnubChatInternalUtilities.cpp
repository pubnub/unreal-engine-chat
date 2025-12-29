// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatInternalUtilities.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"
#include "PubnubChatConst.h"
#include "PubnubChatInternalConverters.h"
#include "PubnubChatMessage.h"
#include "PubnubChatUser.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"


FString UPubnubChatInternalUtilities::GetSoftDeletedObjectPropertyKey()
{
	return Pubnub_Chat_Soft_Deleted_Property_Name;
}

FString UPubnubChatInternalUtilities::AddDeletedPropertyToCustom(FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->SetBoolField(GetSoftDeletedObjectPropertyKey(), true);
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::RemoveDeletedPropertyFromCustom(FString CurrentCustom)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(CurrentCustom, JsonObject);
	JsonObject->RemoveField(GetSoftDeletedObjectPropertyKey());
	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::ChatMessageToPublishString(const FString ChatMessage)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);

	JsonObject->SetStringField(ANSI_TO_TCHAR("text"), ChatMessage);
	//Currently the only supported type is "text"
	JsonObject->SetStringField(ANSI_TO_TCHAR("type"), "text");

	return UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::PublishedStringToChatMessage(const FString PublishedMessage)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(PublishedMessage, JsonObject);
	return JsonObject->GetStringField(ANSI_TO_TCHAR("text"));
}

FString UPubnubChatInternalUtilities::SendTextMetaFromParams(const FPubnubChatSendTextParams& SendTextParams)
{
	bool AnyDataAdded = false;
	
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	//Handle Meta
	if(!SendTextParams.Meta.IsEmpty())
	{
		UPubnubJsonUtilities::StringToJsonObject(SendTextParams.Meta, JsonObject);
		AnyDataAdded = true;
	}

	//Add quoted message
	if(SendTextParams.QuotedMessage)
	{
		FPubnubChatMessageData QuotedMessageData = SendTextParams.QuotedMessage->GetMessageData();
		
		TSharedPtr<FJsonObject> QuotedMessageJsonObject = MakeShareable(new FJsonObject);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("timetoken"), SendTextParams.QuotedMessage->GetMessageTimetoken());
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("text"), QuotedMessageData.Text);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("userID"), QuotedMessageData.UserID);
		QuotedMessageJsonObject->SetStringField(ANSI_TO_TCHAR("channelID"), QuotedMessageData.ChannelID);
		JsonObject->SetObjectField(ANSI_TO_TCHAR("quotedMessage"), QuotedMessageJsonObject);

		AnyDataAdded = true;
	}

	//Return any form of Json only if there was actually any data provided
	if(AnyDataAdded)
	{
		return UPubnubJsonUtilities::JsonObjectToString(JsonObject); 
	}

	return "";
	
}

EPubnubChatEventMethod UPubnubChatInternalUtilities::GetDefaultChatEventMethodForEventType(EPubnubChatEventType EventType)
{
	switch(EventType)
	{
	case EPubnubChatEventType::PCET_Receipt:
		return EPubnubChatEventMethod::PCEM_Signal;
	case EPubnubChatEventType::PCET_Typing:
		return EPubnubChatEventMethod::PCEM_Signal;
	default:
		return EPubnubChatEventMethod::PCEM_Publish;
	}
}

FPubnubChatEvent UPubnubChatInternalUtilities::GetEventFromPubnubMessageData(const FPubnubMessageData& MessageData)
{
	FPubnubChatEvent Event;
	Event.Timetoken = MessageData.Timetoken;
	Event.ChannelID = MessageData.Channel;
	Event.UserID = MessageData.UserID;

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(MessageData.Message, JsonObject);

	//Type is in Message content, so we need to extract it from there
	Event.Type = UPubnubChatInternalConverters::StringToChatEventType(JsonObject->GetStringField(ANSI_TO_TCHAR("type")));

	//Event type shouldn't be in the payload, so we have to remove it. Remaining message content is the payload
	JsonObject->RemoveField(ANSI_TO_TCHAR("type"));
	Event.Payload = UPubnubJsonUtilities::JsonObjectToString(JsonObject);

	return Event;
}

FString UPubnubChatInternalUtilities::GetReceiptEventPayload(const FString& Timetoken)
{
	return FString::Printf(TEXT(R"({"messageTimetoken": "%s"})"), *Timetoken);
}

FString UPubnubChatInternalUtilities::GetInviteEventPayload(const FString ChannelID, const FString ChannelType)
{
	return FString::Printf(TEXT(R"({"channelType": "%s", "channelId": "%s"})"), *ChannelType, *ChannelID);
}

FString UPubnubChatInternalUtilities::GetLastReadMessageTimetokenPropertyKey()
{
	return Pubnub_Chat_LRMT_Property_Name;
}

void UPubnubChatInternalUtilities::AddLastReadMessageTimetokenToMembershipData(FPubnubChatMembershipData& MembershipData, const FString Timetoken)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	if(!MembershipData.Custom.IsEmpty())
	{
		UPubnubJsonUtilities::StringToJsonObject(MembershipData.Custom, JsonObject);
	}
	JsonObject->SetStringField(GetLastReadMessageTimetokenPropertyKey(), Timetoken);
	MembershipData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

FString UPubnubChatInternalUtilities::GetFilterForMultipleUsersID(const TArray<UPubnubChatUser*>& Users)
{
	FString FinalFilter = "";
	for(auto& User : Users)
	{
		if(!User)
		{continue;}
		if(!FinalFilter.IsEmpty())
		{
			FinalFilter.Append(" || ");
		}
		FinalFilter.Append(FString::Printf(TEXT(R"(uuid.id == "%s")"), *User->GetUserID()));
	}
	return FinalFilter;
}

FString UPubnubChatInternalUtilities::GetPinnedMessageTimetokenPropertyKey()
{
	return Pubnub_Chat_PinnedMessageTimetoken_Property_Name;
}

FString UPubnubChatInternalUtilities::GetPinnedMessageChannelIDPropertyKey()
{
	return Pubnub_Chat_PinnedMessageChannelID_Property_Name;
}

void UPubnubChatInternalUtilities::AddPinnedMessageToChannelData(FPubnubChatChannelData& ChannelData, UPubnubChatMessage* Message)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	JsonObject->SetStringField(GetPinnedMessageTimetokenPropertyKey(), Message->GetMessageTimetoken());
	JsonObject->SetStringField(GetPinnedMessageChannelIDPropertyKey(), Message->GetMessageData().ChannelID);
	ChannelData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
}

bool UPubnubChatInternalUtilities::RemovePinnedMessageFromChannelData(FPubnubChatChannelData& ChannelData)
{
	bool RemovedPinnedMessage = false;
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	UPubnubJsonUtilities::StringToJsonObject(ChannelData.Custom, JsonObject);
	if(JsonObject->HasField(GetPinnedMessageTimetokenPropertyKey()))
	{
		RemovedPinnedMessage = true;
		JsonObject->RemoveField(GetPinnedMessageTimetokenPropertyKey());
	}
	if(JsonObject->HasField(GetPinnedMessageChannelIDPropertyKey()))
	{
		RemovedPinnedMessage = true;
		JsonObject->RemoveField(GetPinnedMessageChannelIDPropertyKey());
	}
	if(RemovedPinnedMessage)
	{
		ChannelData.Custom = UPubnubJsonUtilities::JsonObjectToString(JsonObject);
	}
	return RemovedPinnedMessage;
}

bool UPubnubChatInternalUtilities::CheckResourcePermission(const TSharedPtr<FJsonObject>& ResourcesObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr)
{
	if(!ResourcesObject.IsValid() || ResourceTypeStr.IsEmpty() || ResourceName.IsEmpty() || PermissionStr.IsEmpty())
	{
		return false;
	}

	// Get the resource type object (Channels or Uuids)
	const TSharedPtr<FJsonObject>* ResourceTypeObjectPtr = nullptr;
	if(!ResourcesObject->TryGetObjectField(ResourceTypeStr, ResourceTypeObjectPtr) || !ResourceTypeObjectPtr || !(*ResourceTypeObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceTypeObject = *ResourceTypeObjectPtr;

	// Get the specific resource object
	const TSharedPtr<FJsonObject>* ResourceObjectPtr = nullptr;
	if(!ResourceTypeObject->TryGetObjectField(ResourceName, ResourceObjectPtr) || !ResourceObjectPtr || !(*ResourceObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceObject = *ResourceObjectPtr;

	// Check if the permission field exists and is true
	if(!ResourceObject->HasField(PermissionStr))
	{
		return false;
	}

	return ResourceObject->GetBoolField(PermissionStr);
}

bool UPubnubChatInternalUtilities::CheckPatternPermission(const TSharedPtr<FJsonObject>& PatternsObject, const FString& ResourceTypeStr, const FString& ResourceName, const FString& PermissionStr)
{
	if(!PatternsObject.IsValid() || ResourceTypeStr.IsEmpty() || ResourceName.IsEmpty() || PermissionStr.IsEmpty())
	{
		return false;
	}

	// Get the resource type object (Channels or Uuids)
	const TSharedPtr<FJsonObject>* ResourceTypeObjectPtr = nullptr;
	if(!PatternsObject->TryGetObjectField(ResourceTypeStr, ResourceTypeObjectPtr) || !ResourceTypeObjectPtr || !(*ResourceTypeObjectPtr).IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>& ResourceTypeObject = *ResourceTypeObjectPtr;

	// Iterate through all patterns and check if any match the resource name
	TArray<FString> PatternKeys;
	ResourceTypeObject->Values.GetKeys(PatternKeys);

	// Check all matching patterns - return true if ANY pattern grants the permission
	for(const FString& PatternKey : PatternKeys)
	{
		// Check if the pattern matches the resource name using regex
		FRegexMatcher PatternMatcher(FRegexPattern(PatternKey), ResourceName);
		if(PatternMatcher.FindNext())
		{
			// Verify that the match spans the entire string (full match, not substring)
			// This ensures patterns like "channel-[A-Za-z0-9]" don't match "channel-abc123"
			int32 MatchStart = PatternMatcher.GetMatchBeginning();
			int32 MatchEnd = PatternMatcher.GetMatchEnding();
			
			// Only consider it a match if it spans the entire resource name
			if(MatchStart == 0 && MatchEnd == ResourceName.Len())
			{
				// Pattern matches fully, check the permission
				const TSharedPtr<FJsonObject>* PatternObjectPtr = nullptr;
				if(ResourceTypeObject->TryGetObjectField(PatternKey, PatternObjectPtr) && PatternObjectPtr && (*PatternObjectPtr).IsValid())
				{
					const TSharedPtr<FJsonObject>& PatternObject = *PatternObjectPtr;
					if(PatternObject->HasField(PermissionStr))
					{
						// If this pattern grants permission, return true immediately
						if(PatternObject->GetBoolField(PermissionStr))
						{
							return true;
						}
						// If this pattern explicitly denies permission, continue checking other patterns
						// (another pattern might grant it)
					}
				}
			}
		}
	}

	return false;
}

uint64 UPubnubChatInternalUtilities::HashString(const FString& Str, int32 Seed)
{
	// Convert FString to UTF-8 bytes for cross-platform consistency
	FTCHARToUTF8 UTF8Converter(*Str);
	const char* UTF8Bytes = UTF8Converter.Get();
	const int32 UTF8Length = UTF8Converter.Length();

	// Initialize hash accumulators with constants XORed with seed
	int32 h1 = 0xdeadbeef ^ Seed;
	int32 h2 = 0x41c6ce57 ^ Seed;

	// Process each UTF-8 byte
	for (int32 i = 0; i < UTF8Length; ++i)
	{
		const uint8 ch = static_cast<uint8>(UTF8Bytes[i]);
		h1 = (h1 ^ static_cast<int32>(ch)) * 0x85ebca77;
		h2 = (h2 ^ static_cast<int32>(ch)) * 0xc2b2ae3d;
	}

	// Final mixing
	h1 = h1 ^ ((h1 ^ (h2 >> 15)) * 0x735a2d97);
	h2 = h2 ^ ((h2 ^ (h1 >> 15)) * 0xcaf649a9);
	h1 = h1 ^ (h2 >> 16);
	h2 = h2 ^ (h1 >> 16);

	// Combine h1 and h2 into 64-bit result
	// Note: We use unsigned arithmetic to avoid sign issues
	const int64 Result = (2097152LL * static_cast<int64>(h2)) + (static_cast<int64>(h1) >> 11);

	// Handle negative result by masking upper bits (53-bit result)
	if (Result < 0)
	{
		// Mask to 53 bits: clear bits 53-63
		const uint64 Mask53Bits = ~(0xFFFFFFFFFFFFFFFFULL << 53);
		return static_cast<uint64>(Result) & Mask53Bits;
	}
	else
	{
		return static_cast<uint64>(Result);
	}
}