// Copyright 2024 PubNub Inc. All Rights Reserved.

using System;
using System.IO;
using UnrealBuildTool;

public class ChatSDKModule : ModuleRules
{

    public ChatSDKModule(ReadOnlyTargetRules Target) : base(Target)
    {
	    Type = ModuleType.External;

		var SDKPath = Path.Combine(new string[] { ModuleDirectory, ".." });
		
		
		
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string BuildLocation = "lib/win64";
			
			PublicAdditionalLibraries.Add(Path.Combine(SDKPath, BuildLocation, "pubnub-chat.lib"));
			
			PublicDelayLoadDLLs.Add("pubnub-chat.dll");
			
			// Ensure that the DLL is staged along with the executable
			RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/sdk/lib/win64/pubnub-chat.dll");
		}
		else if(Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDelayLoadDLLs.Add(Path.Combine(SDKPath, "lib", "macos", "libpubnub-chat.dylib"));
			RuntimeDependencies.Add("$(PluginDir)/Source/ThirdParty/sdk/lib/macos/libpubnub-chat.dylib");
		}


		PublicIncludePaths.AddRange(
			new string[] {
				SDKPath,
				Path.Combine(SDKPath, "Include"),
			}
		);
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        
    }
}
