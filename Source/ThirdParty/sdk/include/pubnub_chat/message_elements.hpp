#ifndef PN_MESSAGE_ELEMENTS_H
#define PN_MESSAGE_ELEMENTS_H

#include "string.hpp"
#include "vector.hpp"
#include "map.hpp"
#include "option.hpp"

namespace Pubnub
{
    struct MentionedUser
    {
        Pubnub::String id = "";
        Pubnub::String name = "";
    };

    struct ReferencedChannel
    {
        Pubnub::String id = "";
        Pubnub::String name = "";
    };

    struct TextLink
    {
        int start_index = 0;
        int end_index = 0;
        Pubnub::String link = "";
    };

    struct QuotedMessage
    {
        Pubnub::String timetoken = "";
        Pubnub::String text = "";
        Pubnub::String user_id = "";
        Pubnub::String channel_id = "";
    };


}
#endif /* PN_MESSAGE_ELEMENTS_H */
