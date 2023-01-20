#ifndef WARN_MUTE_H
#error "must be used in pair with warn_muted.h"
#endif
#undef WARN_MUTE_H

#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
