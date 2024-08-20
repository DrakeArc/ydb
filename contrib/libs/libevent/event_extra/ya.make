# Generated by devtools/yamaker.

LIBRARY()

WITHOUT_LICENSE_TEXTS()

VERSION(2.1.12)

LICENSE(BSD-3-Clause)

PEERDIR(
    contrib/libs/libc_compat
)

ADDINCL(
    contrib/libs/libevent
    contrib/libs/libevent/include
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DHAVE_CONFIG_H
    -DEVENT__HAVE_STRLCPY=1
)

SRCDIR(contrib/libs/libevent)

SRCS(
    evdns.c
    event_tagging.c
    evrpc.c
    http.c
)

END()
