# CLAUDE.md - Malterlib File Module

This file provides guidance to Claude Code (claude.ai/code) when working with the Malterlib File module.

## Module Overview

The Malterlib File module provides comprehensive cross-platform file system operations, virtual file systems, directory synchronization, file monitoring, and advanced features like RSync and patching. It abstracts platform-specific file operations while providing high-performance file I/O capabilities.

## Key Components

### Core File Operations (`Malterlib_File.h`)
Provides fundamental file system operations with cross-platform compatibility.

#### Key Classes and Structures
- **CFile** - Main file operation class with static methods for path manipulation and file operations
- **CFileProgress** - Base class for monitoring file operation progress
- **CFileHandle** - File handle information including process ownership
- **CUniqueFileIdentifier** - Platform-independent unique file identifier
- **CFileIoTempBuffer** - Temporary I/O buffer management
- **CFileIoTempBufferSecure** - Secure version of temp buffer (memory is cleared)

#### Path Operations
```cpp
// Path manipulation examples following Malterlib code standards
CStr FullPath = CFile::fs_GetFullPath(RelativePath, BasePath);
CStr ExpandedPath = CFile::fs_GetExpandedPath(Path, _bAddCurrentDir);
CStr ExpandedPath2 = CFile::fs_GetExpandedPath(Path, BasePath);
bool bIsAbsolute = CFile::fs_IsPathAbsolute(Path);
CStr RelativePath = CFile::fs_MakePathRelative(AbsolutePath, AbsoluteBase);
CStr PathOnly = CFile::fs_GetPath(FullFilePath);
CStr FileName = CFile::fs_GetFile(FullFilePath);
CStr Extension = CFile::fs_GetExtension(FullFilePath);
CStr Drive = CFile::fs_GetDrive(FullFilePath);
CStr CommonPath = CFile::fs_GetCommonPath(Path0, Path1);
```

#### File Validation
```cpp
CStr Error;
bool bValid = CFile::fs_IsValidFilePath("filename.txt", Error);
CStr NiceFilename = CFile::fs_MakeNiceFilename("com1"); // Handles reserved names
CStr UniqueFilename = CFile::fs_MakeNiceUniqueFilename("duplicate.txt");
```

### File System Enumerations (from Core)

#### EFileOpen - File opening flags
- `EFileOpen_None` - No special flags
- `EFileOpen_Read` - Open for reading
- `EFileOpen_Write` - Open for writing
- `EFileOpen_DontCreate` - Don't create if doesn't exist
- `EFileOpen_DontTruncate` - Don't truncate existing file
- `EFileOpen_DontOpenExisting` - Fail if file exists
- `EFileOpen_ShareRead` - Allow shared read access
- `EFileOpen_ShareWrite` - Allow shared write access
- `EFileOpen_WriteThrough` - Direct disk writes
- `EFileOpen_NoCache` - Disable OS caching
- `EFileOpen_NoLocalCache` - Disable local memory caching
- `EFileOpen_ShareDelete` - Allow shared delete access

#### EFileAttrib - File attributes
- `EFileAttrib_Directory` - Directory
- `EFileAttrib_File` - Regular file
- `EFileAttrib_Link` - Symbolic link
- `EFileAttrib_Hidden` - Hidden file
- `EFileAttrib_ReadOnly` - Read-only file
- `EFileAttrib_System` - System file
- `EFileAttrib_Archive` - Archive attribute
- `EFileAttrib_Executable` - Executable file
- `EFileAttrib_BackedUp` - Backed up (virtual FS only)
- `EFileAttrib_EmulatedLink` - Emulated symbolic link

#### EFileChange - File change notification types
- `EFileChange_Recursive` - Monitor recursively
- `EFileChange_FileName` - File name changes
- `EFileChange_DirectoryName` - Directory name changes
- `EFileChange_Attributes` - Attribute changes
- `EFileChange_FileSize` - Size changes
- `EFileChange_Write` - Write operations
- `EFileChange_Security` - Security changes
- `EFileChange_All` - All changes

### Virtual File System (`Malterlib_File_VirtualFS.h`)

