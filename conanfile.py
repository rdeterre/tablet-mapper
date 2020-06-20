from conans import CMake, ConanFile


class TabletMapperConan(ConanFile):
    name = "tabletmapper"
    version = "0.1"
    description = "Maps tablet inputs to a single monitor on Linux"

    requires = "fmt/6.2.1"
    generators = "cmake"

    options = {"tests": [True, False]}

    default_options = {"tests": False}

    def configure(self):
        if self.options.tests:
            self.requires("catch2/2.12.2")

    def build(self):
        cmake = CMake(self)
        if self.options.tests:
            cmake.definitions["TESTS"] = "ON"
        cmake.configure()
        cmake.build()
        if self.options.tests:
            cmake.test(output_on_failure=True)
