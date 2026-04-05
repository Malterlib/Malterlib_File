// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

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

	template <typename tf_CStr>
	void CDirectorySyncStats::f_Format(tf_CStr &o_Str) const
	{
		using namespace NStr;
		o_Str += typename tf_CStr::CFormat("Files: {} Incoming: {ns } B ({fe2} MB/s) Outgoing: {ns } ({fe2} MB/s)")
			<< m_nSyncedFiles
			<< m_IncomingBytes
			<< f_IncomingBytesPerSecond()/1'000'000.0
			<< m_OutgoingBytes
			<< f_OutgoingBytesPerSecond()/1'000'000.0
		;
	}
}
