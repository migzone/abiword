/* AbiWord
 * Copyright (C) 1998-2002 AbiSource, Inc.
 * Copyright (C) 2002
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

#include "ap_Win32FrameImpl.h"

#include "ap_Win32Frame.h"
#include "ut_Win32Timer.h"
#include "gr_Win32Graphics.h"
#include "ap_Win32TopRuler.h"
#include "ap_Win32LeftRuler.h"
#include "ap_Win32StatusBar.h"

#include <winuser.h>

#include <zmouse.h>
#ifdef __MINGW32__
#include "winezmouse.h"
#include <imm.h>
#endif

#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif

#define GWL(hwnd)		reinterpret_cast<AP_Win32Frame *>(GetWindowLong((hwnd), GWL_USERDATA))
#define SWL(hwnd, f)	reinterpret_cast<AP_Win32Frame *>(SetWindowLong((hwnd), GWL_USERDATA,(LONG)(f)))


// reserve space for static variables
char AP_Win32FrameImpl::s_ContainerWndClassName[MAXCNTWNDCLSNMSIZE];
char AP_Win32FrameImpl::s_DocumentWndClassName[MAXDOCWNDCLSNMSIZE];
//static char s_LeftRulerWndClassName[256];



AP_Win32FrameImpl::AP_Win32FrameImpl(AP_Frame *pFrame) :
	XAP_Win32FrameImpl(static_cast<XAP_Frame *>(pFrame)),
	m_hwndContainer(NULL),
	m_hwndTopRuler(NULL),
	m_hwndLeftRuler(NULL),
	m_hwndDocument(NULL),
	m_hWndHScroll(NULL),
	m_hWndVScroll(NULL),
	m_hWndGripperHack(NULL),
	m_vScale(0),
	m_bMouseWheelTrack(false),
	m_startMouseWheelY(0),
	m_startScrollPosition(0),
	m_bMouseActivateReceived(false),
	m_bFirstAfterFocus(false)
{
}

AP_Win32FrameImpl::~AP_Win32FrameImpl(void)
{
}


XAP_FrameImpl * AP_Win32FrameImpl::createInstance(XAP_Frame *pFrame, XAP_App *pApp)
{
	XAP_FrameImpl *pFrameImpl = new AP_Win32FrameImpl(static_cast<AP_Frame *>(pFrame));

	UT_ASSERT(pFrameImpl);

	return pFrameImpl;
}

void AP_Win32FrameImpl::_initialize(void)
{
	// FrameData initialized by AP_Win32Frame

	XAP_Win32FrameImpl::_initialize();

	_createTopLevelWindow();
}

HWND AP_Win32FrameImpl::_createDocumentWindow(XAP_Frame *pFrame, HWND hwndParent,
							UT_uint32 iLeft, UT_uint32 iTop,
							UT_uint32 iWidth, UT_uint32 iHeight)
{
	UT_ASSERT(0);
	UT_return_val_if_fail(pFrame, NULL);

	// create the window(s) that the user will consider to be the
	// document window -- the thing between the bottom of the toolbars
	// and the top of the status bar.  return the handle to it.

	// create a child window for us -- this will be the 'container'.
	// the 'container' will in turn contain the document window, the
	// rulers, and the variousscroll bars and other dead space.

	m_hwndContainer = CreateWindowEx(WS_EX_CLIENTEDGE, s_ContainerWndClassName, NULL,
									 WS_CHILD | WS_VISIBLE,
									 iLeft, iTop, iWidth, iHeight,
									 hwndParent, NULL, static_cast<XAP_Win32App *>(XAP_App::getApp())->getInstance(), NULL);
	UT_ASSERT(m_hwndContainer);
	SWL(m_hwndContainer, this);

	// now create all the various windows inside the container window.

	RECT r;
	GetClientRect(m_hwndContainer,&r);
	const int cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
	const int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);

	m_hWndHScroll = CreateWindowEx(0, "ScrollBar", 0, WS_CHILD | WS_VISIBLE | SBS_HORZ,
									0, r.bottom - cyHScroll,
									r.right - cxVScroll, cyHScroll,
									m_hwndContainer,
									0, static_cast<XAP_Win32App *>(XAP_App::getApp())->getInstance(), 0);
	UT_ASSERT(m_hWndHScroll);
	SWL(m_hWndHScroll, this);

	m_hWndVScroll = CreateWindowEx(0, "ScrollBar", 0, WS_CHILD | WS_VISIBLE | SBS_VERT,
									r.right - cxVScroll, 0,
									cxVScroll, r.bottom - cyHScroll,
									m_hwndContainer,
									0, static_cast<XAP_Win32App *>(XAP_App::getApp())->getInstance(), 0);
	UT_ASSERT(m_hWndVScroll);
	SWL(m_hWndVScroll, this);

#if 1 // if the StatusBar is enabled, our lower-right corner is a dead spot
#  define XX_StyleBits          (WS_DISABLED | SBS_SIZEBOX)
#else
#  define XX_StyleBits          (SBS_SIZEGRIP)
#endif

	m_hWndGripperHack = CreateWindowEx(0,"ScrollBar", 0,
										WS_CHILD | WS_VISIBLE | XX_StyleBits,
										r.right-cxVScroll, r.bottom-cyHScroll, cxVScroll, cyHScroll,
										m_hwndContainer, NULL, static_cast<XAP_Win32App *>(XAP_App::getApp())->getInstance(), NULL);
	UT_ASSERT(m_hWndGripperHack);
	SWL(m_hWndGripperHack, this);

	// create the rulers, if needed

	int yTopRulerHeight = 0;
	int xLeftRulerWidth = 0;

	if (static_cast<AP_FrameData *>(pFrame->getFrameData())->m_bShowRuler)
	{
		_createRulers(pFrame);
		_getRulerSizes(static_cast<AP_FrameData *>(pFrame->getFrameData()), yTopRulerHeight, xLeftRulerWidth);
	}

	// create a child window for us.
	m_hwndDocument = CreateWindowEx(0, s_DocumentWndClassName, NULL,
									WS_CHILD | WS_VISIBLE,
									xLeftRulerWidth, yTopRulerHeight,
									r.right - xLeftRulerWidth - cxVScroll,
									r.bottom - yTopRulerHeight - cyHScroll,
									m_hwndContainer, NULL, static_cast<XAP_Win32App *>(XAP_App::getApp())->getInstance(), NULL);
	UT_ASSERT(m_hwndDocument);
	SWL(m_hwndDocument, this);

	return m_hwndContainer;
}

HWND AP_Win32FrameImpl::_createStatusBarWindow(XAP_Frame *pFrame, HWND hwndParent,
										   UT_uint32 iLeft, UT_uint32 iTop,
										   UT_uint32 iWidth)
{
	UT_ASSERT(0);
	UT_return_val_if_fail(pFrame, NULL);
	AP_Win32StatusBar * pStatusBar = new AP_Win32StatusBar(pFrame);
	UT_ASSERT(pStatusBar);
	_setHwndStatusBar(pStatusBar->createWindow(hwndParent,iLeft,iTop,iWidth));
	static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pStatusBar = pStatusBar;

	return _getHwndStatusBar();
}

void AP_Win32FrameImpl::_createToolbars() 
{
	UT_ASSERT(UT_NOT_IMPLEMENTED);
}

void AP_Win32FrameImpl::_refillToolbarsInFrameData() 
{
	UT_ASSERT(UT_NOT_IMPLEMENTED);
}

void AP_Win32FrameImpl::_rebuildToolbar(UT_uint32 ibar)
{
	UT_ASSERT(UT_NOT_IMPLEMENTED);
}

void AP_Win32FrameImpl::_createTopRuler(XAP_Frame *pFrame)
{
	RECT r;
	int cxVScroll, cyHScroll;

	GetClientRect(m_hwndContainer,&r);
	cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	cyHScroll = GetSystemMetrics(SM_CYHSCROLL);

	// create the top ruler
	AP_Win32TopRuler * pWin32TopRuler = new AP_Win32TopRuler(pFrame);
	UT_ASSERT(pWin32TopRuler);
	m_hwndTopRuler = pWin32TopRuler->createWindow(m_hwndContainer,
												  0,0, (r.right - cxVScroll));
	static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pTopRuler = pWin32TopRuler;


	// get the width from the left ruler and stuff it into the top ruler.
	UT_uint32 xLeftRulerWidth = 0;
	if( m_hwndLeftRuler )
	{
		AP_Win32LeftRuler * pWin32LeftRuler = NULL;
		pWin32LeftRuler =  (AP_Win32LeftRuler *) static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pLeftRuler;
		xLeftRulerWidth = pWin32LeftRuler->getWidth();
	}
	pWin32TopRuler->setOffsetLeftRuler(xLeftRulerWidth);
}

void AP_Win32FrameImpl::_createLeftRuler(XAP_Frame *pFrame)
{
	RECT r;
	int cxVScroll, cyHScroll;

	GetClientRect(m_hwndContainer,&r);
	cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	cyHScroll = GetSystemMetrics(SM_CYHSCROLL);

	UT_uint32 yTopRulerHeight = 0;

	if( m_hwndTopRuler )
	{
		AP_Win32TopRuler * pWin32TopRuler = NULL;
		pWin32TopRuler =  (AP_Win32TopRuler * ) static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pTopRuler;
		yTopRulerHeight = pWin32TopRuler->getHeight();
	}

	// create the left ruler
	AP_Win32LeftRuler * pWin32LeftRuler = new AP_Win32LeftRuler(pFrame);
	UT_ASSERT(pWin32LeftRuler);
	m_hwndLeftRuler = pWin32LeftRuler->createWindow(m_hwndContainer,0,yTopRulerHeight,
													r.bottom - yTopRulerHeight - cyHScroll);
	static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pLeftRuler = pWin32LeftRuler;

	// get the width from the left ruler and stuff it into the top ruler.
    if( m_hwndTopRuler )
	{
		UT_uint32 xLeftRulerWidth = pWin32LeftRuler->getWidth();
		AP_Win32TopRuler * pWin32TopRuler = NULL;
		pWin32TopRuler =  (AP_Win32TopRuler * ) static_cast<AP_FrameData*>(pFrame->getFrameData())->m_pTopRuler;
		pWin32TopRuler->setOffsetLeftRuler(_UD(xLeftRulerWidth));
	}
}

void AP_Win32FrameImpl::_toggleTopRuler(AP_Win32Frame *pFrame, bool bRulerOn)
{
	UT_return_if_fail(pFrame);
	AP_FrameData *pFrameData = static_cast<AP_FrameData *>(pFrame->getFrameData());
	UT_return_if_fail(pFrameData);

	if (bRulerOn)
	{
		UT_ASSERT(!pFrameData->m_pTopRuler);

		_createTopRuler(pFrame);

		pFrameData->m_pTopRuler->setView(pFrame->getCurrentView(), pFrame->getZoomPercentage());
	}
	else
	{
		// delete the actual widgets
		if (m_hwndTopRuler)
			DestroyWindow(m_hwndTopRuler);

		DELETEP(pFrameData->m_pTopRuler);

		m_hwndTopRuler = NULL;
	}

	// repack the child windows
	RECT r;
	GetClientRect(m_hwndContainer, &r);
	_onSize(pFrameData, r.right - r.left, r.bottom - r.top);
}

void AP_Win32FrameImpl::_toggleLeftRuler(AP_Win32Frame *pFrame, bool bRulerOn)
{
	UT_return_if_fail(pFrame);
	AP_FrameData *pFrameData = static_cast<AP_FrameData *>(pFrame->getFrameData());
	UT_return_if_fail(pFrameData);

	if (bRulerOn)
	{
		//
		// If There is an old ruler just return
		//
		if(m_hwndLeftRuler)
		{
			return;
		}
		UT_ASSERT(!pFrameData->m_pLeftRuler);

		_createLeftRuler(pFrame);

		pFrameData->m_pLeftRuler->setView(pFrame->getCurrentView(), pFrame->getZoomPercentage());
	}
	else
	{
		// delete the actual widgets
		if (m_hwndLeftRuler)
			DestroyWindow(m_hwndLeftRuler);

		DELETEP(pFrameData->m_pLeftRuler);

		m_hwndLeftRuler = NULL;
	}

	// repack the child windows
	RECT r;
	GetClientRect(m_hwndContainer, &r);
	_onSize(pFrameData, r.right - r.left, r.bottom - r.top);
}

void AP_Win32FrameImpl::_translateDocumentToScreen(UT_sint32 &x, UT_sint32 &y)
{
	UT_return_if_fail(m_hwndDocument);

	// translate the given document mouse coordinates into absolute screen coordinates.

	POINT pt = { x, y };
	ClientToScreen(m_hwndDocument,&pt);
	x = pt.x;
	y = pt.y;
}

void AP_Win32FrameImpl::_setXScrollRange(AP_FrameData * pData, AV_View *pView)
{
	UT_return_if_fail(m_hwndDocument);

	RECT r;
	GetClientRect(m_hwndDocument, &r);
	const UT_uint32 iWindowWidth = r.right - r.left;
	const UT_uint32 iWidth = _UD(pData->m_pDocLayout->getWidth());

	SCROLLINFO si = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = iWidth;
	si.nPos = ((pView) ? _UD(pView->getXScrollOffset()) : 0);
	si.nPage = iWindowWidth;
	SetScrollInfo(m_hWndHScroll, SB_CTL, &si, TRUE);

	pView->sendHorizontalScrollEvent(_UL(si.nPos),_UL(si.nMax-si.nPage));
}

void AP_Win32FrameImpl::_setYScrollRange(AP_FrameData * pData, AV_View *pView)
{
	UT_return_if_fail(m_hwndDocument);

	RECT r;
	GetClientRect(m_hwndDocument, &r);
	const UT_uint32 iWindowHeight = r.bottom - r.top;
	const UT_uint32 iHeight = _UD(pData->m_pDocLayout->getHeight());

	SCROLLINFO si = { 0 };

	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL | SIF_DISABLENOSCROLL;
	si.nMin = 0;
	si.nMax = iHeight;
	si.nPos = ((pView) ? _UD(pView->getYScrollOffset()) : 0);
	si.nPage = iWindowHeight;
	_setVerticalScrollInfo(&si);

	pView->sendVerticalScrollEvent(_UL(si.nPos),_UL(si.nMax-si.nPage));
}

bool AP_Win32FrameImpl::_RegisterClass(XAP_Win32App * app)
{
	// This is a static method, so can't access 'this' or other non-static items directly

	if (!XAP_Win32FrameImpl::_RegisterClass(app))
		return false;

	WNDCLASSEX  wndclass;
	ATOM a;

	// register class for the container window (this will contain the document
	// and the rulers and the scroll bars)

	snprintf(s_ContainerWndClassName, MAXCNTWNDCLSNMSIZE, "%sContainer", app->getApplicationName());

	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = CS_DBLCLKS | CS_OWNDC;
	wndclass.lpfnWndProc   = AP_Win32FrameImpl::_ContainerWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = app->getInstance();
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = s_ContainerWndClassName;
	wndclass.hIconSm       = NULL;

	a = RegisterClassEx(&wndclass);
	UT_return_val_if_fail(a, false);

	// register class for the actual document window
	snprintf(s_DocumentWndClassName, MAXDOCWNDCLSNMSIZE, "%sDocument", app->getApplicationName());

	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize        = sizeof(wndclass);
	wndclass.style         = CS_DBLCLKS | CS_OWNDC;
	wndclass.lpfnWndProc   = AP_Win32FrameImpl::_DocumentWndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = app->getInstance();
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = s_DocumentWndClassName;
	wndclass.hIconSm       = NULL;

	a = RegisterClassEx(&wndclass);
	UT_return_val_if_fail(a, false);

	if (!AP_Win32TopRuler::RegisterClass(app) ||
		!AP_Win32LeftRuler::RegisterClass(app))
	{
		return false;
	}

	return true;
}

/*****************************************************************/

