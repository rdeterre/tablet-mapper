///
/// Tiny C++ wrapper around gdk.h
///

#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>

#include <gdk/gdk.h>

namespace gdk {

class init {
  init() { gdk_init(nullptr, nullptr); }

public:
  static init &instance() {
    static init i{};
    return i;
  }
};

struct rectangle {
  int x;
  int y;
  int width;
  int height;
};

std::ostream &operator<<(std::ostream &os, const rectangle &rect) {
  os << "{ x: " << rect.x << ", y:" << rect.y << ", width: " << rect.width
     << ", height: " << rect.height << "}";
  return os;
}

struct position {
  int x;
  int y;
};

std::ostream &operator<<(std::ostream &os, const position &pos) {
  os << "{" << pos.x << ", " << pos.y << "}";
  return os;
}

rectangle from_c(GdkRectangle rect) {
  return rectangle{rect.x, rect.y, rect.width, rect.height};
}

class monitor {
  GdkMonitor *m_monitor;

public:
  monitor(GdkMonitor *monitor) : m_monitor(monitor) {}

  rectangle get_geometry() {
    GdkRectangle rect;
    gdk_monitor_get_geometry(m_monitor, &rect);
    return from_c(rect);
  }
};

class device {
  GdkDevice *m_device;

public:
  device(GdkDevice *device) : m_device(device) {}

  position get_position() {
    position p;
    gdk_device_get_position(m_device, nullptr, &p.x, &p.y);
    return p;
  }
};

class seat {
  GdkSeat *m_seat;

public:
  seat(GdkSeat *seat) : m_seat(seat) {}

  device get_pointer() { return device{gdk_seat_get_pointer(m_seat)}; }
};

class display {
  GdkDisplay *m_display;

  /// Indicates if the display object is the "owner" of `m_display`, i.e.
  /// responsible for closing it.
  bool m_owning;

  display(GdkDisplay *display, bool owning)
      : m_display(display), m_owning(owning) {
    if (display == nullptr)
      throw std::runtime_error("NULL GDK display");
  }

public:
  display(const display &) = delete;

  display(display &&other)
      : m_display(other.m_display), m_owning(other.m_owning) {
    other.m_display = nullptr;
    other.m_owning = false;
  }

  display &operator=(const display &) = delete;

  display &operator=(display &&other) {
    if (m_owning)
      gdk_display_close(m_display);
    m_display = other.m_display;
    m_owning = other.m_owning;
    other.m_display = nullptr;
    other.m_owning = false;
    return *this;
  }

  ~display() {
    if (m_owning)
      gdk_display_close(m_display);
  }

  static display &get_default() {
    init &init = init::instance();
    static display disp{gdk_display_get_default(), false};
    return disp;
  }

  static display open(std::string name) {
    init &init = init::instance();
    return display{gdk_display_open(name.c_str()), true};
  }

  int get_n_monitors() { return gdk_display_get_n_monitors(m_display); }

  std::vector<monitor> get_monitors() {
    std::vector<monitor> monitors;
    for (int i = 0; i < get_n_monitors(); i++) {
      monitors.push_back(monitor(gdk_display_get_monitor(m_display, i)));
    }
    return monitors;
  }

  seat get_default_seat() {
    return seat{gdk_display_get_default_seat(m_display)};
  }
};

} // namespace gdk
