#ifdef WARN_MUTE_H
#error "must be used in pair with warn_unmuted.h"
#endif
#define WARN_MUTE_H

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#pragma warning(push, 0)
#endif
