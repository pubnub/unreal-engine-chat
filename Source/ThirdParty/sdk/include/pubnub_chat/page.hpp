#ifndef PN_PAGE_H
#define PN_PAGE_H

#include "string.hpp"

namespace Pubnub
{
    struct Page
    {
        Pubnub::String next;
        Pubnub::String prev;
    };
}
#endif /* PN_PAGE_H */
