#include <chrono>
#include <iostream>
#include <thread>

#include "fmt/ostream.h"

#include "gdk.hpp"
#include "libinput.hpp"

using namespace std::literals;

int main(int argc, char *argv[]) {
  auto &disp = gdk::display::get_default();
  auto monitors = disp.get_monitors();
  std::cout << "Number of monitors:" << monitors.size() << "\n";
  for (auto &mon : monitors) {
    std::cout << mon.get_geometry() << "\n";
  }
  auto pointer = disp.get_default_seat().get_pointer();

  li::libinput libi = li::libinput::from_udev();

  auto is_tablet = [](li::device &dev) {
    return dev.has_capability(LIBINPUT_DEVICE_CAP_TABLET_TOOL);
  };

  for (auto &dev : libi.get_devices()) {
    if (!is_tablet(dev))
      continue;
    fmt::print("Found a tablet: {}.", dev.get_name());
    auto mat = dev.get_matrix();
    if (mat) {
      fmt::print("Matrix : {}", *mat);
      dev.set_matrix({0.5, 0, 0, 0, 1, 0});
      fmt::print("\nMatrix after set: {}", *dev.get_matrix());
    }
    fmt::print("\n");
  }

  // libi.dispatch([](libinput_event *ev) {
  //   if (libinput_event_get_type(ev) != LIBINPUT_EVENT_DEVICE_ADDED)
  //     return;
  //   struct libinput_device *dev = libinput_event_get_device(ev);
  //   fmt::print(
  //       "Device {} added. Tablet? {}\n", libinput_device_get_name(dev),
  //       libinput_device_has_capability(dev,
  //       LIBINPUT_DEVICE_CAP_TABLET_TOOL));
  // });

  while (true) {
    std::this_thread::sleep_for(500ms);
    std::cout << "Pointer position: " << pointer.get_position() << "\n";
  }
}

// xrandr | grep " connected " | awk '{ print$1 }'
// xinput set-prop "UGTABLET DECO 01 Pen (0)" --type=float "Coordinate
// Transformation Matrix" 0.5 0 0 0 1 0 0 0 1
