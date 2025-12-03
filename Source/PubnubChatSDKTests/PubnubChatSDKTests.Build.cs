// Copyright 2025 PubNub Inc. All Rights Reserved.

using UnrealBuildTool;

public class PubnubChatSDKTests : ModuleRules
{
	public PubnubChatSDKTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		

		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"PubnubChatSDK",
				"PubnubLibrary",
				"Projects"
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"Json",
				"JsonUtilities"
			}
			);
	}
}

