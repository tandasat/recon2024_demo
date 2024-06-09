#pragma once
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <algorithm>
#include <print>

#include <Windows.h>
#include <SetupAPI.h>
#include <newdev.h>
#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Newdev.lib")

HANDLE install_dbutil();
std::optional<std::vector<uint8_t>> dbutil_read(HANDLE dbutil, uint64_t address, size_t length);
bool dbutil_write(HANDLE dbUtil, uint64_t address, const void* data, size_t length);
