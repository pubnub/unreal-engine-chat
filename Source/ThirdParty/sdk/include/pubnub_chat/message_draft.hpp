#ifndef PN_CHAT_MESSAGE_DRAFT_H
#define PN_CHAT_MESSAGE_DRAFT_H

#include "string.hpp"
#include <map>
#include <vector>

#include "option.hpp"
#include "message_elements.hpp"
#include "channel.hpp"
#include "user.hpp"
#include "message.hpp"

class MessageService;
class DraftService;
class MessageDraftDAO;

namespace Pubnub
{
    class Channel;
    class User;

    PN_CHAT_EXPORT class MentionTarget {
        public:
            enum class Type {
                USER,
                CHANNEL, 
                URL
            };

            PN_CHAT_EXPORT MentionTarget() = default;

            PN_CHAT_EXPORT static MentionTarget user(const Pubnub::String& user_id);
            PN_CHAT_EXPORT static MentionTarget channel(const Pubnub::String& channel);
            PN_CHAT_EXPORT static MentionTarget url(const Pubnub::String& url);

            PN_CHAT_EXPORT Pubnub::String get_target() const;
            PN_CHAT_EXPORT Type get_type() const;
        
        private:
            MentionTarget(const Pubnub::String& target, const Type type);

            Pubnub::String target;
            Type type;
    };

    PN_CHAT_EXPORT struct MessageElement {
        public:
            PN_CHAT_EXPORT static MessageElement plain_text(const Pubnub::String& text);
            PN_CHAT_EXPORT static MessageElement link(const Pubnub::String& text, const Pubnub::MentionTarget& target);

            Pubnub::String text;
            Pubnub::Option<Pubnub::MentionTarget> target;
        private:
            MessageElement(const Pubnub::String& text, const Pubnub::Option<Pubnub::MentionTarget>& target);
    };

    struct SuggestedMention {
        std::size_t offset;
        Pubnub::String replace_from;
        Pubnub::String replace_to;
        Pubnub::MentionTarget target;
    };

    PN_CHAT_EXPORT class MessageDraft {
        public:
            PN_CHAT_EXPORT enum class UserSuggestionsSource {
                CHANNELS,
                GLOBAL
            };
            PN_CHAT_EXPORT ~MessageDraft();
            PN_CHAT_EXPORT MessageDraft(const MessageDraft& other);
            PN_CHAT_EXPORT MessageDraft& operator=(const MessageDraft& other);

            PN_CHAT_EXPORT void insert_text(std::size_t position, const Pubnub::String& text);
            PN_CHAT_EXPORT void remove_text(std::size_t position, std::size_t length);
            PN_CHAT_EXPORT void insert_suggested_mention(const SuggestedMention& suggested_mention, const Pubnub::String& text);
            PN_CHAT_EXPORT void add_mention(std::size_t position, std::size_t length, const Pubnub::MentionTarget& target);
            PN_CHAT_EXPORT void remove_mention(std::size_t position);
            PN_CHAT_EXPORT void update(const Pubnub::String& text);
            PN_CHAT_EXPORT void send(SendTextParams send_params = SendTextParams());

#ifndef PN_CHAT_C_ABI
            PN_CHAT_EXPORT void add_change_listener(std::function<void(Pubnub::Vector<Pubnub::MessageElement>)> listener);
            PN_CHAT_EXPORT void add_change_listener(std::function<void(Pubnub::Vector<Pubnub::MessageElement>, Pubnub::Vector<Pubnub::SuggestedMention>)> listener);
#else 
            std::vector<Pubnub::MessageElement> consume_message_elements();
            std::vector<Pubnub::SuggestedMention> consume_suggested_mentions();
            void set_search_for_suggestions(bool search_for_suggestions);
#endif

        private:
            MessageDraft(
                    const Pubnub::Channel& channel,
                    const Pubnub::MessageDraftConfig& draft_config,
                    std::shared_ptr<const ChannelService> channel_service,
                    std::shared_ptr<const UserService> user_service);
            void trigger_typing_indicator();

            Pubnub::Channel channel;
            MessageDraftConfig draft_config;
            std::unique_ptr<MessageDraftDAO> value;
            std::shared_ptr<DraftService> draft_service;

            friend ::MessageService;
    };
}
#endif /* PN_CHAT_MESSAGE_DRAFT_H */
