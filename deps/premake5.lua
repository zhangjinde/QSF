--
-- Premake script (http://premake.github.io)
--

solution '3rdlibs'
    configurations {'Debug', 'Release'}
    --flags {'ExtraWarnings'}
    targetdir '../bin'
    location '../'
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
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_CRT_NONSTDC_NO_DEPRECATE',
            'NOMINMAX',
        }

    project 'lua5.3'
        language 'C'
        kind 'SharedLib'
        location 'build'
        if os.get() == 'linux' then
        defines 'LUA_USE_LINUX'
        links{ 'dl', 'libreadline'}
        else
        defines 'LUA_BUILD_AS_DLL'
        end
        files
        {
            'lua/src/*.h',
            'lua/src/*.c',
        }
        excludes
        {
            'lua/src/lua.c',
            'lua/src/luac.c',
        }
        
    project 'msgpack'
        language 'C'
        kind 'StaticLib'
        location 'build'
        includedirs 
        {
            'msgpack/include'
        }
        files
        {
            'msgpack/**.h',
            'msgpack/**.c',
        }
        
    -- take `./configure && make` in linux 
    if os.get() ~= 'windows' then return end
    
    project 'libuv'
        language 'C'
        kind 'SharedLib'
        location 'build'
        defines 'BUILDING_UV_SHARED'
        files
        {
            'libuv/include/uv.h',
            'libuv/include/tree.h',
            'libuv/include/uv-version.h',
            'libuv/include/uv-errno.h',
            'libuv/src/*.h',
            'libuv/src/*.c',
        }
        files
        {
            'libuv/src/win/*.c',
        }
        includedirs 
        {
            'libuv/include',
        }
        links 
        {
            'ws2_32',
            'psapi',
            'iphlpapi',
        }