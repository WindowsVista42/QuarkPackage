#include "package.hpp"
#include <stdio.h>
#include <assert.h>

namespace quark {
  PackageInfo create_package_info(PackageFlags flags, uint64_t memory_block_capacity) {
    PackageInfo info = {};
    info.memory_block_capacity = memory_block_capacity;
    info.data_size_in_bytes = 0;
    info.memory_block = (uint8_t*)malloc(memory_block_capacity);

    info.entries = {};
    info.filenames = {};

    info.flags = flags;

    return info;
  }

  void destroy_package_info(PackageInfo* info) {
    free(info->memory_block);

    *info = {};
  }

  bool load_package_version_1(Package* package, FILE* f) {
    package->entries = (PackageEntry*)malloc(package->header.entries_count * sizeof(PackageEntry));
    fread(package->entries, sizeof(PackageEntry), package->header.entries_count, f);

    package->data = (uint8_t*)malloc(package->header.uncompressed_data_size_in_bytes);
    fread(package->data, 1, package->header.uncompressed_data_size_in_bytes, f);

    fclose(f);

    for(uint64_t i = 0; i < package->header.entries_count; i += 1) {
      package->filename_hash_to_data.insert(std::make_pair(package->entries[i].filename_hash, package->data + package->entries[i].data_offset_in_bytes));
      package->filename_hash_to_data_size.insert(std::make_pair(package->entries[i].filename_hash, package->entries[i].data_size_in_bytes));
    }

    return true;
  }

  bool load_package_manifest_version_1(PackageManifest* manifest, FILE* f) {
    manifest->filenames = (char*)malloc(manifest->filenames_size_in_bytes);
    fread(manifest->filenames, 1, manifest->filenames_size_in_bytes, f);

    fclose(f);

    char* filenames = manifest->filenames;

     while(filenames < (manifest->filenames + manifest->filenames_size_in_bytes)) {
      manifest->filename_hash_to_filename.insert(std::make_pair(hash_filename(filenames), filenames));

      for(; *filenames; filenames += 1) {}

      filenames += 1;
    }

    return true;
  }

  bool load_package(Package* package, const char* filename) {
    FILE* f;
    if(fopen_s(&f, filename, "rb") != 0) {
      printf("In load_package() failed to open file for reading at path '%s'\n", filename);
      goto ERR_EXIT;
    }

    {
      fseek(f, 0L, SEEK_END);
      uint64_t file_size = ftell(f);
      fseek(f, 0L, SEEK_SET);

      if(file_size < sizeof(PackageHeader)) {
        printf("In load_package() tried to load package '%s' but it was too small to be a valid!\n", filename);
        goto ERR_EXIT;
      }

      fread(&package->header, sizeof(PackageHeader), 1, f);
    }

    if(package->header.magic != PACKAGE_MAGIC) {
      printf("In load_package() tried to load package '%s' but the magic number was wrong (got '%d', expected '%d')", filename, package->header.magic, PACKAGE_MAGIC);
      goto ERR_EXIT;
    }

    if(package->header.version > PACKAGE_VERSION) {
      printf("In load_package() tried to load package '%s' but the version was too high (got '%d', we only support up to '%d')", filename, package->header.version, PACKAGE_VERSION);
      goto ERR_EXIT;
    }

    // @Todo: do some extra data validation in the header
    // (ie check that the segment sizes are validly within the size of the file)

    // Package is at least a valid file, and the passed-in package has a valid header

    if(package->header.version == 1) {
      return load_package_version_1(package, f);
    }

    printf("In load_package() we failed to match a loader to the package '%s' (file version was '%d', which we don't support!)\n", filename, package->header.version);

ERR_EXIT:
    fclose(f);
    return false;
  }

  bool save_package(PackageInfo* info, const char* filename) {
    PackageHeader header = {};
    header.magic = PACKAGE_MAGIC;
    header.version = PACKAGE_VERSION;
    header.flags = info->flags;

    header.entries_count = info->entries.size();
    header.uncompressed_data_size_in_bytes = info->data_size_in_bytes;

    FILE* f;
    if(fopen_s(&f, filename, "wb") != 0) {
      printf("In save_package() failed to open file for reading at path '%s'\n", filename);
      goto ERR_EXIT;
    }

    {
      fwrite(&header, sizeof(PackageHeader), 1, f);

      uint64_t data_offset = 0;

      for(uint64_t i = 0; i < info->entries.size(); i += 1) {
        EntryInfo* entry_info = &info->entries[i];

        PackageEntry entry = {};
        entry.filename_hash = entry_info->filename_hash;
        entry.data_size_in_bytes = entry_info->size_in_bytes;
        entry.data_offset_in_bytes = data_offset;

        fwrite(&entry, sizeof(PackageEntry), 1, f);

        data_offset += entry_info->size_in_bytes;
      }

      for(uint64_t i = 0; i < info->entries.size(); i += 1) {
        EntryInfo* entry_info = &info->entries[i];

        fwrite(entry_info->data, entry_info->size_in_bytes, 1, f);
      }
    }

    fclose(f);

    return true;

ERR_EXIT:
    fclose(f);
    return false;
  }

