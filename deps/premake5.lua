--
-- Premake script (http://premake.github.io)
--

solution '3rdlibs'
    configurations  {'Debug', 'Release'}
    platforms       {'Win32', 'Win64'}
    targetdir       '../bin'
    
    filter 'platforms:Win32'
        architecture 'x32'
        
    filter 'platforms:Win64'
        architecture 'x64'
    
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
        