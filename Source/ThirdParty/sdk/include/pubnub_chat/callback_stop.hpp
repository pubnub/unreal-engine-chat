#ifndef PN_CALLBACK_STOP_H
#define PN_CALLBACK_STOP_H

#include "string.hpp"
#include <functional>

namespace Pubnub
{
    class CallbackStop
    {
        public:

            explicit CallbackStop(std::function<void()> stop_func): 
            stop_func(std::move(stop_func)), 
            stopped(false) 
            {}

            void operator()(){
                if(!stopped)
                {
                    stop_func();
                    stopped = true;
                }
            }


        private:
            std::function<void()> stop_func;
            bool stopped; // Ensures stop is called only once
    };

}
#endif /* PN_CALLBACK_STOP_H */
