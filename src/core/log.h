#ifndef SA_LOG_H
#define SA_LOG_H

typedef enum { SA_LOG_INFO, SA_LOG_WARN, SA_LOG_ERROR } LogLevel;

void sa_log(LogLevel lvl, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
/* Number of errors logged since start; smoke tests/CI use this. */
int sa_log_error_count(void);

#define LOGI(...) sa_log(SA_LOG_INFO, __VA_ARGS__)
#define LOGW(...) sa_log(SA_LOG_WARN, __VA_ARGS__)
#define LOGE(...) sa_log(SA_LOG_ERROR, __VA_ARGS__)

#endif
