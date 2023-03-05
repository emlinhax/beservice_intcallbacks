#include "common.h"

int main(void) {

    //OutputDebugStringA("[SuperBE]\n");
    //output.close();

    syms::initialize();

    if (!instrumentation::initialize()) {
        //OutputDebugStringA("[+] Couldn't initialize instrumentation callbacks.\n");
    }

    while (true) {
        Sleep(1);
    }

    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main, 0, 0, 0);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

