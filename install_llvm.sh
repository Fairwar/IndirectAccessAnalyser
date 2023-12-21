git submodule init
git submodule update --progress
cd submodules/src/llvm-project
git checkout llvmorg-10.0.0
mkdir build
cd build
cmake -G "Unix Makefiles" -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(pwd)/../../../llvm-project ../llvm
make
make install