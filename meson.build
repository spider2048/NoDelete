project('No Delete', 'cpp')
cc = meson.get_compiler('cpp')

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

cpp_args = ['/std:c++20']


dep_dia = declare_dependency(
    include_directories: ['D:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\DIA SDK\\include'],
    dependencies: [
        cc.find_library('diaguids', dirs: 'D:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\DIA SDK\\lib\\amd64')
    ]
)

shared_library(
    'NoDeleteH',
    ['src/dllmain.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/hook.cpp'],
    dependencies: [dep_detours, dep_spdlog, dep_cereal],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)

executable(
    'NoDelete',
    ['src/main.cpp', 'src/util.cpp', 'src/winapi_helper.cpp', 'src/inject.cpp'],
    dependencies: [dep_detours, dep_spdlog, dep_dia, dep_cereal],
    cpp_args: cpp_args,
    include_directories: include_directories('./include')
)