#define SCROLL_LINE_SIZE 20

void AP_Win32FrameImpl::_getRulerSizes(AP_FrameData * pData, int &yTopRulerHeight, int &xLeftRulerWidth)
{
	UT_return_if_fail(pData);

	if (pData->m_pTopRuler)
		yTopRulerHeight = _UD(pData->m_pTopRuler->getHeight());
	else
		yTopRulerHeight = 0;

	if (pData->m_pLeftRuler)
		xLeftRulerWidth = _UD(pData->m_pLeftRuler->getWidth());
	else
		xLeftRulerWidth = 0;
}

void AP_Win32FrameImpl::_onSize(AP_FrameData* pData, int nWidth, int nHeight)
{
	UT_return_if_fail(m_hwndDocument);
	UT_return_if_fail(m_hWndVScroll);
	UT_return_if_fail(m_hWndHScroll);
	UT_return_if_fail(m_hWndGripperHack);

	const int cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
	const int cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
	int yTopRulerHeight = 0;
	int xLeftRulerWidth = 0;

	_getRulerSizes(pData, yTopRulerHeight, xLeftRulerWidth);

	MoveWindow(m_hWndVScroll, nWidth-cxVScroll, 0, cxVScroll, nHeight-cyHScroll, TRUE);
	MoveWindow(m_hWndHScroll, 0, nHeight-cyHScroll, nWidth - cxVScroll, cyHScroll, TRUE);
	MoveWindow(m_hWndGripperHack, nWidth-cxVScroll, nHeight-cyHScroll, cxVScroll, cyHScroll, TRUE);


	if (m_hwndTopRuler)
	{
		MoveWindow(m_hwndTopRuler, 0, 0, nWidth - cxVScroll, yTopRulerHeight, TRUE);
		InvalidateRect(m_hwndTopRuler, NULL, TRUE);
	}

	if (m_hwndLeftRuler)
		MoveWindow(m_hwndLeftRuler, 0, yTopRulerHeight,
				   xLeftRulerWidth, nHeight - yTopRulerHeight - cyHScroll, TRUE);

	MoveWindow(m_hwndDocument, xLeftRulerWidth, yTopRulerHeight,
			   nWidth - xLeftRulerWidth - cxVScroll,
			   nHeight - yTopRulerHeight - cyHScroll, TRUE);
}


