// Copyright 2026 PubNub Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class PubnubChatSDKTests : ModuleRules
{
	public PubnubChatSDKTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PrivateIncludePaths.Add(Path.Combine(new string[] { ModuleDirectory, "..", "PubnubChatSDK" }));
		
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

