--
-- Premake4 build script (http://industriousone.com/premake/download)
--

assert(os.get() == 'windows' or os.get() == 'linux')

solution 'SharedService'
    configurations {'Debug', 'Release'}
    language 'C++'
    --flags {'ExtraWarnings'}
    targetdir '../../bin'
    location '../../'
    platforms {'x32', 'x64'}

    configuration 'Debug'
        defines { 'DEBUG' }
        flags { 'Symbols' }

    configuration 'Release'
        defines { 'NDEBUG' }
        flags { 'Symbols', 'Optimize' }

    configuration 'vs*'
        defines
        {
            'WIN32',
            'WIN32_LEAN_AND_MEAN',
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            '_WINSOCK_DEPRECATED_NO_WARNINGS',
            'NOMINMAX',
        }
        includedirs
        {
            '../../deps/libzmq/include',
        }
        links
        {
            'ws2_32',
        }

    configuration 'gmake'
        buildoptions '-std=c++11 -mcrc32 -msse4.2 -rdynamic'
        defines
        {
            '__STDC_LIMIT_MACROS',
            '_ELPP_STACKTRACE_ON_CRASH',
        }
        links
        {
            'z',
            'rt',
            'dl',
            'pthread',
        }

    project 'SDKAuth'
        location 'build'
        kind 'SharedLib'
        uuid 'C29D5D99-B285-455E-8987-CB96C81EB68A'
        defines
        {
            '_ELPP_THREAD_SAFE',
        }
        files
        {
            '../core/platform.h',
            '../core/scope_guard.h',
            '../core/logging.h',
            '../core/logging.cpp',
            '../core/conv.h',
            '../core/conv.cpp',
            '../core/range.h',
            '../core/range.cpp',
            '../core/random.h',
            '../core/random.cpp',
            '../core/strings.h',
            '../core/strings-inl.h',
            '../core/strings.cpp',
            'sdkauth/*.h',
            'sdkauth/*.cpp',
        }
        includedirs
        {
            '../../src',
        }
        libdirs '../../bin'
        links
        {
            'zmq',
        }
