// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSDK/Private/FunctionLibraries/PubnubChatInternalUtilities.h"
#include "PubnubClient.h"
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FunctionLibraries/PubnubJsonUtilities.h"

// ============================================================================
// ACCESS MANAGER UNIT TESTS - CheckResourcePermission and CheckPatternPermission
// ============================================================================

/**
 * Helper function to parse a JSON string and extract Resources object
 */
static TSharedPtr<FJsonObject> ParseResourcesFromJson(const FString& JsonString)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	if(!UPubnubJsonUtilities::StringToJsonObject(JsonString, RootObject))
	{
		return nullptr;
	}
	
	const TSharedPtr<FJsonObject>* ResourcesObjectPtr = nullptr;
	if(!RootObject->TryGetObjectField(TEXT("Resources"), ResourcesObjectPtr) || !ResourcesObjectPtr || !(*ResourcesObjectPtr).IsValid())
	{
		return nullptr;
	}
	
	return *ResourcesObjectPtr;
}

/**
 * Helper function to parse a JSON string and extract Patterns object
 */
static TSharedPtr<FJsonObject> ParsePatternsFromJson(const FString& JsonString)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	if(!UPubnubJsonUtilities::StringToJsonObject(JsonString, RootObject))
	{
		return nullptr;
	}
	
	const TSharedPtr<FJsonObject>* PatternsObjectPtr = nullptr;
	if(!RootObject->TryGetObjectField(TEXT("Patterns"), PatternsObjectPtr) || !PatternsObjectPtr || !(*PatternsObjectPtr).IsValid())
	{
		return nullptr;
	}
	
	return *PatternsObjectPtr;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckResourcePermissionValidTest, "PubnubChat.Unit.AccessManager.CheckResourcePermission.Valid", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckResourcePermissionValidTest::RunTest(const FString& Parameters)
{
	// Parse Resources from JSON string matching token format
	const FString TokenJson = TEXT(R"({"Resources":{"Channels":{"my_channel":{"Read":true,"Write":false,"Manage":true}}}})");
	TSharedPtr<FJsonObject> ResourcesObject = ParseResourcesFromJson(TokenJson);
	TestTrue("Resources object should be parsed", ResourcesObject.IsValid());
	
	if(!ResourcesObject.IsValid())
	{
		return false;
	}
	
	// Test valid permission
	bool HasReadPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("my_channel"), TEXT("Read"));
	TestTrue("Should have Read permission", HasReadPermission);
	
	// Test denied permission
	bool HasWritePermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("my_channel"), TEXT("Write"));
	TestFalse("Should not have Write permission", HasWritePermission);
	
	// Test another valid permission
	bool HasManagePermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("my_channel"), TEXT("Manage"));
	TestTrue("Should have Manage permission", HasManagePermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckResourcePermissionUuidsTest, "PubnubChat.Unit.AccessManager.CheckResourcePermission.Uuids", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckResourcePermissionUuidsTest::RunTest(const FString& Parameters)
{
	// Parse Resources from JSON string for Uuids
	const FString TokenJson = TEXT(R"({"Resources":{"Uuids":{"User1":{"Delete":true,"Get":false,"Update":true}}}})");
	TSharedPtr<FJsonObject> ResourcesObject = ParseResourcesFromJson(TokenJson);
	TestTrue("Resources object should be parsed", ResourcesObject.IsValid());
	
	if(!ResourcesObject.IsValid())
	{
		return false;
	}
	
	// Test valid permissions
	bool HasDeletePermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Uuids"), TEXT("User1"), TEXT("Delete"));
	TestTrue("Should have Delete permission", HasDeletePermission);
	
	bool HasUpdatePermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Uuids"), TEXT("User1"), TEXT("Update"));
	TestTrue("Should have Update permission", HasUpdatePermission);
	
	// Test denied permission
	bool HasGetPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Uuids"), TEXT("User1"), TEXT("Get"));
	TestFalse("Should not have Get permission", HasGetPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckResourcePermissionMissingResourceTest, "PubnubChat.Unit.AccessManager.CheckResourcePermission.MissingResource", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckResourcePermissionMissingResourceTest::RunTest(const FString& Parameters)
{
	// Parse Resources from JSON string
	const FString TokenJson = TEXT(R"({"Resources":{"Channels":{"my_channel":{"Read":true}}}})");
	TSharedPtr<FJsonObject> ResourcesObject = ParseResourcesFromJson(TokenJson);
	TestTrue("Resources object should be parsed", ResourcesObject.IsValid());
	
	if(!ResourcesObject.IsValid())
	{
		return false;
	}
	
	// Test missing resource
	bool HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("non_existent_channel"), TEXT("Read"));
	TestFalse("Should not have permission for non-existent resource", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckResourcePermissionMissingPermissionFieldTest, "PubnubChat.Unit.AccessManager.CheckResourcePermission.MissingPermissionField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckResourcePermissionMissingPermissionFieldTest::RunTest(const FString& Parameters)
{
	// Parse Resources from JSON string without Write permission field
	const FString TokenJson = TEXT(R"({"Resources":{"Channels":{"my_channel":{"Read":true}}}})");
	TSharedPtr<FJsonObject> ResourcesObject = ParseResourcesFromJson(TokenJson);
	TestTrue("Resources object should be parsed", ResourcesObject.IsValid());
	
	if(!ResourcesObject.IsValid())
	{
		return false;
	}
	
	// Test missing permission field
	bool HasWritePermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("my_channel"), TEXT("Write"));
	TestFalse("Should not have permission when field is missing", HasWritePermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckResourcePermissionInvalidInputsTest, "PubnubChat.Unit.AccessManager.CheckResourcePermission.InvalidInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckResourcePermissionInvalidInputsTest::RunTest(const FString& Parameters)
{
	const FString TokenJson = TEXT(R"({"Resources":{"Channels":{"my_channel":{"Read":true}}}})");
	TSharedPtr<FJsonObject> ResourcesObject = ParseResourcesFromJson(TokenJson);
	TestTrue("Resources object should be parsed", ResourcesObject.IsValid());
	
	if(!ResourcesObject.IsValid())
	{
		return false;
	}
	
	// Test null ResourcesObject
	bool HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		nullptr, TEXT("Channels"), TEXT("my_channel"), TEXT("Read"));
	TestFalse("Should return false for null ResourcesObject", HasPermission);
	
	// Test empty ResourceTypeStr
	HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT(""), TEXT("my_channel"), TEXT("Read"));
	TestFalse("Should return false for empty ResourceTypeStr", HasPermission);
	
	// Test empty ResourceName
	HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT(""), TEXT("Read"));
	TestFalse("Should return false for empty ResourceName", HasPermission);
	
	// Test empty PermissionStr
	HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("Channels"), TEXT("my_channel"), TEXT(""));
	TestFalse("Should return false for empty PermissionStr", HasPermission);
	
	// Test missing ResourceType
	HasPermission = UPubnubChatInternalUtilities::CheckResourcePermission(
		ResourcesObject, TEXT("NonExistentType"), TEXT("my_channel"), TEXT("Read"));
	TestFalse("Should return false for non-existent ResourceType", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionValidTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.Valid", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionValidTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string matching token format
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-[A-Za-z0-9]*":{"Read":true,"Write":true,"Manage":false}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test pattern match with permission granted
	bool HasReadPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-abc123"), TEXT("Read"));
	TestTrue("Should have Read permission for matching pattern", HasReadPermission);
	
	bool HasWritePermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-xyz789"), TEXT("Write"));
	TestTrue("Should have Write permission for matching pattern", HasWritePermission);
	
	// Test pattern match with permission denied
	bool HasManagePermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-test"), TEXT("Manage"));
	TestFalse("Should not have Manage permission when explicitly denied", HasManagePermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionNoMatchTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.NoMatch", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionNoMatchTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-[A-Za-z0-9]*":{"Read":true}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test resource name that doesn't match pattern
	bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("other-channel"), TEXT("Read"));
	TestFalse("Should not have permission when pattern doesn't match", HasPermission);
	
	// Test resource name with invalid characters
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-test!"), TEXT("Read"));
	TestFalse("Should not have permission when pattern doesn't match invalid chars", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionMultiplePatternsTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.MultiplePatterns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionMultiplePatternsTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string with multiple patterns
	// One pattern denies, another grants - should return true if any grants
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-.*":{"Read":false},"channel-test.*":{"Read":true}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test resource that matches both patterns - should return true because one grants
	bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-test123"), TEXT("Read"));
	TestTrue("Should have permission when at least one pattern grants it", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionUuidsTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.Uuids", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionUuidsTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string for Uuids
	const FString TokenJson = TEXT(R"({"Patterns":{"Uuids":{"User[0-9]+":{"Delete":true,"Get":false,"Update":true}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test pattern match
	bool HasDeletePermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Uuids"), TEXT("User123"), TEXT("Delete"));
	TestTrue("Should have Delete permission for matching pattern", HasDeletePermission);
	
	bool HasUpdatePermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Uuids"), TEXT("User456"), TEXT("Update"));
	TestTrue("Should have Update permission for matching pattern", HasUpdatePermission);
	
	// Test denied permission
	bool HasGetPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Uuids"), TEXT("User789"), TEXT("Get"));
	TestFalse("Should not have Get permission when explicitly denied", HasGetPermission);
	
	// Test non-matching resource
	bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Uuids"), TEXT("AdminUser"), TEXT("Delete"));
	TestFalse("Should not have permission when pattern doesn't match", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionInvalidInputsTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.InvalidInputs", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionInvalidInputsTest::RunTest(const FString& Parameters)
{
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-.*":{"Read":true}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test null PatternsObject
	bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		nullptr, TEXT("Channels"), TEXT("channel-test"), TEXT("Read"));
	TestFalse("Should return false for null PatternsObject", HasPermission);
	
	// Test empty ResourceTypeStr
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT(""), TEXT("channel-test"), TEXT("Read"));
	TestFalse("Should return false for empty ResourceTypeStr", HasPermission);
	
	// Test empty ResourceName
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT(""), TEXT("Read"));
	TestFalse("Should return false for empty ResourceName", HasPermission);
	
	// Test empty PermissionStr
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-test"), TEXT(""));
	TestFalse("Should return false for empty PermissionStr", HasPermission);
	
	// Test missing ResourceType
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("NonExistentType"), TEXT("channel-test"), TEXT("Read"));
	TestFalse("Should return false for non-existent ResourceType", HasPermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionMissingPermissionFieldTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.MissingPermissionField", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionMissingPermissionFieldTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string without Write permission field
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-.*":{"Read":true}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test missing permission field - should continue checking other patterns
	bool HasWritePermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-test"), TEXT("Write"));
	TestFalse("Should not have permission when field is missing", HasWritePermission);
	
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCheckPatternPermissionComplexRegexTest, "PubnubChat.Unit.AccessManager.CheckPatternPermission.ComplexRegex", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCheckPatternPermissionComplexRegexTest::RunTest(const FString& Parameters)
{
	// Parse Patterns from JSON string with complex regex pattern similar to example token
	const FString TokenJson = TEXT(R"({"Patterns":{"Channels":{"channel-[A-Za-z0-9]":{"Read":true,"Write":true,"Join":false}}}})");
	TSharedPtr<FJsonObject> PatternsObject = ParsePatternsFromJson(TokenJson);
	TestTrue("Patterns object should be parsed", PatternsObject.IsValid());
	
	if(!PatternsObject.IsValid())
	{
		return false;
	}
	
	// Test exact pattern match (single character after dash)
	bool HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-A"), TEXT("Read"));
	TestTrue("Should match pattern with single character", HasPermission);
	
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-5"), TEXT("Read"));
	TestTrue("Should match pattern with single digit", HasPermission);
	
	// Test pattern that doesn't match (multiple characters)
	HasPermission = UPubnubChatInternalUtilities::CheckPatternPermission(
		PatternsObject, TEXT("Channels"), TEXT("channel-abc123"), TEXT("Read"));
	TestFalse("Should not match pattern requiring single character", HasPermission);
	
	return true;
}

// ============================================================================
// ACCESS MANAGER INTEGRATION TESTS - CanI, ParseToken, SetAuthToken, SetPubnubOrigin, GetPubnubOrigin
// ============================================================================

#include "PubnubChat.h"
#include "PubnubChatAccessManager.h"
#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "PubnubChatEnumLibrary.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// CANI TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanINotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanINotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		if(AccessManager)
		{
			// CanI should return false when not initialized
			bool Result = AccessManager->CanI(
				EPubnubChatAccessManagerPermission::PCAMP_Read,
				EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
				TEXT("test_channel"));
			TestFalse("CanI should return false when not initialized", Result);
		}
	}
	
	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIEmptyResourceNameTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.1Validation.EmptyResourceName", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIEmptyResourceNameTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_empty_resource";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		TestNotNull("AccessManager should exist", AccessManager);
		
		if(AccessManager)
		{
			// CanI should return false with empty ResourceName
			bool Result = AccessManager->CanI(
				EPubnubChatAccessManagerPermission::PCAMP_Read,
				EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
				TEXT(""));
			TestFalse("CanI should return false with empty ResourceName", Result);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIEmptyTokenTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.2HappyPath.EmptyToken", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIEmptyTokenTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_empty_token";
	
	FPubnubChatConfig ChatConfig;
	// Don't set AuthKey - token will be empty
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		TestNotNull("AccessManager should exist", AccessManager);
		
		if(AccessManager)
		{
			// With empty token, CanI should return true (no PAM)
			bool Result = AccessManager->CanI(
				EPubnubChatAccessManagerPermission::PCAMP_Read,
				EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
				TEXT("test_channel"));
			TestTrue("CanI should return true with empty token (no PAM)", Result);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIWithResourcesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.2HappyPath.WithResources", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIWithResourcesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_resources";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		CleanUp();
		return false;
	}
		
	UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with Resources permissions
	FPubnubGrantTokenPermissions Permissions;
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	ChannelGrant.Permissions.Write = false;
	ChannelGrant.Permissions.Manage = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		// If token granting fails (e.g., no SecretKey), skip this test
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		// Set the token
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Test Read permission (should be true)
		bool HasRead = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestTrue("Should have Read permission", HasRead);
		
		// Test Write permission (should be false)
		bool HasWrite = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Write,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestFalse("Should not have Write permission", HasWrite);
		
		// Test Manage permission (should be true)
		bool HasManage = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Manage,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestTrue("Should have Manage permission", HasManage);
		
		// Test non-existent resource (should be false)
		bool HasPermission = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("non_existent_channel"));
		TestFalse("Should not have permission for non-existent resource", HasPermission);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIWithPatternsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.2HappyPath.WithPatterns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIWithPatternsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_patterns";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		CleanUp();
		return false;
	}
		
	UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return false;
	}
	
	// Grant a token with Patterns permissions
	FPubnubGrantTokenPermissions Permissions;
	FChannelGrant ChannelPattern;
	ChannelPattern.Channel = TEXT("channel-[A-Za-z0-9]*");
	ChannelPattern.Permissions.Read = true;
	ChannelPattern.Permissions.Write = true;
	ChannelPattern.Permissions.Manage = false;
	Permissions.ChannelPatterns.Add(ChannelPattern);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		// If token granting fails (e.g., no SecretKey), skip this test
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		// Set the token
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Test matching pattern (should be true)
		bool HasRead = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("channel-abc123"));
		TestTrue("Should have Read permission for matching pattern", HasRead);
		
		// Test Write permission (should be true)
		bool HasWrite = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Write,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("channel-xyz789"));
		TestTrue("Should have Write permission for matching pattern", HasWrite);
		
		// Test Manage permission (should be false)
		bool HasManage = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Manage,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("channel-test"));
		TestFalse("Should not have Manage permission", HasManage);
		
		// Test non-matching resource (should be false)
		bool HasPermission = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("other-channel"));
		TestFalse("Should not have permission for non-matching pattern", HasPermission);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// FULL TESTS (All Parameters)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIAllPermissionsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.3Full.AllPermissions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIAllPermissionsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_all_permissions";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		CleanUp();
		return false;
	}
	
	UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with all permissions
	FPubnubGrantTokenPermissions Permissions;
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	ChannelGrant.Permissions.Write = true;
	ChannelGrant.Permissions.Manage = true;
	ChannelGrant.Permissions.Delete = true;
	ChannelGrant.Permissions.Get = true;
	ChannelGrant.Permissions.Update = true;
	ChannelGrant.Permissions.Join = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Test all permissions
		TestTrue("Should have Read permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Read, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Write permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Write, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Manage permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Manage, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Delete permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Delete, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Get permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Get, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Update permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Update, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		TestTrue("Should have Join permission", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Join, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIAllResourceTypesTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.3Full.AllResourceTypes", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIAllResourceTypesTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_all_resource_types";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
		CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with permissions for both Channels and Uuids
	FPubnubGrantTokenPermissions Permissions;
	
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FUserGrant UserGrant;
	UserGrant.User = TEXT("test_user");
	UserGrant.Permissions.Get = true;
	UserGrant.Permissions.Update = true;
	Permissions.Users.Add(UserGrant);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
	CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Test Channels resource type
		TestTrue("Should have Read permission for Channels", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Read, EPubnubChatAccessManagerResourceType::PCAMRT_Channels, TEXT("test_channel")));
		
		// Test Uuids resource type
		TestTrue("Should have Get permission for Uuids", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Get, EPubnubChatAccessManagerResourceType::PCAMRT_Users, TEXT("test_user")));
		TestTrue("Should have Update permission for Uuids", AccessManager->CanI(EPubnubChatAccessManagerPermission::PCAMP_Update, EPubnubChatAccessManagerResourceType::PCAMRT_Users, TEXT("test_user")));
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// ADVANCED SCENARIO TESTS
// ============================================================================

