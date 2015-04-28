--
-- Premake script (http://premake.github.io)
--

solution '3rdlibs'
    configurations  {'Debug', 'Release'}
    targetdir       '../bin'
    architecture    'x64'
    
    filter 'configurations:Debug'
        defines     { 'DEBUG' }
        flags       { 'Symbols' }

    filter 'configurations:Release'
        defines     { 'NDEBUG' }
        flags       { 'Symbols' }
        optimize    'On'

    filter 'action:vs*'
        defines
        {
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_CRT_NONSTDC_NO_DEPRECATE',
            'NOMINMAX',
        }

    project 'lua5.3'
        targetname  'lua5.3'
        language    'C'
        kind        'SharedLib'
        location    'build'
        
        files
        {
            'lua/src/*.h',
            'lua/src/*.c',
        }
        removefiles
        {
            'lua/src/lua.c',
            'lua/src/luac.c',
        }
                
        filter 'system:windows'
            defines 'LUA_BUILD_AS_DLL'
            
        filter 'system:linux'
            defines 'LUA_USE_LINUX'
            links   { 'dl', 'readline'}
        
        filter 'system:macosx'
            defines 'DLUA_USE_LINUX'
            links 'readline'
        
    project 'msgpack'
        targetname  'msgpack'
        language    'C'
        kind        'StaticLib'
        location    'build'
        includedirs 
        {
            'msgpack/include'
        }
        files
        {
            'msgpack/include/*.h',
            'msgpack/src/*.c',
        }
       
    project 'libuv'
        language    'C'
        kind        'SharedLib'
        location    'build'
        
        filter 'action:vs*'
            defines     'BUILDING_UV_SHARED'
            includedirs 'libuv/include'
            files
            {
                'libuv/include/uv.h',
                'libuv/include/tree.h',
                'libuv/include/uv-version.h',
                'libuv/include/uv-errno.h',
                'libuv/src/*.h',
                'libuv/src/*.c',
                'libuv/src/win/*.c',
            }
            links 
            {
                'ws2_32',
                'psapi',
                'iphlpapi',
            }
        