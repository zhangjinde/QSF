--
-- Premake4 build script (http://industriousone.com/premake/download)
--

assert(os.get() == 'windows' or os.get() == 'linux')

-- https://github.com/ichenq/PreCompiledWinLib
local LIB_DIR = 'E:/Library/PreCompiledWinLib/usr'

solution 'QSF'
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
        includedirs { LIB_DIR .. '/include'}

    configuration 'gmake'
        buildoptions '-std=c++11 -mcrc32 -msse4.2 -rdynamic'
        defines
        {
            '__STDC_LIMIT_MACROS',
            'HAVE_UNISTD_H',
        }
        links
        {
            'rt',
            'dl',
            'pthread',
        }

    project 'QSF'
        location 'build'
        kind 'ConsoleApp'
        uuid '65BCF1EB-A936-4688-B1F4-7073B4ACE736'
        defines
        {
            'ASIO_STANDALONE',
            'BOOST_DATE_TIME_NO_LIB',
            'BOOST_REGEX_NO_LIB',
        }
        files
        {
            'src/**.h',
            'src/**.cpp',
            'src/**.c',
        }
        excludes
        {
            'src/module/**.*',
            'src/test/*.*',
            'src/core/test/*.*',
            'src/net/test/*.*',
        }
        includedirs
        {
            'src',
            'deps',
            'deps/asio/include',
            LIB_DIR,
        }
        libdirs 'bin'
        
        if os.get() == 'windows' then
        links {'libzmq', 'zlib', 'lua5.3'}
        else
        links {'zmq', 'z', 'lua5.3', 'uuid'}
        end

    project 'TestCore'
        location 'build'
        kind 'ConsoleApp'
        uuid '9E30CCC3-DA13-47FB-9902-7BF6D4792380'
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

    project 'TestNet'
        location 'build'
        kind 'ConsoleApp'
        uuid '7EAB00F8-E324-45FC-83FA-3ADD6439BB89'
        defines
        {
            'ASIO_STANDALONE',
            'BOOST_DATE_TIME_NO_LIB',
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
            'deps/asio/include',
            'deps/gtest',
            'deps/gtest/include',
        }
        libdirs
        {
            'bin',
        }
        if os.get() == 'windows' then
        links 'zlib'
        else
        links 'z'
        end
        
