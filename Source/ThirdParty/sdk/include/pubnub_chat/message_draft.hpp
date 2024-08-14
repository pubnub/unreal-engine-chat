#ifndef PN_CHAT_MESSAGE_DRAFT_H
#define PN_CHAT_MESSAGE_DRAFT_H

#include "string.hpp"
#include <map>
#include <vector>

#include "option.hpp"
#include "message_elements.hpp"
#include "message_draft_config.hpp"
#include "channel.hpp"
#include "user.hpp"
#include "message.hpp"

class MessageService;

namespace Pubnub
{
    class Channel;
    class User;

    PN_CHAT_EXPORT class MessageDraft
    {
        public:
            Pubnub::Channel channel;
            Pubnub::String value;
            MessageDraftConfig draft_config;

            PN_CHAT_EXPORT Pubnub::Message quoted_message();
            PN_CHAT_EXPORT void add_quote(Pubnub::Message message);
            PN_CHAT_EXPORT void remove_quote();

        private:
            PN_CHAT_EXPORT MessageDraft(const Pubnub::Channel& channel, const Pubnub::MessageDraftConfig& draft_config, std::shared_ptr<const MessageService> message_service);

        
            Pubnub::String previous_value;
            std::map<int, Pubnub::User> mentioned_users;
            std::map<int, Pubnub::Channel> referenced_channels;
            std::vector<TextLink> text_links;
            Option<Pubnub::Message> quoted_message_internal;
    
            std::shared_ptr<const MessageService> message_service;

            friend class ::MessageService;
    };
}
#endif /* PN_CHAT_MESSAGE_DRAFT_H */
