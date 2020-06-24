///
/// @file
///
/// Very incomplete C++ wrapper around some libinput APIs
///
/// References:
///  - Reference API documentation:
///    https://wayland.freedesktop.org/libinput/doc/latest/api/
///  - Sample code to list devices:
///    https://github.com/wayland-project/libinput/blob/master/tools/libinput-list-devices.c
///    https://github.com/wayland-project/libinput/blob/master/tools/shared.c
///  - libfmt colored output:
///    https://github.com/fmtlib/fmt/issues/231#issuecomment-158096169
///

#include <optional>

#include <stdio.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <unistd.h>

#include <libinput.h>

#include "fmt/format.h"
#include "fmt/ostream.h"

#include "udev.hpp"

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

static void _log_handler(struct libinput *li,
                         enum libinput_log_priority priority,
                         const char *format, va_list args) {
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

static const struct libinput_interface _interface = {
    .open_restricted =
        [](const char *path, int flags, void *) {
          int fd = open(path, flags);

          if (fd < 0)
            fprintf(stderr, "Failed to open %s (%s)\n", path, strerror(errno));

          return fd < 0 ? -errno : fd;
        },
    .close_restricted = [](int fd, void *) { close(fd); },
};

namespace li {

struct transformation_matrix {
  float a;
  float b;
  float c;
  float d;
  float e;
  float f;

  static transformation_matrix scale(float xs, float ys) {
    return transformation_matrix{xs, 0, 0, 0, ys, 0};
  }

  static transformation_matrix translate(float x, float y) {
    return transformation_matrix{1, 0, x, 0, 1, y};
  }
};

inline transformation_matrix operator*(const transformation_matrix &l,
                                       const transformation_matrix &r) {
  return transformation_matrix{
      l.a * r.a + l.b * r.d,       l.a * r.b + l.b * r.e,
      l.a * r.c + l.b * r.f + l.c, l.d * r.a + l.e * r.d,
      l.d * r.b + l.e * r.e,       l.d * r.c + l.e * r.f + l.f};
}

inline bool operator==(const transformation_matrix &l,
                       const transformation_matrix &r) {
  return (l.a == r.a) && (l.b == r.b) && (l.c == r.c) && (l.d == r.d) &&
         (l.e == r.e) && (l.f == r.f);
}

inline std::ostream &operator<<(std::ostream &os,
                                const transformation_matrix &mat) {
  os << "{" << mat.a << ", " << mat.b << ", " << mat.c << ", " << mat.d << ", "
     << mat.e << ", " << mat.f << "}";
  return os;
}

class device {
  struct libinput_device *m_device;

  device(struct libinput_device *dev) : m_device(dev) {
    if (dev == nullptr)
      throw std::runtime_error("Invalid libinput device");
  }

public:
  static device from_c(struct libinput_device *dev) { return device(dev); }

  device(const device &) = delete;
  device(device &&other) noexcept : m_device(other.m_device) {
    other.m_device = nullptr;
  }

  device &operator=(const device &) = delete;
  device &operator=(device &&other) noexcept {
    swap(other);
    return *this;
  }

  void swap(device &other) noexcept {
    struct libinput_device *dev = m_device;
    m_device = other.m_device;
    other.m_device = dev;
  }

  std::optional<transformation_matrix> get_matrix() const {
    if (libinput_device_config_calibration_has_matrix(m_device) == 0)
      return std::nullopt;
    float matrix[6];
    libinput_device_config_calibration_get_matrix(m_device, &matrix[0]);
    return transformation_matrix{matrix[0], matrix[1], matrix[2],
                                 matrix[3], matrix[4], matrix[5]};
  }

  void set_matrix(transformation_matrix mat) {
    // On my system, with an XP-PEN Deco 01 tablet, this function doesn't seem
    // to have any effect on the actual tablet pointer.
    float matrix[6] = {mat.a, mat.b, mat.c, mat.d, mat.e, mat.f};
    auto status =
        libinput_device_config_calibration_set_matrix(m_device, &matrix[0]);
    switch (status) {
    case LIBINPUT_CONFIG_STATUS_SUCCESS:
      return;
    case LIBINPUT_CONFIG_STATUS_UNSUPPORTED:
      throw std::runtime_error("set_matrix unsupported for device");
    case LIBINPUT_CONFIG_STATUS_INVALID:
      throw std::runtime_error("set_matrix returned invalid");
    default:
      throw std::runtime_error("set_matrix returned an unknown error code");
    }
  }

  bool has_capability(enum libinput_device_capability cap) {
    return libinput_device_has_capability(m_device, cap);
  }

  std::string get_name() { return libinput_device_get_name(m_device); }
};

void swap(device &l, device &r) noexcept { l.swap(r); }

class libinput {

  struct ::libinput *m_libinput;
  libinput(::libinput *m_lib, std::string seat) : m_libinput(m_lib) {
    if (!m_libinput) {
      throw std::runtime_error("libinput is null");
    }
    libinput_log_set_handler(m_libinput, _log_handler);

    if (libinput_udev_assign_seat(m_libinput, seat.c_str())) {
      throw std::runtime_error("Failed to set seat");
    }
  }

public:
  static libinput from_c(::libinput *lib, std::string seat = "seat0") {
    return libinput(lib, seat);
  }
  static libinput from_udev(std::string seat = "seat0") {
    udv::udev dev = {};
    return libinput(
        libinput_udev_create_context(&_interface, nullptr, dev.get_c()), seat);
  }

  libinput(const libinput &) = delete;
  libinput(libinput &&other) : m_libinput(other.m_libinput) {
    other.m_libinput = nullptr;
  }
  libinput &operator=(const libinput &) = delete;
  libinput &operator=(libinput &&other) {
    if (m_libinput)
      libinput_unref(m_libinput);
    m_libinput = other.m_libinput;
    other.m_libinput = nullptr;
    return *this;
  }
  ~libinput() {
    if (m_libinput)
      libinput_unref(m_libinput);
  }

  void dispatch(std::function<void(libinput_event *)> process_event) {
    libinput_dispatch(m_libinput);
    libinput_event *ev = nullptr;
    while ((ev = libinput_get_event(m_libinput))) {

      process_event(ev);

      libinput_event_destroy(ev);
      libinput_dispatch(m_libinput);
    }
  }

  std::vector<device> get_devices() {
    std::vector<device> devs;
    dispatch([&devs](libinput_event *ev) {
      if (libinput_event_get_type(ev) != LIBINPUT_EVENT_DEVICE_ADDED)
        return;
      devs.push_back(device::from_c(libinput_event_get_device(ev)));
    });
    return devs;
  }
};
}; // namespace li
