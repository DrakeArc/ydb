# Generated by devtools/yamaker from nixpkgs 22.11.

LIBRARY()

LICENSE(BSL-1.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

VERSION(1.86.0)

ORIGINAL_SOURCE(https://github.com/boostorg/bind/archive/boost-1.86.0.tar.gz)

PEERDIR(
    contrib/restricted/boost/config
    contrib/restricted/boost/core
)

ADDINCL(
    GLOBAL contrib/restricted/boost/bind/include
)

NO_COMPILER_WARNINGS()

NO_UTIL()

END()
