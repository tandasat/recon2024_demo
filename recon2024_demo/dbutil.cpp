#include "dbutil.hpp"

using namespace std;

static HANDLE start_dbutil();

static const wchar_t DBUTIL_HARDWARE_ID[] = L"ROOT\\DBUtilDrv2";

#define IOCTL_DBUTIL_VA_READ      0x9b0c1ec4
#define IOCTL_DBUTIL_VA_WRITE     0x9b0c1ec8

struct DbutilVaReadWriteData {
    uint64_t ignored;   // unused by us
    uint64_t target_address;
    uint64_t offset;    // unused by us
#pragma warning(suppress: 4200)
    uint8_t buffer[0];
};
static_assert(sizeof(DbutilVaReadWriteData) == 24);

HANDLE install_dbutil() {
    // Get the INF file path. Expect it to exist in the same directory as this executable file.
    wchar_t module_path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, module_path, _countof(module_path));
    *wcsrchr(module_path, L'\\') = UNICODE_NULL;
    wstring inf_path = module_path;
    inf_path += L"\\dbutildrv2.inf";

    // Install the driver.
    GUID class_guid = {};
    wchar_t class_name[32] = {};
    if (!SetupDiGetINFClassW(inf_path.c_str(), &class_guid, class_name, _countof(class_name), nullptr)) {
        println("SetupDiGetINFClassW failed: {:#x}", GetLastError());
        return nullptr;
    }

    auto dev_info = SetupDiGetClassDevsW(&class_guid, nullptr, nullptr, DIGCF_PRESENT);
    if (dev_info != INVALID_HANDLE_VALUE) {
        bool found = false;
        for (DWORD member_index = 0; /**/; member_index++) {
            SP_DEVINFO_DATA dev_info_data = { sizeof(dev_info_data) };
            if (!SetupDiEnumDeviceInfo(dev_info, member_index, &dev_info_data)) {
                break;
            }

            wchar_t hardware_id[64] = {};
            if (!SetupDiGetDeviceRegistryPropertyW(dev_info, &dev_info_data, SPDRP_HARDWAREID, nullptr, (PBYTE)hardware_id, sizeof(hardware_id), nullptr)) {
                continue;
            }

            if (wcscmp(hardware_id, DBUTIL_HARDWARE_ID) == 0) {
                found = true;
                break;
            }
        }
        SetupDiDestroyDeviceInfoList(dev_info);
        if (found) {
            return start_dbutil();
        }
    }

    dev_info = SetupDiCreateDeviceInfoList(&class_guid, nullptr);
    if (dev_info == INVALID_HANDLE_VALUE) {
        println("SetupDiCreateDeviceInfoList failed: {:#x}", GetLastError());
        return nullptr;
    }

    SP_DEVINFO_DATA dev_info_data = { sizeof(dev_info_data) };
    if (!SetupDiCreateDeviceInfoW(dev_info, class_name, &class_guid, nullptr, nullptr, DICD_GENERATE_ID, &dev_info_data)) {
        println("SetupDiCreateDeviceInfoW failed: {:#x}", GetLastError());
        return nullptr;
    }

    if (!SetupDiSetDeviceRegistryPropertyW(dev_info, &dev_info_data, SPDRP_HARDWAREID, (BYTE*)DBUTIL_HARDWARE_ID, (DWORD)sizeof(DBUTIL_HARDWARE_ID))) {
        println("SetupDiSetDeviceRegistryPropertyW failed: {:#x}", GetLastError());
        return nullptr;
    }

    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE, dev_info, &dev_info_data)) {
        println("SetupDiCallClassInstaller failed: {:#x}", GetLastError());
        return nullptr;
    }

    BOOL reboot_required = FALSE;
    if (!UpdateDriverForPlugAndPlayDevicesW(nullptr, DBUTIL_HARDWARE_ID, inf_path.c_str(), INSTALLFLAG_FORCE | INSTALLFLAG_NONINTERACTIVE, &reboot_required)) {
        println("UpdateDriverForPlugAndPlayDevicesW failed: {:#x}", GetLastError());
        return nullptr;
    }

    return start_dbutil();
}

static HANDLE start_dbutil() {
    auto dbutil = CreateFileW(L"\\\\.\\DBUtil_2_5", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dbutil == INVALID_HANDLE_VALUE) {
        println("CreateFileW failed: {:#x}", GetLastError());
        return nullptr;
    }
    return dbutil;
}

optional<vector<uint8_t>> dbutil_read(HANDLE dbutil, uint64_t address, size_t length) {
    vector<uint8_t> ioctl_data_buffer;
    ioctl_data_buffer.resize(sizeof(DbutilVaReadWriteData) + length);
    auto ioctl_data = (DbutilVaReadWriteData*)ioctl_data_buffer.data();
    ioctl_data->target_address = address;

    DWORD bytes_returned = 0;
    auto buffer = ioctl_data_buffer.data();
    auto size = (DWORD)ioctl_data_buffer.size();
    auto ok = DeviceIoControl(dbutil, IOCTL_DBUTIL_VA_READ, buffer, size, buffer, size, &bytes_returned, nullptr);
    if (!ok) {
        println("DeviceIoControl failed: {:#x}", GetLastError());
        return nullopt;
    }

    std::vector<uint8_t> read_data;
    read_data.resize(length);
    memcpy(read_data.data(), ioctl_data->buffer, length);
    return make_optional(move(read_data));
}

bool dbutil_write(HANDLE dbutil, uint64_t address, const void* data, size_t length) {
    vector<uint8_t> ioctl_data_buffer;
    ioctl_data_buffer.resize(sizeof(DbutilVaReadWriteData) + length);
    auto ioctl_data = (DbutilVaReadWriteData*)ioctl_data_buffer.data();
    ioctl_data->target_address = address;
    memcpy(ioctl_data->buffer, data, length);

    DWORD bytes_returned = 0;
    auto buffer = ioctl_data_buffer.data();
    auto size = (DWORD)ioctl_data_buffer.size();
    auto ok = DeviceIoControl(dbutil, IOCTL_DBUTIL_VA_WRITE, buffer, size, buffer, size, &bytes_returned, nullptr);
    if (!ok) {
        println("DeviceIoControl failed: {:#x}", GetLastError());
        return false;
    }
    return true;
}