  bool save_package_manifest(PackageInfo* info, const char* filename) {
    PackageManifestHeader header = {};
    header.magic = MANIFEST_MAGIC;
    header.version = MANIFEST_VERSION;

    uint64_t filenames_size_in_bytes = 0;

    FILE* f;
    if(fopen_s(&f, filename, "wb") != 0) {
      printf("In save_package_manifest() failed to open file for writing at path '%s'\n", filename);
      goto ERR_EXIT;
    }

    fwrite(&header, sizeof(PackageManifestHeader), 1, f);

    {
      for(uint64_t i = 0; i < info->filenames.size(); i += 1) {
        filenames_size_in_bytes += fwrite(info->filenames[i].c_str(), 1, info->filenames[i].size(), f);

        // write terminating zero
        uint8_t zero = 0;
        filenames_size_in_bytes += fwrite(&zero, 1, 1, f);
      }
    }

    fseek(f, offsetof(PackageManifestHeader, filenames_size_in_bytes), SEEK_SET);
    fwrite(&filenames_size_in_bytes, sizeof(uint64_t), 1, f);

    fclose(f);

    return true;

ERR_EXIT:
    fclose(f);
    return false;
  }

  bool load_package_manifest(PackageManifest* manifest, const char* filename) {
    PackageManifestHeader header = {};

    FILE* f;
    if(fopen_s(&f, filename, "rb") != 0) {
      printf("In load_package_manifest() failed to open file for reading at path '%s'\n", filename);
      goto ERR_EXIT;
    }

    {
      fseek(f, 0L, SEEK_END);
      uint64_t file_size = ftell(f);
      fseek(f, 0L, SEEK_SET);

      if(file_size < sizeof(PackageManifestHeader)) {
        printf("In load_package_manifest() tried to load manifest '%s' but it was too small to be a valid!\n", filename);
        goto ERR_EXIT;
      }

      fread(&header, sizeof(PackageManifestHeader), 1, f);
    }

    if(header.magic != MANIFEST_MAGIC) {
      printf("In load_package_manifest() tried to load manifest '%s' but the magic number was wrong (got '%d', expected '%d')", filename, header.magic, MANIFEST_MAGIC);
      goto ERR_EXIT;
    }

    if(header.version > MANIFEST_VERSION) {
      printf("In load_package_manifest() tried to load manifest '%s' but the version was too high (got '%d', we only support up to '%d')", filename, header.version, MANIFEST_VERSION);
      goto ERR_EXIT;
    }

    manifest->filenames_size_in_bytes = header.filenames_size_in_bytes;

    if(header.version == 1) {
      return load_package_manifest_version_1(manifest, f);
    }

    printf("In load_package_manifest() we failed to match a loader to the manifest '%s' (file version was '%d', which we don't support!)\n", filename, header.version);

ERR_EXIT:
    fclose(f);
    return false;
  }

  void add_package_entry_from_memory(PackageInfo* info, const char* filename, uint64_t size_in_bytes, void* data) {
    EntryInfo entry = {};
    entry.size_in_bytes = size_in_bytes;
    entry.data = (uint8_t*)data;
    entry.filename_hash = hash_filename(filename);

    info->entries.push_back(entry);
    info->filenames.push_back(filename);
    info->data_size_in_bytes += size_in_bytes;
  }

  bool add_package_entry_from_disk(PackageInfo* info, const char* filename) {
    uint64_t file_size = 0;
    uint8_t* ptr = info->memory_block + info->data_size_in_bytes;

    FILE* f;
    if(fopen_s(&f, filename, "rb") != 0) {
      printf("In add_file_from_disk() failed to open file for reading at path '%s'\n", filename);
      goto ERR_EXIT;
    }

    {
      fseek(f, 0L, SEEK_END);
      file_size = ftell(f);
      fseek(f, 0L, SEEK_SET);

      if(info->data_size_in_bytes + file_size > info->memory_block_capacity) {
        printf("In add_file_from_disk() failed to write file to memory because we ran out of space\n");
        goto ERR_EXIT;
      }

      fread(ptr, 1, file_size, f);
    }

    fclose(f);

    add_package_entry_from_memory(info, filename, file_size, ptr);

    return true;

ERR_EXIT:
    fclose(f);
    return false;
  }
};
