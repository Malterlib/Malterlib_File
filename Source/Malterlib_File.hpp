// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

namespace NMib::NFile
{
	template <typename tf_CStr>
	bool fs_IsPathAbsolute_Local(tf_CStr const& _Path)
	{
		if (_Path.f_IsEmpty())
			return false;

		if (_Path[0] == '/')
			return true;


		mint Len = _Path.f_GetLen();

		if
			(
				NSys::NFile::fg_FileSystemHasDrives()
				&& Len > 2
				&& _Path[1] == ':'
				&& _Path[2] == '/'
			)
		{
			return true;
		}

		return false;
	}

	// Assumes _Path uses only / deliminators.
	template <typename tf_CStr>
	tf_CStr fs_ExtractPathRoot(tf_CStr &_Path)
	{
		if (_Path.f_IsEmpty())
		{
			return tf_CStr();
		}

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
		EPathRootType_None,
		EPathRootType_LocalDrive,
		EPathRootType_Network,
	};

	// Assumes delimator is only /
	template <typename tf_CStr>
	bool fs_ComparePathRoots(tf_CStr const& _RootA, tf_CStr const& _RootB)
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

		auto fl_GetType = [](tf_CStr const& _Root, tf_CStr &_ID) -> EPathRootType
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
		};

		tf_CStr IDA, IDB;

		EPathRootType TypeA = fl_GetType(_RootA, IDA);
		EPathRootType TypeB = fl_GetType(_RootB, IDB);

		if (TypeA != TypeB)
			return false;

		if (IDA.f_CmpNoCase(IDB) != 0)
			return false;

		return true;
