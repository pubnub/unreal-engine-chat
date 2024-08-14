#ifndef PN_MESSAGE_DRAFT_CONFIG_H
#define PN_MESSAGE_DRAFT_CONFIG_H

#include "string.hpp"

namespace Pubnub
{
    struct MessageDraftConfig
    {
        //"channel"  or "global"
        Pubnub::String user_suggestion_source = "channel"; 
        bool is_typing_indicator_triggered = true;
        int user_limit = 10;
        int channel_limit = 10;
    };
}
#endif /* PN_MESSAGE_DRAFT_CONFIG_H */
