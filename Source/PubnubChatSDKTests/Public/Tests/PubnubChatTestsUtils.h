// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/App.h"
#include "Misc/AutomationTest.h"
#include "Templates/Function.h"
#include "PubnubChatSubsystem.h"
#include "PubnubClient.h"
#include "PubnubEnumLibrary.h"
#include "PubnubStructLibrary.h"


class UPubnubChatSubsystem;
class UGameInstance;


namespace PubnubChatTests
{
	constexpr float MAX_WAIT_TIME = 10.0f;
	const FString SDK_PREFIX = "UE_ChatSDK_";
	
	/**
	 * Gets the Publish Key from environment variable PUBNUB_PUBLISH_KEY
	 * Falls back to "demo" if not set
	 */
	FString GetTestPublishKey();
	
	/**
	 * Gets the Subscribe Key from environment variable PUBNUB_SUBSCRIBE_KEY
	 * Falls back to "demo" if not set
	 */
	FString GetTestSubscribeKey();

	/**
	 * Subscribe returns when the subscription is registered locally. The handshake
	 * finishes later on UPubnubClient::OnSubscriptionStatusChangedNative.
	 * PSS_Connected and PSS_SubscriptionChanged both count. Chat does not forward
	 * PSS_SubscriptionChanged, so tests must listen on the client directly.
	 * The SDK copies the active channel and group lists from that callback.
	 */
	struct FChatHandshakeTracker
	{
		TSharedPtr<int32> Epoch = MakeShared<int32>(0);
		TSharedPtr<TArray<FString>> Channels = MakeShared<TArray<FString>>();
		TSharedPtr<TArray<FString>> Groups = MakeShared<TArray<FString>>();
		TSharedPtr<bool> bFailed = MakeShared<bool>(false);
		TSharedPtr<FString> FailureReason = MakeShared<FString>();
	};

	bool HandshakeListContainsAll(const TArray<FString>& Have, const TArray<FString>& Need);
	bool IsHandshakeReady(const FChatHandshakeTracker& Tracker, const TSharedPtr<int32>& Baseline, const TArray<FString>& ExpectedChannels, const TArray<FString>& ExpectedGroups);
	void TrackHandshake(UPubnubClient* Client, const FChatHandshakeTracker& Tracker);
	TSharedPtr<int32> SnapshotHandshakeEpoch(const FChatHandshakeTracker& Tracker);
}


class FWaitUntilLatentCommand : public IAutomationLatentCommand
{
public:
	FWaitUntilLatentCommand(TFunction<bool()> InCondition, float InTimeoutSeconds)
		: Condition(MoveTemp(InCondition))
		, TimeoutSeconds(InTimeoutSeconds)
		, ElapsedTime(0.0f)
	{}

	virtual bool Update() override
	{
		ElapsedTime += FApp::GetDeltaTime();
		if (Condition())
		{
			return true; // Done
		}
		return ElapsedTime >= TimeoutSeconds; // Fail if timeout
	}

private:
	TFunction<bool()> Condition;
	float TimeoutSeconds;
	float ElapsedTime;
};

class FWaitForChatHandshakeCommand : public IAutomationLatentCommand
{
public:
	FWaitForChatHandshakeCommand(PubnubChatTests::FChatHandshakeTracker InTracker, TSharedPtr<int32> InBaseline, TArray<FString> InChannels, TArray<FString> InGroups, float InTimeoutSeconds)
		: Tracker(MoveTemp(InTracker))
		, Baseline(MoveTemp(InBaseline))
		, ExpectedChannels(MoveTemp(InChannels))
		, ExpectedGroups(MoveTemp(InGroups))
		, TimeoutSeconds(InTimeoutSeconds)
		, ElapsedTime(0.0f)
	{}

	virtual bool Update() override
	{
		ElapsedTime += FApp::GetDeltaTime();
		if (*Tracker.bFailed || PubnubChatTests::IsHandshakeReady(Tracker, Baseline, ExpectedChannels, ExpectedGroups))
		{
			return true;
		}
		return ElapsedTime >= TimeoutSeconds;
	}

private:
	PubnubChatTests::FChatHandshakeTracker Tracker;
	TSharedPtr<int32> Baseline;
	TArray<FString> ExpectedChannels;
	TArray<FString> ExpectedGroups;
	float TimeoutSeconds;
	float ElapsedTime;
};

/**
 * This is dedicated class for all PubnubChat automated tests.
 * It has helper functions to initialize and clean up all required systems.
 */

class FPubnubChatAutomationTestBase: public FAutomationTestBase
{
	
public:
	FPubnubChatAutomationTestBase(const FString& InName, const bool bInComplexTask)
		: FAutomationTestBase(InName, bInComplexTask)
	{}

	//Initializes systems required by the test. This has to be called at the beginning of every test.
	bool InitTest();
	//Cleans up test systems. Call this at the end of every test
	void CleanUp();
	
	//Deletes user which was created during initializing chat
	void CleanUpCurrentChatUser(UPubnubChat* Chat);

	UPubnubChatSubsystem* ChatSubsystem = nullptr;
	UGameInstance* GameInstance = nullptr;
};


#endif // WITH_DEV_AUTOMATION_TESTS

