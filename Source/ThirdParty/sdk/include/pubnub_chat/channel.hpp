#ifndef PN_CHAT_CHANNEL_HPP
#define PN_CHAT_CHANNEL_HPP

#include "event.hpp"
#include "string.hpp"
#include "helpers/export.hpp"
#include "restrictions.hpp"
#include "message_elements.hpp"
#include "message.hpp"
#include "enums.hpp"
#include "vector.hpp"
#include "callback_stop.hpp"
#include "page.hpp"
#include "map.hpp"
#include <memory>
#include <vector>
#include <functional>
#include <map>

#include "message_draft_config.hpp"

#ifdef PN_CHAT_C_ABI
#include "pubnub_helper.h"
#endif

class ChannelService; 
class ChatService;
class PresenceService;
class RestrictionsService;
class MessageService;
class MembershipService;
class ChannelDAO;

namespace Pubnub 
{
    class Message;
    class Membership;
    class User;
    class MessageDraft;
    struct EventsHistoryWrapper;

    struct ChatChannelData
    {
        Pubnub::String channel_name = "";
        Pubnub::String description = "";
        Pubnub::String custom_data_json = "";
        Pubnub::String updated = "";
        Pubnub::String status = "";
        Pubnub::String type = "";
    };

    struct SendTextParams
    {
        bool store_in_history = true;
        bool send_by_post = false;
        Pubnub::String meta = "";
        Pubnub::Map<int, Pubnub::MentionedUser> mentioned_users;
        Pubnub::Map<int, Pubnub::ReferencedChannel> referenced_channels;
        Pubnub::Vector<Pubnub::TextLink> text_links;
        Pubnub::Option<Pubnub::Message> quoted_message;
    };

    struct MembersResponseWrapper
    {
        Pubnub::Vector<Pubnub::Membership> memberships;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };

    struct UsersRestrictionsWrapper
    {
        Pubnub::Vector<Pubnub::UserRestriction> restrictions;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };

    class Channel {
        public:
            PN_CHAT_EXPORT Channel();
            PN_CHAT_EXPORT Channel(const Channel& other);
            PN_CHAT_EXPORT ~Channel();

            PN_CHAT_EXPORT Pubnub::Channel& operator =(const Pubnub::Channel& other);

            PN_CHAT_EXPORT Pubnub::String channel_id() const;
            PN_CHAT_EXPORT Pubnub::ChatChannelData channel_data() const;

            PN_CHAT_EXPORT Pubnub::Channel update(const ChatChannelData& in_additional_channel_data) const;
#ifndef PN_CHAT_C_ABI
            PN_CHAT_EXPORT void connect(std::function<void(Message)> message_callback) const;
            PN_CHAT_EXPORT void join(std::function<void(Message)> message_callback, const Pubnub::String& additional_params = "") const;
            PN_CHAT_EXPORT void disconnect() const;
            PN_CHAT_EXPORT void leave() const;
#endif
            PN_CHAT_EXPORT void delete_channel() const;

            PN_CHAT_EXPORT virtual void send_text(const Pubnub::String& message, SendTextParams text_params = SendTextParams());
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::String> who_is_present() const;
            PN_CHAT_EXPORT bool is_present(const Pubnub::String& user_id) const;
            PN_CHAT_EXPORT void set_restrictions(const Pubnub::String& user_id, Pubnub::Restriction restrictions) const;
            PN_CHAT_EXPORT Pubnub::Restriction get_user_restrictions(const Pubnub::User& user) const;
            PN_CHAT_EXPORT UsersRestrictionsWrapper get_users_restrictions(const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::Message> get_history(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 25) const;
            PN_CHAT_EXPORT Pubnub::Message get_message(const Pubnub::String& timetoken) const;
            PN_CHAT_EXPORT MembersResponseWrapper get_members(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            PN_CHAT_EXPORT Pubnub::Membership invite(const Pubnub::User& user) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::Membership> invite_multiple(Pubnub::Vector<Pubnub::User> users) const;
            PN_CHAT_EXPORT void start_typing() const;
            PN_CHAT_EXPORT void stop_typing() const;
            PN_CHAT_EXPORT CallbackStop get_typing(std::function<void(Pubnub::Vector<Pubnub::String>)> typing_callback) const;
            PN_CHAT_EXPORT Pubnub::Channel pin_message(const Pubnub::Message& message) const;
            PN_CHAT_EXPORT Pubnub::Channel unpin_message() const;
            PN_CHAT_EXPORT Pubnub::Message get_pinned_message() const;
            PN_CHAT_EXPORT void forward_message(const Pubnub::Message& message) const;
            PN_CHAT_EXPORT virtual void emit_user_mention(const Pubnub::String& user_id, const Pubnub::String& timetoken, const Pubnub::String& text) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::Membership> get_user_suggestions(Pubnub::String text, int limit = 10) const;

            PN_CHAT_EXPORT CallbackStop stream_updates(std::function<void(const Pubnub::Channel&)> channel_callback) const;
            PN_CHAT_EXPORT CallbackStop stream_updates_on(Pubnub::Vector<Pubnub::Channel> channels, std::function<void(Pubnub::Vector<Pubnub::Channel>)> channel_callback);
            PN_CHAT_EXPORT CallbackStop stream_presence(std::function<void(Pubnub::Vector<Pubnub::String>)> presence_callback) const;
            PN_CHAT_EXPORT CallbackStop stream_read_receipts(std::function<void(Pubnub::Map<Pubnub::String, Pubnub::Vector<Pubnub::String>, Pubnub::StringComparer>)> read_receipts_callback) const;

            PN_CHAT_EXPORT Pubnub::EventsHistoryWrapper get_messsage_reports_history(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 100) const;
#ifndef PN_CHAT_C_ABI
            PN_CHAT_EXPORT CallbackStop stream_message_reports(std::function<void(const Pubnub::Event&)> event_callback) const;
#endif

            PN_CHAT_EXPORT Pubnub::MessageDraft create_message_draft(Pubnub::MessageDraftConfig message_draft_config = Pubnub::MessageDraftConfig()) const;

        protected:
            PN_CHAT_EXPORT Channel(
                    Pubnub::String channel_id,
                    std::shared_ptr<const ChatService> chat_service,
                    std::shared_ptr<const ChannelService> channel_service,
                    std::shared_ptr<const PresenceService> presence_service,
                    std::shared_ptr<const RestrictionsService> restrictions_service,
                    std::shared_ptr<const MessageService> message_service,
                    std::shared_ptr<const MembershipService> membership_service,
                    std::unique_ptr<ChannelDAO> data);
            
            Pubnub::String channel_id_internal;
            std::unique_ptr<ChannelDAO> data;
            std::shared_ptr<const ChatService> chat_service;
            std::shared_ptr<const PresenceService> presence_service;
            std::shared_ptr<const RestrictionsService> restrictions_service;
            std::shared_ptr<const MessageService> message_service;
            std::shared_ptr<const MembershipService> membership_service;
            std::shared_ptr<const ChannelService> channel_service;

        friend class ::ChannelService;

#ifdef PN_CHAT_C_ABI
        public:
        std::vector<pubnub_v2_message> connect() const;
        std::vector<pubnub_v2_message> join(const Pubnub::String& additional_params = "") const;
        std::vector<pubnub_v2_message> disconnect() const;
        std::vector<pubnub_v2_message> leave() const;
        Pubnub::Channel update_with_base(const Pubnub::Channel& base_channel) const;
#endif
    };
};

#endif // PN_CHAT_CHANNEL_HPP
