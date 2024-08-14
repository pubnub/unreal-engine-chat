#ifndef PN_CHAT_USER_H
#define PN_CHAT_USER_H

#include "string.hpp"
#include "helpers/export.hpp"
#include "restrictions.hpp"
#include "vector.hpp"
#include "callback_stop.hpp"
#include "page.hpp"
#include <memory>
#include <vector>
#include <functional>

class UserService;
class ChatService;
class PresenceService;
class RestrictionsService;
class MembershipService;
class UserDAO;

namespace Pubnub
{
    class Membership;
    class Channel;

    struct ChatUserData
    {
        Pubnub::String user_name = "";
        Pubnub::String external_id = "";
        Pubnub::String profile_url = "";
        Pubnub::String email = "";
        Pubnub::String custom_data_json = "";
        Pubnub::String status = "";
        Pubnub::String type = "";
    };

    struct MembershipsResponseWrapper
    {
        Pubnub::Vector<Pubnub::Membership> memberships;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };

    
    struct ChannelsRestrictionsWrapper
    {
        Pubnub::Vector<Pubnub::ChannelRestriction> restrictions;
        Pubnub::Page page;
        int total;
        Pubnub::String status;
    };


    class User
    {
        public:
            PN_CHAT_EXPORT User(const User& other);
            PN_CHAT_EXPORT ~User();

            PN_CHAT_EXPORT Pubnub::User& operator =(const Pubnub::User& other);

            PN_CHAT_EXPORT Pubnub::String user_id() const;
            PN_CHAT_EXPORT Pubnub::ChatUserData user_data() const;

            PN_CHAT_EXPORT Pubnub::User update(const Pubnub::ChatUserData& user_data) const;
            PN_CHAT_EXPORT void delete_user() const;
            PN_CHAT_EXPORT Pubnub::Vector<Pubnub::String> where_present() const;
            PN_CHAT_EXPORT bool is_present_on(const Pubnub::String& channel_id) const;

            PN_CHAT_EXPORT void set_restrictions(const Pubnub::String& channel_id, const Pubnub::Restriction& restrictions) const;
            PN_CHAT_EXPORT Pubnub::Restriction get_channel_restrictions(const Pubnub::Channel& channel) const;
            PN_CHAT_EXPORT ChannelsRestrictionsWrapper get_channels_restrictions(const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;
            //Deprecated in JS chat
            //PN_CHAT_EXPORT void report(const Pubnub::String& reason) const;

            PN_CHAT_EXPORT MembershipsResponseWrapper get_memberships(const Pubnub::String& filter = "", const Pubnub::String& sort = "", int limit = 0, const Pubnub::Page& page = Pubnub::Page()) const;

            PN_CHAT_EXPORT CallbackStop stream_updates(std::function<void(const User&)> user_callback) const;
            PN_CHAT_EXPORT CallbackStop stream_updates_on(Pubnub::Vector<Pubnub::User> users, std::function<void(Pubnub::Vector<Pubnub::User>)> user_callback) const;

        private:
            PN_CHAT_EXPORT User(
                    Pubnub::String user_id,
                    std::shared_ptr<const ChatService> chat_service,
                    std::shared_ptr<const UserService> user_service,
                    std::shared_ptr<const PresenceService> presence_service,
                    std::shared_ptr<const RestrictionsService> restrictions_service,
                    std::shared_ptr<const MembershipService> membership_service,
                    std::unique_ptr<UserDAO> data);
            
            Pubnub::String user_id_internal;
            std::unique_ptr<UserDAO> data;
            std::shared_ptr<const UserService> user_service;
            std::shared_ptr<const ChatService> chat_service;
            std::shared_ptr<const PresenceService> presence_service;
            std::shared_ptr<const RestrictionsService> restrictions_service;
            std::shared_ptr<const MembershipService> membership_service;

        friend class ::UserService;

#ifdef PN_CHAT_C_ABI
        public:
        Pubnub::User update_with_base(const Pubnub::User& base_user) const;
#endif
    };
}
#endif /* PN_CHAT_USER_H */
