// Copyright © 2025 Unbroken AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include "Malterlib_File_Generators.h"

#include <Mib/Concurrency/AsyncDestroy>

namespace NMib::NFile
{
	NConcurrency::TCAsyncGenerator<NContainer::CIOByteVector> fg_ReadFileGenerator(CReadFileGeneratorParams _Params)
	{
		auto CaptureScope = co_await NConcurrency::g_CaptureExceptions;

		auto fReturnError = [&](NException::CExceptionPointer &&_pException) -> NException::CExceptionPointer
			{
				if (_Params.m_fOnError)
					_Params.m_fOnError(_pException).f_DiscardResult();

				return fg_Move(_pException);
			}
		;

		NStorage::TCSharedPointer<NFile::CFile> pFile;
		uint64 FileSize = 0;
		{
			auto BlockingActorCheckout = NConcurrency::fg_BlockingActor();
			auto FileResult = co_await
				(
					NConcurrency::g_Dispatch(BlockingActorCheckout) / [Path = _Params.m_Path, ParamFileSize = _Params.m_FileSize]() mutable
					{
						NStorage::TCSharedPointer<NFile::CFile> pFile = fg_Construct();
						pFile->f_Open(Path, NFile::EFileOpen_Read | NFile::EFileOpen_ShareAll | NFile::EFileOpen_NoLocalCache);
						uint64 FileSize = ParamFileSize != TCLimitsInt<uint64>::mc_Max ? ParamFileSize : pFile->f_GetLength();
						return fg_Tuple(fg_Move(pFile), FileSize);
					}
				)
				.f_Wrap()
			;

			if (!FileResult)
				co_return fReturnError(FileResult.f_GetException());

			auto [pFileResult, FileSizeResult] = fg_Move(*FileResult);

			pFile = fg_Move(pFileResult);
			FileSize = FileSizeResult;
		}

		auto DestroyFile = co_await NConcurrency::fg_AsyncDestroyGeneric
			(
				[&]() -> NConcurrency::TCFuture<void>
				{
					auto pLocalFile = fg_Move(pFile);

					auto BlockingActorCheckout = NConcurrency::fg_BlockingActor();
					co_await
						(
							NConcurrency::g_Dispatch(BlockingActorCheckout) / [pLocalFile = fg_Move(pLocalFile)]() mutable
							{
								pLocalFile->f_Close();
							}
						)
					;

					co_return {};
				}
			)
		;

		mint ReadAhead = _Params.m_ReadAhead;
		NContainer::TCLinkedList<NConcurrency::TCFuture<NContainer::CIOByteVector>> ReadAheadFutures;

		NConcurrency::CRoundRobinBlockingActors BlockingActors(4);

		auto ChunkSize = _Params.m_ChunkSize;

		uint64 Position = _Params.m_StartPosition;

		while (Position < FileSize)
		{
			mint ThisTime = fg_Min(FileSize - Position, ChunkSize);

			ReadAheadFutures.f_Insert
				(
					NConcurrency::g_Dispatch(*BlockingActors) / [pFile, ThisTime, Position]() mutable
					{
						NContainer::CIOByteVector Buffer;
						Buffer.f_SetLen(ThisTime);

						pFile->f_ReadNoLocalCache(Position, Buffer.f_GetArray(), ThisTime);

						return Buffer;
					}
				)
			;

			if (ReadAhead == 0)
			{
				auto ReadResult = co_await ReadAheadFutures.f_PopFirst().f_Wrap();
				if (!ReadResult)
					co_return fReturnError(ReadResult.f_GetException());

				co_yield fg_Move(*ReadResult);
			}
			else
				--ReadAhead;

			Position += ThisTime;

			if (_Params.m_fOnProgress)
				_Params.m_fOnProgress(ThisTime, Position - _Params.m_StartPosition, FileSize - _Params.m_StartPosition).f_DiscardResult();
		}

		while (!ReadAheadFutures.f_IsEmpty())
		{
			auto ReadResult = co_await ReadAheadFutures.f_PopFirst().f_Wrap();
			if (!ReadResult)
				co_return fReturnError(ReadResult.f_GetException());

			co_yield fg_Move(*ReadResult);
		}

		co_return {};
	}
}

