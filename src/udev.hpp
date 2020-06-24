///
///
/// @file
///
/// Very incomplete C++ wrapper around libudev.h
///

#include <stdexcept>

#include <libudev.h>

namespace udv {

class udev {
  struct ::udev *m_udev;

public:
  udev() : m_udev(udev_new()) {
    if (!m_udev)
      throw std::runtime_error("udev_new() failed");
  }
  udev(const udev &) = delete;
  udev &operator=(const udev &) = delete;
  ~udev() { udev_unref(m_udev); }
  struct ::udev *get_c() {
    return m_udev;
  }
};

} // namespace udv
