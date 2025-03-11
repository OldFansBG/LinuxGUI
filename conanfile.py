from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class LinuxISOProConan(ConanFile):
    name = "LinuxISOPro"
    version = "1.0"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"
    exports_sources = "CMakeLists.txt", "src/*", "resources/*", "scripts/*", "themes.json", "config.yaml"

    def requirements(self):
        self.requires("libcurl/7.84.0")
        self.requires("wxwidgets/3.2.6")
        self.requires("libarchive/3.7.2")
        self.requires("yaml-cpp/0.7.0")
        self.requires("rapidjson/1.1.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["LinuxISOPro"]

    def configure(self):
        if self.settings.compiler.cppstd:
            self.settings.compiler.cppstd = "17"