project('No Delete', 'cpp')

cc = meson.get_compiler('cpp')

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

dep_windlls = [cc.find_library('ntdll'), cc.find_library('Winmm')]

cpp_args = ['-DSPDLOG_WCHAR_TO_UTF8_SUPPORT']
sources = ['src/main.cpp', 'src/util.cpp']

shared_library(
    'NoDelete',
    sources,
    dependencies: [dep_detours, dep_spdlogd, dep_windlls],
    cpp_args: cpp_args + '/DDLIBRARY',
    include_directories: include_directories('./include')
)
