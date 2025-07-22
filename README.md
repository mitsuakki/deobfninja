# deobfninja

**deobfninja** is a powerful plugin designed to streamline and automate the deobfuscation process within [Binary Ninja](https://binary.ninja/), a popular reverse engineering platform. Its primary goal is to assist reverse engineers and security researchers in analyzing binaries that have been obfuscated to hinder static analysis.

With deobfninja, you can:
- **Automatically detect and simplify common obfuscation patterns** such as opaque predicates, control flow flattening, and junk code.
- **Customize and extend deobfuscation recipes** to target specific obfuscators or proprietary protections.
- **Accelerate reverse engineering workflows** by reducing manual effort and improving code readability.

Whether you're analyzing malware, unpacking protected software, or studying advanced obfuscation techniques, deobfninja provides a flexible and extensible toolkit to make your job easier and more efficient.

---

## Overview

<img width="10405" height="4129" alt="image" src="https://github.com/user-attachments/assets/7dc72bf2-26a1-4ec7-8f00-978063ccad22" />

---

## Installation

### Clone the Repository
```bash
git clone git@github.com:mitsuakki/bninja-recipes.git
```

### Set Up the Binary Ninja API
```bash
cd binaryninjaapi
git submodule update --init --recursive
git checkout $(cat $BN_INSTALL_DIR/api_REVISION.txt | awk -F/ '{print $NF}')
```

### Configure an Out-of-Source Build (API)
```bash
cmake -S . -B build
```

### Compile the API
```bash
cmake --build build -j8
```

### Build the Recipes
```bash
cd ..
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### Install
```bash
cmake --install build
```

---

> [!NOTE] 
> Ensure `$BN_INSTALL_DIR` points to your Binary Ninja installation directory.

---
