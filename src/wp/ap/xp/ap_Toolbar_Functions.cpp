/* AbiWord
 * Copyright (C) 1998-2001 AbiSource, Inc.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ut_types.h"
#include "ut_assert.h"
#include "ut_string.h"
#include "ut_units.h"
#include "ut_debugmsg.h"

#include "ap_Toolbar_Id.h"
#include "ap_Toolbar_Functions.h"
#include "ev_Toolbar_Actions.h"
#include "ev_Toolbar_Labels.h"
#include "xap_App.h"
#include "xap_Clipboard.h"
#include "xap_Frame.h"
#include "fv_View.h"
#include "gr_Graphics.h"
#include "fl_AutoNum.h"
#include "fl_BlockLayout.h"
#include "ap_Prefs_SchemeIds.h"
#include "ap_FrameData.h"
#include "pd_Document.h"

#define ABIWORD_VIEW  	FV_View * pView = static_cast<FV_View *>(pAV_View)


/****************************************************************/

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Changes)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;
	
	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch (id)
	{
	case AP_TOOLBAR_ID_FILE_SAVE:
	  if (!pView->getDocument()->isDirty() || !pView->canDo(true))
	    s = EV_TIS_Gray;
	  break;
	case AP_TOOLBAR_ID_EDIT_UNDO:
		if (!pView->canDo(true))
			s = EV_TIS_Gray;
		break;

	case AP_TOOLBAR_ID_EDIT_REDO:
		if (!pView->canDo(false))
			s = EV_TIS_Gray;
		break;

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Selection)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch (id)
	{
	case AP_TOOLBAR_ID_EDIT_CUT:
	case AP_TOOLBAR_ID_EDIT_COPY:
		if (pView->isSelectionEmpty())
			s = EV_TIS_Gray;
		break;

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Clipboard)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch (id)
	{
	case AP_TOOLBAR_ID_EDIT_PASTE:
		s = ( (pView->getApp()->canPasteFromClipboard()) ? EV_TIS_ZERO : EV_TIS_Gray );
		break;

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}


Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_HdrFtr)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;


	switch (id)
	{
	case AP_TOOLBAR_ID_EDIT_REMOVEHEADER:
		if (!pView->isHeaderOnPage()) 
		  s = EV_TIS_Gray;
		break;

	case AP_TOOLBAR_ID_EDIT_REMOVEFOOTER:
		if (!pView->isFooterOnPage()) 
		  s = EV_TIS_Gray;
		break;
	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}
	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Style)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch (id)
	{
	case AP_TOOLBAR_ID_FMT_STYLE:
		{
			const XML_Char * sz;
			if (!pView->getStyle(&sz))
			{
				static const char * sz2 ="None";
				*pszState = sz2;
				s = EV_TIS_UseString;
			}

			if (sz)
			{	
				static const char * sz2;
				sz2 = sz;
				*pszState = sz2;
				s = EV_TIS_UseString;
			}
			else
			{
				static const char * sz2 ="None";
				*pszState = sz2;
				s = EV_TIS_UseString;
			}
			break;
		}

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}
	return s;
}


Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Bullets)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	EV_Toolbar_ItemState s = EV_TIS_ZERO;
        if(pView->getDocument()->areStylesLocked()) {
            return EV_TIS_Gray;
        }

	fl_BlockLayout * pBlock = pView->getCurrentBlock();
        UT_ASSERT(pBlock);
	if(pBlock->isListItem() == false)
	        return s;
	if(pBlock->getListType() == BULLETED_LIST)
	        s = EV_TIS_Toggled;
        return s;
}


Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Numbers)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	EV_Toolbar_ItemState s = EV_TIS_ZERO;
        if(pView->getDocument()->areStylesLocked()) {
            return EV_TIS_Gray;
        }

	fl_BlockLayout * pBlock = pView->getCurrentBlock();
        UT_ASSERT(pBlock);
	if(pBlock->isListItem() == false)
	        return s;
	if(pBlock->getListType() == NUMBERED_LIST)
	        s = EV_TIS_Toggled;
        return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_CharFmt)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);
	bool bMultiple = false;
	bool bSize = false;
	bool bString = false;

	const XML_Char * prop = NULL;
	const XML_Char * val  = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	if(pView->getDocument()->areStylesLocked()) {
	  return EV_TIS_Gray;
	}

	switch (id)
	{
	case AP_TOOLBAR_ID_FMT_FONT:
		prop = "font-family";
		val  = "";
		bString = true;
		break;

	case AP_TOOLBAR_ID_FMT_SIZE:
		prop = "font-size";
		val  = "";
		bSize = true;
		break;

	case AP_TOOLBAR_ID_FMT_BOLD:
		prop = "font-weight";
		val  = "bold";
		break;

	case AP_TOOLBAR_ID_FMT_ITALIC:
		prop = "font-style";
		val  = "italic";
		break;

	case AP_TOOLBAR_ID_FMT_UNDERLINE:
		prop = "text-decoration";
		val  = "underline";
		bMultiple = true;
		break;

	case AP_TOOLBAR_ID_FMT_OVERLINE:
		prop = "text-decoration";
		val  = "overline";
		bMultiple = true;
		break;

	case AP_TOOLBAR_ID_FMT_STRIKE:
		prop = "text-decoration";
		val  = "line-through";
		bMultiple = true;
		break;


	case AP_TOOLBAR_ID_FMT_TOPLINE:
		prop = "text-decoration";
		val  = "topline";
		bMultiple = true;
		break;


	case AP_TOOLBAR_ID_FMT_BOTTOMLINE:
		prop = "text-decoration";
		val  = "bottomline";
		bMultiple = true;
		break;

	case AP_TOOLBAR_ID_FMT_SUPERSCRIPT:
		prop = "text-position";
		val  = "superscript";
		bMultiple = true;
		break;

	case AP_TOOLBAR_ID_FMT_SUBSCRIPT:
		prop = "text-position";
		val  = "subscript";
		bMultiple = true;
		break;
#ifdef BIDI_ENABLED
	case AP_TOOLBAR_ID_FMT_DIR_OVERRIDE_LTR:
		prop = "dir-override";
		val  = "ltr";
		break;

	case AP_TOOLBAR_ID_FMT_DIR_OVERRIDE_RTL:
		prop = "dir-override";
		val  = "rtl";
		break;
#endif	
	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	if (prop && val)
	{
		// get current char properties from pView
		const XML_Char ** props_in = NULL;
		const XML_Char * sz = NULL;

		if (!pView->getCharFormat(&props_in))
			return s;

		// NB: maybe *no* properties are consistent across the selection
		if (props_in && props_in[0])
			sz = UT_getAttribute(prop, props_in);

		if (sz)
		{
			if (bSize)
			{	
				static char buf[7];
				sprintf(buf, "%s", std_size_string((float)UT_convertToPoints(sz)));
				*pszState = buf;
				s = EV_TIS_UseString;
			}
			else if (bString)
			{	
				static const char * sz2;
				sz2 = sz;
				*pszState = sz2;
				s = EV_TIS_UseString;
			}
			else if (bMultiple)
			{	
				// some properties have multiple values
				if (strstr(sz, val))
					s = EV_TIS_Toggled;
			}
			else
			{
				if (0 == UT_strcmp(sz, val))
					s = EV_TIS_Toggled;
			}
		}
		
		free(props_in);
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_SectionFmt)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;

	const XML_Char * prop = "";
	const XML_Char * val  = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch (id)
	{
	case AP_TOOLBAR_ID_1COLUMN:
		prop = "columns";
		val = "1";
		break;
	case AP_TOOLBAR_ID_2COLUMN:
		prop = "columns";
		val = "2";
		break;
	case AP_TOOLBAR_ID_3COLUMN:
		prop = "columns";
		val = "3";
		break;
	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}
	s = EV_TIS_ZERO;
	if (prop && val)
	{
		// get current block properties from pView
		const XML_Char ** props_in = NULL;
		const XML_Char * sz = NULL;

        bool bResult = pView->getSectionFormat(&props_in);

        if (!bResult)
		{
			return s;
		}
		// NB: maybe *no* properties are consistent across the selection
		if (props_in && props_in[0])
			sz = UT_getAttribute(prop, props_in);

		if (sz)
		{
			if (0 == UT_strcmp(sz, val))
			{
				s = EV_TIS_Toggled;
			}
			else
			{
				s = EV_TIS_ZERO;
			}
		}
		
		free(props_in);
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_BlockFmt)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	if (pszState)
		*pszState = NULL;

	bool bPoints = false;

	const XML_Char * prop = "text-align";
	const XML_Char * val  = NULL;

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

        if(pView->getDocument()->areStylesLocked()) {
	    return EV_TIS_Gray;
        }
	
	switch (id)
	{
	case AP_TOOLBAR_ID_ALIGN_LEFT:
		val  = "left";
		break;

	case AP_TOOLBAR_ID_ALIGN_CENTER:
		val  = "center";
		break;

	case AP_TOOLBAR_ID_ALIGN_RIGHT:
		val  = "right";
		break;

	case AP_TOOLBAR_ID_ALIGN_JUSTIFY:
		val  = "justify";
		break;

	case AP_TOOLBAR_ID_PARA_0BEFORE:
		prop = "margin-top";
		val = "0pt";
		bPoints = true;
		break;

	case AP_TOOLBAR_ID_PARA_12BEFORE:
		prop = "margin-top";
		val = "12pt";
		bPoints = true;
		break;

	case AP_TOOLBAR_ID_SINGLE_SPACE:
		prop = "line-height";
		val = "1.0";
		break;

	case AP_TOOLBAR_ID_MIDDLE_SPACE:
		prop = "line-height";
		val = "1.5";
		break;

	case AP_TOOLBAR_ID_DOUBLE_SPACE:
		prop = "line-height";
		val = "2.0";
		break;
#ifdef BIDI_ENABLED
	case AP_TOOLBAR_ID_FMT_DOM_DIRECTION:
		prop = "dom-dir";
		val = "rtl";
		xxx_UT_DEBUGMSG(("ap_ToolbarGetState_BlockFmt: dom-dir\n"));
		break;
#endif

	default:
		UT_DEBUGMSG(("id=%d", id));
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}
	
	if (prop && val)
	{
		// get current block properties from pView
		const XML_Char ** props_in = NULL;
		const XML_Char * sz = NULL;

		if (!pView->getBlockFormat(&props_in))
			return s;

		// NB: maybe *no* properties are consistent across the selection
		if (props_in && props_in[0])
			sz = UT_getAttribute(prop, props_in);
	
		if (sz)
		{
			if (bPoints)
			{
				if (((int) UT_convertToPoints(sz)) == ((int) UT_convertToPoints(val)))
					s = EV_TIS_Toggled;
			}
			else
			{
				if (0 == UT_strcmp(sz, val))
					s = EV_TIS_Toggled;
			}
		}
		
		free(props_in);
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_Zoom)
{
	UT_UNUSED(id);
	
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	XAP_Frame * pFrame = static_cast<XAP_Frame *> ( pView->getParentData());
	const XAP_StringSet * pSS = XAP_App::getApp()->getStringSet();
	
	EV_Toolbar_ItemState s = EV_TIS_UseString;

	static char buf[10];

	UT_uint32 iZoom = pView->getGraphics()->getZoomPercentage();
	
	sprintf(buf, "%d%%", iZoom);
	switch(pFrame->getZoomType())
	{
	// special cases
	case XAP_Frame::z_PAGEWIDTH:
		sprintf(buf, "%s", pSS->getValue(XAP_STRING_ID_TB_Zoom_PageWidth));
		break;
	case XAP_Frame::z_WHOLEPAGE:
		sprintf(buf, "%s", pSS->getValue(XAP_STRING_ID_TB_Zoom_WholePage));
		break;	
	default:
		;
	}

	*pszState = buf;

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_View)
{
	ABIWORD_VIEW;
	UT_ASSERT(pView);

	XAP_Frame * pFrame = static_cast<XAP_Frame *> (pAV_View->getParentData());
	UT_ASSERT(pFrame);

	AP_FrameData *pFrameData = static_cast<AP_FrameData *> (pFrame->getFrameData());
	UT_ASSERT(pFrameData);

	EV_Toolbar_ItemState s = EV_TIS_ZERO;

	switch(id)
	{
	case AP_TOOLBAR_ID_VIEW_SHOWPARA:
        if ( pFrameData->m_bShowPara )
            s = EV_TIS_Toggled;
        break;

	default:
		UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
		break;
	}

	return s;
}

Defun_EV_GetToolbarItemState_Fn(ap_ToolbarGetState_StylesLocked)
{
        ABIWORD_VIEW;
        UT_ASSERT(pView);

        if(pView->getDocument()->areStylesLocked()) {
            return EV_TIS_Gray;
        }

        return EV_TIS_ZERO;
}

