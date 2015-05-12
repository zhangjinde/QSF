--
-- Premake script (http://premake.github.io)
--


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
            'deps/zeromq/include',
        }
        libdirs 'bin'

        filter 'system:windows'
            links
            {
                'lua51',
                'libuv',
                'libzmq',
            }

        filter 'system:linux'
            defines 'USE_JEMALLOC'
            links
            {
                'z',
                'uv',
                'zmq',
                'uuid',
                'luajit-5.1',
                'jemalloc',
            }
