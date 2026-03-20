#pragma once

#include "custom_types.h"
#include <physfs.h>
#include <vector>

#include "logger.h"

class File;
class FileSystem {
public:
	static void Init();
	static void Deinit();

	static void Mount(const str& fileOrPath, const str& mountPoint);
	static void Mount(const void* memory, size_t size, const str& newPath, const str& mountPoint);

	static void Unmount(const str& mountPoint);

	static str GetCwd();
};

enum class FileMode {
	read = 0,
	write,
	append
};

class File {
public:
	File() = default;
	File(const str& fileName);
	File(const str& fileName, FileMode mode);
	~File();

	template <typename T>
	T Read()
	{
		if (mFile == nullptr) {
			utils::LogE("File is not open: {}", mFileName);
			return T{};
		}

		if (mMode != FileMode::read) {
			utils::LogE("File is not open for reading: {}", mFileName);
			return T{};
		}

		size_t size = sizeof(T);
		std::vector<byte> bytes(size);
		if (PHYSFS_readBytes(mFile, bytes.data(), size) != size) {
			utils::LogE("Failed to read file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
			return T{};
		}

		T out;
		::memcpy(&out, bytes.data(), size);

		return out;
	}

	std::vector<byte> ReadAll();
	std::vector<str> ReadLines();
	str Scan();

	template <typename T>
	void Write(const T& data) {
		if (mFile == nullptr) {
			utils::LogE("File is not open: {}", mFileName);
			return;
		}

		if (mMode != FileMode::write && mMode != FileMode::append) {
			utils::LogE("File is not open for writing: {}", mFileName);
			return;
		}

		size_t size = sizeof(T);
		std::vector<byte> bytes(size);
		::memcpy(bytes.data(), &data, size);
		if (PHYSFS_writeBytes(mFile, bytes.data(), size) != size) {
			utils::LogE("Failed to write file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		}
	}

	void Write(const str& data) {
		if (mFile == nullptr) {
			utils::LogE("File is not open: {}", mFileName);
			return;
		}

		if (mMode != FileMode::write && mMode != FileMode::append) {
			utils::LogE("File is not open for writing: {}", mFileName);
			return;
		}

		if (PHYSFS_writeBytes(mFile, data.c_str(), data.size()) != data.size()) {
			utils::LogE("Failed to write file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		}
	}

	void Write(const byte* data, size_t size) {
		if (mFile == nullptr) {
			utils::LogE("File is not open: {}", mFileName);
			return;
		}

		if (mMode != FileMode::write && mMode != FileMode::append) {
			utils::LogE("File is not open for writing: {}", mFileName);
			return;
		}

		if (PHYSFS_writeBytes(mFile, data, size) != size) {
			utils::LogE("Failed to write file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		}
	}

	bool IsOpen() const { return mFile != nullptr; }
	bool IsDirectory() const;
	bool IsEOF() const;
	
	i64 GetSize() const;

private:
	friend class FileSystem;

	PHYSFS_File* mFile{ nullptr };
	PHYSFS_Stat mStat{};
	str mFileName{ "" };
	FileMode mMode{ FileMode::read };
};