#ifndef SHELL_H_
#define SHELL_H_

#include <stddef.h>
#include <stdbool.h>
#include "heap.h"
#include "logger.h"

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
#endif

// -----------------------------------------------------------------------------
// types
// -----------------------------------------------------------------------------

typedef struct {
    char*  std_out;
    char*  std_err;
    int    exit_code;
} ShellResult;

// -----------------------------------------------------------------------------
// internal: drain a readable fd into a heap buffer
// -----------------------------------------------------------------------------

#ifndef _WIN32
static char* drain_fd(int fd) {
    size_t cap  = 4096;
    size_t len  = 0;
    char*  buf  = (char*)MALLOC(cap);
    if (!buf) return NULL;

    char   chunk[1024];
    ssize_t n;
    while ((n = read(fd, chunk, sizeof(chunk))) > 0) {
        if (len + (size_t)n + 1 > cap) {
            cap  *= 2;
            char* grown = (char*)REALLOC(buf, cap);
            if (!grown) { FREE(buf); return NULL; }
            buf = grown;
        }
        memcpy(buf + len, chunk, (size_t)n);
        len += (size_t)n;
    }
    buf[len] = '\0';
    return buf;
}
#endif


//
// WIN32 Error message
//
#ifdef _WIN32

static inline char* GetLastErrorAsString(void) {
    DWORD error = GetLastError();
    if (error == 0) return STRDUP("");

    LPSTR msg_buf = NULL;

    DWORD size = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM     |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg_buf,
        0,
        NULL
    );

    if (size == 0 || !msg_buf) return STRDUP("");

    // strip trailing \r\n that FormatMessage appends
    while (size > 0 && (msg_buf[size-1] == '\n' ||
                        msg_buf[size-1] == '\r' )) {
        msg_buf[--size] = '\0';
    }

    // copy into our own heap so caller can FREE() it normally
    char* result = STRDUP(msg_buf);
    LocalFree(msg_buf);   // FormatMessage allocated this — must use LocalFree
    return result;
}

#endif // _WIN32



// -----------------------------------------------------------------------------
// shell_exec
// -----------------------------------------------------------------------------

static inline ShellResult shell_exec(const char* cmd) {
    ShellResult res = { NULL, NULL, -1 };
    if (!cmd) return res;

#ifdef _WIN32
    // -- Windows: use CreateProcess with redirected pipes ---------------------

    HANDLE out_r, out_w, err_r, err_w;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

    if (!CreatePipe(&out_r, &out_w, &sa, 0) ||
        !CreatePipe(&err_r, &err_w, &sa, 0)) {
        wrn("[shell] failed to create pipes");
        return res;
    }

    // child must not inherit the read ends
    SetHandleInformation(out_r, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(err_r, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags     = STARTF_USESTDHANDLES;
    si.hStdOutput  = out_w;
    si.hStdError   = err_w;
    si.hStdInput   = GetStdHandle(STD_INPUT_HANDLE);

    PROCESS_INFORMATION pi = {0};

    // CreateProcess needs a mutable cmd buffer
    char* cmd_buf = STRDUP(cmd);
    if (!cmd_buf) return res;

    BOOL ok = CreateProcessA(NULL, cmd_buf, NULL, NULL,
                             TRUE, 0, NULL, NULL, &si, &pi);
    FREE(cmd_buf);

    if (!ok) {
        char* err = GetLastErrorAsString();
        wrn("[shell] CreateProcess failed (%lu) %s", GetLastError(), err);
        CloseHandle(out_r); CloseHandle(out_w);
        CloseHandle(err_r); CloseHandle(err_w);
        FREE(err);
        return res;
    }

    // close write ends in parent so ReadFile returns EOF when child exits
    CloseHandle(out_w);
    CloseHandle(err_w);

    // drain stdout
    size_t out_cap = 1024, out_len = 0;
    size_t err_cap = 1024, err_len = 0;
    char*  out_buf = (char*)MALLOC(out_cap); memset(out_buf,0,out_cap);
    char*  err_buf = (char*)MALLOC(err_cap); memset(err_buf,0,err_cap);
    char   chunk[1024];
    DWORD  n;

    while (ReadFile(out_r, chunk, sizeof(chunk), &n, NULL) && n > 0) {
        if (out_len + n + 1 > out_cap) {
            out_cap *= 2;
            char* g = (char*)REALLOC(out_buf, out_cap);
            if (!g) { FREE(out_buf); out_buf = NULL; break; }
            out_buf = g;
        }
        memcpy(out_buf + out_len, chunk, n);
        out_len += n;
    }
    if (out_buf) {
        trim(out_buf,out_len);
    }

    while (ReadFile(err_r, chunk, sizeof(chunk), &n, NULL) && n > 0) {
        if (err_len + n + 1 > err_cap) {
            err_cap *= 2;
            char* g = (char*)REALLOC(err_buf, err_cap);
            if (!g) { FREE(err_buf); err_buf = NULL; break; }
            err_buf = g;
        }
        memcpy(err_buf + err_len, chunk, n);
        err_len += n;
    }
    if (err_buf) {
        trim(err_buf,err_len);
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(out_r);
    CloseHandle(err_r);

    res.std_out   = out_buf;
    res.std_err   = err_buf;
    res.exit_code = (int)exit_code;

#else
    // -- POSIX: fork + pipe + exec --------------------------------------------

    int out_pipe[2], err_pipe[2];

    if (pipe(out_pipe) < 0 || pipe(err_pipe) < 0) {
        wrn("[shell] pipe() failed");
        return res;
    }

    pid_t pid = fork();

    if (pid < 0) {
        wrn("[shell] fork() failed");
        close(out_pipe[0]); close(out_pipe[1]);
        close(err_pipe[0]); close(err_pipe[1]);
        return res;
    }

    if (pid == 0) {
        // -- child ------------------------------------------------------------
        close(out_pipe[0]);   // close read ends
        close(err_pipe[0]);

        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(err_pipe[1], STDERR_FILENO);

        close(out_pipe[1]);
        close(err_pipe[1]);

        execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);
        _exit(127);           // exec failed
    }

    // -- parent ---------------------------------------------------------------
    close(out_pipe[1]);       // close write ends
    close(err_pipe[1]);

    res.std_out = drain_fd(out_pipe[0]);
    res.std_err = drain_fd(err_pipe[0]);

    close(out_pipe[0]);
    close(err_pipe[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    res.exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;

#endif

    return res;
}

// -----------------------------------------------------------------------------
// cleanup
// -----------------------------------------------------------------------------

static inline void shell_result_free(ShellResult* res) {
    if (!res) return;
    FREE(res->std_out);
    FREE(res->std_err);
    res->std_out  = NULL;
    res->std_err  = NULL;
    res->exit_code = -1;
}

static inline void execute_shell(const char* cmd, int print_to_console) {
    #ifdef _WIN32
    char* _cmd = tmp_sprintf("c:\\windows\\system32\\cmd.exe /c \"%s\"", cmd);
    #else
    char* _cmd = (char*)cmd;
    #endif
    

    ShellResult shell = shell_exec(_cmd);
    if (print_to_console) {
        trc("SHELL: [%s] ", cmd);
        inf(" out: %s",shell.std_out);
        if (strlen(shell.std_err) > 0) {
            err(" err: %s",shell.std_err);
        }
    }
    shell_result_free(&shell);
}

#endif // SHELL_H_
