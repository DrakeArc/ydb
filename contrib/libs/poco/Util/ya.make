# Generated by devtools/yamaker.

LIBRARY()

VERSION(1.9.4)

LICENSE(BSL-1.0)

LICENSE_TEXTS(.yandex_meta/licenses.list.txt)

PEERDIR(
    contrib/libs/expat
    contrib/libs/poco/Foundation
    contrib/libs/poco/JSON
    contrib/libs/poco/XML
)

ADDINCL(
    GLOBAL contrib/libs/poco/Util/include
    contrib/libs/expat
    contrib/libs/poco/Foundation/include
    contrib/libs/poco/JSON/include
    contrib/libs/poco/Util/src
    contrib/libs/poco/XML/include
)

NO_COMPILER_WARNINGS()

NO_UTIL()

CFLAGS(
    -DPOCO_ENABLE_CPP11
    -DPOCO_ENABLE_CPP14
    -DPOCO_NO_AUTOMATIC_LIBS
    -DPOCO_UNBUNDLED
)

IF (OS_DARWIN)
    CFLAGS(
        -DPOCO_OS_FAMILY_UNIX
        -DPOCO_NO_STAT64
    )
ELSEIF (OS_LINUX)
    CFLAGS(
        -DPOCO_OS_FAMILY_UNIX
        -DPOCO_HAVE_FD_EPOLL
    )
ELSEIF (OS_WINDOWS)
    CFLAGS(
        -DPOCO_OS_FAMILY_WINDOWS
    )
ENDIF()

SRCS(
    src/AbstractConfiguration.cpp
    src/Application.cpp
    src/ConfigurationMapper.cpp
    src/ConfigurationView.cpp
    src/FilesystemConfiguration.cpp
    src/HelpFormatter.cpp
    src/IniFileConfiguration.cpp
    src/IntValidator.cpp
    src/JSONConfiguration.cpp
    src/LayeredConfiguration.cpp
    src/LoggingConfigurator.cpp
    src/LoggingSubsystem.cpp
    src/MapConfiguration.cpp
    src/Option.cpp
    src/OptionCallback.cpp
    src/OptionException.cpp
    src/OptionProcessor.cpp
    src/OptionSet.cpp
    src/PropertyFileConfiguration.cpp
    src/RegExpValidator.cpp
    src/ServerApplication.cpp
    src/Subsystem.cpp
    src/SystemConfiguration.cpp
    src/Timer.cpp
    src/TimerTask.cpp
    src/Validator.cpp
    src/XMLConfiguration.cpp
)

END()