void AP_Win32FrameImpl::_setVerticalScrollInfo(const SCROLLINFO * psi)
{
	// internal interface to GetScrollInfo() and SetScrollInfo()
	// to deal with 16-bit limitations of SB_THUMBTRACK.

	UT_uint32 scale, x;
	for (x=psi->nMax, scale=0; (x > 0x0000ffff); x>>=1, scale++)
		;

	m_vScale = scale;

	if (scale == 0)
	{
		SetScrollInfo(m_hWndVScroll, SB_CTL, psi, TRUE);
		return;
	}

	SCROLLINFO si = *psi;				// structure copy
	si.nMin >>= scale;
	si.nMax >>= scale;
	si.nPos >>= scale;
	si.nPage >>= scale;

	SetScrollInfo(m_hWndVScroll, SB_CTL, &si, TRUE);
	return;
}

void AP_Win32FrameImpl::_getVerticalScrollInfo(SCROLLINFO * psi)
{
	GetScrollInfo(m_hWndVScroll, SB_CTL, psi);

	if (m_vScale)
	{
		psi->nMin <<= m_vScale;
		psi->nMax <<= m_vScale;
		psi->nPos <<= m_vScale;
		psi->nPage <<= m_vScale;
	}

	return;
}

int AP_Win32FrameImpl::_getMouseWheelLines()
{
 	OSVERSIONINFO Info = { 0 };

 	Info.dwOSVersionInfoSize = sizeof(Info);

 	if (GetVersionEx(&Info) &&
 		Info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&
 		Info.dwMajorVersion == 4 &&
 		Info.dwMinorVersion == 0)
 	{
 		// Win95
 		UINT msgMSHWheelGetScrollLines = RegisterWindowMessage(MSH_SCROLL_LINES);
 		HWND hdlMSHWheel = FindWindow(MSH_WHEELMODULE_CLASS, MSH_WHEELMODULE_TITLE);
 		if (hdlMSHWheel && msgMSHWheelGetScrollLines)
 		{
 			return SendMessage(hdlMSHWheel, msgMSHWheelGetScrollLines, 0, 0);
 		}
 	}
 	else
 	{
 		// Win98, NT, 2K
 		UINT nScrollLines;
 		if (SystemParametersInfo(	SPI_GETWHEELSCROLLLINES,
 									0,
 									(PVOID) &nScrollLines,
 									0))
 		{
 			return nScrollLines;
 		}
 	}

 	return 3;
}

