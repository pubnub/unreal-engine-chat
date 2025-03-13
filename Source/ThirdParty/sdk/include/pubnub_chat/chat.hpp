#ifndef PN_CHAT_CHAT_HPP
#define PN_CHAT_CHAT_HPP

#include "access_manager.hpp"
#include "callback_handle.hpp"
#include "enums.hpp"
#include "mentions.hpp"
#include "string.hpp"
#include "channel.hpp"
#include "thread_channel.hpp"
#include "message.hpp"
#include "user.hpp"
#include "membership.hpp"
#include "restrictions.hpp"
#include "vector.hpp"
#include "page.hpp"
#include "event.hpp"
#include "callback_stop.hpp"
#include <memory>
#include <vector>
#include <functional>

class ChatService;
class ChannelService;
class UserService;
class PresenceService;
class RestrictionsService;
class MessageService;
class MembershipService;
class AccessManagerService;
class CallbackService;


#ifdef PN_CHAT_C_ABI
struct pubnub_v2_message;
#endif

#ifndef PN_CHAT_C_ABI
class CallbackService;
#endif

namespace Pubnub {
    struct ChannelRateLimits {
        int direct_conversation = 0;
        int group_conversation = 0;
        int public_conversation = 0;
        int unknown_conversation = 0;
    };

    struct ChatConfig {
        Pubnub::String auth_key = "";
        int typing_timeout = 5000;
        int typing_timeout_difference = 1000;
        int store_user_activity_interval = 600000;
        bool store_user_activity_timestamps = false;
        float rate_limit_factor = 1.2;
        ChannelRateLimits rate_limit_per_channel;
    };

    struct CreatedChannelWrapper
    {
        Pubnub::Channel created_channel;
        Pubnub::Membership host_membership;
        Pubnub::Vector<Pubnub::Membership> invitees_memberships;

        CreatedChannelWrapper(Pubnub::Channel in_channel, Pubnub::Membership in_host_membership, Pubnub::Vector<Pubnub::Membership> in_invitees_memberships) :
        created_channel(in_channel),
        host_membership(in_host_membership),
        invitees_memberships(in_invitees_memberships)
        {}

        CreatedChannelWrapper(Pubnub::Channel in_channel, Pubnub::Membership in_host_membership) :
        created_channel(in_channel),
        host_membership(in_host_membership)
        {}
    };

    struct UnreadMessageWrapper
    {
        Pubnub::Channel channel;
        Pubnub::Membership membership;
        int count;
    };

    struct MarkMessagesAsReadWrapper
    {
        Pubnub::Page page;
        int total;
        int status;
        Pubnub::Vector<Pubnub::Membership> memberships;
    };

    struct ChannelsResponseWrapper
    {
        Pubnub::Vector<Pubnub::Channel> channels;
        Pubnub::Page page;
        int total;
    };

    struct UsersResponseWrapper
    {
        Pubnub::Vector<Pubnub::User> users;
        Pubnub::Page page;
        int total;
    };

    struct EventsHistoryWrapper
    {
        Pubnub::Vector<Pubnub::Event> events;
        bool is_more;
    };



    class Chat {
        public:
            PN_CHAT_EXPORT static Pubnub::Chat init(const Pubnub::String& publish_key, const Pubnub::String& subscribe_key, const Pubnub::String& user_id, const ChatConfig& config);

            /* CHANNELS */

