///
/// @file
///
/// C++ wrapper around some APIs of libinput
///
/// References:
///  - Reference API documentation:
///    https://wayland.freedesktop.org/libinput/doc/latest/api/
///  - Sample code to list devices:
///    https://github.com/wayland-project/libinput/blob/master/tools/libinput-list-devices.c
///  - libfmt colored output:
///    https://github.com/fmtlib/fmt/issues/231#issuecomment-158096169
///

#include <stdio.h>

#include <unistd.h>

#include <libinput.h>
#include <libudev.h>

#define ANSI_HIGHLIGHT "\x1B[0;1;39m"
#define ANSI_RED "\x1B[0;31m"
#define ANSI_GREEN "\x1B[0;32m"
#define ANSI_YELLOW "\x1B[0;33m"
#define ANSI_BLUE "\x1B[0;34m"
#define ANSI_MAGENTA "\x1B[0;35m"
#define ANSI_CYAN "\x1B[0;36m"
#define ANSI_BRIGHT_RED "\x1B[0;31;1m"
#define ANSI_BRIGHT_GREEN "\x1B[0;32;1m"
#define ANSI_BRIGHT_YELLOW "\x1B[0;33;1m"
#define ANSI_BRIGHT_BLUE "\x1B[0;34;1m"
#define ANSI_BRIGHT_MAGENTA "\x1B[0;35;1m"
#define ANSI_BRIGHT_CYAN "\x1B[0;36;1m"
#define ANSI_NORMAL "\x1B[0m"

static void log_handler(struct libinput *li,
                        enum libinput_log_priority priority, const char *format,
                        va_list args) {
  static int is_tty = -1;

  if (is_tty == -1)
    is_tty = isatty(STDOUT_FILENO);

  if (is_tty) {
    if (priority >= LIBINPUT_LOG_PRIORITY_ERROR)
      printf(ANSI_RED);
    else if (priority >= LIBINPUT_LOG_PRIORITY_INFO)
      printf(ANSI_HIGHLIGHT);
  }

  vprintf(format, args);

  if (is_tty && priority >= LIBINPUT_LOG_PRIORITY_INFO)
    printf(ANSI_NORMAL);
}

static struct libinput *tools_open_udev(const char *seat, bool verbose,
                                        bool *grab) {
  struct libinput *li;
  struct udev *udev = udev_new();

  if (!udev) {
    fprintf(stderr, "Failed to initialize udev\n");
    return NULL;
  }

  li = libinput_udev_create_context(&interface, grab, udev);
  if (!li) {
    fprintf(stderr, "Failed to initialize context from udev\n");
    goto out;
  }

  libinput_log_set_handler(li, log_handler);
  if (verbose)
    libinput_log_set_priority(li, LIBINPUT_LOG_PRIORITY_DEBUG);

  if (libinput_udev_assign_seat(li, seat)) {
    fprintf(stderr, "Failed to set seat\n");
    libinput_unref(li);
    li = NULL;
    goto out;
  }

out:
  udev_unref(udev);
  return li;
}

///
