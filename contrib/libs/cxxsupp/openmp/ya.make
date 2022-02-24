# Generated by devtools/yamaker from nixpkgs 21.11.

LIBRARY()

OWNER(
    pg
    setser
    g:contrib
    g:cpp-contrib
)

VERSION(5.0)

ORIGINAL_SOURCE(https://github.com/llvm/llvm-project/archive/f9cc9d7392adeffc52a4cdf3f93d42f2a18b68f9.tar.gz)

LICENSE(
    Intel-LLVM-SGA AND
    MIT AND
    NCSA
)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

ADDINCL(
    GLOBAL contrib/libs/cxxsupp/openmp
    contrib/libs/cxxsupp/openmp/thirdparty/ittnotify
)

NO_COMPILER_WARNINGS()

NO_PLATFORM()

NO_UTIL()

CFLAGS(
    -fno-exceptions
)

SET_APPEND(CFLAGS -fno-lto)

COMPILE_C_AS_CXX()

IF (SANITIZER_TYPE == thread)
    NO_SANITIZE()
    CFLAGS(
        -fPIC
    )
ENDIF()

IF (SANITIZER_TYPE == memory)
    NO_SANITIZE()
    CFLAGS(
        -fPIC
    )
ENDIF()

SRCS(
    asm.S
    kmp_affinity.cpp
    kmp_alloc.c
    kmp_atomic.c
    kmp_barrier.cpp
    kmp_cancel.cpp
    kmp_csupport.c
    kmp_debug.c
    kmp_dispatch.cpp
    kmp_environment.c
    kmp_error.c
    kmp_ftn_cdecl.c
    kmp_ftn_extra.c
    kmp_global.c
    kmp_gsupport.c
    kmp_i18n.c
    kmp_io.c
    kmp_itt.c
    kmp_lock.cpp
    kmp_runtime.c
    kmp_sched.cpp
    kmp_settings.c
    kmp_str.c
    kmp_taskdeps.cpp
    kmp_tasking.c
    kmp_taskq.c
    kmp_threadprivate.c
    kmp_utility.c
    kmp_version.c
    kmp_wait_release.cpp
    thirdparty/ittnotify/ittnotify_static.c
    z_Linux_util.c
)

END()
