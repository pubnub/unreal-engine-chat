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
			
			// Ensure that the DLL is staged along with the executable and pdb in Debug/Development build
			RuntimeDependencies.Add("$(BinaryOutputDir)/pubnub-chat.dll", "$(PluginDir)/Source/ThirdParty/sdk/lib/win64/pubnub-chat.dll");
			RuntimeDependencies.Add("$(BinaryOutputDir)/pubnub-chat.pdb", "$(PluginDir)/Source/ThirdParty/sdk/lib/win64/pubnub-chat.pdb", StagedFileType.DebugNonUFS);
		}
		else if(Target.Platform == UnrealTargetPlatform.Mac)
		{
			PublicDelayLoadDLLs.Add(Path.Combine(SDKPath, "lib", "macos", "libpubnub-chat.dylib"));
			RuntimeDependencies.Add("$(BinaryOutputDir)/libpubnub-chat.dylib", "$(PluginDir)/Source/ThirdParty/sdk/lib/macos/libpubnub-chat.dylib");
		}
		else if(Target.Platform == UnrealTargetPlatform.Linux)
		{
			string LibPath = Path.Combine(SDKPath, "lib", "Linux", "libpubnub-chat.so");

			PublicAdditionalLibraries.Add(LibPath);
			PublicDelayLoadDLLs.Add(LibPath);
			RuntimeDependencies.Add("$(BinaryOutputDir)/libpubnub-chat.so", LibPath);
		}

		PublicIncludePaths.AddRange(
			new string[] {
				SDKPath,
				Path.Combine(SDKPath, "include"),
			}
		);
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        
    }
}
