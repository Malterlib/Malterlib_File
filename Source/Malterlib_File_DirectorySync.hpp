// Copyright © 2015 Hansoft AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

namespace NMib::NFile
{
	template <typename tf_CStream>
	void CDirectorySyncStats::f_Stream(tf_CStream &_Stream)
	{
		_Stream % m_nSyncedFiles;
		_Stream % m_OutgoingBytes;
		_Stream % m_IncomingBytes;
		_Stream % m_nSeconds;
	}

	template <typename tf_CStream>
	void CDirectorySyncSend::CSyncResult::f_Stream(tf_CStream &_Stream)
	{
		_Stream % m_Stats;
		_Stream % m_bFinished;
	}
}