/////////////////////////////////////////////////////////////////////////

void AP_Win32FrameImpl::_startTracking(UT_sint32 x, UT_sint32 y)
{
	m_startMouseWheelY = y;
	m_bMouseWheelTrack = true;

	m_startScrollPosition = GetScrollPos(m_hWndVScroll, SB_CTL);

	SetCapture(m_hwndDocument);
}

void AP_Win32FrameImpl::_endTracking(UT_sint32 x, UT_sint32 y)
{
	m_bMouseWheelTrack = false;
	ReleaseCapture();
}

void AP_Win32FrameImpl::_track(UT_sint32 x, UT_sint32 y)
{
	UT_sint32 Delta = y - m_startMouseWheelY;

	// map delta to scroll bar range.

	int iMin, iMax;
	RECT rect;
	GetClientRect(m_hwndDocument, &rect);

	if(y < rect.top || y > rect.bottom)
		return;

	GetScrollRange(m_hWndVScroll, SB_CTL, &iMin, &iMax);

	if(Delta < 0)
	{
		int iNewPosition = (m_startScrollPosition - iMin) * (y - rect.top) / (m_startMouseWheelY - rect.top);

		SendMessage(m_hwndContainer, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, (WORD)iNewPosition), NULL);
	}
	else
	{
		int iNewPosition = m_startScrollPosition + (iMax - m_startScrollPosition) * (y - m_startMouseWheelY) / (rect.bottom - m_startMouseWheelY);

		SendMessage(m_hwndContainer, WM_VSCROLL, MAKEWPARAM(SB_THUMBTRACK, (WORD)iNewPosition), NULL);
	}

}

