// Copyright (C) 2014-2015 chenqiang@chaoyuehudong.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf_trace.h"
#include "qsf.h"
#include <string.h>

#ifdef _WIN32
# include <Windows.h>
# include <DbgHelp.h>
# pragma comment(lib, "dbghelp")
#else
#include <execinfo.h>
#endif


static char* append_string(const char* s)
{
    static QSF_TLS char static_buffer[8192];
    return strcat(static_buffer, s);
}


#ifdef _WIN32

#define TRACE_MAX_STACK_FRAMES          40
#define TRACE_MAX_FUNCTION_NAME_LENGTH  512

const char* qsf_trace_stack(int max_depth)
{
    void* stack[TRACE_MAX_STACK_FRAMES];
    HANDLE hProcess = GetCurrentProcess();
    SymInitialize(hProcess, NULL, TRUE);
    int num_frames = CaptureStackBackTrace(0, TRACE_MAX_STACK_FRAMES, stack, NULL);
    max_depth = (max_depth > 0 ? QSF_MIN(max_depth, num_frames) : num_frames);
    int symbol_bytes = sizeof(SYMBOL_INFO) + sizeof(char) * (TRACE_MAX_FUNCTION_NAME_LENGTH - 1);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)qsf_malloc(symbol_bytes);
    symbol->MaxNameLen = TRACE_MAX_FUNCTION_NAME_LENGTH;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    DWORD dispacement;
    IMAGEHLP_LINE64* line = (IMAGEHLP_LINE64*)qsf_malloc(sizeof(IMAGEHLP_LINE64));
    line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    for (int i = 1; i < num_frames; i++)
    {
        char text[256];
        DWORD64 address = (DWORD64)stack[i];
        SymFromAddr(hProcess, address, NULL, symbol);
        if (SymGetLineFromAddr64(hProcess, address, &dispacement, line))
        {
            sprintf_s(text, 256, "%s[%d]: %s() at 0x%p.\n", line->FileName, line->LineNumber,
                symbol->Name, symbol->Address);
        }
        else
        {
            sprintf_s(text, 256, "unknown file: %s() at 0x%p.\n", symbol->Name, symbol->Address);
        }
        append_string(text);
    }
    SymCleanup(hProcess);
    qsf_free(line);
    qsf_free(symbol);
    CloseHandle(hProcess);
    return append_string("");
}
#endif

#ifdef __GNUC__
#define STACK_LENGTH    64
const char* qsf_trace_stack(int max_depth)
{
    void* stack[STACK_LENGTH];
    int size = backtrace(stack, STACK_LENGTH);
    max_depth = (max_depth > 0 ? QSF_MIN(max_depth, size) : size);
    char** symbols = backtrace_symbols(stack, max_depth);
    if (symbols)
    {
        for (int i = 0; i < max_depth; i++)
        {
            char text[256];
            snprintf(text, 256, "%s\n", symbols[i]);
            append_string(text);
        }
        free(symbols);
        return append_string("");
    }
    return "";
}
#endif
