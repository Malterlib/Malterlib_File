// Copyright © 2025 Unbroken AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>

#include <Mib/Concurrency/AsyncGenerator>
#include <Mib/Concurrency/ActorFunctorWeak>

namespace NMib::NFile
{
	struct CReadFileGeneratorParams
	{
		NStr::CStr m_Path;
		NConcurrency::TCActorFunctorWeak<NConcurrency::TCFuture<void> (NException::CExceptionPointer _pError)> m_fOnError;
		NConcurrency::TCActorFunctorWeak<NConcurrency::TCFuture<void> (uint64 _nBytesProgress, uint64 _nTransferredBytes, uint64 _nTotalBytes)> m_fOnProgress;
		umint m_ReadAhead = 16;
		umint m_ChunkSize = NFile::gc_IdealIoSize;
		uint64 m_StartPosition = 0;
		uint64 m_FileSize = TCLimitsInt<uint64>::mc_Max;
	};

	NConcurrency::TCAsyncGenerator<NContainer::CIOByteVector> fg_ReadFileGenerator(CReadFileGeneratorParams _Params);
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NFile;
#endif
