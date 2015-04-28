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
            '_CRT_RAND_S',
            'NOMINMAX',
            'inline=__inline',
            'snprintf=_snprintf',
            'strncasecmp=_strnicmp',
        }
        --buildoptions [[/wd"4127" /wd"4204" /wd"4201"]]
        includedirs { USR_DIR .. '/include' }
        libdirs { USR_DIR .. '/lib/x64' }
        links
        {
            'ws2_32',
            'iphlpapi',
            'psapi',
        }

    if os.get() == 'linux' then
    configuration 'gmake'
        buildoptions '-std=c99 -mrdrnd'
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
            'm',
            'rt',
            'dl',
            'pthread',
        }
    end

    project 'qsf'
        location 'build'
        kind 'ConsoleApp'
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
            'lua5.3',
            'libuv',
            'msgpack',
            'libzmq',
            'zlib',
            'libmysql',
            'libeay32'
        }
        elseif os.get() == 'linux' then
        links
        {
            'z',
            'uv',
            'zmq',
            'lua5.3',
            'msgpack',
            'uuid',
            'crypto',
            'jemalloc',
            'mysqlclient',
        }
        end


