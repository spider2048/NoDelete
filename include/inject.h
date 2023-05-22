#include <common.h>

class inject {
    static fs::path dll_path;
    static void     download_pdb_file(const fs::path& dest);
    static void     save_offsets(const fs::path& pdb_path, const fs::path& output_path);

   public:
    static void               init();
    static void               inject_to_pid(DWORD pid);
    static void               eject_from_pid(DWORD pid) {}
    static std::vector<DWORD> get_explorer_pids();
};
