// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

namespace NMib::NSys::NFile
{
	inline_always bool fg_FileSystemHasDrives()
	{
		return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasDrives;
	}

	inline_always bool fg_FileSystemHasExecuteAttrib()
	{
		return fg_GetFileSystemFeatures() & NMib::NFile::EFileSystemFeature_HasExecuteAttrib;
	}
}

namespace NMib::NFile
{
	template <typename tf_CStr>
	bool fg_IsPathAbsolute_Local(tf_CStr const &_Path)
	{
		if (_Path.f_IsEmpty())
			return false;

		if (_Path[0] == '/')
			return true;

		umint Len = _Path.f_GetLen();

		if (NSys::NFile::fg_FileSystemHasDrives() && Len > 2 && _Path[1] == ':' && _Path[2] == '/')
			return true;

		return false;
	}

	// Assumes _Path uses only / deliminators.
	template <typename tf_CStr>
	tf_CStr fg_ExtractPathRoot(tf_CStr &_Path)
	{
		if (_Path.f_IsEmpty())
			return tf_CStr();

#ifdef DPlatformFamily_Windows
		if (NStr::fg_StrCmpNoCase(_Path, "//?/UNC/", 8) == 0)
		{
			// Long UNC style, network.
			aint iComputerEnd = NStr::fg_StrFindNoCase(8, _Path, "/");

			if (iComputerEnd != -1)
			{
				iComputerEnd += 1;
				tf_CStr Root = _Path.f_Left(iComputerEnd);
				_Path = _Path.f_Extract(iComputerEnd);
				return Root;
			}
			else
				return tf_CStr(); // Malformed.
		}
		else if (NStr::fg_StrCmpNoCase(_Path, "//?/", 4) == 0)
		{
			// Long UNC style, local.
			aint iDriveEnd = NStr::fg_StrFindNoCase(4, _Path, ":/");

			if (iDriveEnd != -1)
			{
				iDriveEnd += 2;
				tf_CStr Root = _Path.f_Left(iDriveEnd);
				_Path = _Path.f_Extract(iDriveEnd);
				return Root;
			}
			else
				return tf_CStr(); // Malformed.
		}
		else if (NStr::fg_StrCmpNoCase(_Path, "//", 2) == 0)
		{
			// Normal UNC style
			aint iComputerEnd = NStr::fg_StrFindNoCase(2, _Path, "/");

			if (iComputerEnd != -1)
			{
				iComputerEnd += 1;
				tf_CStr Root = _Path.f_Left(iComputerEnd);
				_Path = _Path.f_Extract(iComputerEnd);
				return Root;
			}
			else
				return tf_CStr(); // Malformed.
		}
#endif

		if (NSys::NFile::fg_FileSystemHasDrives())
		{
			aint iColon = _Path.f_Find(":/");

			if (iColon != -1)
			{
				iColon += 2;
				tf_CStr Drive = _Path.f_Left(iColon);
				_Path = _Path.f_Extract(iColon);
				return Drive;
			}
		}

		if (fg_Const(_Path)[0] == '/')
		{
			_Path = _Path.f_Extract(1);
			return "/";
		}

		return tf_CStr();
	}

	enum EPathRootType
	{
		EPathRootType_None
		, EPathRootType_LocalDrive
		, EPathRootType_Network
	};

