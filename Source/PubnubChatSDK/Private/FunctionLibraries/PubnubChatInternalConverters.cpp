// Copyright 2025 PubNub Inc. All Rights Reserved.


#include "FunctionLibraries/PubnubChatInternalConverters.h"

EPubnubChatConnectionStatus UPubnubChatInternalConverters::SubscriptionStatusToChatConnectionStatus(EPubnubSubscriptionStatus SubscriptionStatus)
{
	switch(SubscriptionStatus)
	{
	case EPubnubSubscriptionStatus::PSS_Connected:
		return EPubnubChatConnectionStatus::PCCS_CONNECTION_ONLINE;
	case EPubnubSubscriptionStatus::PSS_Disconnected:
		return EPubnubChatConnectionStatus::PCCS_CONNECTION_OFFLINE;
	case EPubnubSubscriptionStatus::PSS_ConnectionError:
		return EPubnubChatConnectionStatus::PCCS_CONNECTION_ERROR;
	case EPubnubSubscriptionStatus::PSS_DisconnectedUnexpectedly:
		return EPubnubChatConnectionStatus::PCCS_CONNECTION_ERROR;
	default:
		return EPubnubChatConnectionStatus::PCCS_CONNECTION_ERROR;
	} 
}

FPubnubChatConnectionStatusData UPubnubChatInternalConverters::SubscriptionStatusDataToChatConnectionStatusData(const FPubnubSubscriptionStatusData& SubscriptionStatusData)
{
	FPubnubChatConnectionStatusData Data;
	Data.Reason = SubscriptionStatusData.Reason;
	return Data;
}

FString UPubnubChatInternalConverters::ChatMessageActionTypeToString(EPubnubChatMessageActionType ActionType)
{
	switch(ActionType)
	{
	case EPubnubChatMessageActionType::PCMAT_Reaction:
		return TEXT("reaction");
	case EPubnubChatMessageActionType::PCMAT_Receipt:
		return TEXT("receipt");
	case EPubnubChatMessageActionType::PCMAT_Custom:
		return TEXT("custom");
	case EPubnubChatMessageActionType::PCMAT_Edited:
		return TEXT("edited");
	case EPubnubChatMessageActionType::PCMAT_Deleted:
		return TEXT("deleted");
	default:
		return TEXT("reaction");
	}
}

EPubnubChatMessageActionType UPubnubChatInternalConverters::StringToChatMessageActionType(const FString& ActionTypeString)
{
	if (ActionTypeString.Equals(TEXT("reaction"), ESearchCase::IgnoreCase))
	{
		return EPubnubChatMessageActionType::PCMAT_Reaction;
	}
	else if (ActionTypeString.Equals(TEXT("receipt"), ESearchCase::IgnoreCase))
	{
		return EPubnubChatMessageActionType::PCMAT_Receipt;
	}
	else if (ActionTypeString.Equals(TEXT("custom"), ESearchCase::IgnoreCase))
	{
		return EPubnubChatMessageActionType::PCMAT_Custom;
	}
	else if (ActionTypeString.Equals(TEXT("edited"), ESearchCase::IgnoreCase))
	{
		return EPubnubChatMessageActionType::PCMAT_Edited;
	}
	else if (ActionTypeString.Equals(TEXT("deleted"), ESearchCase::IgnoreCase))
	{
		return EPubnubChatMessageActionType::PCMAT_Deleted;
	}
	else
	{
		// Default to Reaction if unknown
		return EPubnubChatMessageActionType::PCMAT_Reaction;
	}
}