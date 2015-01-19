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
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            'NOMINMAX',
        }

    project 'lua5.3'
        language 'C'
        kind 'SharedLib'
        location 'build'
        uuid 'C9A112FB-08C0-4503-9AFD-8EBAB5B3C204'
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
        