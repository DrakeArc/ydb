# Generated by devtools/yamaker.

LIBRARY()

VERSION(1.2.2)

LICENSE(BSD-3-Clause WITH Google-Patent-License-Webm)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

ADDINCL(
    contrib/libs/libwebp/dec
    contrib/libs/libwebp/webp
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DHAVE_CONFIG_H
)

SRCS(
    alpha_dec.c
    buffer_dec.c
    frame_dec.c
    idec_dec.c
    io_dec.c
    quant_dec.c
    tree_dec.c
    vp8_dec.c
    vp8l_dec.c
    webp_dec.c
)

END()
