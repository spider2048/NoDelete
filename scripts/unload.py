import ctypes
from ctypes import wintypes
import psutil
import os

kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)

PROCESS_ALL_ACCESS = 0x1F0FFF
INVALID_HANDLE_VALUE = -1
MEM_RELEASE = 0x8000

def unload_file_from_process(pid, filename):
    os.system(f"Injector -e -p {pid} \"{filename}\"")

filename = r"D:\Cpp\No Delete\builddir\NoDelete.exe"
filename2 = r"D:\Cpp\No Delete\builddir\NoDeleteH.dll"
def find_process_by_name(process_name):
    pids = []
    for proc in psutil.process_iter(['pid', 'name']):
        if process_name in proc.info['name']:
            pids.append(proc.info['pid'])
    return pids

for p in find_process_by_name('explorer.exe'):
    unload_file_from_process(p, filename)
    unload_file_from_process(p, filename2)

