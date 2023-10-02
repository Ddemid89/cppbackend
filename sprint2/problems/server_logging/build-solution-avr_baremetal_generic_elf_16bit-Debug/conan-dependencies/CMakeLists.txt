
        cmake_minimum_required(VERSION 3.15)
        project(conan-setup)
        include("/usr/share/qtcreator/package-manager/conan.cmake")
        conan_cmake_run(
          CONANFILE "/home/aleksey/cppbackend/sprint2/problems/server_logging/solution/conanfile.txt"
          INSTALL_FOLDER "/home/aleksey/cppbackend/sprint2/problems/server_logging/build-solution-avr_baremetal_generic_elf_16bit-Debug/conan-dependencies"
          GENERATORS cmake_paths json
          BUILD missing
          ENV CONAN_CMAKE_TOOLCHAIN_FILE="/home/aleksey/cppbackend/sprint2/problems/server_logging/build-solution-avr_baremetal_generic_elf_16bit-Debug/conan-dependencies/toolchain.cmake"
        )