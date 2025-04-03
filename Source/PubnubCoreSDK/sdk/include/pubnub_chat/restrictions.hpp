#ifndef PN_RESTRICTIONS_H
#define PN_RESTRICTIONS_H

#include "string.hpp"

namespace Pubnub
{
    struct Restriction
    {
        bool ban = false;
        bool mute = false;
        Pubnub::String reason = "";
    };

    struct ChannelRestriction
    {
        bool ban = false;
        bool mute = false;
        Pubnub::String reason = "";
        Pubnub::String channel_id = "";
    };

    struct UserRestriction
    {
        bool ban = false;
        bool mute = false;
        Pubnub::String reason = "";
        Pubnub::String user_id = "";
    };
}
#endif /* PN_RESTRICTIONS_H */
