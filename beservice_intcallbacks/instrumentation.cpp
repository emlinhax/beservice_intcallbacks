#include "common.h"
#pragma warning(disable:4996)

void DebugOut(const wchar_t* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    wchar_t dbg_out[4096];
    vswprintf_s(dbg_out, fmt, argp);
    va_end(argp);
    OutputDebugString(dbg_out);
}

DWORD instrumentation::tls_index;
int repeat = 0;

bool g_shouldshould = true;

void callback(CONTEXT* ctx) {

    auto teb = reinterpret_cast<uint64_t>(NtCurrentTeb());
    ctx->Rip = *reinterpret_cast<uint64_t*>(teb + 0x2d8);
    ctx->Rsp = *reinterpret_cast<uint64_t*>(teb + 0x2e0);
    ctx->Rcx = ctx->R10;

    if(repeat == 4) {
        RtlRestoreContext(ctx, nullptr);
        return;
    }

    if (instrumentation::is_thread_handling_syscall()) {
        RtlRestoreContext(ctx, nullptr);
    }

    if (!instrumentation::set_thread_handling_syscall(true)) {
        RtlRestoreContext(ctx, nullptr);
    }

    auto return_address = reinterpret_cast<void*>(ctx->Rip);
    auto return_value = reinterpret_cast<void*>(ctx->Rax);

    uint64_t offset_into_function;
    auto function_name = syms::g_parser->get_function_sym_by_address(
        return_address, &offset_into_function);

    if (strcmp(function_name.c_str(), "ZwCreateFile") == 0)
    {
        HANDLE hOrig = *(HANDLE*)(ctx->Rsp + 0x80);
        if (hOrig != INVALID_HANDLE_VALUE && hOrig > (HANDLE)0x10 && hOrig < (HANDLE)0x1000)
        {
            size_t size = sizeof(FILE_NAME_INFO) + sizeof(WCHAR) * MAX_PATH;
            FILE_NAME_INFO* cInfo = (FILE_NAME_INFO*)(malloc(size));
            if (cInfo)
            {
                BOOL status = GetFileInformationByHandleEx(hOrig, FileNameInfo, cInfo, size);
                if (status)
                {
                    if (wcsstr(cInfo->FileName, L"SPINF.dll") != 0)
                    {
                        repeat++;
                        //Who wouldve thought? SPINF.dll is ksuser.dll all of a sudden. Weird... ;)
                        HANDLE hksuser = CreateFileA("C:\\Users\\emlin\\desktop\\ksuser.dll", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                        *(HANDLE*)(ctx->Rsp + 0x80) = hksuser;

                        //dont close the handle since battleye does that for us.
                    }
                }
            }
        }
    }

    instrumentation::set_thread_handling_syscall(false);
    RtlRestoreContext(ctx, nullptr);
}

bool* instrumentation::get_thread_data_pointer() {
    void* thread_data = nullptr;
    bool* data_pointer = nullptr;

    thread_data = TlsGetValue(instrumentation::tls_index);

    if (thread_data == nullptr) {
        thread_data = reinterpret_cast<void*>(LocalAlloc(LPTR, 256));

        if (thread_data == nullptr) {
            return nullptr;
        }

        RtlZeroMemory(thread_data, 256);


        if (!TlsSetValue(instrumentation::tls_index, thread_data)) {
            return nullptr;
        }
    }

    data_pointer = reinterpret_cast<bool*>(thread_data);

    return data_pointer;
}

bool instrumentation::set_thread_handling_syscall(bool value) {
    if (auto data_pointer = get_thread_data_pointer()) {
        *data_pointer = value;
        return true;
    }

    return false;
}

bool instrumentation::is_thread_handling_syscall() {
    if (auto data_pointer = get_thread_data_pointer()) {
        return *data_pointer;
    }

    return false;
}

bool instrumentation::initialize() {

    auto nt_dll = LoadLibrary(L"ntdll.dll");
    if (!nt_dll)
        return false;

    auto nt_set_information_process = reinterpret_cast<instrumentation::nt_set_information_process_t>(GetProcAddress(nt_dll, "NtSetInformationProcess"));

    if (!nt_set_information_process)
        return false;

    instrumentation::tls_index = TlsAlloc();

    if (instrumentation::tls_index == TLS_OUT_OF_INDEXES)
        return false;

    process_instrumentation_callback_info_t info;
    info.version = 0;  // x64 mode
    info.reserved = 0;
    info.callback = bridge;

    nt_set_information_process(GetCurrentProcess(),static_cast<PROCESS_INFORMATION_CLASS>(0x28),&info, sizeof(info));

    return true;
}