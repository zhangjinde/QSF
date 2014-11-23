--
-- Premake4 build script (http://industriousone.com/premake/download)
--


solution '3rdlibs'
    configurations {'Debug', 'Release'}
    --flags {'ExtraWarnings'}
    targetdir '../bin'
    location '../'
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
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            'NOMINMAX',
        }

    project 'zmq'
        language 'C++'
        kind 'SharedLib'
        location 'build'
        uuid 'A75AF625-DDF0-4E60-97D8-A2FDC6229AF7'
        defines 
        { 
            'DLL_EXPORT', 
            'FD_SETSIZE=1024', 
            'HAVE_LIBSODIUM',
        }
        files
        {
            'libzmq/include/*.h',
            'libzmq/src/*.hpp',
            'libzmq/src/*.cpp',
        }
        includedirs
        {
            'libsodium/src/libsodium/include',
            'libzmq/include',
            'libzmq/builds/msvc',
        }
        links {'ws2_32', 'sodium'}

    project 'sodium'
        language 'C'
        kind 'SharedLib'
        location 'build'
        uuid 'CB19F7EE-55D6-4C40-849D-64E2D3849041'
        defines 
        {
            'SODIUM_DLL_EXPORT',
            'inline=__inline',
        }
        files
        {
            'libsodium/src/**.h',
            'libsodium/src/**.c',
        }
        includedirs
        {
            'libsodium/src/libsodium/include',
            'libsodium/src/libsodium/include/sodium',
        }        
        
    project 'lua5.2'
        language 'C'
        kind 'SharedLib'
        location 'build'
        uuid 'C9A112FB-08C0-4503-9AFD-8EBAB5B3C204'
        defines 'LUA_BUILD_AS_DLL'
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
        
    project 'zlib'
        language 'C'
        kind 'SharedLib'
        location 'build'
        uuid '9C08AC41-18D8-4FB9-80F2-01F603917025'
        defines {'ZLIB_DLL', 'Z_HAVE_STDARG_H'}
        files
        {
            'zlib/*.h',
            'zlib/*.c',
        } 
