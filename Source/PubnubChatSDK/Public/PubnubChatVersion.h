// Copyright 2026 PubNub Inc. All Rights Reserved.

#pragma once

#define PUBNUB_CHAT_VERSION_MAJOR 1
#define PUBNUB_CHAT_VERSION_MINOR 0
#define PUBNUB_CHAT_VERSION_PATCH 0
#define PUBNUB_CHAT_VERSION ((PUBNUB_CHAT_VERSION_MAJOR * 10000) + (PUBNUB_CHAT_VERSION_MINOR * 100) + PUBNUB_CHAT_VERSION_PATCH)

/** Minimum PubnubLibrary version required. Bump when Chat needs API from a newer PubnubLibrary. Encoding: (major*10000)+(minor*100)+patch. */
#define PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION 20001
#define PUBNUB_CHAT_REQUIRES_LIBRARY_ERROR_MSG "PubnubChat requires PubnubLibrary 1.2.0 or newer. Please update the PubnubLibrary plugin."

#include "PubnubLibraryVersion.h"
#if PUBNUB_LIBRARY_VERSION < PUBNUB_CHAT_REQUIRES_LIBRARY_VERSION
#error PUBNUB_CHAT_REQUIRES_LIBRARY_ERROR_MSG
#endif
