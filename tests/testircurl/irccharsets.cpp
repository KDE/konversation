
#include "irccharsets.h"


namespace Konversation
{

    struct IRCCharsetsSingleton
    {
        IRCCharsets charsets;
    };

}

Q_GLOBAL_STATIC(Konversation::IRCCharsetsSingleton, s_charsets)

namespace Konversation
{

    IRCCharsets *IRCCharsets::self()
    {
        return &s_charsets->charsets;
    }
}
