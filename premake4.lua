--
-- Premake4 build script (http://industriousone.com/premake/download)
--

local BOOST_ROOT = os.getenv('BOOST_ROOT') or ''

solution 'qsf'
    configurations {'Debug', 'Release'}
    language 'C++'
    --flags {'ExtraWarnings'}
    targetdir 'bin'
    platforms {'x64'}

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
            'NOMINMAX',
        }
        includedirs { BOOST_ROOT }
        libdirs { BOOST_ROOT .. '/stage/lib' }
        links 'ws2_32'
        
    configuration 'gmake'
        buildoptions '-std=c++11 -mcrc32 -msse4.2 -rdynamic'
        defines
        {
            '__STDC_LIMIT_MACROS',
            '_ELPP_STACKTRACE_ON_CRASH',
            'HAVE_UNISTD_H',
        }
        links
        {
            'rt',
            'pthread',
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
            'deps/zlib',
            'deps/lua/src',
            'deps/zeromq/include',
        }
        libdirs 'bin'
        links
        {
            'zmq',
            'lua',
            'zlib',
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