	// Assumes delimator is only /
	template <typename tf_CStr>
	bool fg_ComparePathRoots(tf_CStr const &_RootA, tf_CStr const &_RootB)
	{
#ifdef DPlatformFamily_Windows
		if (_RootA.f_CmpNoCase(_RootB) == 0)
			return true;
#else
		if (_RootA.f_Cmp(_RootB) == 0)
			return true;
#endif

#ifdef DPlatformFamily_Windows
		// Windows path insanity

		auto fGetType = [](tf_CStr const &_Root, tf_CStr &_ID) -> EPathRootType
			{
				_ID.f_Clear();

				if (_Root.f_CmpNoCase("//?/UNC/", 8) == 0)
				{
					aint iComputerEnd = NStr::fg_StrFind(8, _Root, "/");
					if (iComputerEnd == -1)
						return EPathRootType_None;

					_ID = _Root.f_Extract(8, iComputerEnd - 8);
					return EPathRootType_Network;
				}
				else if (_Root.f_Cmp("//?/", 4) == 0)
				{
					aint iDriveEnd = NStr::fg_StrFind(4, _Root, ":/");
					if (iDriveEnd == -1)
						return EPathRootType_None;

					_ID = _Root.f_Extract(4, iDriveEnd - 4);
					return EPathRootType_LocalDrive;
				}
				else if (_Root.f_Cmp("//", 2) == 0)
				{
					aint iComputerEnd = NStr::fg_StrFind(2, _Root, "/");
					if (iComputerEnd == -1)
						return EPathRootType_None;

					_ID = _Root.f_Extract(2, iComputerEnd - 2);
					return EPathRootType_Network;
				}
				else
				{
					aint iDriveEnd = _Root.f_Find(":/");
					if (iDriveEnd == -1)
						return EPathRootType_None;

					_ID = _Root.f_Extract(0, iDriveEnd);
					return EPathRootType_LocalDrive;
				}
			}
		;

		tf_CStr IDA, IDB;

		EPathRootType TypeA = fGetType(_RootA, IDA);
		EPathRootType TypeB = fGetType(_RootB, IDB);

		if (TypeA != TypeB)
			return false;

		if (IDA.f_CmpNoCase(IDB) != 0)
			return false;

		return true;
#endif

		return false;
	}

	template <typename tf_CStream>
	void CFile::CFoundFile::f_Feed(tf_CStream &_Stream) const
	{
		_Stream << m_Path;
		_Stream << m_Attribs;
	}

	template <typename tf_CStream>
	void CFile::CFoundFile::f_Consume(tf_CStream &_Stream)
	{
		_Stream >> m_Path;
		_Stream >> m_Attribs;
	}

