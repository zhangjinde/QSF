--
-- Premake script (http://premake.github.io)
--

assert(os.get() == 'windows' or os.get() == 'linux')

-- git clone https://github.com/ichenq/usr
local USR_DIR = os.getenv('USR_DIR') or 'E:/usr'

solution 'qsf'
    configurations {'Debug', 'Release'}
    language 'C'
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
            'WIN32_LEAN_AND_MEAN',
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            '_WINSOCK_DEPRECATED_NO_WARNINGS',
            'NOMINMAX',
            'inline=__inline',
            'alignof=__alignof',
            'noexcept=_NOEXCEPT',
            'snprintf=_snprintf',
            'strncasecmp=_strnicmp',
        }
        includedirs { USR_DIR .. '/include' }
        libdirs { USR_DIR .. '/lib/x64' }
        links
        {
            'ws2_32',
            'iphlpapi',
            'psapi',
        }        

    configuration 'gmake'
        buildoptions '-std=c99'
        defines
        {
            '_GNU_SOURCE',
            'USE_JEMALLOC',
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
        files
        {
            'src/**.h',
            'src/**.c',
        }
        excludes
        {
            'src/test/*.*',
        }
        includedirs
        {
            'src',
            'deps/lua/src',
            'deps/libuv/include',
            'deps/msgpack/include',
        }
        libdirs 'bin'
        if os.get() == 'windows' then
        links
        {
            'libzmq',
            'libuv',
            'zlib',
            'lua5.3',
            'msgpack',
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
            'z',
            'uv',
            'zmq',
            'uuid',
            'lua5.3',
            'msgpack',
            'crypto',
            'jemalloc',
            'mysqlclient',
        }
        end



