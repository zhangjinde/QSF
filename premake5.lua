--
-- Premake script (http://premake.github.io)
--

-- git clone https://github.com/ichenq/usr
local USR_DIR = os.getenv('USR_DIR') or 'E:/usr'

solution 'qsf'
    configurations  {'Debug', 'Release'}
    language        'C'
    targetdir       'bin'
    architecture    'x64'

    filter 'configurations:Debug'
        defines     { 'DEBUG' }
        flags       { 'Symbols' }

    filter 'configurations:Release'
        defines     { 'NDEBUG' }
        flags       { 'Symbols'}
        optimize    'On'

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
        libdirs     { USR_DIR .. '/lib/x64' }
        links
        {
            'ws2_32',
            'iphlpapi',
            'psapi',
        }

    filter 'action:gmake'
        buildoptions    '-std=c99 -mrdrnd'
        defines         '_GNU_SOURCE'
        
    filter 'system:linux'
        defines     'USE_JEMALLOC'
        includedirs '/usr/include/mysql'
        links
        {
            'm',
            'rt',
            'dl',
            'pthread',
        }

    filter 'system:macosx'
        toolset     'clang'
        
    project 'qsf'
        targetname  'qsf'
        location    'build'
        kind        'ConsoleApp'
        files
        {
            'src/**.h',
            'src/**.c',
        }
        includedirs
        {
            'src',
            'deps/luajit/src',
            'deps/libuv/include',
            'deps/msgpack/include',
        }
        libdirs 'bin'

        filter 'system:windows'
            links
            {
                'lua51',
                'libuv',
                'msgpack',
                'libzmq',
                'zlib',
                'libeay32'
            }

        filter 'system:linux'
            links
            {
                'z',
                'uv',
                'zmq',
                'lua51',
                'msgpack',
                'uuid',
                'crypto',
                'jemalloc',
            }
