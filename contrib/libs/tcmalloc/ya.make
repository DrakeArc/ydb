LIBRARY()

LICENSE(Apache-2.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)
ALLOCATOR_IMPL()

VERSION(2021-10-04-45c59ccbc062ac96d83710205033c656e490d376)

ORIGINAL_SOURCE(https://github.com/google/tcmalloc/archive/45c59ccbc062ac96d83710205033c656e490d376.tar.gz)

SRCS(
    # Options
    tcmalloc/want_hpaa.cc
)

INCLUDE(common.inc)

CFLAGS(
    -DTCMALLOC_256K_PAGES
)

END()

IF (NOT DLL_FOR)
    RECURSE(
        default
        dynamic
        malloc_extension
        no_percpu_cache
        numa_256k
        numa_large_pages
        small_but_slow
    )
ENDIF()
