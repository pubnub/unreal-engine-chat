// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatCallbackStop.generated.h"


/**
 * Handle returned by ListenForEvents (and similar) to stop listening. Call Stop() to unsubscribe and release the listener.
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatCallbackStop : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;
	friend class UPubnubChatUser;
	friend class UPubnubChatChannel;
	friend class UPubnubChatMessage;

public:
	virtual void BeginDestroy() override;
	
	/**
	 * Stops the listener associated with this handle (e.g. unsubscribes from events). No-op if already stopped.
	 *
	 * @return Operation result. Success if the listener was stopped (or was already stopped).
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Chat|CallbackStop")
	FPubnubChatOperationResult Stop();

private:
	void InitCallbackStop(TFunction<FPubnubChatOperationResult()> InStopCallback);

	bool IsStopped = false;
	
	TFunction<FPubnubChatOperationResult()> StopCallback = nullptr;
};

