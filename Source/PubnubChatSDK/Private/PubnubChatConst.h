// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once


const FString Pubnub_Chat_Custom_Property_Prefix = "PN_PRIV.";
const FString Pubnub_Chat_Soft_Deleted_Property_Name = "deleted";
const FString Pubnub_Chat_Soft_Deleted_Action_Value = "deleted";
const FString Pubnub_Chat_Invited_User_Membership_status = "pending";
//Last Read Message Timetoken field name in Json
const FString Pubnub_Chat_LRMT_Property_Name = "lastReadMessageTimetoken";
//Pinned Message Timetoken field name in Json
const FString Pubnub_Chat_PinnedMessageTimetoken_Property_Name = "pinnedMessageTimetoken";
//Pinned Message Channel ID field name in Json
const FString Pubnub_Chat_PinnedMessageChannelID_Property_Name = "pinnedMessageChannelID";
//Prefix for restrictions channels
const FString Pubnub_Chat_Moderation_Channel_Prefix = "PUBNUB_INTERNAL_MODERATION";
//OriginalPublisher of Forwarded Message in new message Meta
const FString Pubnub_Chat_ForwardMessage_OriginalPublisher_Property_Name = "originalPublisher";
//OriginalChannelID of Forwarded Message in new message Meta
const FString Pubnub_Chat_ForwardMessage_OriginalChannelID_Property_Name = "originalChannelId";
//Some functions can't accept "0" or "" as empty timetoken, so we use this one for such purpose
const FString Pubnub_Chat_Empty_Timetoken = "17000000000000000";
//Minimal Typing Indicator Timeout in MS
constexpr int Pubnub_Chat_Min_Typing_Indicator_Timeout = 1000;
constexpr int Pubnub_Chat_Typing_Timeout_Margin = 500;
//Prefix for ChannelID that is a Thread
const FString Pubnub_Chat_Message_Thread_ID_Prefix = "PUBNUB_INTERNAL_THREAD";
//Maximum SendText delay calculated by rate limiter
constexpr int Pubnub_Chat_Max_Rate_Limiter_Delay = 10000;
//Last Active Timestamp field name in Json
const FString Pubnub_Chat_LastActiveTimestamp_Property_Name = "lastActiveTimestamp";
//Minimum StoreUserActivityInterval in milliseconds (1 minute)
constexpr int Pubnub_Chat_Min_StoreUserActivityInterval = 60000;