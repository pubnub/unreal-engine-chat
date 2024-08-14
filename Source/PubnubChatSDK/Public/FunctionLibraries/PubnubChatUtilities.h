// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatSDK.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PubnubChatUtilities.generated.h"

class UPubnubChannel;
class UPubnubMessage;
class UPubnubUser;
class UPubnubMembership;
class UPubnubThreadMessage;
struct FPubnubMessageAction;

/**
 * 
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	static FString PubnubStringToFString(Pubnub::String PubnubString);
	static Pubnub::String FStringToPubnubString(FString UEString);

	static TArray<FString> PubnubStringsToFStrings(Pubnub::Vector<Pubnub::String> &PubnubStrings);
	static Pubnub::Vector<Pubnub::String> FStringsToPubnubStrings(TArray<FString> &UEStrings);

	static TArray<UPubnubChannel*> CppChannelsToUnrealChannels(Pubnub::Vector<Pubnub::Channel> &CppChannels);
	static TArray<UPubnubUser*> CppUsersToUnrealUsers(Pubnub::Vector<Pubnub::User> &CppUsers);
	static TArray<UPubnubMessage*> CppMessagesToUnrealMessages(Pubnub::Vector<Pubnub::Message> &CppMessages);
	static TArray<UPubnubThreadMessage*> CppThreadMessagesToUnrealTMessages(Pubnub::Vector<Pubnub::ThreadMessage> &CppThreadMessages);
	static TArray<UPubnubMembership*> CppMembershipsToUnrealMemberships(Pubnub::Vector<Pubnub::Membership> &CppMemberships);
	static TArray<FPubnubMessageAction> CppMessageActionsToUnrealMessageActions(Pubnub::Vector<Pubnub::MessageAction> &CppMessageActions);

	static Pubnub::Vector<Pubnub::Channel> UnrealChannelsToCppChannels(TArray<UPubnubChannel*> &PubnubChannels);
	static Pubnub::Vector<Pubnub::User> UnrealUsersToCppUsers(TArray<UPubnubUser*> &PubnubUsers);
	static Pubnub::Vector<Pubnub::Message> UnrealMessagesToCppMessages(TArray<UPubnubMessage*> &PubnubMessages);
	static Pubnub::Vector<Pubnub::ThreadMessage> UnrealThreadMessagesToCppTMessages(TArray<UPubnubThreadMessage*> &PubnubThreadMessages);
	static Pubnub::Vector<Pubnub::Membership> UnrealMembershipsToCppMemberships(TArray<UPubnubMembership*> &PubnubMemberships);
	static Pubnub::Vector<Pubnub::MessageAction> UnrealMessageActionsToCppMessageActions(TArray<FPubnubMessageAction> &PubnubMessageActions);
};