	inline void CFile::fp_CheckOpen() const
	{
		if (!mp_pFile)
			DMibErrorFile("File not open");
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramPath()
	{
		return fs_GetProgramPathNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramPath()
	{
		return fs_GetProgramPath();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramDirectory()
	{
		return fs_GetProgramDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramDirectory()
	{
		return fs_GetProgramDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetCurrentDirectory()
	{
		return fs_GetCurrentDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetCurrentDirectory()
	{
		return fs_GetCurrentDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetUserProgramDirectory()
	{
		return fs_GetUserProgramDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetUserProgramDirectory()
	{
		return fs_GetUserProgramDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetUserLocalProgramDirectory()
	{
		return fs_GetUserLocalProgramDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetUserLocalProgramDirectory()
	{
		return fs_GetUserLocalProgramDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetTemporaryDirectory()
	{
		return fs_GetTemporaryDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetRawTemporaryDirectory()
	{
		return fs_GetRawTemporaryDirectoryNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetTemporaryDirectory()
	{
		return fs_GetTemporaryDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetRawTemporaryDirectory()
	{
		return fs_GetRawTemporaryDirectory();
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetModulePath(void *_pCode)
	{
		return fs_GetModulePathNonTracked(_pCode);
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetModulePath(void *_pCode)
	{
		return fs_GetModulePath(_pCode);
	}

	template <typename tf_CStr>
	TCEnableIf<NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramRoot()
	{
		return fs_GetProgramRootNonTracked();
	}

	template <typename tf_CStr>
	TCEnableIf<!NTraits::cIsSame<typename tf_CStr::CAllocator, NMemory::CAllocator_NonTrackedHeap>, tf_CStr> CFile::fs_GetProgramRoot()
	{
		return fs_GetProgramRoot();
	}

	template <typename tf_CStr>
	bool CFile::fs_IsPathAbsolute(tf_CStr const &_Path)
	{
		tf_CStr Path = _Path;
		fg_StrReplaceChar(Path, '\\', '/');

		return fg_IsPathAbsolute_Local(Path);
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetFullPath(tf_CStr const &_Path, tf_CStr const &_Base)
	{
		if (_Base.f_IsEmpty() || fs_IsPathAbsolute(_Path))
			return _Path;
		else
			return _Base + "/" + _Path;
	}

	template <typename tf_CStr>
	bool CFile::fs_HasRelativeComponents(tf_CStr const &_Path)
	{
		auto const *pParse = _Path.f_GetStr();
		auto const *pDirStart = pParse;

		auto fCheckPath = [&]
			{
				if
				(
					((pParse - pDirStart) >= 2 && NStr::fg_StrCmp(pDirStart, "..", pParse - pDirStart) == 0)
					|| ((pParse - pDirStart) >= 1 && NStr::fg_StrCmp(pDirStart, ".", pParse - pDirStart) == 0)
				)
				{
					return true;
				}

				return false;
			}
		;

		while (*pParse)
		{
			if (*pParse == '/')
			{
				if (fCheckPath())
					return true;
				++pParse;
				pDirStart = pParse;
				continue;
			}

			++pParse;
		}

		if (fCheckPath())
			return true;

		return false;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_MakePathRelative(tf_CStr const &_AbsolutePath, tf_CStr const &_AbsoluteBase)
	{
		//auto Filename = fs_GetFile(_AbsolutePath);

		auto Path = _AbsolutePath;
		auto Base = _AbsoluteBase;

		fg_StrReplaceChar(Path, '\\', '/');
		fg_StrReplaceChar(Base, '\\', '/');

		auto PathRoot = fg_ExtractPathRoot(Path);
		auto BaseRoot = fg_ExtractPathRoot(Base);

		if (!fg_ComparePathRoots(PathRoot, BaseRoot))
			return _AbsolutePath;

		tf_CStr PathSection, BaseSection;
		tf_CStr Output;

		while (1)
		{
			PathSection = NStr::fg_GetStrSepNoTrim(Path, "/");
			BaseSection = NStr::fg_GetStrSepNoTrim(Base, "/");

			if (PathSection.f_IsEmpty() && BaseSection.f_IsEmpty())
				break;

#ifdef DPlatformFamily_Windows
			if (PathSection.f_CmpNoCase(BaseSection) != 0)
#else
			if (PathSection.f_Cmp(BaseSection) != 0)
#endif
			{
				if (!BaseSection.f_IsEmpty())
					Output += "../";

				while (!Base.f_IsEmpty())
				{
					BaseSection = NStr::fg_GetStrSepNoTrim(Base, "/");
					Output += "../";
				}
				if (!PathSection.f_IsEmpty())
					Output += PathSection + "/";
				if (!Path.f_IsEmpty())
					Output += Path + "/";
				Output += "Dummy";
				return fs_GetPath(Output);
			}
		}

		return "";
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetExpandedPath(tf_CStr const &_Path, tf_CStr const &_BasePath)
	{
		if (_Path.f_IsEmpty())
			return _Path;

		tf_CStr TempValue = _Path;
		fg_StrReplaceChar(TempValue, '\\', '/');
		tf_CStr const &Temp = TempValue;

		tf_CStr NewPath;

		aint Len = Temp.f_GetLen();
		if (Len > 0)
		{
			if (Len > 1 && Temp[0] == '/' && Temp[1] == '/')
				;
			else if (Len > 1 && Temp[1] == ':')
				;
			else
			{
				if (Temp[0] == '/')
				{
					if (NSys::NFile::fg_FileSystemHasDrives())
						NewPath = fs_GetDrive(_BasePath);
				}
				else
					NewPath = _BasePath;
			}
		}
		else
			NewPath = _BasePath;

		aint iParse = 0;
		while (iParse < Len && Temp[iParse] == '/')
		{
			NewPath.f_AddChar('/');
			++iParse;
		}

		while (iParse < Len)
		{
			aint iFind = NStr::fg_StrFindChar(iParse, Temp, '/');
			tf_CStr SubPath;
			if (iFind < 0)
			{
				SubPath = Temp.f_Extract(iParse);
				iParse = Len;
			}
			else
			{
				umint FindLen = iFind - iParse;
				SubPath = Temp.f_Extract(iParse, FindLen);
				iParse += FindLen + 1;
			}
			if (SubPath == "..")
			{
				if (!NewPath.f_IsEmpty())
				{
					tf_CStr New = fs_GetPath(NewPath);
					umint Len = NewPath.f_GetLen();
					if (Len > 0 && fg_Const(NewPath)[Len - 1] == '/')
						;
					else if (Len > 1 && fg_Const(NewPath)[1] == ':' && New.f_IsEmpty())
						;
					else if (Len > 1 && fg_Const(NewPath)[0] == '/' && fg_Const(NewPath)[1] == '/' && New.f_GetLen() <= 2)
						;
					else
						NewPath = New;
				}
			}
			else if (SubPath == ".")
			{
			}
			else
			{
				if (!NewPath.f_IsEmpty() && NewPath[NewPath.f_GetLen() - 1] != '/')
				{
					NewPath += "/";
					NewPath += SubPath;
				}
				else
					NewPath += SubPath;
			}
		}

		return NewPath;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetExpandedPath(tf_CStr const &_Path, bool _bAddCurrentDir)
	{
		if (_Path.f_IsEmpty())
			return _Path;

		tf_CStr TempValue = _Path;
		fg_StrReplaceChar(TempValue, '\\', '/');
		tf_CStr const &Temp = TempValue;

		tf_CStr NewPath;

		aint Len = Temp.f_GetLen();
		if (Len > 0)
		{
			if (Len > 1 && Temp[0] == '/' && Temp[1] == '/')
				;
			else if (Len > 1 && Temp[1] == ':')
				;
			else
			{
				if (Temp[0] == '/')
				{
					if (NSys::NFile::fg_FileSystemHasDrives())
						NewPath = fs_GetDrive(fs_GetCurrentDirectory<tf_CStr>());
				}
				else if (_bAddCurrentDir)
					NewPath = fs_GetCurrentDirectory<tf_CStr>();
			}
		}
		else
			NewPath = fs_GetCurrentDirectory<tf_CStr>();

		aint iParse = 0;
		while (iParse < Len && Temp[iParse] == '/')
		{
			NewPath.f_AddChar('/');
			++iParse;
		}

		while (iParse < Len)
		{
			aint iFind = NStr::fg_StrFindChar(iParse, Temp, '/');
			tf_CStr SubPath;
			if (iFind < 0)
			{
				SubPath = Temp.f_Extract(iParse);
				iParse = Len;
			}
			else
			{
				umint FindLen = iFind - iParse;
				SubPath = Temp.f_Extract(iParse, FindLen);
				iParse += FindLen + 1;
			}
			if (SubPath == "..")
			{
				if (!NewPath.f_IsEmpty())
				{
					tf_CStr New = fs_GetPath(NewPath);
					umint Len = NewPath.f_GetLen();
					if (Len > 0 && fg_Const(NewPath)[Len - 1] == '/')
						;
					else if (Len > 1 && fg_Const(NewPath)[1] == ':' && New.f_IsEmpty())
						;
					else if (Len > 1 && fg_Const(NewPath)[0] == '/' && fg_Const(NewPath)[1] == '/' && New.f_GetLen() <= 2)
						;
					else
						NewPath = New;
				}
			}
			else if (SubPath == ".")
			{
			}
			else
			{
				if (!NewPath.f_IsEmpty() && NewPath[NewPath.f_GetLen() - 1] != '/')
				{
					NewPath += "/";
					NewPath += SubPath;
				}
				else
					NewPath += SubPath;
			}
		}

		return NewPath;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetPath(tf_CStr const &_File)
	{
		if (_File.f_IsEmpty() || _File == "/")
			return tf_CStr();

		tf_CStr Ret = _File;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iChar = fg_StrFindCharReverse(Ret, '/');

		if (iChar == 0)
			Ret = "/";
		else if (iChar >= 0)
			Ret = Ret.f_Left(iChar);
		else
			Ret = "";

		return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetMalterlibPath(tf_CStr const &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		return Ret;
	}

	template <typename tf_CStr, typename tf_CToAppend>
	tf_CStr CFile::fs_AppendPath(tf_CStr const &_Path, tf_CToAppend &&_Append)
	{
		if (_Path.f_IsEmpty())
			return fg_Forward<tf_CToAppend>(_Append);
		else if (NStr::fg_StrIsEmpty(_Append))
			return _Path;
		else
		{
			if (_Path[_Path.f_GetLen() - 1] == '/')
			{
				tf_CStr Ret = _Path;
				Ret += fg_Forward<tf_CToAppend>(_Append);
				return Ret;
			}
			else
			{
				tf_CStr Ret = _Path;
				Ret += "/";
				Ret += fg_Forward<tf_CToAppend>(_Append);
				return Ret;
			}
		}
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_RemovePathTopLevels(tf_CStr const &_Path, umint _Levels)
	{
		tf_CStr Ret = _Path;
		while (_Levels && !Ret.f_IsEmpty())
		{
			aint iFind = Ret.f_FindChar('/');
			if (iFind >= 0)
				Ret = Ret.f_Extract(iFind + 1);
			else
				Ret.f_Clear();
			--_Levels;
		}

		return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetFile(tf_CStr const &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iChar = fg_StrFindCharReverse(Ret, '/');

		if (iChar >= 0)
			return Ret.f_Extract(iChar + 1);
		else
			return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetExtension(tf_CStr const &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iDir = fg_StrFindCharReverse(Ret, '/');
		aint iDot = fg_StrFindCharReverse(Ret, '.');

		if (iDot >= 0 && iDot > iDir)
			return Ret.f_Extract(iDot + 1);
		else
			return "";
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetDrive(tf_CStr const &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		umint Len = Ret.f_GetLen();
		if (Len > 1 && fg_Const(Ret)[1] == ':')
			return Ret.f_Left(2);

		if (Len > 1 && fg_Const(Ret)[0] == '/' && fg_Const(Ret)[1] == '/')
		{
			Ret = Ret.f_Extract(2);
			aint iDir = fg_StrFindChar(Ret, '/');
			if (iDir >= 0)
				return "//" + Ret.f_Left(iDir);
			else
				return "//" + Ret;
		}

		return "";
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetFileNoExt(tf_CStr const &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iDir = fg_StrFindCharReverse(Ret, '/');
		aint iDot = fg_StrFindCharReverse(Ret, '.');

		bool bHasDot = iDot >= 0 && (iDot > iDir || iDir < 0);

		if (bHasDot)
		{
			if (iDir >= 0)
				return Ret.f_Extract(iDir + 1, iDot - (iDir + 1));
			else
				return Ret.f_Left(iDot);
		}
		else
		{
			if (iDir >= 0)
				return Ret.f_Extract(iDir + 1);
			else
				return Ret;
		}
	}

	namespace NPrivate
	{
		template <typename tf_CStr>
		umint fg_GetCommonPathIndex(tf_CStr const &_Path0, tf_CStr const &_Path1)
		{
			umint Path0Len = _Path0.f_GetLen();
			umint Path1Len = _Path1.f_GetLen();
			umint Len = fg_Min(Path0Len, Path1Len);

			if (Len == 0)
				return 0;

			umint MaxLen = fg_Max(Path0Len, Path1Len);

			auto fCharIsPathSeparator = [](typename tf_CStr::CChar _Char)
				{
					return _Char == '/' || _Char == '\\';
				}
			;

			umint iPath = 0;
			umint iLastFullPath = 0;
			auto *pPath0 = _Path0.f_GetStr();
			auto *pPath1 = _Path1.f_GetStr();

			while (iPath < Len)
			{
				auto Char0 = pPath0[iPath];
				auto Char1 = pPath1[iPath];

				if (fCharIsPathSeparator(Char0) && fCharIsPathSeparator(Char1))
					iLastFullPath = iPath;
				else if (Char0 != Char1)
					break;

				++iPath;
			}

			if (iPath == MaxLen)
				return TCLimitsInt<umint>::mc_Max;

			if (iPath == Path0Len && Path1Len >= iPath && fCharIsPathSeparator(pPath1[iPath]))
				iLastFullPath = iPath;
			else if (iPath == Path1Len && Path0Len >= iPath && fCharIsPathSeparator(pPath0[iPath]))
				iLastFullPath = iPath;

			return iLastFullPath;
		}
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetCommonPathAndMakeRelative(tf_CStr &o_Path0, tf_CStr &o_Path1)
	{
		umint iLastFullPath = NPrivate::fg_GetCommonPathIndex(o_Path0, o_Path1);

		if (iLastFullPath == 0)
			return {};
		else if (iLastFullPath == TCLimitsInt<umint>::mc_Max)
		{
			tf_CStr Ret = fg_Move(o_Path0);
			o_Path0.f_Clear();
			o_Path1.f_Clear();
			return Ret;
		}

		tf_CStr Ret = o_Path0.f_Left(iLastFullPath);
		o_Path0 = o_Path0.f_Extract(iLastFullPath + 1);
		o_Path1 = o_Path1.f_Extract(iLastFullPath + 1);
		return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetCommonPath(tf_CStr const &_Path0, tf_CStr const &_Path1)
	{
		umint iLastFullPath = NPrivate::fg_GetCommonPathIndex(_Path0, _Path1);

		if (iLastFullPath == 0)
			return {};
		else if (iLastFullPath == TCLimitsInt<umint>::mc_Max)
			return _Path0;
		else if (_Path0.f_GetLen() == iLastFullPath)
			return _Path0;
		else if (_Path1.f_GetLen() == iLastFullPath)
			return _Path1;

		tf_CStr Ret = _Path0.f_Left(iLastFullPath);
		return Ret;
	}

	template <typename tf_CLength>
	inline_small CFileIoTempBuffer::CUseBufferResult CFileIoTempBuffer::f_UseBuffer(tf_CLength _Length)
	{
		umint ThisTime = fg_Min(NTraits::TCUnsigned<tf_CLength>(_Length), gc_IdealIoSize);
		mp_nUsedBytes = fg_Max(mp_nUsedBytes, ThisTime);
		if (ThisTime)
		{
			if (mp_Data.f_IsEmpty())
				fp_CreateBuffer();

			return
				{
					.m_pBuffer = mp_Data.f_GetArray()
					, .m_nBytes = ThisTime
				}
			;
		}

		return
			{
				.m_pBuffer = nullptr
				, .m_nBytes = ThisTime
			}
		;
	}

	template <typename t_CStreamType>
	TCBinaryStreamFile<t_CStreamType>::TCBinaryStreamFile()
	{
	}

	template <typename t_CStreamType>
	template <typename tf_CType, typename tf_CStr>
	tf_CType TCBinaryStreamFile<t_CStreamType>::fs_ReadFile(tf_CStr const &_FileName, EFileOpen _OpenFlags)
	{
		TCBinaryStreamFile Stream;
		Stream.f_Open(_FileName, _OpenFlags);
		tf_CType Data;
		Stream >> Data;
		return Data;
	}

	template <typename t_CStreamType>
	template <typename tf_CType, typename tf_CStr>
	void TCBinaryStreamFile<t_CStreamType>::fs_WriteFile(tf_CType const &_Data, tf_CStr const &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes)
	{
		TCBinaryStreamFile Stream;
		Stream.f_Open(_FileName, _OpenFlags, _Attributes);
		Stream << _Data;
	}

	template <typename t_CStreamType>
	template <typename tf_CStr>
	void TCBinaryStreamFile<t_CStreamType>::f_Open(tf_CStr const &_FileName, EFileOpen _OpenFlags, EFileAttrib _Attributes)
	{
		m_File.f_Open(_FileName, _OpenFlags, _Attributes);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_Close()
	{
		m_File.f_Close();
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_Reset()
	{
		m_File.f_Flush(false);
		m_File.f_SetLength(0);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_FeedBytes(void const *_pMem, umint _nBytes)
	{
		m_File.f_Write(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_ConsumeBytes(void *_pMem, umint _nBytes)
	{
		m_File.f_Read(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFile<t_CStreamType>::f_IsValid() const
	{
		return m_File.f_IsValid();
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFile<t_CStreamType>::f_IsAtEndOfStream() const
	{
		return m_File.f_IsAtEndOfFile();
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStreamFile<t_CStreamType>::f_GetPosition() const
	{
		return m_File.f_GetPosition();
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_SetPosition(NStream::CFilePos _Pos)
	{
		m_File.f_SetPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		m_File.f_SetPositionFromEnd(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_AddPosition(NStream::CFilePos _Pos)
	{
		m_File.f_AddPosition(_Pos);
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFile<t_CStreamType>::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		return m_File.f_IsValidReadPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_Flush(bool _bLocalCacheOnly)
	{
		return m_File.f_Flush(_bLocalCacheOnly);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_SetCacheSize(umint _CacheSize)
	{
		m_File.f_SetCacheSize(_CacheSize);
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStreamFile<t_CStreamType>::f_GetLength() const
	{
		return m_File.f_GetLength();
	}

	template <typename t_CStreamType>
	umint TCBinaryStreamFile<t_CStreamType>::f_ContainerLengthLimit() const
	{
		return NStream::fg_CapLengthLimit(f_GetLength() - f_GetPosition());
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFile<t_CStreamType>::f_SetLength(NStream::CFilePos _Length)
	{
		return m_File.f_SetLength(_Length);
	}

	template <typename t_CStreamType>
	TCBinaryStreamFilePtr<t_CStreamType>::TCBinaryStreamFilePtr()
	{
		mp_pFile = nullptr;
	}

	template <typename t_CStreamType>
	TCBinaryStreamFilePtr<t_CStreamType>::TCBinaryStreamFilePtr(CFile *_pFile)
	{
		mp_pFile = _pFile;
	}

	template <typename t_CStreamType>
	TCBinaryStreamFilePtr<t_CStreamType>::TCBinaryStreamFilePtr(CFile &_File)
	{
		mp_pFile = &_File;
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_Open(CFile *_pFile)
	{
		mp_pFile = _pFile;
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_Open(CFile &_File)
	{
		mp_pFile = &_File;
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_Close()
	{
		mp_pFile = nullptr;
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_Reset()
	{
		mp_pFile->f_SetLength(0);
		mp_pFile->f_Flush(false);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_FeedBytes(void const *_pMem, umint _nBytes)
	{
		mp_pFile->f_Write(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_ConsumeBytes(void *_pMem, umint _nBytes)
	{
		mp_pFile->f_Read(_pMem, _nBytes);
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFilePtr<t_CStreamType>::f_IsValid() const
	{
		return mp_pFile->f_IsValid();
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFilePtr<t_CStreamType>::f_IsAtEndOfStream() const
	{
		return mp_pFile->f_IsAtEndOfFile();
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStreamFilePtr<t_CStreamType>::f_GetPosition() const
	{
		return mp_pFile->f_GetPosition();
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_SetPosition(NStream::CFilePos _Pos)
	{
		mp_pFile->f_SetPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_SetPositionFromEnd(NStream::CFilePos _Pos)
	{
		mp_pFile->f_SetPositionFromEnd(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_AddPosition(NStream::CFilePos _Pos)
	{
		mp_pFile->f_AddPosition(_Pos);
	}

	template <typename t_CStreamType>
	bool TCBinaryStreamFilePtr<t_CStreamType>::f_IsValidReadPosition(NStream::CFilePos _Pos) const
	{
		return mp_pFile->f_IsValidReadPosition(_Pos);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_Flush(bool _bLocalCacheOnly)
	{
		mp_pFile->f_Flush(_bLocalCacheOnly);
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_SetCacheSize(umint _CacheSize)
	{
		mp_pFile->f_SetCacheSize(_CacheSize);
	}

	template <typename t_CStreamType>
	NStream::CFilePos TCBinaryStreamFilePtr<t_CStreamType>::f_GetLength() const
	{
		return mp_pFile->f_GetLength();
	}

	template <typename t_CStreamType>
	umint TCBinaryStreamFilePtr<t_CStreamType>::f_ContainerLengthLimit() const
	{
		return NStream::fg_CapLengthLimit(mp_pFile->f_GetLength() - mp_pFile->f_GetPosition());
	}

	template <typename t_CStreamType>
	void TCBinaryStreamFilePtr<t_CStreamType>::f_SetLength(NStream::CFilePos _Length)
	{
		return mp_pFile->f_SetLength(_Length);
	}
}

namespace NMib::NStr
{
	template <typename tf_CTCStrTraitsLeft, typename tf_CRight>
	TCStr<tf_CTCStrTraitsLeft> operator / (TCStr<tf_CTCStrTraitsLeft> const &_StrLeft, tf_CRight &&_StrRight)
	{
		return NFile::CFile::fs_AppendPath(_StrLeft, fg_Forward<tf_CRight>(_StrRight));
	}

	template <typename tf_CTCStrTraitsLeft, typename tf_CRight>
	TCStr<tf_CTCStrTraitsLeft> operator / (TCStr<tf_CTCStrTraitsLeft> const &_StrLeft, typename TCStr<tf_CTCStrTraitsLeft>::CFormat &&_StrRight)
	{
		return NFile::CFile::fs_AppendPath(_StrLeft, TCStr<tf_CTCStrTraitsLeft>(_StrRight));
	}

	template <typename tf_CTCStrTraitsLeft, typename tf_CRight>
	TCStr<tf_CTCStrTraitsLeft> &operator /= (TCStr<tf_CTCStrTraitsLeft> &_StrLeft, tf_CRight &&_StrRight)
	{
		_StrLeft = NFile::CFile::fs_AppendPath(_StrLeft, fg_Forward<tf_CRight>(_StrRight));
		return _StrLeft;
	}
}