#### ICFileSystemInterface
Abstract interface for file system operations, allowing virtual file systems and abstraction layers.

```cpp
class ICFileSystemInterface
{
public:
	// Core operations
	virtual NStorage::TCSharedPointer<NStream::CBinaryStreamDefaultRef> f_OpenStream
		(
			NStr::CStr const &_FileName
			, EFileOpen _OpenFlags
		) const = 0;

	virtual void f_DeleteFile(NStr::CStr const &_File) const = 0;
	virtual void f_DeleteDirectory(NStr::CStr const &_File) const = 0;
	virtual void f_RenameFile(NStr::CStr const &_FileFrom, NStr::CStr const &_FileTo) const = 0;
	virtual void f_CreateDirectory(NStr::CStr const &_Path) const = 0;

	// File enumeration
	virtual NContainer::TCVector<NStr::CStr> f_FindFiles
		(
			NStr::CStr const &_FindPath
			, EFileAttrib _AttribMask = EFileAttrib_Directory | EFileAttrib_File
			, bool _bRecursive = false
			, bool _bFollowLinks = true
		) const = 0;

	// File information
	virtual bool f_FileExists(NStr::CStr const &_File, EFileAttrib _AttribMask) const = 0;
	virtual EFileAttrib f_GetAttributes(NStr::CStr const &_File) const = 0;
	virtual CMibFilePos f_GetFileSize(NStr::CStr const &_Path) const = 0;

	// Time operations
	virtual void f_SetWriteTime(NStr::CStr const &_File, NTime::CTime const &_Time) = 0;
	virtual NTime::CTime f_GetWriteTime(NStr::CStr const &_File) const = 0;

	// Advanced operations
	virtual void f_CopyFile(NStr::CStr const &_FileFrom, NStr::CStr const &_FileTo) const;
	virtual void f_DeleteDirectoryRecursive(NStr::CStr const &_File, bool _bRemoveWriteProtection) const;
	virtual NCryptography::CHashDigest_MD5 f_GetFileChecksum(NStr::CStr const &_Path) const;
};
```

#### CFileSystemInterface_Disk
Concrete implementation for actual disk file system operations.

### MalterlibFS (`Malterlib_File_MalterlibFS.h`)

Custom virtual file system format supporting:
- Directory structures
- File attributes preservation
- Efficient random access

### ExeFS (`Malterlib_File_ExeFS.h`)

Embed virtual file systems within executables.

```cpp
class CExeFS
{
public:
	CVirtualFS m_FileSystem;
};

// Open ExeFS attached to current executable
bool fg_OpenExeFS(CExeFS &_FS);

// Read file from ExeFS
bool fg_ReadExeFSFile(NStr::CStr _FileName, NContainer::CByteVector &_ReadTo);
```

### Directory Synchronization (`Malterlib_File_DirectorySync.h`)

Advanced directory synchronization with delta transfer support. CDirectorySyncClient, CDirectorySyncSend, CDirectorySyncReceive;

### Directory Manifest (`Malterlib_File_DirectoryManifest.h`)

Track and compare directory contents with checksums. CDirectoryManifest;
### RSync Implementation (`Malterlib_File_RSync.h`)

RSync-like delta transfer protocol.

### Binary Patching (`Malterlib_File_Patch.h`)

Create and apply binary patches between files.

```cpp
// Create patch from original to changed
NContainer::CByteVector Patch = fg_MalterlibPatchEncode(OriginalData, ChangedData);

// Apply patch to original to get changed
NContainer::CByteVector Result = fg_MalterlibPatchDecode(OriginalData, PatchData);
```

### File Change Notifications (`Malterlib_File_ChangeNotificationActor.h`)

Monitor file system changes with coalescing support. CFileChangeNotificationActor;

### File Generators (`Malterlib_File_Generators.h`)

Async file reading with progress reporting.

