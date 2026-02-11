// Copyright 2025 PubNub Inc. All Rights Reserved.

#include "PubnubChatSubsystem.h"
#include "PubnubChat.h"
#include "PubnubChatUser.h"
#include "PubnubChatChannel.h"
#include "StructLibraries/PubnubChatStructLibrary.h"
#include "StructLibraries/PubnubChatUserStructLibrary.h"
#include "StructLibraries/PubnubChatChannelStructLibrary.h"
#include "Kismet/GameplayStatics.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Tests/PubnubChatTestsUtils.h"
#include "Tests/PubnubChatTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace PubnubChatTests;
using namespace PubnubChatTestHelpers;

// ============================================================================
// CLEANUP TESTS
// ============================================================================

/**
 * Cleanup test that removes all users.
 * Loops through GetUsers with pagination and deletes all users (no filter).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCleanUpUsersTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ZZCleanUp.Users", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCleanUpUsersTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_cleanup_users_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError("InitChat failed - cannot proceed with cleanup");
		CleanUp();
		return false;
	}
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat is null - cannot proceed with cleanup");
		CleanUp();
		return false;
	}
	
	// No filter - clean ALL users
	const FString Filter = TEXT("");
	const int PageLimit = 100; // Get up to 100 users per page
	
	AddInfo(TEXT("Starting user cleanup. Cleaning ALL users (no filter)."));
	
	int TotalDeletedUsers = 0;
	int TotalPagesProcessed = 0;
	const int MaxPages = 1000; // Safety limit to prevent infinite loops (allows up to 100,000 users with 100 per page)
	
	FPubnubPage CurrentPage; // Start with empty page (first page)
	
	// Loop through all pages until no more pages are available
	while(TotalPagesProcessed < MaxPages)
	{
		// Get users for current page
		FPubnubChatGetUsersResult GetUsersResult = Chat->GetUsers(PageLimit, Filter, FPubnubGetAllSort(), CurrentPage);
		
		if(GetUsersResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("GetUsers failed on page %d: %s"), TotalPagesProcessed + 1, *GetUsersResult.Result.ErrorMessage));
			break;
		}
		
		// If no users found, we're done
		if(GetUsersResult.Users.Num() == 0)
		{
			break;
		}
		
		AddInfo(FString::Printf(TEXT("Processing page %d: Found %d users"), TotalPagesProcessed + 1, GetUsersResult.Users.Num()));
		
		// Delete all users found on this page
		int DeletedOnThisPage = 0;
		for(UPubnubChatUser* User : GetUsersResult.Users)
		{
			if(User)
			{
				const FString UserID = User->GetUserID();
				FPubnubChatOperationResult DeleteResult = Chat->DeleteUser(UserID);
				
				if(DeleteResult.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to delete user %s: %s"), *UserID, *DeleteResult.ErrorMessage);
				}
				else
				{
					DeletedOnThisPage++;
					TotalDeletedUsers++;
				}
			}
		}
		
		AddInfo(FString::Printf(TEXT("Page %d: Deleted %d users (Total so far: %d)"), TotalPagesProcessed + 1, DeletedOnThisPage, TotalDeletedUsers));
		UE_LOG(LogTemp, Log, TEXT("Deleted %d users from page %d"), DeletedOnThisPage, TotalPagesProcessed + 1);
		
		// Wait a bit for server to process deletions
		FPlatformProcess::Sleep(0.5f);
		
		TotalPagesProcessed++;
		
		// Check if there's a next page
		if(GetUsersResult.Page.Next.IsEmpty())
		{
			// No more pages, we're done
			break;
		}
		
		// Move to next page
		CurrentPage = FPubnubPage();
		CurrentPage.Next = GetUsersResult.Page.Next;
	}
	
	// Check if we hit the safety limit
	if(TotalPagesProcessed >= MaxPages)
	{
		AddInfo(FString::Printf(TEXT("WARNING: Hit safety limit of %d pages. There may be more users to clean."), MaxPages));
	}
	
	// Final verification - get users one more time to see if any remain
	FPubnubChatGetUsersResult FinalCheckResult = Chat->GetUsers(PageLimit, Filter);
	int RemainingUsers = 0;
	if(!FinalCheckResult.Result.Error)
	{
		RemainingUsers = FinalCheckResult.Users.Num();
		// If there are remaining users and we haven't hit the limit, there might be more pages
		if(RemainingUsers > 0 && TotalPagesProcessed < MaxPages)
		{
			AddInfo(FString::Printf(TEXT("WARNING: %d users still remain. This may indicate pagination issue or new users were created during cleanup."), RemainingUsers));
		}
	}
	
	// Log final summary
	AddInfo(FString::Printf(TEXT("=== USER CLEANUP SUMMARY ===")));
	AddInfo(FString::Printf(TEXT("Total users deleted: %d"), TotalDeletedUsers));
	AddInfo(FString::Printf(TEXT("Pages processed: %d"), TotalPagesProcessed));
	AddInfo(FString::Printf(TEXT("Users remaining: %d"), RemainingUsers));
	AddInfo(FString::Printf(TEXT("===========================")));
	
	UE_LOG(LogTemp, Log, TEXT("Cleanup completed: Deleted %d users across %d pages"), TotalDeletedUsers, TotalPagesProcessed);
	
	if(RemainingUsers > 0)
	{
		AddInfo(FString::Printf(TEXT("WARNING: %d users still remain after cleanup"), RemainingUsers));
		UE_LOG(LogTemp, Warning, TEXT("Warning: %d users still remain after cleanup"), RemainingUsers);
	}
	
	CleanUp();
	return true;
}

/**
 * Cleanup test that removes all channels.
 * Loops through GetChannels with pagination and deletes all channels (no filter).
 */
