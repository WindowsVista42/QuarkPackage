# QuarkPackage
A simple package format with a simple API.

## Requirements
C++ 17 and a recent c++ compiler.

## Basic Usage
Include **package.hpp** and **package.cpp** into your build system.

```c++
// Include package.hpp
// Use quark namespace
// See examples/example.cpp for more details

// Create PackageInfo struct
PackageFlags flags = {};
flags.compression_mode = CompressionMode::None;
PackageInfo package_info = create_package_info(flags, 1024 * 1024); // 1 MB

// Add an entry to a package
uint64_t apple_data = 5;
add_package_entry_from_memory(&package_info, "apple", 8, &apple_data);

// Save the package to disk
if(!save_package(&package_info, "my_package.package")) {
  printf("Failed to save package!\n");
  return -1;
}

// Load the package from disk
Package package;
if(!load_package(&package, "my_package.package")) {
  printf("Failed to load package!\n");
  return -1;
}

// Read an entry from a package
uint64_t loaded_apple_data = *(uint64_t*)get_package_entry(package, "apple");

// Prints: 5
printf("%d\n", loaded_apple_data);
```
A more detailed example can be viewed in [examples/example.cpp](examples/example.cpp).

## Future Additions
-  Basic support for package-level LZ4 and Deflate compression

## Notes
- If you want to incrementally load a package it's best to break up the package into multiple packages and load each one on demand.  
- For compression modes, the best practice will be to create multiple packages based on the compression mode being used.
