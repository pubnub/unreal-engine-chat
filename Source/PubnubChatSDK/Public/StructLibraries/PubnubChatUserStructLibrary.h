// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PubnubStructLibrary.h"
#include "PubnubChatStructLibrary.h"
#include "PubnubChatUserStructLibrary.generated.h"

class UPubnubChatUser;


/**
 * Basic user data structure used when creating or viewing user profiles.
 * Contains editable metadata for user configuration.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUserData
{
	GENERATED_BODY()
	
	/** Display name for the user. Shown in UI for user-friendly identification. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserName = "";
	/** User's identifier in an external system (e.g., your backend user ID). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ExternalID = "";
	/** The URL of the user's profile picture/avatar. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ProfileUrl = "";
	/** The user's email address. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Email = "";
	/** JSON object providing custom user data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** User status. Max. 50 characters. Examples: "online", "away", "busy", "offline". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** User type. Max. 50 characters. Examples: "admin", "moderator", "member". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";

	FPubnubUserInputData ToPubnubUserInputData() const;
	static FPubnubChatUserData FromPubnubUserData(const FPubnubUserData &PubnubUserData);
};

/**
 * User data structure for partial updates with ForceSet flags.
 * Use ForceSet flags to explicitly set fields to empty values when needed.
 * Without ForceSet, empty strings are treated as "no change" during updates.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUpdateUserInputData
{
	GENERATED_BODY()
	
	/** Display name for the user. Shown in UI for user-friendly identification. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString UserName = "";
	/** User's identifier in an external system (e.g., your backend user ID). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ExternalID = "";
	/** The URL of the user's profile picture/avatar. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ProfileUrl = "";
	/** The user's email address. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Email = "";
	/** JSON object providing custom user data. Only a single level of key-value pairs is allowed. Nested JSON objects or arrays are not supported. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Custom = "";
	/** User status. Max. 50 characters. Examples: "online", "away", "busy", "offline". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Status = "";
	/** User type. Max. 50 characters. Examples: "admin", "moderator", "member". */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Type = "";
	
	/** When true, UserName will be updated even if empty. Use to explicitly clear the username. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetUserName = false;
	
	/** When true, ExternalID will be updated even if empty. Use to explicitly clear the external ID. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetExternalID = false;
	
	/** When true, ProfileUrl will be updated even if empty. Use to explicitly clear the profile URL. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetProfileUrl = false;
	
	/** When true, Email will be updated even if empty. Use to explicitly clear the email. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetEmail = false;
	
	/** When true, Custom will be updated even if empty. Use to explicitly clear custom data. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetCustom = false;
	
	/** When true, Status will be updated even if empty. Use to explicitly clear the status. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetStatus = false;
	
	/** When true, Type will be updated even if empty. Use to explicitly clear the type. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, AdvancedDisplay, Category = "PubnubChat") 
	bool ForceSetType = false;
	
	/**
	 * Sets all ForceSet flags to true. Useful for full replacement of User Data.
	 */
	void ForceSetAllFields()
	{
		ForceSetUserName = true;
		ForceSetExternalID = true;
		ForceSetProfileUrl = true;
		ForceSetEmail = true;
		ForceSetCustom = true;
		ForceSetStatus = true;
		ForceSetType = true;
	}

	FPubnubUserInputData ToPubnubUserInputData() const;
	static FPubnubChatUpdateUserInputData FromChatUserData(const FPubnubChatUserData &PubnubUserData);
};

/**
 * Result of a single user retrieval or modification operation.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUserResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** The user object. Null if the operation failed. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") UPubnubChatUser* User = nullptr;
};

/**
 * Result of a paginated user list query.
 * Contains multiple users with pagination information for fetching more results.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetUsersResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of retrieved user objects matching the query criteria. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatUser*> Users;
	/** Pagination tokens for retrieving next/previous pages of results. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubPage Page;
	/** Total number of users matching the query (may exceed returned count due to pagination). */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") int Total = 0;
};

/**
 * Result of a user suggestion query for mentions.
 * Used by message drafts to suggest users when typing @user references.
 */
USTRUCT(BlueprintType)
struct FPubnubChatGetUserSuggestionsResult
{
	GENERATED_BODY()

	/** Operation result containing success/error status and detailed step information. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FPubnubChatOperationResult Result;
	/** Array of suggested users matching the search text. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") TArray<UPubnubChatUser*> Users;
};

/**
 * Represents a user mention event containing information about where and by whom the user was mentioned.
 */
USTRUCT(BlueprintType)
struct FPubnubChatUserMention
{
	GENERATED_BODY()
	
	/** Timetoken of the message containing the mention. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString MessageTimetoken = "";
	/** ID of the channel where the mention occurred. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ChannelID = "";
	/** ID of the parent channel (for thread mentions). Empty if not in a thread. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString ParentChannelID = "";
	/** User ID of the user who mentioned this user. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString MentionedByUserID = "";
	/** Text content of the message containing the mention. */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "PubnubChat") FString Text = "";
};
