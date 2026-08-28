/*
 * ac_feature_demo.h - compile-time feature probes for AC 4.0 (post-PDC fork).
 * Included twice from ac_feature_demo.c to exercise #pragma once.
 */

#pragma once

#warning ac_feature_demo: AC 4.0 feature demonstration build

#ifndef AC_FEATURE_DEMO_H
#define AC_FEATURE_DEMO_H

#include <stdio.h>

/*
 * Nested conditional compilation (deep #if and defined() with ||).
 * AC 4.0 predefines __AMIGA__ and __SASC__ in GetSym.c.
 */
#if defined(__AMIGA__) || defined(__SASC__)
#if defined(__STDC__)
#define DEMO_COMPILER_NAME "AC on Amiga (ANSI C)"
#else
#define DEMO_COMPILER_NAME "AC on Amiga (pre-ANSI)"
#endif
#else
#define DEMO_COMPILER_NAME "AC (unknown host)"
#endif

#if defined(__AMIGA__)
#if defined(__SASC__)
#if defined(__STDC__)
#define DEMO_NEST_OK 1
#else
#define DEMO_NEST_OK 0
#endif
#else
#define DEMO_NEST_OK 0
#endif
#else
#define DEMO_NEST_OK 0
#endif

/* Variadic macro with __VA_ARGS__ (AC 4.0 CHANGELOG). */
#define DEMO_VLOG(fmt, ...) \
    printf("  VLOG: " fmt "\n", __VA_ARGS__)

/* Stringification (#) and token pasting (##) operators. */
#define DEMO_STR(x) #x
#define DEMO_JOIN2(a, b) a##b

/* Simple log lines without varargs. */
#define DEMO_LOG(msg) printf("  LOG: %s\n", (msg))

#endif /* AC_FEATURE_DEMO_H */
