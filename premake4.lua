--
-- Premake4 build script (http://industriousone.com/premake/download)
--

local BOOST_ROOT = os.getenv('BOOST_ROOT') or ''

solution 'qsf'
    configurations {'Debug', 'Release'}
    language 'C++'
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
            'WIN32',
            'WIN32_LEAN_AND_MEAN',
            '_WIN32_WINNT=0x0600',
            '_CRT_SECURE_NO_WARNINGS',
            '_SCL_SECURE_NO_WARNINGS',
            'NOMINMAX',
        }      

    project 'qsf'
        location 'build'
        kind 'ConsoleApp'
        uuid '65BCF1EB-A936-4688-B1F4-7073B4ACE736'
        defines
        {
            '_ELPP_THREAD_SAFE',
        }
        files
        {
            'src/**.h',
            'src/**.cpp',
        }
        excludes 'src/test/**.*'
        includedirs
        {
            'src',
            BOOST_ROOT,
            'deps/lua/src',
            'deps/zeromq/include',
        }
        libdirs 'bin'
        links
        {
            'ws2_32',
        }


    project 'unittest'
        location 'build'
        kind 'ConsoleApp'
        uuid '9E30CCC3-DA13-47FB-9902-7BF6D4792380'
        defines 
        {
            '_ELPP_THREAD_SAFE',
        }
        files
        {
            'deps/gtest/src/gtest-all.cc',
            'src/core/*.*',
            'src/test/*.*', 
        }
        includedirs
        {
            'src',
            'deps/gtest',
            'deps/gtest/include',
        }
        libdirs 
        {
            'bin',
        }
        