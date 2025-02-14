# How to build the plEngine custom version of clang-tidy

All commands given should be executed in a powershell.

 * get the llvm source: `git clone https://github.com/llvm/llvm-project`
 * `cd llvm-project`
 * checkout the latest release version (for the current build llvm-16.0.6 is used): `git checkout llvmorg-16.0.6`
 * Apply the llvm.patch: `git apply llvm.patch`
 * Copy the pl folder to `llvm-project/clang-tools-extra/clang-tidy/pl`
 * create a build folder `mkdir build`
 * `cd build`
 * Run cmake: `cmake -B . -S ..\llvm -DLLVM_ENABLE_PROJECTS="clang-tools-extra;clang" -DLLVM_TARGETS_TO_BUILD=X86`
 * Open the generated solution and build the clang-tidy executable (Clang executables folder) in Release.
 * Copy the resulting executable from `build/Release/bin/clang-tidy.exe` to `plEngine/Data/Tools/Precompiled/clang-tidy/clang-tidy.exe` when done

## Known Pitfalls
 
 * When running clang-tidy.exe out of the directory it was build into, it will pick up a different set of header files and compile errors might appear. So copy the exectable out of the build directory before attempting to run it on the plEngine source code. For local minimal test cases the clang-tidy.exe can remain in the build output directory.