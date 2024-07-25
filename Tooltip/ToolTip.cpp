#include <afxwin.h>

#include "ToolTip.h"

#include "Controls/DefaultWindowProc.h"

namespace controls {

std::list<CToolTip> existingTooltips;

BEGIN_MESSAGE_MAP(CToolTip, CToolTipCtrl)
END_MESSAGE_MAP()

CToolTip::CToolTip(CWnd& wnd, const CString& text)
{
	SetTooltip(wnd, text);
}

void CToolTip::SetTooltip(CWnd& wnd, const CString& text)
{
	ASSERT(::IsWindow(wnd.m_hWnd));

	// remove old callback on reattaching to new window
	if (m_attachedWnd)
		DefaultWindowProc::RemoveAnyMessageCallback(*m_attachedWnd, this);
	m_attachedWnd = &wnd;

	if (!IsWindow(m_hWnd))
		Create(&wnd);

	// CStatic doesn't receive WM_MOUSEMOVE without SS_NOTIFY
	if (wnd.IsKindOf(RUNTIME_CLASS(CStatic)))
		wnd.ModifyStyle(0, SS_NOTIFY);

	DefaultWindowProc::OnAnyWindowMessage(wnd, [&](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
		{
			switch (message)
			{
			case WM_LBUTTONDOWN:
			case WM_LBUTTONUP:
			case WM_MOUSEMOVE:
			{
				MSG msg;
				msg.hwnd = hWnd;
				msg.message = message;
				msg.wParam = wParam;
				msg.lParam = lParam;
				msg.time = 0;
				msg.pt.x = GET_X_LPARAM(lParam);
				msg.pt.y = GET_Y_LPARAM(lParam);
				RelayEvent(&msg);
			}
			break;
			case WM_NCDESTROY:
				m_attachedWnd = nullptr;
                break;
			default:
				break;
			}
		}, this);

	// split text by '\n' and set max width to the longest line
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(GetFont());

	CString line;
	int start = 0, maxWidth = 0;
	do {
		line = text.Tokenize(_T("\n"), start);
		if (line.IsEmpty())
			break;

		const auto lineWidth = dc.GetTextExtent(line).cx;
		if (lineWidth > maxWidth)
            maxWidth = lineWidth;
	} while (!line.IsEmpty());
	SetMaxTipWidth(maxWidth);
	
	dc.SelectObject(pOldFont);

	AddTool(&wnd, text);
}

void SetTooltip(CWnd& wnd, const CString& text)
{
	if (text.IsEmpty())
	{
		for (auto it = existingTooltips.begin(), end = existingTooltips.end(); it != end; ++it)
		{
			if (it->m_attachedWnd == &wnd)
			{
				it->DestroyWindow();
				existingTooltips.erase(it);
				break;
			}
		}
		return;
	}

	for (auto it = existingTooltips.begin(), end = existingTooltips.end(); it != end; ++it)
	{
		if (it->m_attachedWnd == &wnd)
		{
			it->SetTooltip(wnd, text);
			return;
		}
	}

	auto& tooltip = existingTooltips.emplace_back(wnd, text);
	DefaultWindowProc::OnWindowMessage(tooltip, WM_NCDESTROY, [](HWND hWnd, WPARAM wParam, LPARAM lParam, LRESULT& result)
	{
		for (auto it = existingTooltips.begin(), end = existingTooltips.end(); it != end; ++it)
		{
			if (it->m_hWnd != hWnd)
				continue;

			DefaultWindowProc::RemoveCallback(*it, WM_NCDESTROY, &*it);
			if (auto attachedTo = it->m_attachedWnd; !!attachedTo)
				DefaultWindowProc::RemoveAnyMessageCallback(*attachedTo, &*it);

			existingTooltips.erase(it);
			break;
		}
	}, &tooltip);
}

} // namespace controls
