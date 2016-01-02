// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#define DMibAllowCodeStandardViolations 1


void fg_PropertiesWindow();

class CEditFileSystemClass
{
public:
	CStr m_Heading;

	CStr m_VolumeName;
	CStr m_FilePath;
	int m_ClusterSize;
	int m_InitialSize;
	int m_GrowSize;
	bint m_bCreateFile;

	void *m_pParentWindow;
};

aint fg_EditFileSystem(CEditFileSystemClass &_Params);
void fg_ErrorBox(void *_pParent, const ch8 *_pMessage);

