#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include <utility>

/*************************************************************************************************
	Данный контрол позволяет использовать CEdit соединенный с CSpinButtonCtrl

	Для обработки сообщений от CSpinCtrl необходимо ловить сообщения с идентификатором CEdit`а который вы создали
	Для задания отдельного идентификатора воспользуйтесь специальным конструктором

	Для дополнительной настройки CSpinCtrl используйте фунецию GetSpinCtrl()

	You can catch ON_NOTIFY_REFLECT(UDN_DELTAPOS, &CClass::OnDeltapos) and return 1 if you want to specify changes
*************************************************************************************************/
class CSpinButton : public CWnd
{
public:
	DECLARE_MESSAGE_MAP()
public:
	// Managing range of values
	void SetRange(_In_	double Left, _In_  double  Right);
	void GetRange(_Out_	double&Left, _Out_ double& Right) const;

	// Sets the up-down control's buddy window.
	void SetBuddy(_In_ CWnd* pWndBuddy);
	// Retrieves the up-down control's current buddy window.
	CWnd* GetBuddy() const;

	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

private:
	CRect GetUpRect() const;
	CRect GetDownRect() const;

	enum class HitTest
	{
		eUnknown,
		eUp,
		eDown
	};
	HitTest GetHitTest() const;

	void RestartTimer();

protected:
	std::pair<double, double> m_spinRange = { -DBL_MAX, DBL_MAX };
	HitTest m_buttonDown = HitTest::eUnknown;
	HitTest m_hoveredButton = HitTest::eUnknown;

	CWnd* m_linkedBuddy = nullptr;

	size_t m_timerTickCount = 0;

	size_t m_changeValueCount = 0;
	size_t m_changeValueOrder = 0;

public:
	afx_msg BOOL OnDeltapos(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
};
