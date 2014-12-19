--
-- Premake4 build script (http://industriousone.com/premake/download)
--

assert(os.get() == 'windows' or os.get() == 'linux')

local BOOST_ROOT = os.getenv('BOOST_ROOT') or ''

solution 'qsf'
    configurations {'Debug', 'Release'}
    language 'C++'
    --flags {'ExtraWarnings'}
    targetdir 'bin'
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
            'deps/zlib',
            'deps/lua/src',
            'deps/libzmq/include',
            BOOST_ROOT,
        }
        libdirs { BOOST_ROOT .. '/stage/lib' }
        links
        {
            'ws2_32',
            'zlib',
        }

    configuration 'gmake'
        buildoptions '-std=c++11 -mcrc32 -msse4.2 -rdynamic'
        defines
        {
            '__STDC_LIMIT_MACROS',
            '_ELPP_STACKTRACE_ON_CRASH',
            'HAVE_UNISTD_H',
        }
        includedirs '/usr/include/lua5.2'
        links
        {
            'z',
            'rt',
            'dl',
            'pthread',
            'boost_system',
        }


    project 'qsf'
        location 'build'
        kind 'ConsoleApp'
        uuid '65BCF1EB-A936-4688-B1F4-7073B4ACE736'
        defines
        {
            '_ELPP_THREAD_SAFE',
            'BOOST_ASIO_SEPARATE_COMPILATION',
            'BOOST_REGEX_NO_LIB',
        }
        files
        {
            'src/**.h',
            'src/**.cpp',
        }
        excludes
        {
            'src/test/*.*',
            'src/core/test/*.*',
            'src/net/test/*.*',
        }
        includedirs
        {
            'src',
        }
        libdirs 'bin'
        links
        {
            'zmq',
            'lua5.2',
        }


    project 'test-core'
        location 'build'
        kind 'ConsoleApp'
        uuid '9E30CCC3-DA13-47FB-9902-7BF6D4792380'
        defines
        {
            '_ELPP_THREAD_SAFE',
        }
        files
        {
            'src/core/**.*',
            'deps/gtest/src/gtest-all.cc',
        }
        includedirs
        {
            'src',
            'deps/gtest',
            'deps/gtest/include',
        }
        libdirs
        {
            'bin',
        }

    project 'test-net'
        location 'build'
        kind 'ConsoleApp'
        uuid '7EAB00F8-E324-45FC-83FA-3ADD6439BB89'
        defines
        {
            '_ELPP_THREAD_SAFE',
            'BOOST_ASIO_SEPARATE_COMPILATION',
            'BOOST_REGEX_NO_LIB',
        }
        files
        {
            'src/core/*.*',
            'src/net/**.*',
            'deps/gtest/src/gtest-all.cc',
        }
        includedirs
        {
            'src',
            'deps/gtest',
            'deps/gtest/include',
        }
        libdirs
        {
            'bin',
        }
