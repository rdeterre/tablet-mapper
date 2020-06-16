#include <chrono>
#include <iostream>
#include <thread>

#include "gdk.hpp"

using namespace std::literals;

int main(int argc, char *argv[]) {
  auto &disp = gdk::display::get_default();
  auto monitors = disp.get_monitors();
  std::cout << "Number of monitors:" << monitors.size() << "\n";
  for (auto &mon : monitors) {
    std::cout << mon.get_geometry() << "\n";
  }
  auto pointer = disp.get_default_seat().get_pointer();
  while (true) {
    std::this_thread::sleep_for(500ms);
    std::cout << "Pointer position: " << pointer.get_position() << "\n";
  }
}

// xrandr | grep " connected " | awk '{ print$1 }'
// xinput set-prop "UGTABLET DECO 01 Pen (0)" --type=float "Coordinate Transformation Matrix" 0.5 0 0 0 1 0 0 0 1
