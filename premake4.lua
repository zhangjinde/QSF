--
-- Premake4 build script (http://industriousone.com/premake/download)
--

assert(os.get() == 'windows' or os.get() == 'linux')

local USR_DIR = os.getenv('USR_DIR') or 'E:/Library/usr'

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
            'WIN32_LEAN_AND_MEAN',
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            '_WINSOCK_DEPRECATED_NO_WARNINGS',
            'NOMINMAX',
            '_ALLOW_KEYWORD_MACROS',         
        }        
        includedirs { USR_DIR .. '/include' }
        libdirs { USR_DIR .. '/lib/x86' }

    configuration 'gmake'
        buildoptions '-std=c++11 -std=c99 -mcrc32 -msse4.2'
        defines
        {
            '__STDC_LIMIT_MACROS',
            '_POSIX_C_SOURCE=200112L',
        }
        includedirs 
        {
            '/usr/include/mysql', 
        }
        links
        {
            'rt',
            'dl',
            'pthread',
        }

    project 'qsf'
        location 'build'
        kind 'ConsoleApp'
        uuid '65BCF1EB-A936-4688-B1F4-7073B4ACE736'
        defines
        {
        }
        files
        {
            'src/**.h',
            'src/**.cpp',
            'src/**.c',
        }
        excludes
        {
            'src/test/*.*',
        }
        includedirs
        {
            'src',
            'deps/cppzmq',
            'deps/lua/src',
        }
        libdirs 'bin'
        if os.get() == 'windows' then
        links 
        {
            'libzmq', 
            'libuv', 
            'zlib', 
            'lua5.3',
            'libeay32',
            'libmysql',
        }
        includedirs
        {
            'deps/libuv/include',        
        }
        else
        links 
        {
            'zmq', 
            'uv', 
            'z', 
            'uuid', 
            'lua5.3',
            'mysqlclient',
        }
        end

    project 'unittest'
        location 'build'
        kind 'ConsoleApp'
        uuid '9E30CCC3-DA13-47FB-9902-7BF6D4792380'
        files
        {
            'src/core/*.cpp',
            'src/test/*.cpp',
            'deps/gtest/src/gtest-all.cc',
        }
        includedirs
        {
            'src',
            'deps/libuv/include',
            'deps/gtest/include',
            'deps/gtest',
        }
        libdirs 'bin'
        if os.get() == 'windows' then
        links {'libuv', 'zlib'}
        else
        links {'uv', 'z'}
        end
        
