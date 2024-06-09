#include "dbutil.hpp"

using namespace std;

static void usage();
static uint64_t to_uint64(const wstring& str);
static void read64(HANDLE dbutil, uint64_t va);
static void write64(HANDLE dbutil, uint64_t va, uint64_t value);

int wmain(int argc, wchar_t* argv[]) {
    vector<wstring> args(argv + 1, argv + argc);
    if (args.empty()) {
        usage();
    }

    auto dbutil = install_dbutil();
    if (dbutil == nullptr) {
        return EXIT_FAILURE;
    }

    if (args[0] == L"--read64" && args.size() == 2) {
        auto va = to_uint64(args[1]);
        read64(dbutil, va);
    }
    else if (args[0] == L"--write64" && args.size() == 3) {
        auto va = to_uint64(args[1]);
        auto value = to_uint64(args[2]);
        write64(dbutil, va, value);
    }
    else {
        usage();
    }

    return EXIT_SUCCESS;
}

static void usage() {
    println("> this.exe --read64 va");
    println("> this.exe --write64 va value");
    exit(EXIT_FAILURE);
}

static uint64_t to_uint64(const wstring& str) {
    // Handle the format used in Windbg by deleting a backtick.
    auto normalized = str;
    normalized.erase(remove(normalized.begin(), normalized.end(), L'`'), normalized.end());
    return wcstoull(normalized.c_str(), nullptr, 16);
}

static void read64(HANDLE dbutil, uint64_t va) {
    auto read = dbutil_read(dbutil, va, sizeof(uint64_t));
    if (!read) {
        println("dbutil_read failed");
        exit(EXIT_FAILURE);
    }
    auto value = *(uint64_t*)read.value().data();
    println("{:#x}: {:#x}", va, value);
}

static void write64(HANDLE dbutil, uint64_t va, uint64_t value) {
    if (!dbutil_write(dbutil, va, &value, sizeof(value))) {
        println("dbutil_write failed");
        exit(EXIT_FAILURE);
    }
}
