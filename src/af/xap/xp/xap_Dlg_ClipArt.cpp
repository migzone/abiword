/* AbiWord
 * Copyright (C) 2001 AbiSource, Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
 * 02110-1301 USA.
 */

#include "xap_Dlg_ClipArt.h"
#include "ut_string.h"

XAP_Dialog_ClipArt::XAP_Dialog_ClipArt(XAP_DialogFactory * pDlgFactory, 
									   XAP_Dialog_Id id)
	: XAP_Dialog_NonPersistent (pDlgFactory, id), m_answer(a_CANCEL),
	  m_szGraphicName(nullptr), m_szInitialDir(nullptr)

{
}

XAP_Dialog_ClipArt::~XAP_Dialog_ClipArt()
{
	FREEP(m_szGraphicName);
	FREEP(m_szInitialDir);
}

void XAP_Dialog_ClipArt::setInitialDir (const char * szInitialDir)
{
	FREEP(m_szInitialDir);
	m_szInitialDir = g_strdup (szInitialDir);
}

void XAP_Dialog_ClipArt::setGraphicName (const char * name)
{
	FREEP(m_szGraphicName);
	m_szGraphicName = g_strdup (name);
}

