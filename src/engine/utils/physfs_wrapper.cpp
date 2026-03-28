#include "physfs_wrapper.h"

#include "string_utils.h"

File::File(const str& fileName)
	: File(fileName, FileMode::read)
{}

File::File(const str& fileName, FileMode mode)
{
	mFileName = fileName;
	mMode = mode;

	switch (mode) {
		case FileMode::read:
			mFile = PHYSFS_openRead(fileName.c_str());
			break;
		case FileMode::write:
			mFile = PHYSFS_openWrite(fileName.c_str());
			break;
		case FileMode::append:
			mFile = PHYSFS_openAppend(fileName.c_str());
			break;
	}

	if (mFile) {
		if (!PHYSFS_stat(fileName.c_str(), &mStat)) {
			utils::LogE("Failed to get file stat: {}\n{}", fileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		}
	}
	else {
		utils::LogE("Failed to open file: {}\n{}", fileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
	}
}

File::~File()
{
	if (mFile) {
		if (!PHYSFS_close(mFile)) {
			utils::LogE("Failed to close file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		}
		mFile = nullptr;
	}
}

std::vector<byte> File::ReadAll()
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return std::vector<byte>();
	}

	if (mMode != FileMode::read) {
		utils::LogE("File is not open for reading: {}", mFileName);
		return std::vector<byte>();
	}

	std::vector<byte> buffer(mStat.filesize);
	if (PHYSFS_readBytes(mFile, buffer.data(), mStat.filesize) != mStat.filesize) {
		utils::LogE("Failed to read file: {}\n{}", mFileName, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return std::vector<byte>();
	}
	return buffer;
}

std::vector<str> File::ReadLines()
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return std::vector<str>();
	}

	if (mMode != FileMode::read) {
		utils::LogE("File is not open for reading: {}", mFileName);
		return std::vector<str>();
	}

	auto allData = ReadAll();
	if (allData.empty()) {
		return std::vector<str>();
	}

	str allText(allData.begin(), allData.end());
	return utils::SplitLines(allText);
}

str File::Scan()
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return str();
	}

	if (mMode != FileMode::read) {
		utils::LogE("File is not open for reading: {}", mFileName);
		return "";
	}

	std::vector<char> buffer(1024);
	char current = '?';
	while (!PHYSFS_eof(mFile) && current != ' ') {
		PHYSFS_readBytes(mFile, &current, 1);
		buffer.push_back(current);
	}
	PHYSFS_readBytes(mFile, &current, 1);
	buffer.push_back('\0');
	return str(buffer.data());
}

bool File::IsDirectory() const
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return false;
	}
	return mStat.filetype == PHYSFS_FILETYPE_DIRECTORY;
}

bool File::IsEOF() const
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return true;
	}
	return PHYSFS_eof(mFile);
}

i64 File::GetSize() const
{
	if (mFile == nullptr) {
		utils::LogE("File is not open: {}", mFileName);
		return 0;
	}
	return mStat.filesize;
}

void FileSystem::Init()
{
	if (PHYSFS_isInit()) {
		utils::LogW("PHYSFS is already initialized");
		return;
	}
	PHYSFS_init(nullptr);
	utils::LogI("PHYSFS initialized");
}

void FileSystem::Deinit()
{
	if (!PHYSFS_isInit()) {
		utils::LogW("PHYSFS is not initialized");
		return;
	}
	PHYSFS_deinit();
	utils::LogI("PHYSFS deinitialized");
}

PHYSFS_EnumerateCallbackResult DirCallback(void* data, const char* origdir, const char* fname) 
{
	utils::LogD("\tMounted file: {}{}", origdir, fname);
	return PHYSFS_ENUM_OK;
}

void FileSystem::Mount(const str& fileOrPath, const str& mountPoint)
{
	if (!PHYSFS_isInit()) {
		utils::LogE("PHYSFS is not initialized");
		return;
	}

	if (!PHYSFS_mount(fileOrPath.c_str(), mountPoint.c_str(), 1)) {
		utils::LogE("Failed to mount file/path: {}\n{}", fileOrPath, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return;
	}
	utils::LogI("Mounted file/path: {} => {}", fileOrPath, mountPoint);

	// print all mounted paths
#ifdef IS_DEBUG
	PHYSFS_enumerate(
		mountPoint.c_str(),
		DirCallback,
		nullptr
	);
#endif
}

void FileSystem::Mount(const void* memory, size_t size, const str& newPath, const str& mountPoint)
{
	if (!PHYSFS_isInit()) {
		utils::LogE("PHYSFS is not initialized");
		return;
	}
	if (!PHYSFS_mountMemory(memory, size, nullptr, newPath.c_str(), mountPoint.c_str(), 1)) {
		utils::LogE("Failed to mount file/path: {}\n{}", newPath, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return;
	}
	utils::LogI("Mounted memory: {} => {}", newPath, mountPoint);
}

void FileSystem::Unmount(const str& mountPoint)
{
	if (!PHYSFS_isInit()) {
		utils::LogE("PHYSFS is not initialized");
		return;
	}
	if (!PHYSFS_unmount(mountPoint.c_str())) {
		utils::LogE("Failed to unmount: {}\n{}", mountPoint, PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode()));
		return;
	}
	utils::LogI("Unmounted: {}", mountPoint);
}

str FileSystem::GetCwd() {
	return str(PHYSFS_getBaseDir());
}
