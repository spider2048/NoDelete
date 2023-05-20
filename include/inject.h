#include <common.h>

class inject {
    static fs::path dll_path;
    static void     download_pdb_file(const std::string& dest);
    static void     save_offsets(const std::string& pdb_path, const std::string& output_path);

   public:
    static void               init();
    static void               inject_to_pid(DWORD pid);
    static void               eject_from_pid(DWORD pid) {}
    static std::vector<DWORD> get_explorer_pids();
};
