project('No Delete', 'cpp')
cc = meson.get_compiler('cpp')

target_name = 'NoDelete'
target_dll_name = 'NoDeleteH'
target_dll = target_dll_name + '.dll'

dep_detours = declare_dependency(
    include_directories: include_directories('D:\Libraries\Detours\include'),
    dependencies: [cc.find_library('detours', dirs: 'D:\Libraries\Detours\lib.X64'), cc.find_library('syelog', dirs: 'D:\Libraries\Detours\lib.X64')],
)

dep_spdlogd = declare_dependency(
    include_directories: ['D:\Libraries\SpeedLog\include'],
    dependencies: [cc.find_library('spdlogd', dirs: 'D:\\Libraries\\SpeedLog\\build2\\Debug')]
)

dep_spdlogrel = declare_dependency(
    include_directories: ['D:\Libraries\SpeedLog\include'],
    dependencies: [cc.find_library('spdlog', dirs: 'D:\\Libraries\\SpeedLog\\build2\\MinSizeRel')]
)

dep_cereal = declare_dependency(
    include_directories: ['D:\Libraries\cereal\include']
)

dep_windlls = [cc.find_library('ntdll'), cc.find_library('Winmm')]
cpp_args = ['-DSPDLOG_WCHAR_TO_UTF8_SUPPORT', '/DDLIBRARY', '/std:c++20',  '/DTARGET_DLL="' + target_dll + '"']

shared_library(
    target_dll_name,
    ['src/dllmain.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/db.cpp'],
    dependencies: [dep_detours, dep_spdlogrel, dep_windlls],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)

executable(
    target_name,
    ['src/main.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/db.cpp'],
    dependencies: [dep_detours, dep_spdlogd, dep_windlls, dep_cereal],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)