            PN_CHAT_EXPORT Pubnub::Channel create_public_conversation(const Pubnub::String& channel_id, const ChatChannelData& channel_data) const;
            PN_CHAT_EXPORT CreatedChannelWrapper create_direct_conversation(const Pubnub::User& user, const Pubnub::String& channel_id, const ChatChannelData& channel_data, const Pubnub::String& membership_data = "") const;
            PN_CHAT_EXPORT CreatedChannelWrapper create_group_conversation(Pubnub::Vector<Pubnub::User> users, const Pubnub::String& channel_id, const ChatChannelData& channel_data, const Pubnub::String& membership_data = "") const;
            PN_CHAT_EXPORT Channel get_channel(const Pubnub::String& channel_id) const;
            PN_CHAT_EXPORT ChannelsResponseWrapper get_channels(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            PN_CHAT_EXPORT Pubnub::Channel update_channel(const Pubnub::String& channel_id, const ChatChannelData& channel_data) const;
            PN_CHAT_EXPORT void delete_channel(const Pubnub::String& channel_id) const;
            PN_CHAT_EXPORT void pin_message_to_channel(const Pubnub::Message& message, const Pubnub::Channel& channel) const;
            PN_CHAT_EXPORT void unpin_message_from_channel(const Pubnub::Channel& channel) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::Channel> get_channel_suggestions(Pubnub::String text, int limit = 10) const;

            /* USERS */

            PN_CHAT_EXPORT Pubnub::User current_user();
            PN_CHAT_EXPORT Pubnub::User create_user(const Pubnub::String& user_id, const Pubnub::ChatUserData& user_data) const;
            PN_CHAT_EXPORT Pubnub::User get_user(const Pubnub::String& user_id) const;
            PN_CHAT_EXPORT UsersResponseWrapper get_users(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            PN_CHAT_EXPORT Pubnub::User update_user(const Pubnub::String& user_id, const Pubnub::ChatUserData& user_data) const;
            PN_CHAT_EXPORT void delete_user(const Pubnub::String& user_id) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::User> get_user_suggestions(Pubnub::String text, int limit = 10) const;

            /* PRESENCE */

            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::String> where_present(const Pubnub::String& user_id) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::String> who_is_present(const Pubnub::String& channel_id) const;
            PN_CHAT_EXPORT bool is_present(const Pubnub::String& user_id, const Pubnub::String& channel_id) const;

            /* MODERATION */

            PN_CHAT_EXPORT void set_restrictions(const Pubnub::String& user_id, const Pubnub::String& channel_id, const Pubnub::Restriction& restrictions) const;
            PN_CHAT_EXPORT void emit_chat_event(pubnub_chat_event_type chat_event_type, const Pubnub::String& channel_id, const Pubnub::String& payload, EventMethod event_method = EventMethod::Default) const;
            PN_CHAT_EXPORT EventsHistoryWrapper get_events_history(const Pubnub::String& channel_id, const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 100) const;
            PN_CHAT_EXPORT Pubnub::CallbackHandle listen_for_events(const Pubnub::String& channel_id, pubnub_chat_event_type chat_event_type, std::function<void(const Pubnub::Event&)> event_callback) const;

            /* MESSAGES */

            PN_CHAT_EXPORT void forward_message(const Pubnub::Message& message, const Pubnub::Channel& channel) const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::UnreadMessageWrapper> get_unread_messages_counts(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            PN_CHAT_EXPORT MarkMessagesAsReadWrapper mark_all_messages_as_read(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;

            PN_CHAT_EXPORT Pubnub::UserMentionDataList get_current_user_mentions(const Pubnub::String& start_timetoken, const Pubnub::String& end_timetoken, int count = 100) const;


            /* THREADS */

            PN_CHAT_EXPORT Pubnub::ThreadChannel create_thread_channel(const Pubnub::Message& message) const;
            PN_CHAT_EXPORT Pubnub::ThreadChannel get_thread_channel(const Pubnub::Message& message) const;
            PN_CHAT_EXPORT void remove_thread_channel(const Pubnub::Message& message) const;

            /* PAM */
            PN_CHAT_EXPORT Pubnub::AccessManager access_manager() const;

        private:
            Chat(const Pubnub::String& publish_key, const Pubnub::String& subscribe_key, const Pubnub::String& user_id, const ChatConfig& config);
            void store_user_activity_timestamp() const;

            std::shared_ptr<const ChatService> chat_service;
            std::shared_ptr<const UserService> user_service;
            std::shared_ptr<const PresenceService> presence_service;
            std::shared_ptr<const RestrictionsService> restrictions_service;
            std::shared_ptr<const MessageService> message_service;
            std::shared_ptr<const MembershipService> membership_service;
            std::shared_ptr<const ChannelService> channel_service;
            std::shared_ptr<const CallbackService> callback_service;
#ifdef PN_CHAT_C_ABI
        public:
            const ChatService* get_chat_service() const;
            std::shared_ptr<const ChatService> shared_chat_service() const;
#endif

    };
}

#endif // PN_CHAT_CHAT_HPP
