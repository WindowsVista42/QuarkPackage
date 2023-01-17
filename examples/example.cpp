#include "../package.hpp"

int main() {
  using namespace quark;

  const char* package_name = "example.package";
  const char* manifest_name = "example.manifest";

  //
  // Creating package info struct
  //

  PackageFlags flags = {};
  flags.compression_mode = CompressionMode::None;

  PackageInfo package_info = create_package_info(flags, 1024 * 1024); // 1 MB

  uint64_t apple_data = 5;
  float banana_data[4] = {1.0f, 2.0f, 3.0f, 4.0f};
  
  //
  // Adding data to a package
  //

  add_package_entry_from_memory(&package_info, "apple", 8, &apple_data);
  add_package_entry_from_memory(&package_info, "banana", 4 * 4, banana_data);
  add_package_entry_from_disk(&package_info, "simple.txt");

  printf("Added entry 'apple' with data: '%llu'\n", apple_data);

  printf("Added entry 'banana' with data: '");
  printf("%.2f", banana_data[0]);
  for(uint64_t i = 1; i < 4; i += 1) {
    printf(", %.2f", banana_data[i]);
  }
  printf("'\n");

  printf("Added entry 'simple.txt' with data from file: 'simple.txt'\n");
  printf("\n");

  //
  // Saving packages and package manifests
  //

  if(!save_package(&package_info, package_name)) {
    printf("Failed to save package!\n");
    return -1;
  }

  if(!save_package_manifest(&package_info, manifest_name)) {
    printf("Failed to save package manifest!\n");
    return -1;
  }

  printf("Saved package '%s'\n", package_name);
  printf("Saved manifest '%s'\n", manifest_name);
  printf("\n");

  //
  // Loading packages and package manifests
  //

  Package package;
  if(!load_package(&package, package_name)) {
    printf("Failed to load package!\n");
    return -1;
  }

  PackageManifest manifest;
  if(!load_package_manifest(&manifest, manifest_name)) {
    printf("Failed to load package manifest!\n");
    return -1;
  }

  printf("Loaded package '%s'\n", package_name);
  printf("Loaded manifest '%s'\n", manifest_name);
  printf("\n");

  //
  // Reading package data
  //

  uint64_t loaded_apple_data = get_package_entry(package, "apple", uint64_t);
  float* loaded_banana_data  = get_package_entry_ptr(package, "banana", float*);
  char* loaded_simple_data = get_package_entry_ptr(package, "simple.txt", char*);

  printf("Loaded entry '%s' with data: '%llu'\n", "apple", loaded_apple_data);
  printf("Loaded entry '%s' with data: '", "banana");

  printf("%.2f", loaded_banana_data[0]);
  for(uint64_t i = 1; i < 4; i += 1) {
    printf(", %.2f", loaded_banana_data[i]);
  }
  printf("'\n");

  printf("Loaded entry '%s' with data: '%s'\n", "simple.txt", loaded_simple_data);
  printf("\n");

  //
  // Rebuilding filenames from package manifest
  //

  for(uint64_t i = 0; i < package.header.entries_count; i += 1) {
    printf("Entry with hash '0x%llx' has source file: '%s'\n", package.entries[i].filename_hash, manifest.filename_hash_to_filename.at(package.entries[i].filename_hash));
  }
  printf("\n");

  return 0;
}