```cpp
struct CReadFileGeneratorParams
{
	NStr::CStr m_Path;
	NConcurrency::TCActorFunctorWeak<NConcurrency::TCFuture<void> (NException::CExceptionPointer)> m_fOnError;
	NConcurrency::TCActorFunctorWeak<NConcurrency::TCFuture<void> (uint64, uint64, uint64)> m_fOnProgress;
	mint m_ReadAhead = 16;
	mint m_ChunkSize = NFile::gc_IdealIoSize;
	uint64 m_StartPosition = 0;
	uint64 m_FileSize = TCLimitsInt<uint64>::mc_Max;
};

// Create async generator for file reading
NConcurrency::TCAsyncGenerator<NContainer::CIOByteVector> Generator = fg_ReadFileGenerator(Params);

// Read file chunks asynchronously
for (auto iChunk = co_await Generator.f_GetIterator(); iChunk, co_await ++iChunk)
{
	// Process chunk
}
```

## Platform-Specific Considerations

### Windows
- Uses Windows API for file operations
- Supports UNC paths and network drives
- Handles reserved names (COM1, LPT1, etc.)
- Drive letters (C:, D:, etc.)
- Case-insensitive file system by default

### macOS
- Uses POSIX and macOS-specific APIs
- Supports HFS+ and APFS features
- Case-insensitive by default (configurable)
- Resource forks and extended attributes

### Linux
- Uses POSIX APIs
- Case-sensitive file system
- Various file system types (ext4, btrfs, etc.)
- Different permission model than Windows

## Common Usage Patterns

### Basic File Operations
```cpp
// Check if file exists
if (CFile::fs_FileExists("/path/to/file.txt"))
{
	// Read file
	NContainer::CByteVector Data = CFile::fs_ReadFile("/path/to/file.txt");

	// Write file
	CFile::fs_WriteFile("/path/to/output.txt", Data);

	// Copy file
	CFile::fs_CopyFile("/source/file.txt", "/dest/file.txt");

	// Delete file
	CFile::fs_DeleteFile("/path/to/delete.txt");
}

// Directory operations
CFile::fs_CreateDirectory("/new/directory/path");
CFile::fs_DeleteDirectoryRecursive("/directory/to/delete", true); // Remove write protection

// Find files
NContainer::TCVector<NStr::CStr> Files = CFile::fs_FindFiles
	(
		"/search/path/*"
		, EFileAttrib_File  // Files only
		, true              // Recursive
		, false             // Don't follow links
	)
;
```

### Working with Streams
```cpp
// Open file stream
TCBinaryStreamFile<> FileStream;
FileStream.f_Open("/path/to/file.bin", EFileOpen_Read);

// Read data
int32 Data = 0;
FileStream >> Data;

// Write data
TCBinaryStreamFile<> OutStream;
OutStream.f_Open("/path/to/output.bin", EFileOpen_Write);
int32 Data = 5;
FileStream << Data;
```

## Performance Considerations

1. **Buffer Sizes**: Use `NFile::gc_IdealIoSize` for optimal I/O buffer sizes
2. **Caching**: Control caching with `EFileOpen_NoCache` and `EFileOpen_NoLocalCache`
3. **Async Operations**: Use generators and actors for large file operations
4. **Memory Mapping**: Available through stream interfaces for large files
5. **Direct I/O**: Use `EFileOpen_WriteThrough` for critical data

## Testing

Test files are located in `Test/` directory:
- `Test_Malterlib_File.cpp` - Core file operations tests
- `Test_Malterlib_File_Patch.cpp` - Binary patching tests
- `Test_Malterlib_File_RSync.cpp` - RSync implementation tests
- `Test_Malterlib_File_VirtualFS.cpp` - Virtual file system tests

Run tests:
```bash
MalterlibBuildShowProgress=false ./mib build Tests
/opt/Deploy/Tests/RunAllTests --paths '["Malterlib/File/*"]'
```

## Important Notes

- Always use absolute paths when possible to avoid ambiguity
- Check return values and handle exceptions for file operations
- Use virtual file systems for packaging and distribution
- File change notifications may be coalesced on some platforms
- RSync implementation is optimized for network transfers
- Binary patches are most efficient for similar files
- The module follows Malterlib naming conventions strictly
