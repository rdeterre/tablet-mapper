///
/// @file
///
/// C++ module that has *some* of the functionality of the `xinput` utility
///
/// references:
///  - https://www.x.org/releases/current/doc/libX11/libX11/libX11.html
///

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "X11/X.h"
#include "X11/Xatom.h"
#include "X11/Xlib.h"
#include "X11/extensions/XInput.h"

namespace x {

class display;

struct atom {
  static atom from_c(Atom catom, Display *disp) {
    char *cname = XGetAtomName(disp, catom);
    std::string nam = std::string(cname);
    XFree(cname);
    return atom(catom, nam);
  }

  const Atom c_atom;
  const std::string name;
};

using property_value =
    std::variant<std::vector<char>, std::vector<short>, std::vector<long>,
                 std::vector<unsigned char>, std::vector<unsigned short>,
                 std::vector<unsigned long>, std::vector<std::string>,
                 std::vector<atom>, float>;

template <class T>
std::vector<T> parse_integer_property(void *data, int nitems) {
  size_t size = sizeof(T);
  std::vector<T> out;
  T *tdata = (T *)data;
  for (int i = 0; i < nitems; i++) {
    out.push_back(tdata[i]);
  }
  return out;
}

std::vector<std::string> parse_string_property(void *data, int nitems) {
  std::vector<std::string> out;
  char *ptr = (char *)data;
  for (int i = 0; i < nitems; i++) {
    out.push_back(ptr);
    ptr += strlen(ptr);
    ptr += 1; // Goes over the terminating 0
  }
  return out;
}

std::vector<atom> parse_atom_property(void *data, int act_format, int nitems,
                                      Display *disp) {
  int size;
  switch (act_format) {
  case 8:
    size = sizeof(char);
    break;
  case 16:
    size = sizeof(short);
    break;
  case 32:
    size = sizeof(long);
    break;
  default:
    throw std::runtime_error("Invalid format for parsing atoms");
  }

  std::vector<atom> out;
  unsigned char *ptr = (unsigned char *)data;
  for (int i = 0; i < nitems; i++) {
    out.push_back(atom::from_c(*(Atom *)ptr, disp));
    ptr += size;
  }
  return out;
}

property_value parse_property(Display *dpy, XDevice *dev, Atom property) {
  Atom act_type;
  int act_format;
  unsigned long nitems, bytes_after;
  unsigned char *data, *ptr;
  int j, done = false, size = 0;

  int rc =
      XGetDeviceProperty(dpy, dev, property, 0, 1000, False, AnyPropertyType,
                         &act_type, &act_format, &nitems, &bytes_after, &data);
  if (rc != Success) {
    throw std::runtime_error("XGetDeviceProperty failed");
  }

  Atom float_atom = XInternAtom(dpy, "FLOAT", True);

  switch (act_type) {
  case XA_INTEGER:
    switch (act_format) {
    case 8:
      return parse_integer_property<char>(data, nitems);
    case 16:
      return parse_integer_property<short>(data, nitems);
    case 32:
      return parse_integer_property<long>(data, nitems);
    default:
      throw std::runtime_error("Invalid format for parsing integers");
    }
    break;
  case XA_CARDINAL:
    switch (act_format) {
    case 8:
      return parse_integer_property<unsigned char>(data, nitems);
    case 16:
      return parse_integer_property<unsigned short>(data, nitems);
    case 32:
      return parse_integer_property<unsigned long>(data, nitems);
    default:
      throw std::runtime_error("Invalid format for parsing cardinals");
    }
    break;
  case XA_STRING:
    if (act_format != 8)
      throw std::runtime_error("Unknown string format");
    return parse_string_property(data, nitems);
  case XA_ATOM:
    return parse_atom_property(data, act_format, nitems, dpy);
  default:
    if (float_atom != 0 && act_type == float_atom) {
      return *(float *)ptr;
    }
    throw std::runtime_error("Unkown type");
  }
}

struct device_property {
  atom prop_atom;

  std::string get_name() { return prop_atom.name; }
};

class device {
  XID m_id;
  Atom m_type;
  std::string m_name;
  XDevice *m_device;

  device(XDevice *device, XDeviceInfo dev)
      : m_id(dev.id), m_type(dev.type), m_name(dev.name), m_device(device) {
    if (m_device == nullptr) {
      throw std::runtime_error("Device is null");
    }
  }

public:
  static from_c(XDisplay *disp, XDeviceInfo dev) {
    XDevice *device = XOpenDevice(disp.c(), dev.id);
    return device(device, dev);
  }

  device(const device &) = delete;
  device(device &&other)
      : m_id(other.m_id), m_type(other.m_type), m_name(other.m_name),
        m_display(other.m_display), m_device(other.m_device) {}

  device &operator=(const device &) = delete;
  device &operator=(device &&other) { swap(other); }

  ~device() {
    if (m_device)
      XCloseDevice(m_display.c(), m_device);
  }

  void swap(device &other) noexcept {
    XID id = m_id;
    Atom type = m_type;
    std::string name = m_name;
    display &display = m_display;
    XDevice *device = m_device;
    m_id = other.m_id;
    m_type = other.m_type;
    m_name = other.m_name;
    m_display = other.m_display;
    m_device = other.m_device;
    other.m_id = id;
    other.m_type = type;
    other.m_name = name;
    other.m_display = display;
    other.m_device = device;
  }

  std::vector<device_property> get_properties() {
    int nprops;
    Atom *cprops = XListDeviceProperties(display.c(), m_device, &nprops);
    std::vector props;
    for (int i = 0; i < nprops; i++)
      props.push_back(device_property::from_c(cprops[i], m_display));
    XFree(cprops);
    return props;
  }
};

void swap(device &left, device &right) noexcept { left.swap(right); }

class display {
  Display *m_display;

public:
  display() : m_display(XOpenDisplay(nullptr)) {
    if (m_display == nullptr)
      throw std::runtime_error("Display is NULL");
  }

  display(std::string name) : m_display(XOpenDisplay(name.c_str())) {}

  ~display() {
    if (m_display)
      XCloseDisplay(m_display);
  }

  Display *c() { return m_display; }

  std::vector<device> get_devices() {
    int n_devices;
    XDeviceInfo *cdevs = XListInputDevices(m_display, &n_devices);
    std::vector<device> devices(n_devices);
    for (int i = 0; i < n_devices; i++) {
      devices[i] = device::from_c(cdevs[i]);
    }
    XFreeDeviceList(cdevs);
    return devices;
  }
};
} // namespace x
