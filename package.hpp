#include <stdint.h>
#include <vector>
#include <unordered_map>
#include <string>

// Package File Format (Version 1)
//
// This is a simple package format for bundling files together. It consists of a
// header section, an entries section, and a data section. The header includes
// file metadata, and flags. The entries section lists details about each file,
// including the data offset, size, and a hash of its filename. filenames are
// not directly stored in the package entry, but you can create a separate
// manifest file to reconstruct this information if needed. Finally, the data
// section stores the actual data for each file in a contiguous block, with the
// offsets and sizes for each file listed in its corresponding entry in the
// entries section.
//
// --------------------- Begin of File
// --------------------- Header Section
// (u32) MAGIC
// (u32) VERSION
// (u64) PACKAGE_FLAGS (Internally PackageFlags struct)
//
// (u64) ENTRY_COUNT
// (u64) UNCOMPRESSED_DATA_SIZE
//
// (u64) RESERVED
// (u64) RESERVED
// (u64) RESERVED
// (u64) RESERVED
//
// --------------------- Entries Section
// --------------------- Entry 0
// (u64) DATA_OFFSET
// (u64) DATA_SIZE
// (u64) FILENAME_HASH
//
// --------------------- Entry 1
// (u64) DATA_OFFSET
// (u64) DATA_SIZE
// (u64) FILENAME_HASH
//
// --------------------- ...
// ...
//
// --------------------- Entry N (ENTRY_COUNT - 1)
// (u64) DATA_OFFSET
// (u64) DATA_SIZE
// (u64) FILENAME_HASH
//
// --------------------- Data Section, Origin used for DATA_OFFSET
// --------------------- Entry 0
// (Entry0.DATA_SIZE bytes) DATA
//
// --------------------- Entry 1
// (Entry1.DATA_SIZE bytes) DATA
//
// --------------------- ...
// ...
//
// --------------------- Entry N (ENTRY_COUNT - 1)
// (EntryN.DATA_SIZE bytes) DATA
//
// --------------------- End of File

// Package Manifest File Format (Version 1)
//
// This is a simple manifest format used to store the filenames of entries in a
// package. It includes a header section and a filenames section. The filenames
// section is just a list of zero-terminated strings representing filenames.
// This can then be used with a loaded package to reconstruct the filenames
// associated with entries.
//
// --------------------- Begin of File
// --------------------- Header Section
// (u32) MAGIC
// (u32) VERSION
// (u64) FILENAMES_SIZE_IN_BYTES
//
// --------------------- Filenames Section
// (Zero Terminated String)
// (Zero Terminated String)
// ...
// (Zero Terminated String)
//
// --------------------- End of File

namespace quark {
  inline uint64_t hash_filename(const char* filename) {
    // FNV-1a hash
    uint64_t hash = 0xCBF29CE484222325;
    for (; *filename; filename += 1) {
      hash = (hash ^ *filename) * 0x100000001B3;
    }
    return hash;
  }

  const uint32_t PACKAGE_MAGIC = *(uint32_t*)("qpck");
  const uint32_t PACKAGE_VERSION = 1;

  const uint32_t MANIFEST_MAGIC = *(uint32_t*)("qmnf");
  const uint32_t MANIFEST_VERSION = 1;

  enum struct CompressionMode : uint8_t {
    None    = 0x0,
    Lz4     = 0x1,
    Deflate = 0x2,
  };

  struct PackageFlags {
    CompressionMode compression_mode;
    uint8_t reserved[7];
  };

  struct EntryInfo {
    uint64_t size_in_bytes;
    uint8_t* data;
    uint64_t filename_hash;
  };

  struct PackageInfo {
    uint64_t memory_block_capacity;
    uint64_t data_size_in_bytes;
    uint8_t* memory_block;

    std::vector<EntryInfo> entries;
    std::vector<std::string> filenames;

    PackageFlags flags;
  };

  struct PackageEntry {
    uint64_t data_offset_in_bytes;
    uint64_t data_size_in_bytes;
    uint64_t filename_hash;
  };

  struct PackageHeader {
    uint32_t magic;
    uint32_t version;
    PackageFlags flags;

    uint64_t entries_count;
    uint64_t uncompressed_data_size_in_bytes;

    uint64_t reserved[4];
  };

  struct Package {
    PackageHeader header;
    PackageEntry* entries;
    uint8_t* data;

    std::unordered_map<uint64_t, uint8_t*> filename_hash_to_data;
    std::unordered_map<uint64_t, uint64_t> filename_hash_to_data_size;
  };

  struct PackageManifestHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t filenames_size_in_bytes;
  };

  struct PackageManifest {
    uint64_t filenames_size_in_bytes;
    char* filenames;

    std::unordered_map<uint64_t, const char*> filename_hash_to_filename;
  };

  PackageInfo create_package_info(PackageFlags flags, uint64_t memory_block_capacity);
  void destroy_package_info(PackageInfo* info);

  bool load_package(Package* package, const char* filename);
  bool save_package(PackageInfo* info, const char* filename);

  bool save_package_manifest(PackageInfo* info, const char* filename);
  bool load_package_manifest(PackageManifest* manifest, const char* filename);

  void add_package_entry_from_memory(PackageInfo* info, const char* filename, uint64_t size_in_bytes, void* data);
  bool add_package_entry_from_disk(PackageInfo* info, const char* filename);

  #define get_package_entry(package, name, type) *(type*)package.filename_hash_to_data.at(hash_filename(name))
  #define get_package_entry_ptr(package, name, type_ptr) (type_ptr)package.filename_hash_to_data.at(hash_filename(name))
};