#endif

		return false;
	}



	template <typename tf_CStr>
	bool CFile::fs_IsPathAbsolute(tf_CStr _Path)
	{
		fg_StrReplaceChar(_Path, '\\', '/');

		return fs_IsPathAbsolute_Local(_Path);
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetFullPath(const tf_CStr &_Path, const tf_CStr &_Base)
	{
		if (_Base.f_IsEmpty() || fs_IsPathAbsolute(_Path))
			return _Path;
		else
		{
			return _Base + "/" + _Path;
		}
	}

	template <typename tf_CStr>
	bool CFile::fs_HasRelativeComponents(const tf_CStr &_Path)
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
	tf_CStr CFile::fs_MakePathRelative(tf_CStr const& _AbsolutePath, tf_CStr const& _AbsoluteBase)
	{
		//auto Filename = fs_GetFile(_AbsolutePath);

		auto Path = _AbsolutePath;
		auto Base = _AbsoluteBase;

		fg_StrReplaceChar(Path, '\\', '/');
		fg_StrReplaceChar(Base, '\\', '/');

		auto PathRoot = fs_ExtractPathRoot(Path);
		auto BaseRoot = fs_ExtractPathRoot(Base);

		if (!fs_ComparePathRoots(PathRoot, BaseRoot))
			return _AbsolutePath;

		tf_CStr PathSection, BaseSection;
		tf_CStr Output;

		while(1)
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

				while(!Base.f_IsEmpty())
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
	tf_CStr CFile::fs_GetExpandedPath(const tf_CStr &_Path, const tf_CStr &_BasePath)
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
		while (Temp[iParse] == '/')
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
				mint FindLen = iFind - iParse;
				SubPath = Temp.f_Extract(iParse, FindLen);
				iParse += FindLen + 1;
			}
			if (SubPath == "..")
			{
				if (!NewPath.f_IsEmpty())
				{
					tf_CStr New = fs_GetPath(NewPath);
					mint Len = NewPath.f_GetLen();
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
				if (!NewPath.f_IsEmpty() && NewPath[NewPath.f_GetLen()-1] != '/')
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
	tf_CStr CFile::fs_GetExpandedPath(const tf_CStr &_Path, bool _bAddCurrentDir)
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
				mint FindLen = iFind - iParse;
				SubPath = Temp.f_Extract(iParse, FindLen);
				iParse += FindLen + 1;
			}
			if (SubPath == "..")
			{
				if (!NewPath.f_IsEmpty())
				{
					tf_CStr New = fs_GetPath(NewPath);
					mint Len = NewPath.f_GetLen();
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
	tf_CStr CFile::fs_GetPath(const tf_CStr &_File)
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
	tf_CStr CFile::fs_GetMalterlibPath(const tf_CStr &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		return Ret;

	}

	template <typename tf_CStr, typename tf_CToAppend>
	tf_CStr CFile::fs_AppendPath(const tf_CStr &_Path, tf_CToAppend &&_Append)
	{
		if (_Path.f_IsEmpty())
			return fg_Forward<tf_CToAppend>(_Append);
		else if (NStr::fg_StrIsEmpty(_Append))
			return _Path;
		else
		{
			if (_Path[_Path.f_GetLen()-1] == '/')
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
	tf_CStr CFile::fs_RemovePathTopLevels(const tf_CStr &_Path, mint _Levels)
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
	tf_CStr CFile::fs_GetFile(const tf_CStr &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iChar = fg_StrFindCharReverse(Ret, '/');

		if (iChar >= 0)
		{
			return Ret.f_Extract(iChar + 1);
		}
		else
			return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetExtension(const tf_CStr &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		aint iDir = fg_StrFindCharReverse(Ret, '/');
		aint iDot = fg_StrFindCharReverse(Ret, '.');

		if (iDot >= 0 && iDot > iDir)
		{
			return Ret.f_Extract(iDot + 1);
		}
		else
			return "";
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetDrive(const tf_CStr &_File)
	{
		tf_CStr Ret = _File;

		if (!Ret.f_GetLen())
			return Ret;

		fg_StrReplaceChar(Ret, '\\', '/');

		mint Len = Ret.f_GetLen();
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
	tf_CStr CFile::fs_GetFileNoExt(const tf_CStr &_File)
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
				return Ret.f_Extract(iDir+1, iDot - (iDir + 1));
			else
				return Ret.f_Left(iDot);
		}
		else
		{
			if (iDir >= 0)
				return Ret.f_Extract(iDir+1);
			else
				return Ret;
		}
	}

	namespace NPrivate
	{
		template <typename tf_CStr>
		mint fg_GetCommonPathIndex(tf_CStr const &_Path0, tf_CStr const &_Path1)
		{
			mint Path0Len = _Path0.f_GetLen();
			mint Path1Len = _Path1.f_GetLen();
			mint Len = fg_Min(Path0Len, Path1Len);

			if (Len == 0)
				return 0;

			mint MaxLen = fg_Max(Path0Len, Path1Len);

			auto fCharIsPathSeparator = [](typename tf_CStr::CChar _Char)
				{
					return _Char == '/' || _Char == '\\';
				}
			;

			mint iPath = 0;
			mint iLastFullPath = 0;
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
				return TCLimitsInt<mint>::mc_Max;

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
		mint iLastFullPath = NPrivate::fg_GetCommonPathIndex(o_Path0, o_Path1);

		if (iLastFullPath == 0)
			return {};
		else if (iLastFullPath == TCLimitsInt<mint>::mc_Max)
		{
			tf_CStr Ret = fg_Move(o_Path0);
			o_Path0.f_Clear();
			o_Path1.f_Clear();
			return Ret;
		}

		tf_CStr Ret = o_Path0.f_Left(iLastFullPath);
		o_Path0 = o_Path0.f_Extract(iLastFullPath+1);
		o_Path1 = o_Path1.f_Extract(iLastFullPath+1);
		return Ret;
	}

	template <typename tf_CStr>
	tf_CStr CFile::fs_GetCommonPath(tf_CStr const &_Path0, tf_CStr const &_Path1)
	{
		mint iLastFullPath = NPrivate::fg_GetCommonPathIndex(_Path0, _Path1);

		if (iLastFullPath == 0)
			return {};
		else if (iLastFullPath == TCLimitsInt<mint>::mc_Max)
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
		mint ThisTime = fg_Min(NTraits::TCUnsigned<tf_CLength>(_Length), gc_IdealIoSize);
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
