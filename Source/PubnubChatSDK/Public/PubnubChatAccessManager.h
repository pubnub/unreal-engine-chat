// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "PubnubChatEnumLibrary.h"
#include "PubnubChatAccessManager.generated.h"

class UPubnubClient;

/**
 * Provides access control (PAM) helpers for the Chat SDK: check permissions from the current auth token, parse tokens, set auth token, and get/set PubNub origin.
 * Used internally by the SDK (e.g. before emitting events); can also be used for debugging or to enforce UI based on permissions.
 */
UCLASS()
class PUBNUBCHATSDK_API UPubnubChatAccessManager : public UObject
{
	GENERATED_BODY()

	friend class UPubnubChat;

public:
	
	/**
	 * Checks whether the current auth token grants the given permission for the given resource. Parses the token and checks Resources (exact match) and Patterns (regex match).
	 * Local: does not perform any network requests. If no auth token is set, returns true (no PAM). Returns false if object is not initialized, ResourceName is empty, or token does not grant the permission.
	 *
	 * @param Permission The permission to check (e.g. Read, Write).
	 * @param ResourceType The resource type (e.g. Channels, Users).
	 * @param ResourceName The resource identifier (e.g. channel ID, user ID). Must be non-empty.
	 * @return True if the token grants the permission (or no token is set); false otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	bool CanI(EPubnubChatAccessManagerPermission Permission, EPubnubChatAccessManagerResourceType ResourceType, const FString ResourceName);

	/**
	 * Decodes the given token and returns a string representation of the embedded permissions (e.g. JSON). Useful for debugging or to inspect token TTL and granted permissions.
	 * Local: does not perform any network requests.
	 *
	 * @param Token The auth token to decode. Typically the current token set via SetAuthToken.
	 * @return Decoded token content as a string, or empty if object is not initialized or decode fails.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	FString ParseToken(const FString Token);

	/**
	 * Sets the authentication token used for permission checks (CanI) and for PubNub client auth. Call this when your server issues a new token for the user.
	 * Local: only updates internal state; does not perform network requests.
	 *
	 * @param Token The auth token with embedded permissions granted by your server.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	void SetAuthToken(const FString Token);

	/**
	 * Sets the PubNub origin (host) used by the underlying client. Use a custom origin only when required by your deployment.
	 * Local: updates client configuration; a new connection may be required for the change to take effect.
	 *
	 * @param Origin The origin URL/host to use.
	 * @return 0 if origin was set, +1 if it will be applied on next connection, -1 if setting origin is not enabled.
	 */
	UFUNCTION(BlueprintCallable, Category = "Pubnub Access Manager")
	int SetPubnubOrigin(const FString Origin);

	/**
	 * Returns the current PubNub origin (host) used by the underlying client.
	 * Local: does not perform any network requests.
	 *
	 * @return Current origin string, or empty if not set.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Pubnub Access Manager")
	FString GetPubnubOrigin() const;

private:
	UPROPERTY()
	TObjectPtr<UPubnubClient> PubnubClient = nullptr;
	
	UPROPERTY()
	bool IsInitialized = false;

	FString CurrentAuthToken = "";

	void InitAccessManager(UPubnubClient* InPubnubClient);
};

