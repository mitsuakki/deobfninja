🥷 deobfninja

**deobfninja** is a powerful plugin designed to streamline and automate the deobfuscation process within [Binary Ninja](https://binary.ninja/), a popular reverse engineering platform. <br>
Its primary goal is to assist reverse engineers and security researchers in analyzing binaries that have been obfuscated to hinder static analysis.

With deobfninja, you can:

- **Automatically detect and simplify common obfuscation patterns** such as opaque predicates, control flow flattening, and junk code insertion.
- **Integrate seamlessly with Binary Ninja's API**, leveraging its robust analysis capabilities while extending them with custom deobfuscation logic.
- **Customize and extend deobfuscation recipes** to target specific obfuscators or proprietary protections.
- **Visualize deobfuscated control flow graphs** for easier understanding and navigation of previously obfuscated code.
- **Accelerate reverse engineering workflows** by reducing manual effort and improving code readability.

Whether you're analyzing malware, unpacking protected software, or studying advanced obfuscation techniques, deobfninja provides a flexible and extensible toolkit to make your job easier and more efficient.

---

## 🚀 Installation

Follow these steps to set up **deobfninja**:

1. **Clone the Repository**
```bash
git clone git@github.com:mitsuakki/bninja-recipes.git
```

2. **Set Up the Binary Ninja API**
```bash
cd binaryninjaapi
git submodule update --init --recursive
git checkout $(cat $BN_INSTALL_DIR/api_REVISION.txt | awk -F/ '{print $NF}')
```

3. **Configure an Out-of-Source Build (API)**
```bash
cmake -S . -B build
```

4. **Compile the API**
```bash
cmake --build build -j8
```

5. **Build the Recipes**
```bash
cd ..
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

6. **Install**
```bash
cmake --install build
```

---

> [!NOTE] 
> Ensure `$BN_INSTALL_DIR` points to your Binary Ninja installation directory.

---

Enjoy hacking with Binary Ninja! 🐱‍💻