IMPLEMENT_CUSTOM_SIMPLE_AUTOMATION_TEST(FPubnubChatCleanUpChannelsTest, FPubnubChatAutomationTestBase, "PubnubChat.Integration.ZZCleanUp.Channels", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPubnubChatCleanUpChannelsTest::RunTest(const FString& Parameters)
{
	if(!InitTest())
	{
		AddError("TestInitialization failed");
		return false;
	}

	const FString TestPublishKey = GetTestPublishKey();
	const FString TestSubscribeKey = GetTestSubscribeKey();
	const FString InitUserID = SDK_PREFIX + "test_cleanup_channels_init";
	
	FPubnubChatConfig ChatConfig;
	FPubnubChatInitChatResult InitResult = ChatSubsystem->InitChat(TestPublishKey, TestSubscribeKey, InitUserID, ChatConfig);
	
	if(InitResult.Result.Error)
	{
		AddError("InitChat failed - cannot proceed with cleanup");
		CleanUp();
		return false;
	}
	
	UPubnubChat* Chat = InitResult.Chat;
	if(!Chat)
	{
		AddError("Chat is null - cannot proceed with cleanup");
		CleanUp();
		return false;
	}
	
	// No filter - clean ALL channels
	const FString Filter = TEXT("");
	const int PageLimit = 100; // Get up to 100 channels per page
	
	AddInfo(TEXT("Starting channel cleanup. Cleaning ALL channels (no filter)."));
	
	int TotalDeletedChannels = 0;
	int TotalPagesProcessed = 0;
	const int MaxPages = 1000; // Safety limit to prevent infinite loops (allows up to 100,000 channels with 100 per page)
	
	FPubnubPage CurrentPage; // Start with empty page (first page)
	
	// Loop through all pages until no more pages are available
	while(TotalPagesProcessed < MaxPages)
	{
		// Get channels for current page
		FPubnubChatGetChannelsResult GetChannelsResult = Chat->GetChannels(PageLimit, Filter, FPubnubGetAllSort(), CurrentPage);
		
		if(GetChannelsResult.Result.Error)
		{
			AddError(FString::Printf(TEXT("GetChannels failed on page %d: %s"), TotalPagesProcessed + 1, *GetChannelsResult.Result.ErrorMessage));
			break;
		}
		
		// If no channels found, we're done
		if(GetChannelsResult.Channels.Num() == 0)
		{
			break;
		}
		
		AddInfo(FString::Printf(TEXT("Processing page %d: Found %d channels"), TotalPagesProcessed + 1, GetChannelsResult.Channels.Num()));
		
		// Delete all channels found on this page
		int DeletedOnThisPage = 0;
		for(UPubnubChatChannel* Channel : GetChannelsResult.Channels)
		{
			if(Channel)
			{
				const FString ChannelID = Channel->GetChannelID();
				FPubnubChatOperationResult DeleteResult = Chat->DeleteChannel(ChannelID);
				
				if(DeleteResult.Error)
				{
					UE_LOG(LogTemp, Warning, TEXT("Failed to delete channel %s: %s"), *ChannelID, *DeleteResult.ErrorMessage);
				}
				else
				{
					DeletedOnThisPage++;
					TotalDeletedChannels++;
				}
			}
		}
		
		AddInfo(FString::Printf(TEXT("Page %d: Deleted %d channels (Total so far: %d)"), TotalPagesProcessed + 1, DeletedOnThisPage, TotalDeletedChannels));
		UE_LOG(LogTemp, Log, TEXT("Deleted %d channels from page %d"), DeletedOnThisPage, TotalPagesProcessed + 1);
		
		// Wait a bit for server to process deletions
		FPlatformProcess::Sleep(0.5f);
		
		TotalPagesProcessed++;
		
		// Check if there's a next page
		if(GetChannelsResult.Page.Next.IsEmpty())
		{
			// No more pages, we're done
			break;
		}
		
		// Move to next page
		CurrentPage = FPubnubPage();
		CurrentPage.Next = GetChannelsResult.Page.Next;
	}
	
	// Check if we hit the safety limit
	if(TotalPagesProcessed >= MaxPages)
	{
		AddInfo(FString::Printf(TEXT("WARNING: Hit safety limit of %d pages. There may be more channels to clean."), MaxPages));
	}
	
	// Final verification - get channels one more time to see if any remain
	FPubnubChatGetChannelsResult FinalCheckResult = Chat->GetChannels(PageLimit, Filter);
	int RemainingChannels = 0;
	if(!FinalCheckResult.Result.Error)
	{
		RemainingChannels = FinalCheckResult.Channels.Num();
		// If there are remaining channels and we haven't hit the limit, there might be more pages
		if(RemainingChannels > 0 && TotalPagesProcessed < MaxPages)
		{
			AddInfo(FString::Printf(TEXT("WARNING: %d channels still remain. This may indicate pagination issue or new channels were created during cleanup."), RemainingChannels));
		}
	}
	
	// Log final summary
	AddInfo(FString::Printf(TEXT("=== CHANNEL CLEANUP SUMMARY ===")));
	AddInfo(FString::Printf(TEXT("Total channels deleted: %d"), TotalDeletedChannels));
	AddInfo(FString::Printf(TEXT("Pages processed: %d"), TotalPagesProcessed));
	AddInfo(FString::Printf(TEXT("Channels remaining: %d"), RemainingChannels));
	AddInfo(FString::Printf(TEXT("=================================")));
	
	UE_LOG(LogTemp, Log, TEXT("Cleanup completed: Deleted %d channels across %d pages"), TotalDeletedChannels, TotalPagesProcessed);
	
	if(RemainingChannels > 0)
	{
		AddInfo(FString::Printf(TEXT("WARNING: %d channels still remain after cleanup"), RemainingChannels));
		UE_LOG(LogTemp, Warning, TEXT("Warning: %d channels still remain after cleanup"), RemainingChannels);
	}
	
	CleanUp();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

