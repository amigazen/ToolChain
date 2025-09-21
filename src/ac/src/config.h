/*
 *  This is more portable than trying to define everything on the
 *  compilation command line via the Makefile.
 *  Fred Fish, 20-Jul-86
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>
#include <string.h>

#ifndef SIZET
#define SIZET       size_t
#endif

#ifndef VOIDSTAR
#define VOIDSTAR    void *
#endif

#ifndef CONST
#define CONST       const
#endif

#ifndef UNIXERR
#define UNIXERR
#endif

#endif /* CONFIG_H */
