/* Log file for the built game (build-release/), not the setup host.
 *
 * The runtime writes all of its messages to stdout/stderr, which a game
 * started from a desktop shortcut, Explorer or a launcher has nowhere to send,
 * so a crash left nothing to read. baroque_log_start() copies both streams
 * into logs/Baroque_Recompiled-<date>-<time>.log next to the executable and
 * still passes them on to the original stderr when there is one (a terminal,
 * or a launcher reading the output). The newest BAROQUE_LOG_KEEP logs are
 * kept. Set BAROQUE_NO_LOG=1 to turn it off. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#define bq_dup _dup
#define bq_dup2 _dup2
#define bq_close _close
#define bq_read _read
#define bq_write _write
#else
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#define bq_dup dup
#define bq_dup2 dup2
#define bq_close close
#define bq_read read
#define bq_write write
#endif

#define BAROQUE_LOG_KEEP 5
#define BAROQUE_LOG_PREFIX "Baroque_Recompiled-"

static int s_log_fd = -1;     /* the log file */
static int s_orig_fd = -1;    /* where stderr went before (or -1) */
static int s_pipe_rd = -1;
static int s_out_fd = 1, s_err_fd = 2;   /* the C streams' descriptors */
#if defined(_WIN32)
static HANDLE s_thread;
#else
static pthread_t s_thread;
static int s_thread_ok;
#endif

static void bq_write_all(int fd, const char* p, int n) {
    while (n > 0) {
        int w = (int)bq_write(fd, p, (unsigned)n);
        if (w <= 0) return;
        p += w;
        n -= w;
    }
}

#if defined(_WIN32)
static unsigned __stdcall bq_pump(void* arg)
#else
static void* bq_pump(void* arg)
#endif
{
    char buf[4096];
    int n;
    (void)arg;
    while ((n = (int)bq_read(s_pipe_rd, buf, sizeof(buf))) > 0) {
        bq_write_all(s_log_fd, buf, n);
        if (s_orig_fd >= 0) bq_write_all(s_orig_fd, buf, n);
    }
    return 0;
}

/* At exit: close our ends of the pipe so the pump sees EOF, then let it
 * write out what is still in the pipe. */
static void bq_log_stop(void) {
    fflush(stdout);
    fflush(stderr);
    bq_close(s_out_fd);
    bq_close(s_err_fd);
#if defined(_WIN32)
    if (s_thread) {
        WaitForSingleObject(s_thread, 2000);
        CloseHandle(s_thread);
        s_thread = NULL;
    }
#else
    if (s_thread_ok) {
        pthread_join(s_thread, NULL);
        s_thread_ok = 0;
    }
#endif
    if (s_log_fd >= 0) bq_close(s_log_fd);
    s_log_fd = -1;
}

static int bq_exe_dir(char* out, size_t cap) {
    char* slash;
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)cap);
    if (n == 0 || n >= cap) return 0;
    for (char* c = out; *c; ++c)
        if (*c == '\\') *c = '/';
#elif defined(__APPLE__)
    uint32_t sz = (uint32_t)cap;
    if (_NSGetExecutablePath(out, &sz) != 0) return 0;
#else
    ssize_t n = readlink("/proc/self/exe", out, cap - 1);
    if (n <= 0) return 0;
    out[n] = '\0';
#endif
    slash = strrchr(out, '/');
    if (!slash) return 0;
    *slash = '\0';
    return 1;
}

static int bq_name_cmp(const void* a, const void* b) {
    return strcmp(*(char* const*)a, *(char* const*)b);
}

/* Remove the oldest logs so that, with the one about to be written, at most
 * BAROQUE_LOG_KEEP remain. Names sort by date because of the timestamp. */
