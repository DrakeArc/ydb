# Generated by devtools/yamaker.

LIBRARY()

WITHOUT_LICENSE_TEXTS()

VERSION(1.2.2)

LICENSE(BSD-3-Clause)

ADDINCL(
    contrib/libs/libwebp/dsp
    contrib/libs/libwebp/webp
)

NO_COMPILER_WARNINGS()

NO_RUNTIME()

CFLAGS(
    -DHAVE_CONFIG_H
)

SRCDIR(contrib/libs/libwebp/dsp)

SRCS(
    dec_mips32.c
    rescaler_mips32.c
    yuv_mips32.c
)

END()