// Test that Resources are checked before Patterns, and if Resources grants permission, Patterns are not checked
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIResourcesBeforePatternsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.4Advanced.ResourcesBeforePatterns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIResourcesBeforePatternsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_resources_before_patterns";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
			CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with both Resources (denies Write) and Patterns (grants Write)
	// Resources should take precedence
	FPubnubGrantTokenPermissions Permissions;
	
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	ChannelGrant.Permissions.Write = false; // Denied in Resources
	Permissions.Channels.Add(ChannelGrant);
	
	FChannelGrant ChannelPattern;
	ChannelPattern.Channel = TEXT("test_channel");
	ChannelPattern.Permissions.Write = true; // Granted in Patterns
	Permissions.ChannelPatterns.Add(ChannelPattern);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Resources should be checked first - Write should be denied even though Patterns grants it
		bool HasWrite = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Write,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestFalse("Resources should take precedence - Write should be denied", HasWrite);
		
		// Read should be granted from Resources
		bool HasRead = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestTrue("Read should be granted from Resources", HasRead);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// Test that if Resources doesn't grant permission, Patterns is checked
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIResourcesThenPatternsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.4Advanced.ResourcesThenPatterns", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIResourcesThenPatternsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_resources_then_patterns";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
			CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with Resources for one channel, Patterns for another
	FPubnubGrantTokenPermissions Permissions;
	
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("exact_channel");
	ChannelGrant.Permissions.Read = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FChannelGrant ChannelPattern;
	ChannelPattern.Channel = TEXT("pattern-.*");
	ChannelPattern.Permissions.Read = true;
	Permissions.ChannelPatterns.Add(ChannelPattern);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Exact channel should be granted from Resources
		bool HasReadExact = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("exact_channel"));
		TestTrue("Exact channel should be granted from Resources", HasReadExact);
		
		// Pattern channel should be granted from Patterns (Resources doesn't have it)
		bool HasReadPattern = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("pattern-test123"));
		TestTrue("Pattern channel should be granted from Patterns", HasReadPattern);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// Test that if both Resources and Patterns are missing, CanI returns false
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerCanIBothMissingTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.CanI.4Advanced.BothMissing", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerCanIBothMissingTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_cani_both_missing";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
			CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token with empty permissions (no Resources or Patterns)
	FPubnubGrantTokenPermissions Permissions;
	// Don't add any channels or patterns
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// With no Resources or Patterns, CanI should return false
		bool HasPermission = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestFalse("Should return false when both Resources and Patterns are missing", HasPermission);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// PARSETOKEN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerParseTokenNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.ParseToken.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerParseTokenNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		if(AccessManager)
		{
			// ParseToken should return empty string when not initialized
			FString Result = AccessManager->ParseToken(TEXT("test_token"));
			TestTrue("ParseToken should return empty string when not initialized", Result.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerParseTokenHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.ParseToken.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerParseTokenHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_parse_token";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
			CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token
	FPubnubGrantTokenPermissions Permissions;
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		// ParseToken should return a non-empty JSON string
		FString ParsedToken = AccessManager->ParseToken(GrantResult.Token);
		TestFalse("ParseToken should return non-empty string", ParsedToken.IsEmpty());
		
		// Parsed token should contain Resources or Patterns
		TestTrue("Parsed token should contain Resources or Patterns", ParsedToken.Contains(TEXT("Resources")) || ParsedToken.Contains(TEXT("Patterns")));
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// SETAUTHTOKEN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerSetAuthTokenNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.SetAuthToken.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerSetAuthTokenNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		if(AccessManager)
		{
			// SetAuthToken should not crash when not initialized (it checks internally)
			// We can't easily test the internal check, but we can verify it doesn't crash
			AccessManager->SetAuthToken(TEXT("test_token"));
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerSetAuthTokenHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.SetAuthToken.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerSetAuthTokenHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_set_auth_token";
	
		FPubnubChatConfig ChatConfig;
		FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
		
		if(InitResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("InitChat failed: %s"), *InitResult.Result.ErrorMessage));
			CleanUp();
			return false;
		}
		
		UPubnubChat* Chat = InitResult.Chat;
	
	if(!Chat)
	{
		AddError("Chat should exist after InitChat");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	UPubnubClient* PubnubClient = Chat->GetPubnubClient();
	if(!PubnubClient)
	{
		AddError("PubnubClient should exist");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return false;
	}
	
	// Grant a token
	FPubnubGrantTokenPermissions Permissions;
	FChannelGrant ChannelGrant;
	ChannelGrant.Channel = TEXT("test_channel");
	ChannelGrant.Permissions.Read = true;
	Permissions.Channels.Add(ChannelGrant);
	
	FPubnubGrantTokenResult GrantResult = PubnubClient->GrantToken(60, TestUserID, Permissions);
	
	if(GrantResult.Result.Error || GrantResult.Token.IsEmpty())
	{
		AddInfo("Token granting failed or not configured - skipping test");
		CleanUpCurrentChatUser(Chat);
		CleanUp();
		return true;
	}
	
	UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
	TestNotNull("AccessManager should exist", AccessManager);
	
	if(AccessManager)
	{
		// SetAuthToken should succeed
		AccessManager->SetAuthToken(GrantResult.Token);
		
		// Verify token is set by checking CanI
		bool HasRead = AccessManager->CanI(
			EPubnubChatAccessManagerPermission::PCAMP_Read,
			EPubnubChatAccessManagerResourceType::PCAMRT_Channels,
			TEXT("test_channel"));
		TestTrue("Token should be set and CanI should work", HasRead);
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// SETPUBNUBORIGIN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerSetPubnubOriginNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.SetPubnubOrigin.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerSetPubnubOriginNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		if(AccessManager)
		{
			// SetPubnubOrigin should return -1 when not initialized
			int Result = AccessManager->SetPubnubOrigin(TEXT("test.origin.com"));
			TestEqual("SetPubnubOrigin should return -1 when not initialized", Result, -1);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerSetPubnubOriginHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.SetPubnubOrigin.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerSetPubnubOriginHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_set_origin";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		TestNotNull("AccessManager should exist", AccessManager);
		
		if(AccessManager)
		{
			// SetPubnubOrigin should return 0 or 1 (0 = set immediately, 1 = set on next connection)
			int Result = AccessManager->SetPubnubOrigin(TEXT("ps.pndsn.com"));
			TestTrue("SetPubnubOrigin should return 0 or 1", Result == 0 || Result == 1);
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// GETPUBNUBORIGIN TESTS
// ============================================================================

// ============================================================================
// VALIDATION TESTS (Fast Failing Conditions)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerGetPubnubOriginNotInitializedTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.GetPubnubOrigin.1Validation.NotInitialized", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerGetPubnubOriginNotInitializedTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	// Create Chat without initializing
	UPubnubChat* Chat = NewObject<UPubnubChat>();

	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		if(AccessManager)
		{
			// GetPubnubOrigin should return empty string when not initialized
			FString Result = AccessManager->GetPubnubOrigin();
			TestTrue("GetPubnubOrigin should return empty string when not initialized", Result.IsEmpty());
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

// ============================================================================
// HAPPY PATH TESTS (Required Parameters Only)
// ============================================================================

IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatAccessManagerGetPubnubOriginHappyPathTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.AccessManager.GetPubnubOrigin.2HappyPath.RequiredParametersOnly", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatAccessManagerGetPubnubOriginHappyPathTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString TestUserID = SDK_PREFIX + "test_get_origin";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, TestUserID, ChatConfig);
	
	TestFalse("InitChat should succeed", InitResult.Result.Error);
	TestNotNull("Chat object should be created", InitResult.Chat);
	
	UPubnubChat* Chat = InitResult.Chat;
	if(Chat)
	{
		UPubnubChatAccessManager* AccessManager = Chat->GetAccessManager();
		TestNotNull("AccessManager should exist", AccessManager);
		
		if(AccessManager)
		{
			// GetPubnubOrigin should return a non-empty string (default origin)
			FString Origin = AccessManager->GetPubnubOrigin();
			TestFalse("GetPubnubOrigin should return non-empty string", Origin.IsEmpty());
			
			// Set a custom origin
			AccessManager->SetPubnubOrigin(TEXT("custom.origin.com"));
			
			// GetPubnubOrigin should return the custom origin
			FString CustomOrigin = AccessManager->GetPubnubOrigin();
			TestEqual("GetPubnubOrigin should return the set origin", CustomOrigin, TEXT("custom.origin.com"));
		}
	}

	CleanUpCurrentChatUser(Chat);
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
