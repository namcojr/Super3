/*
 * Release builds: nothing is written to the Android log.
 *
 * Each native library (SUPER3, SDL2, SDL2_net) is linked with
 * -Wl,--wrap=<function> for the liblog calls below, so every call, whether
 * made directly or through SDL_Log / Supermodel's logger, lands in these
 * functions instead. __android_log_assert is left alone: it aborts the app.
 * See SUPER3_LOGGING in CMakeLists.txt.
 */
#include <stdarg.h>

struct __android_log_message;

int __wrap___android_log_write(int prio, const char* tag, const char* text)
{
  (void)prio; (void)tag; (void)text;
  return 0;
}

int __wrap___android_log_print(int prio, const char* tag, const char* fmt, ...)
{
  (void)prio; (void)tag; (void)fmt;
  return 0;
}

int __wrap___android_log_vprint(int prio, const char* tag, const char* fmt, va_list ap)
{
  (void)prio; (void)tag; (void)fmt; (void)ap;
  return 0;
}

int __wrap___android_log_buf_write(int bufID, int prio, const char* tag, const char* text)
{
  (void)bufID; (void)prio; (void)tag; (void)text;
  return 0;
}

int __wrap___android_log_buf_print(int bufID, int prio, const char* tag, const char* fmt, ...)
{
  (void)bufID; (void)prio; (void)tag; (void)fmt;
  return 0;
}

void __wrap___android_log_write_log_message(struct __android_log_message* msg)
{
  (void)msg;
}