static void bq_prune(const char* dir) {
    char* names[256];
    int count = 0, i;
    char path[1200];
#if defined(_WIN32)
    WIN32_FIND_DATAA fd;
    HANDLE h;
    snprintf(path, sizeof(path), "%s/" BAROQUE_LOG_PREFIX "*.log", dir);
    h = FindFirstFileA(path, &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (count < 256) names[count++] = _strdup(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* d = opendir(dir);
    struct dirent* e;
    size_t plen = strlen(BAROQUE_LOG_PREFIX), len;
    if (!d) return;
    while ((e = readdir(d)) != NULL && count < 256) {
        len = strlen(e->d_name);
        if (strncmp(e->d_name, BAROQUE_LOG_PREFIX, plen) == 0 && len > 4 &&
            strcmp(e->d_name + len - 4, ".log") == 0)
            names[count++] = strdup(e->d_name);
    }
    closedir(d);
#endif
    qsort(names, (size_t)count, sizeof(names[0]), bq_name_cmp);
    for (i = 0; i < count; ++i) {
        if (count - i >= BAROQUE_LOG_KEEP) {
            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            remove(path);
        }
        free(names[i]);
    }
}

void baroque_log_start(int argc, char** argv) {
    char dir[1100], path[1200], stamp[32];
    int fds[2], i;
    time_t now = time(NULL);
    struct tm tmv;
    const char* off = getenv("BAROQUE_NO_LOG");

    if (s_log_fd >= 0 || (off && off[0] && off[0] != '0')) return;
    if (!bq_exe_dir(dir, sizeof(dir) - 8)) return;
    strcat(dir, "/logs");
#if defined(_WIN32)
    _mkdir(dir);
    localtime_s(&tmv, &now);
#else
    mkdir(dir, 0755);
    localtime_r(&now, &tmv);
#endif
    strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", &tmv);
    bq_prune(dir);
    snprintf(path, sizeof(path), "%s/" BAROQUE_LOG_PREFIX "%s.log", dir, stamp);
#if defined(_WIN32)
    s_log_fd = _open(path, _O_WRONLY | _O_CREAT | _O_TRUNC | _O_BINARY, 0644);
#else
    s_log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
#endif
    if (s_log_fd < 0) return;

    {   /* header: when, and how the game was started */
        char head[1600];
        int n = snprintf(head, sizeof(head), "Baroque Recompiled log, %s\n", stamp);
        for (i = 0; i < argc && n > 0 && n < (int)sizeof(head) - 2; ++i)
            n += snprintf(head + n, sizeof(head) - (size_t)n, "%s%s",
                          i ? " " : "args: ", argv[i]);
        if (n > 0 && n < (int)sizeof(head) - 1) {
            head[n++] = '\n';
            bq_write_all(s_log_fd, head, n);
        }
    }

    fflush(stdout);
    fflush(stderr);
#if defined(_WIN32)
    /* A GUI-subsystem exe started from Explorer has no stdout/stderr at all;
     * give the C streams real descriptors before redirecting them. */
    if (_fileno(stdout) < 0) freopen("NUL", "w", stdout);
    if (_fileno(stderr) < 0) freopen("NUL", "w", stderr);
    if (_pipe(fds, 65536, _O_BINARY | _O_NOINHERIT) != 0) goto fail;
#else
    if (pipe(fds) != 0) goto fail;
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);
#endif
#if defined(_WIN32)
    /* freopen above may have given the streams descriptors other than 1/2. */
    s_out_fd = _fileno(stdout);
    s_err_fd = _fileno(stderr);
#endif
    s_orig_fd = bq_dup(s_err_fd);
#if defined(_WIN32)
    if (s_orig_fd >= 0 && _get_osfhandle(s_orig_fd) == -2) {   /* no stream */
        bq_close(s_orig_fd);
        s_orig_fd = -1;
    }
#endif
    s_pipe_rd = fds[0];
    if (bq_dup2(fds[1], s_out_fd) != 0 || bq_dup2(fds[1], s_err_fd) != 0) {
        bq_close(fds[0]);
        bq_close(fds[1]);
        goto fail;
    }
    bq_close(fds[1]);
#if defined(_WIN32)
    SetStdHandle(STD_OUTPUT_HANDLE, (HANDLE)_get_osfhandle(s_out_fd));
    SetStdHandle(STD_ERROR_HANDLE, (HANDLE)_get_osfhandle(s_err_fd));
    s_thread = (HANDLE)_beginthreadex(NULL, 0, bq_pump, NULL, 0, NULL);
    if (!s_thread) goto fail;
#else
    if (pthread_create(&s_thread, NULL, bq_pump, NULL) != 0) goto fail;
    s_thread_ok = 1;
#endif
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    setvbuf(stderr, NULL, _IONBF, 0);
    atexit(bq_log_stop);
    fprintf(stderr, "baroque: logging to %s\n", path);
    return;
fail:
    bq_write_all(s_log_fd, "baroque: could not redirect output\n", 35);
    bq_close(s_log_fd);
    s_log_fd = -1;
}
