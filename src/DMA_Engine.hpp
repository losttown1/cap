#ifndef DMA_ENGINE_HPP
#define DMA_ENGINE_HPP

#include <Windows.h>
#include <cstdint>

// إعدادات DMA الافتراضية
constexpr DWORD TARGET_PID = 35028;
constexpr ULONG64 TARGET_HANDLE = 0x021EE040;

// دوال التهيئة والإغلاق
bool InitializeZeroDMA();
void ShutdownZeroDMA();

// دوال قراءة الذاكرة
bool ReadMemory(ULONG64 address, void* buffer, SIZE_T size);
bool WriteMemory(ULONG64 address, void* buffer, SIZE_T size);

// دوال المساعدة
ULONG64 GetModuleBase(const char* moduleName);
bool IsConnected();

// Template functions للقراءة والكتابة
template<typename T>
T Read(ULONG64 address) {
    T value{};
    ReadMemory(address, &value, sizeof(T));
    return value;
}

template<typename T>
bool Write(ULONG64 address, const T& value) {
    return WriteMemory(address, (void*)&value, sizeof(T));
}

#endif // DMA_ENGINE_HPP
