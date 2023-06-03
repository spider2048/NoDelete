project('No Delete', 'cpp')
cc = meson.get_compiler('cpp')

target_name = 'NoDelete'
target_dll_name = 'NoDeleteH'

target_dll = target_dll_name + '.dll'
offset_file = 'offsets.xml'

lib_root = 'D:\Libraries' # your lib root

dep_detours = declare_dependency(
    include_directories: include_directories(lib_root / 'Detours\include'),
    dependencies: [cc.find_library('detours', dirs: lib_root / 'Detours\lib.X64'), cc.find_library('syelog', dirs: lib_root / 'Detours\lib.X64')],
)

dep_spdlog = declare_dependency(
    include_directories: [lib_root / 'SpeedLog\include']
)

dep_cereal = declare_dependency(
    include_directories: [lib_root / 'cereal\include']
)

cpp_args = ['/DDLIBRARY', '/std:c++20',  '/DTARGET_DLL="' + target_dll + '"', '/DOFFSET_FILE="' + offset_file + '"']


dep_dia = declare_dependency(
    include_directories: ['D:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\DIA SDK\\include'],
    dependencies: [
        cc.find_library('diaguids', dirs: 'D:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\DIA SDK\\lib\\amd64')
    ]
)

shared_library(
    target_dll_name,
    ['src/dllmain.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/hook.cpp'],
    dependencies: [dep_detours, dep_spdlog, dep_cereal],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)

executable(
    target_name,
    ['src/main.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/inject.cpp'],
    dependencies: [dep_detours, dep_spdlog, dep_dia, dep_cereal],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)