#include "core/log.h"
#include <stdarg.h>
#include <stdio.h>

static int g_errors;

void sa_log(LogLevel lvl, const char *fmt, ...) {
    static const char *tag[] = {"INFO", "WARN", "ERROR"};
    FILE *out = lvl == SA_LOG_ERROR ? stderr : stdout;
    if (lvl == SA_LOG_ERROR) g_errors++;
    fprintf(out, "[%s] ", tag[lvl]);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(out, fmt, ap);
    va_end(ap);
    fputc('\n', out);
}

int sa_log_error_count(void) { return g_errors; }
