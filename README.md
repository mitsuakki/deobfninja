# bninja-recipes

## Installation

```bash
# Get the source
git clone git@github.com:mitsuakki/bninja-recipes.git

# Setup API
cd binaryninjaapi
git submodule update --init --recursive
git checkout $(cat $BN_INSTALL_DIR/api_REVISION.txt | awk -F/ '{print $NF}') # to match the version of the binary ninja installation

# Configure an out-of-source build setup (API)
cmake -S . -B build

# Compile API
cmake --build build -j8

# Build recipes
cd ..
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# Install
cmake --install build
```