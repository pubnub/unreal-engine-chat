// Copyright 2025 PubNub Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FPubnubChatSDKModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
};
