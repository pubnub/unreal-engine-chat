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
