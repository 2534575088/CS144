# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.27

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/mxizhou/.local/lib/python3.8/site-packages/cmake/data/bin/cmake

# The command to remove a file.
RM = /home/mxizhou/.local/lib/python3.8/site-packages/cmake/data/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mxizhou/桌面/sponge

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mxizhou/桌面/sponge/build

# Include any dependencies generated for this target.
include tests/CMakeFiles/fsm_reorder.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include tests/CMakeFiles/fsm_reorder.dir/compiler_depend.make

# Include the progress variables for this target.
include tests/CMakeFiles/fsm_reorder.dir/progress.make

# Include the compile flags for this target's objects.
include tests/CMakeFiles/fsm_reorder.dir/flags.make

tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o: tests/CMakeFiles/fsm_reorder.dir/flags.make
tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o: /home/mxizhou/桌面/sponge/tests/fsm_reorder.cc
tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o: tests/CMakeFiles/fsm_reorder.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/mxizhou/桌面/sponge/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o"
	cd /home/mxizhou/桌面/sponge/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o -MF CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o.d -o CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o -c /home/mxizhou/桌面/sponge/tests/fsm_reorder.cc

tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.i"
	cd /home/mxizhou/桌面/sponge/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mxizhou/桌面/sponge/tests/fsm_reorder.cc > CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.i

tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.s"
	cd /home/mxizhou/桌面/sponge/build/tests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mxizhou/桌面/sponge/tests/fsm_reorder.cc -o CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.s

# Object files for target fsm_reorder
fsm_reorder_OBJECTS = \
"CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o"

# External object files for target fsm_reorder
fsm_reorder_EXTERNAL_OBJECTS =

tests/fsm_reorder: tests/CMakeFiles/fsm_reorder.dir/fsm_reorder.cc.o
tests/fsm_reorder: tests/CMakeFiles/fsm_reorder.dir/build.make
tests/fsm_reorder: tests/libspongechecks.a
tests/fsm_reorder: libsponge/libsponge.a
tests/fsm_reorder: tests/CMakeFiles/fsm_reorder.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/mxizhou/桌面/sponge/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable fsm_reorder"
	cd /home/mxizhou/桌面/sponge/build/tests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/fsm_reorder.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
tests/CMakeFiles/fsm_reorder.dir/build: tests/fsm_reorder
.PHONY : tests/CMakeFiles/fsm_reorder.dir/build

tests/CMakeFiles/fsm_reorder.dir/clean:
	cd /home/mxizhou/桌面/sponge/build/tests && $(CMAKE_COMMAND) -P CMakeFiles/fsm_reorder.dir/cmake_clean.cmake
.PHONY : tests/CMakeFiles/fsm_reorder.dir/clean

tests/CMakeFiles/fsm_reorder.dir/depend:
	cd /home/mxizhou/桌面/sponge/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mxizhou/桌面/sponge /home/mxizhou/桌面/sponge/tests /home/mxizhou/桌面/sponge/build /home/mxizhou/桌面/sponge/build/tests /home/mxizhou/桌面/sponge/build/tests/CMakeFiles/fsm_reorder.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : tests/CMakeFiles/fsm_reorder.dir/depend

