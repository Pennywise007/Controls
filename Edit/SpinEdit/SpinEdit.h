/*************************************************************************************************
	This control allows you to use CEdit connected to CSpinButton

	To process messages from CSpinCtrl, you need to catch messages with the CEdit ID that you created
	To set a separate identifier, use a special constructor

	For additional configuration of CSpinCtrl, use the GetSpinCtrl() function

	You can catch ON_NOTIFY_REFLECT(UDN_DELTAPOS, &CClass::OnDeltapos) and return 1 if you want to specify changes
*************************************************************************************************/
#pragma once

#include "afxwin.h"
#include "afxcmn.h"
#include <memory>
#include <utility>

#include <Controls/SpinButton/SpinButton.h>
#include <Controls/Edit/CEditBase/CEditBase.h>

namespace spin_edit {
// Align side for spin buttons control
enum class SpinAlign
{
	LEFT  = UDS_ALIGNLEFT,
	RIGHT = UDS_ALIGNRIGHT
};
} // namespace spin_edit

class CSpinEdit :
	public CEditBase
{
public:
	// Undefined spin control id, if not specified - spin conrtrol using Id of the edit
	inline static constexpr UINT kUndefinedControlId = -1;
	CSpinEdit(_In_opt_ UINT controlID = kUndefinedControlId);
	~CSpinEdit();
public:
	// Managing range of values
	void SetRange(_In_ double Left, _In_ double Right);
	void GetRange(_Out_ double& Left, _Out_ double& Right) const;

	void GetWindowRect(LPRECT lpRect) const;
	void GetClientRect(LPRECT lpRect) const;

	void UsePositiveDigitsOnly(_In_opt_ bool bUsePositiveDigitsOnly = true) override;
	void SetMinMaxLimits(_In_ double MinVal, _In_ double MaxVal) override { SetRange(MinVal, MaxVal); };

	// Getting spin control for managing
	CSpinButton* GetSpinCtrl() const { return m_spinCtrl.get(); }

	// Setting spin control align side
	void SetSpinAlign(_In_opt_ spin_edit::SpinAlign Align = spin_edit::SpinAlign::RIGHT);

	// Managing spin control width
	void SetSpinWidth(_In_ int NewWidth);
	int GetSpinWidth() const { return m_spinWidth; };
private:
	void InitEdit();	// иницилизация контрола
	void ReposCtrl(_In_opt_ const CRect& EditRect);	// перемасштабирование контрола
private:
	std::unique_ptr<CSpinButton> m_spinCtrl;		// экземпляр спин контрола
	spin_edit::SpinAlign m_spinAlign;	            // привязка спина
	std::pair<double, double> m_spinRange;	        // границы контрола
	int m_spinWidth;					            // ширина контрола
	int m_spinCtrlID;					            // идентификатор спинконтрола
	bool m_bFromCreate;					            // флаг того что контрол создан не динамически
protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual void PreSubclassWindow();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg void OnEnable(BOOL bEnable);
};