/*****************************************************************/

LRESULT CALLBACK AP_Win32FrameImpl::_ContainerWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	AP_Win32Frame * f = reinterpret_cast<AP_Win32Frame *>(GetWindowLong(hwnd, GWL_USERDATA));

	if (!f)
	{
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}

	AP_Win32FrameImpl *fImpl = static_cast<AP_Win32FrameImpl *>(f->getFrameImpl());
	UT_return_val_if_fail(fImpl, DefWindowProc(hwnd, iMsg, wParam, lParam));

	AV_View* pView = f->getCurrentView();
	UT_ASSERT(pView);		/* only fatal for some messages */

	switch (iMsg)
	{
		case WM_SETFOCUS:
		{
			SetFocus(fImpl->m_hwndDocument);
			return 0;
		}

		case WM_SIZE:
		{
			const int nWidth = LOWORD(lParam);
			const int nHeight = HIWORD(lParam);

			fImpl->_onSize(static_cast<AP_FrameData*>(f->getFrameData()), nWidth, nHeight);
			return 0;
		}

		case WM_VSCROLL:
		{
			int nScrollCode = (int) LOWORD(wParam); // scroll bar value

			SCROLLINFO si = { 0 };

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			fImpl->_getVerticalScrollInfo(&si);

			switch (nScrollCode)
			{
				default:
					return 0;

				case SB_PAGEUP:
					si.nPos -= si.nPage;
					if (si.nPos < 0)
					si.nPos = 0;
					break;
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
					break;
				case SB_LINEDOWN:
					si.nPos += SCROLL_LINE_SIZE;
					break;
				case SB_LINEUP:
					si.nPos -= SCROLL_LINE_SIZE;
					if (si.nPos < 0)
						si.nPos = 0;
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					si.nPos = (int)HIWORD(wParam); // dynamic scroll box position -- a 16-bit value
					si.nPos <<= fImpl->m_vScale;
					break;
			}

			fImpl->_setVerticalScrollInfo(&si);				// notify window of new value.
			fImpl->_getVerticalScrollInfo(&si);				// update from window, in case we got clamped
			pView->sendVerticalScrollEvent(_UL(si.nPos));	// now tell the view

			return 0;
		}

		case WM_HSCROLL:
		{
			int nScrollCode = (int) LOWORD(wParam); // scroll bar value
			int nPos = (int) HIWORD(wParam);  // scroll box position

			SCROLLINFO si = { 0 };

			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			GetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si);

			switch (nScrollCode)
			{
				case SB_PAGEUP:
					si.nPos -= si.nPage;
					if (si.nPos < 0)
					{
						si.nPos = 0;
					}
					SetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si, TRUE);
					break;
				case SB_PAGEDOWN:
					si.nPos += si.nPage;
					SetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si, TRUE);
					break;
				case SB_LINEDOWN:
					si.nPos += SCROLL_LINE_SIZE;
					SetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si, TRUE);
					break;
				case SB_LINEUP:
					si.nPos -= SCROLL_LINE_SIZE;
					if (si.nPos < 0)
					{
						si.nPos = 0;
					}
					SetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si, TRUE);
					break;
				case SB_THUMBPOSITION:
				case SB_THUMBTRACK:
					si.nPos = nPos;
					SetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si, TRUE);
					break;
			}

			if (nScrollCode != SB_ENDSCROLL)
			{
				// in case we got clamped
				GetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si);

				// now tell the view
				pView->sendHorizontalScrollEvent(_UL(si.nPos));
			}

			return 0;
		}

		case WM_SYSCOLORCHANGE:
		{
			SendMessage(fImpl->m_hwndTopRuler,WM_SYSCOLORCHANGE,0,0);
			SendMessage(fImpl->m_hwndLeftRuler,WM_SYSCOLORCHANGE,0,0);
			SendMessage(fImpl->m_hwndDocument,WM_SYSCOLORCHANGE,0,0);
			return DefWindowProc(hwnd, iMsg, wParam, lParam);
		}

	 	case WM_MOUSEWHEEL:
 		{
 			// Get delta
 			const int iDelta = (short) HIWORD(wParam);

 			// Calculate the movement offset to an integer resolution
 			const int iMove = (iDelta * _getMouseWheelLines()) / WHEEL_DELTA;

	 		// Get current scroll position
 			SCROLLINFO si = { 0 };

 			si.cbSize = sizeof(si);
 			si.fMask = SIF_ALL;
 			fImpl->_getVerticalScrollInfo(&si);

	 		// Clip new position to limits
 			int iNewPos = si.nPos - (((iMove)?iMove:1) * SCROLL_LINE_SIZE);
 			if (iNewPos > si.nMax) iNewPos = si.nMax;
 			if (iNewPos < si.nMin) iNewPos = si.nMin;

 			if (iNewPos != si.nPos)
 			{
 				// If position has changed set new position
 				SendMessage(hwnd,
 						WM_VSCROLL,
 						MAKELONG(SB_THUMBPOSITION, iNewPos),
 						NULL);
 			}

			return 0;
		}

		default:
			return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}
}

