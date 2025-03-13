#ifndef PN_CHAT_CALLBACK_HANDLE_HPP
#define PN_CHAT_CALLBACK_HANDLE_HPP

#include "helpers/export.hpp"
#include <memory>

class Subscribable;

namespace Pubnub {
    class CallbackHandle {
        public: 
            PN_CHAT_EXPORT CallbackHandle(std::shared_ptr<Subscribable> subscription);
            PN_CHAT_EXPORT CallbackHandle(CallbackHandle& other);
            PN_CHAT_EXPORT CallbackHandle(const CallbackHandle& other);
            PN_CHAT_EXPORT ~CallbackHandle() = default;

            PN_CHAT_EXPORT void close();

        private:
            std::shared_ptr<Subscribable> subscription;
    };
}

#endif // PN_CHAT_CALLBACK_HANDLE_HPP
