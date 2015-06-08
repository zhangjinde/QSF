--
-- Premake script (http://premake.github.io)
--


solution 'qsf'
    configurations  {'Debug', 'Release'}
    language        'C'
    targetdir       'bin'

    filter 'configurations:Debug'
        defines     { 'DEBUG' }
        flags       { 'Symbols' }

    filter 'configurations:Release'
        defines     { 'NDEBUG' }
        flags       { 'Symbols'}
        optimize    'On'

    configuration 'vs*'
        architecture 'x64'
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
        links
        {
            'm',
            'rt',
            'dl',
            'pthread',
        }
        
    project 'qsf'
        targetname  'qsf'
        location    'build'
        kind        'ConsoleApp'
        defines 
        {
            'LUA_COMPAT_5_2',
        }
        files
        {
            'src/**.h',
            'src/**.c',
        }
        includedirs
        {
            'src',
        }

        filter 'system:windows'
            libdirs 'deps/lib'
            includedirs
            {
                'deps/include',
                'deps/include/lua',
                'deps/include/zmq',
                'deps/include/zlib',
                'deps/include/libuv',
                'deps/include/libmysql',            
            }
            links
            {
                'zlib',
                'lua5.3',
                'libuv',
                'libzmq',
                'libeay32',
                'libmysql',
            }

        filter 'system:linux'
            defines 'USE_JEMALLOC'
            includedirs
            {
                '/usr/include/mysql',
            }
            links
            {
                'z',
                'uv',
                'zmq',
                'uuid',
                'lua5.3',
                'crypto',                
                'jemalloc',
                'mysqlclient',
            }
 