LRESULT CALLBACK AP_Win32FrameImpl::_DocumentWndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	AP_Win32Frame * f = reinterpret_cast<AP_Win32Frame *>(GetWindowLong(hwnd, GWL_USERDATA));

	if (!f)
	{
		return DefWindowProc(hwnd, iMsg, wParam, lParam);
	}

	AP_Win32FrameImpl *fImpl = static_cast<AP_Win32FrameImpl *>(f->getFrameImpl());
	UT_return_val_if_fail(fImpl, DefWindowProc(hwnd, iMsg, wParam, lParam));

	AV_View* 		pView = f->getCurrentView();
	EV_Win32Mouse*	pMouse = (EV_Win32Mouse *) fImpl->m_pMouse;

	switch (iMsg)
	{
		case WM_MOUSEACTIVATE:
			fImpl->m_bMouseActivateReceived = true;
			return MA_ACTIVATE;

		case WM_SETFOCUS:			
			if (pView)
			{
				pView->focusChange(AV_FOCUS_HERE);
			
				if (GetKeyState(VK_LBUTTON)>0)
					fImpl->m_bFirstAfterFocus = true;

				if(!fImpl->m_bMouseActivateReceived)
				{
					// HACK:	Capture leaving a tool bar combo.
					// We need to get a mouse down signal.
					// windows is not activated so send a mouse down if the mouse is pressed.

					UT_DEBUGMSG(("%s(%d): Need to set mouse down\n", __FILE__, __LINE__));

					// GetKeyState
				}
			}
			return 0;

		case WM_KILLFOCUS:
			if (pView)
			{
				pView->focusChange(AV_FOCUS_NONE);			
			}
			return 0;

		case WM_CREATE:
			return 0;

		case WM_SETCURSOR:
		{
			FV_View * pFV = static_cast<FV_View *>(pView);
			GR_Graphics * pG = pFV->getGraphics();
			GR_Win32Graphics * pGWin32 = static_cast<GR_Win32Graphics *>(pG);
			pGWin32->handleSetCursorMessage();
			return 1;
		}

		case WM_LBUTTONDOWN:
		
			/* When we get the focus back 					*/
			if (fImpl->m_bFirstAfterFocus)
				fImpl->m_bFirstAfterFocus = false;	
			else
			{
				if(GetFocus() != hwnd)
				{
					SetFocus(hwnd);
					pView = f->getCurrentView();
					pMouse = (EV_Win32Mouse *) fImpl->m_pMouse;
				}
				pMouse->onButtonDown(pView,hwnd,EV_EMB_BUTTON1,wParam,LOWORD(lParam),HIWORD(lParam));
				fImpl->m_bMouseActivateReceived = false;
			}
			return 0;

		case WM_MBUTTONDOWN:
			fImpl->_startTracking(LOWORD(lParam), HIWORD(lParam));
			pMouse->onButtonDown(pView,hwnd,EV_EMB_BUTTON2,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_RBUTTONDOWN:
			pMouse->onButtonDown(pView,hwnd,EV_EMB_BUTTON3,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_LBUTTONDBLCLK:
			pMouse->onDoubleClick(pView,hwnd,EV_EMB_BUTTON1,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_MBUTTONDBLCLK:
			pMouse->onDoubleClick(pView,hwnd,EV_EMB_BUTTON2,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_RBUTTONDBLCLK:
			pMouse->onDoubleClick(pView,hwnd,EV_EMB_BUTTON3,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			UT_DEBUGMSG(("WM_KEYDOWN %d  - %d\n",wParam, lParam));
			ev_Win32Keyboard *pWin32Keyboard = static_cast<ev_Win32Keyboard *>(fImpl->m_pKeyboard);
	    
		     	if (pWin32Keyboard->onKeyDown(pView,hwnd,iMsg,wParam,lParam))
     				return DefWindowProc(hwnd,iMsg,wParam,lParam);
	 		else
 				return false;	    				 	     			
    		
		}

		case WM_SYSCHAR:
		case WM_CHAR:
		{
			UT_DEBUGMSG(("WM_CHAR %d  - %d\n",wParam, lParam));
			ev_Win32Keyboard *pWin32Keyboard = static_cast<ev_Win32Keyboard *>(fImpl->m_pKeyboard);
	    
			pWin32Keyboard->onChar(pView,hwnd,iMsg,wParam,lParam);		
			return DefWindowProc(hwnd,iMsg,wParam,lParam);
		}

		case WM_IME_CHAR:
		{
			ev_Win32Keyboard *pWin32Keyboard = static_cast<ev_Win32Keyboard *>(fImpl->m_pKeyboard);
			if (pWin32Keyboard->onIMEChar(pView,hwnd,iMsg,wParam,lParam))
				return 0;
			return DefWindowProc(hwnd,iMsg,wParam,lParam);
		}

		case WM_MOUSEMOVE:
			if (fImpl->_isTracking())
			{
				fImpl->_track(LOWORD(lParam),HIWORD(lParam));
			}
			else
			{
				pMouse->onButtonMove(pView,hwnd,wParam,LOWORD(lParam),HIWORD(lParam));
			}
			return 0;

		case WM_LBUTTONUP:
			if(GetFocus() != hwnd)	// HACK: get focus back from tool bar combos.
			{
				SetFocus(hwnd);
				pView = f->getCurrentView();
				pMouse = (EV_Win32Mouse *) fImpl->m_pMouse;
				pMouse->onButtonDown(pView,hwnd,EV_EMB_BUTTON1,wParam,LOWORD(lParam),HIWORD(lParam));
			}
			pMouse->onButtonUp(pView,hwnd,EV_EMB_BUTTON1,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_MBUTTONUP:
			fImpl->_endTracking(LOWORD(lParam), HIWORD(lParam));
			pMouse->onButtonUp(pView,hwnd,EV_EMB_BUTTON2,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_RBUTTONUP:
			pMouse->onButtonUp(pView,hwnd,EV_EMB_BUTTON3,wParam,LOWORD(lParam),HIWORD(lParam));
			return 0;

		case WM_SIZE:
		{
			if (pView)
			{
				int nWidth = LOWORD(lParam);
				int nHeight = HIWORD(lParam);

				pView->setWindowSize(nWidth, nHeight);

				// may need to scroll to keep everything in sync.
				// the following is necessary to make sure that the
				// window de-scrolls as it gets larger.

				SCROLLINFO si = { 0 };
				si.cbSize = sizeof(si);
				si.fMask = SIF_ALL;

				fImpl->_getVerticalScrollInfo(&si);
				pView->sendVerticalScrollEvent(_UL(si.nPos),_UL(si.nMax-si.nPage));

				GetScrollInfo(fImpl->m_hWndHScroll, SB_CTL, &si);
				pView->sendHorizontalScrollEvent(_UL(si.nPos),_UL(si.nMax-si.nPage));
			}
			return 0;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			UT_Rect r(ps.rcPaint.left,ps.rcPaint.top,
					ps.rcPaint.right-ps.rcPaint.left,
					ps.rcPaint.bottom-ps.rcPaint.top);
			pView->draw(&r);

			EndPaint(hwnd, &ps);

			return 0;
		}

		case WM_TIMER:
		{
			TIMERPROC * pfn = (TIMERPROC *)lParam;
			UT_ASSERT( (void *)(pfn) == (void *)(Global_Win32TimerProc) );
			Global_Win32TimerProc(hwnd,WM_TIMER,(UINT)wParam,NULL);
			return 0;
		}

		case WM_DESTROY:
			return 0;

		case WM_INPUTLANGCHANGE:	// let the XAP_Win32Frame handle this
			return ::SendMessage(fImpl->_getTopLevelWindow(), WM_INPUTLANGCHANGE, wParam, lParam);

	} /* switch (iMsg) */

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
