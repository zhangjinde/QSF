--
-- Premake4 build script (http://industriousone.com/premake/download)
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
            'WIN32',
            'WIN32_LEAN_AND_MEAN',
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
        defines { 'DLL_EXPORT', 'FD_SETSIZE=1024', }
        files
        {
            'zeromq/include/*.h',
            'zeromq/src/*.hpp',
            'zeromq/src/*.cpp',
        }
        includedirs
        {
            'zeromq/include',
            'zeromq/builds/msvc',
        }
        links 'ws2_32'

    project 'lua'
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
        
