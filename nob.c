#if !defined(__linux__)
#    error "Sorry, your operating system is not supported :("
#endif // defined(__linux__)

#include <stdio.h>
#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#if defined(__linux__)
#    define QBE_VENDOR_DIR "./vendor/qbe"
#    define QBE_VENDOR_EXE "./vendor/qbe/qbe"

#    define CPROC_VENDOR_DIR "./vendor/cproc"
#    define CPROC_VENDOR_EXE "./vendor/cproc/cproc"

#    define PENGC_EXE "./pengc"
#endif // defined(__linux__)

static bool string_ends_with(const char* str, const char* ending) {
    size_t str_len = strlen(str);
    size_t ending_len = strlen(ending);
    return str_len >= ending_len && 0 == strncmp(str + str_len - ending_len, ending, ending_len);
}

bool build_qbe() {
    bool result = true;
    Nob_Cmd cmd = {0};

    if (!nob_file_exists(QBE_VENDOR_DIR)) {
        nob_cmd_append(&cmd, "git", "clone", "git://c9x.me/qbe.git", QBE_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

    if (!nob_file_exists(QBE_VENDOR_EXE)) {
        nob_cmd_append(&cmd, "make", "-C", QBE_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}

bool build_cproc() {
    bool result = true;
    Nob_Cmd cmd = {0};

    if (!nob_file_exists(CPROC_VENDOR_DIR)) {
        nob_cmd_append(&cmd, "git", "clone", "https://git.sr.ht/~mcf/cproc", CPROC_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

#if defined(__linux__)
    if (!nob_file_exists(CPROC_VENDOR_EXE)) {
        nob_cmd_append(&cmd, "make", "-C", CPROC_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }
#endif // defined(__linux__)

defer:;
    nob_cmd_free(cmd);
    return result;
}

static bool collect_files_in_dir(const char* parent, const char* extension, Nob_File_Paths* children) {
    bool result = true;
    Nob_File_Paths child_names = {0};

    if (!nob_read_entire_dir(parent, &child_names)) {
        nob_return_defer(false);
    }

    for (size_t i = 2; i < child_names.count; i++) {
        char* child_path = nob_temp_sprintf("%s/%s", parent, child_names.items[i]);

        if (NOB_FILE_DIRECTORY == nob_get_file_type(child_path)) {
            if (!collect_files_in_dir(child_path, extension, children)) {
                nob_return_defer(false);
            }
        }

        if (!string_ends_with(child_path, extension)) {
            continue;
        }

        nob_da_append(children, child_path);
    }

defer:;
    nob_da_free(child_names);
    return result;
}

static bool build_pengc() {
    bool result = true;
    Nob_Cmd cmd = {0};
    Nob_File_Paths pengc_source_files = {0};
    Nob_File_Paths pengc_header_files = {0};
    Nob_File_Paths pengc_object_files = {0};

    nob_mkdir_if_not_exists(".out");

    if (!collect_files_in_dir("src", ".c", &pengc_source_files)) {
        nob_return_defer(false);
    }

    if (!collect_files_in_dir("src", ".h", &pengc_header_files)) {
        nob_return_defer(false);
    }
    
    if (!nob_needs_rebuild(PENGC_EXE, pengc_source_files.items, pengc_source_files.count)) {
        nob_return_defer(true);
    }

    char* path_env = getenv("PATH");
    if (NULL == path_env) {
        nob_log(NOB_ERROR, "Could not get the value of the 'PATH' envirionment variable.");
        nob_return_defer(false);
    }

    setenv("PATH", nob_temp_sprintf(QBE_VENDOR_DIR ":" CPROC_VENDOR_DIR ":%s", path_env), 1);

    for (size_t i = 0; i < pengc_source_files.count; i++) {
        const char* source_path = pengc_source_files.items[i];
        const char* object_path = nob_temp_sprintf(".out/%s.o", nob_path_name(source_path));
        nob_da_append(&pengc_object_files, object_path);

        if (!(nob_needs_rebuild1(object_path, source_path) || nob_needs_rebuild(object_path, pengc_header_files.items, pengc_header_files.count))) {
            continue;
        }

        cmd.count = 0;
        nob_cmd_append(&cmd, CPROC_VENDOR_EXE, "-o", object_path, "-c", source_path);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }
    }

    cmd.count = 0;
    nob_cmd_append(&cmd, CPROC_VENDOR_EXE, "-o", PENGC_EXE);
    nob_da_append_many(&cmd, pengc_object_files.items, pengc_object_files.count);
    if (!nob_cmd_run_sync(cmd)) {
        nob_return_defer(false);
    }

defer:;
    nob_da_free(pengc_object_files);
    nob_da_free(pengc_header_files);
    nob_da_free(pengc_source_files);
    nob_cmd_free(cmd);
    return result;
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    int result = 0;
    Nob_Cmd cmd = {0};

    if (!build_qbe()) {
        nob_return_defer(1);
    }

    if (!build_cproc()) {
        nob_return_defer(1);
    }

    if (!build_pengc()) {
        nob_return_defer(1);
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}
