#if defined(__linux__)
#    include <unistd.h>
#    include <linux/limits.h>
#else
#    error "Sorry, your operating system is not supported :("
#endif // defined(__linux__)

#include <stdio.h>
#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

#if defined(__linux__)
#    define QBE_VENDOR_DIR "./vendor/qbe"
#    define QBE_VENDOR_EXE "./vendor/qbe/qbe"

#    define HAREC_VENDOR_DIR "./vendor/harec"
#    define HAREC_VENDOR_BIN "./vendor/harec/.bin"
#    define HAREC_VENDOR_EXE "./vendor/harec/.bin/harec"

#    define HARE_VENDOR_DIR "./vendor/hare"
#    define HARE_VENDOR_EXE "./vendor/hare/.bin/hare"

#    define PENGC_EXE "./pengc"
#endif // defined(__linux__)

static char cwd[1024];

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

bool build_harec() {
    bool result = true;
    Nob_Cmd cmd = {0};

    if (!nob_file_exists(HAREC_VENDOR_DIR)) {
        nob_cmd_append(&cmd, "git", "clone", "https://git.sr.ht/~sircmpwn/harec", HAREC_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

#if defined(__linux__)
    if (!nob_file_exists(HAREC_VENDOR_EXE)) {
        if (!nob_file_exists(HAREC_VENDOR_DIR "/config.mk")) {
            nob_copy_file(HAREC_VENDOR_DIR "/configs/linux.mk", HAREC_VENDOR_DIR "/config.mk");
        }

        nob_cmd_append(&cmd, "make", "-C", HAREC_VENDOR_DIR);
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

bool build_hare() {
    bool result = true;
    Nob_Cmd cmd = {0};

    if (!nob_file_exists(HARE_VENDOR_DIR)) {
        nob_cmd_append(&cmd, "git", "clone", "https://git.sr.ht/~sircmpwn/hare", HARE_VENDOR_DIR);
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

    if (!nob_file_exists(HARE_VENDOR_EXE)) {
        if (!nob_file_exists(HARE_VENDOR_DIR "/config.mk")) {
            nob_copy_file(HARE_VENDOR_DIR "/configs/linux.mk", HARE_VENDOR_DIR "/config.mk");
        }

        nob_cmd_append(&cmd, "make", "-C", HARE_VENDOR_DIR);
        nob_cmd_append(&cmd, nob_temp_sprintf("QBE=%s/" QBE_VENDOR_EXE, cwd));
        nob_cmd_append(&cmd, nob_temp_sprintf("HAREC=%s/" HAREC_VENDOR_EXE, cwd));
        if (!nob_cmd_run_sync(cmd)) {
            nob_return_defer(false);
        }

        cmd.count = 0;
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}

static bool collect_source_files_in_dir(const char* parent, Nob_File_Paths* children) {
    bool result = true;
    Nob_File_Paths child_names = {0};

    if (!nob_read_entire_dir(parent, &child_names)) {
        nob_return_defer(false);
    }

    for (size_t i = 2; i < child_names.count; i++) {
        char* child_path = nob_temp_sprintf("%s/%s", parent, child_names.items[i]);

        if (NOB_FILE_DIRECTORY == nob_get_file_type(child_path)) {
            if (!collect_source_files_in_dir(child_path, children)) {
                nob_return_defer(false);
            }
        }

        if (!string_ends_with(child_path, ".ha")) {
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

    if (!collect_source_files_in_dir("src", &pengc_source_files)) {
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

    setenv("HAREPATH", HARE_VENDOR_DIR "", 1);
    setenv("PATH", nob_temp_sprintf(QBE_VENDOR_DIR ":" HAREC_VENDOR_BIN ":%s", path_env), 1);

    nob_cmd_append(&cmd, HARE_VENDOR_EXE, "build", "-o", PENGC_EXE, "-t", "bin", "src");
    if (!nob_cmd_run_sync(cmd)) {
        nob_return_defer(false);
    }

defer:;
    nob_da_free(pengc_source_files);
    nob_cmd_free(cmd);
    return result;
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    int result = 0;
    Nob_Cmd cmd = {0};

    if (NULL == getcwd(cwd, sizeof(cwd))) {
        nob_log(NOB_ERROR, "Could not get the path of the current directory: %s", strerror(errno));
        nob_return_defer(1);
    }

    if (!build_qbe()) {
        nob_return_defer(1);
    }

    if (!build_harec()) {
        nob_return_defer(1);
    }

    if (!build_hare()) {
        nob_return_defer(1);
    }

    if (!build_pengc()) {
        nob_return_defer(1);
    }

defer:;
    nob_cmd_free(cmd);
    return result;
}
