// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_DirectorySync.h"

namespace NMib::NFile
{
	CDirectorySyncClient::CDirectorySyncClient()
	{
		DMibPublishActorFunction(CDirectorySyncClient::f_StartManifestRSync);
		DMibPublishActorFunction(CDirectorySyncClient::f_StartRSync);
		DMibPublishActorFunction(CDirectorySyncClient::f_Finished);
	}
	
	fp64 CDirectorySyncStats::f_IncomingBytesPerSecond() const
	{
		if (m_nSeconds == 0.0)
			return 0.0;

		return fp64(m_IncomingBytes) / m_nSeconds;
	}
	
	fp64 CDirectorySyncStats::f_OutgoingBytesPerSecond() const
	{
		if (m_nSeconds == 0.0)
			return 0.0;

		return fp64(m_OutgoingBytes) / m_nSeconds;
	}

	auto CDirectorySyncFileOptions::f_OpenFile(NStr::CStr const &_FileName, EDirectorySyncStreamType _FileType, EFileOpen _OpenFlags, EFileAttrib _Attributes)
		-> NStorage::TCUniquePointer<NStream::CBinaryStream>
	{
		if (m_fOpenStream)
			return m_fOpenStream(_FileName, _FileType, _OpenFlags, _Attributes);

		NStorage::TCUniquePointer<TCBinaryStreamFile<>> pStream = fg_Construct();
		pStream->f_Open(_FileName, _OpenFlags, _Attributes);

		return pStream;
	}

	NStr::CStr CDirectorySyncFileOptions::f_TransformFileName(NStr::CStr const &_BasePath, NStr::CStr const &_FileName, EDirectorySyncStreamType _FileType)
	{
		if (m_fTransformFilePath)
			return m_fTransformFilePath(_BasePath, _FileName, _FileType);

		return CFile::fs_AppendPath(_BasePath, _FileName);
	}

	CDirectorySyncSend::CConfig::CConfig() = default;

	CDirectorySyncSend::CConfig::CConfig(NStr::CStr const &_FileName)
		: m_BasePath(CFile::fs_GetPath(_FileName))
	{
		CDirectoryManifestConfig ManifestConfig;
		ManifestConfig.m_Root = CFile::fs_GetPath(_FileName);
		ManifestConfig.m_IncludeWildcards = {CFile::fs_GetFile(_FileName)};
		m_Manifest = fg_Move(ManifestConfig);
	}

	CDirectorySyncReceive::CConfig::CConfig() = default;

	CDirectorySyncReceive::CConfig::CConfig(NStr::CStr const &_Destination, EEasyConfigFlag _Flags)
	{
		DMibRequire(CFile::fs_IsPathAbsolute(_Destination));

		using namespace NStr;

		if (_Flags & EEasyConfigFlag_DestinationIsDirectory)
			m_PreviousBasePath = m_BasePath = _Destination;
		else
			m_PreviousBasePath = m_BasePath = CFile::fs_GetPath(_Destination);

		m_FileOptions.m_fTransformFilePath = [_Destination, _Flags](NStr::CStr const &_BasePath, NStr::CStr const &_Filename, EDirectorySyncStreamType _FileType) -> NStr::CStr
			{
				if (!(_FileType & (EDirectorySyncStreamType_Temp | EDirectorySyncStreamType_Manifest)) && !(_Flags & EEasyConfigFlag_DestinationIsDirectory))
				{
					if (!(_Flags & EEasyConfigFlag_AllowOverwrite))
					{
						if (CFile::fs_FileExists(_Destination))
							DMibError("File '{}' already exists"_f << _Destination);
					}

					return _Destination;
				}
				return CFile::fs_AppendPath(_BasePath, _Filename);
			}
		;


		m_FileOptions.m_fOpenStream = [_Destination, _Flags](CStr const &_FileName, EDirectorySyncStreamType _FileType, EFileOpen _OpenFlags, EFileAttrib _Attributes)
			-> NStorage::TCUniquePointer<NStream::CBinaryStream>
			{
				if (!(_FileType & (EDirectorySyncStreamType_Temp | EDirectorySyncStreamType_Manifest)))
				{
					if (!(_Flags & EEasyConfigFlag_AllowOverwrite) && (_OpenFlags & EFileOpen_Write))
					{
						if (CFile::fs_FileExists(_FileName))
							DMibError("File '{}' already exists"_f << _FileName);
					}
				}

				NStorage::TCUniquePointer<TCBinaryStreamFile<>> pFile = fg_Construct();
				pFile->f_Open(_FileName, _OpenFlags, _Attributes);
				return pFile;
			}
		;
	}
}

