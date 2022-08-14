#pragma once
/// Toolbar for tables

#include <afxbutton.h>

#include "afxcmn.h"
#include <afxwin.h>
#include <array>
#include <functional>
#include <optional>
#include "resource.h"

#include <Controls/Button/IconButton/IconButton.h>
#include <Controls/Layout/Layout.h>

namespace controls {
namespace toolbar {

enum class ButtonType : size_t
{
    eAddButton = 0,
    eDeleteButton,
    eMoveUpButton,
    eMoveDownButton,
    eCount
};
constexpr size_t ButtonsCount = static_cast<size_t>(ButtonType::eCount);

const static auto AllButtons = {
    ButtonType::eAddButton,
    ButtonType::eDeleteButton,
    ButtonType::eMoveUpButton,
    ButtonType::eMoveDownButton
};

constexpr auto InitialButtonsWidth = 60;

struct ButtonInfo
{
    explicit ButtonInfo(std::wstring&& _text, UINT _movingRatio, UINT _sizingRatio,
                        UINT width = InitialButtonsWidth,
                        std::optional<UINT> _imageId = std::nullopt) noexcept
        : text(std::move(_text)), initialWidth(width), movingRatio(_movingRatio), sizingRatio(_sizingRatio), imageId(_imageId)
    {}

    bool visible = true;
    // text on button,
    std::wstring text;
    // initial control width
    long initialWidth = InitialButtonsWidth;
    // ratio for moving button on toolbar resizing
    UINT movingRatio = 0;
    // ratio for sizing button on toolbar resizing
    UINT sizingRatio = 0;
    // id of button image
    std::optional<UINT> imageId;
    // button icon size
    int imageSize = kUseImageSize;
};

typedef std::array<ButtonInfo, ButtonsCount> ButtonInfos;
const static ButtonInfos DefaultButtonsInfo = {
    ButtonInfo(L"Add", 0, 25, InitialButtonsWidth
#ifdef IDB_ADD
    , IDB_ADD
#endif
    ),
    ButtonInfo(L"Del", 25, 25, InitialButtonsWidth
#ifdef IDB_DEL
    , IDB_DEL
#endif
    ),
#ifdef IDB_UP
    ButtonInfo(L"", 50, 0, 25, IDB_UP),
#else
    ButtonInfo(L"Up", 50, 0, 30),
#endif
#ifdef IDB_DOWN
    ButtonInfo(L"", 50, 0, 25, IDB_DOWN)
#else
    ButtonInfo(L"Up", 50, 0, 40)
#endif
};
} // namespace toolbar

class CToolbar final : public CWnd
{
public:
    explicit CToolbar(CListCtrl* connectedList) noexcept;

// Set callbacks for push buttons
public:
    typedef std::function<void(CListCtrl* listToInteraction)> AddCallback;
    typedef std::function<void(CListCtrl* listToInteraction, int index)> OnRemoveCallback;
    typedef std::function<void(CListCtrl* listToInteraction, int itemIndexOld, int itemIndexNew)> OnChangePositionCallback;

    // On button add click callback - override default behaviour
    void OnButtonAdd(const AddCallback& callback) noexcept { m_addCallback = callback; }
    // Callback after removing line
    void OnItemRemove(const OnRemoveCallback& callback) noexcept { m_removeCallback = callback; }
    // Callback after moving item on new position in list
    void OnItemChangePosition(const OnChangePositionCallback& callback) noexcept { m_changePositionCallback = callback; }
    // Set toolbar buttons info
    void InitButtons(const toolbar::ButtonInfos& buttonInfos = toolbar::DefaultButtonsInfo) noexcept;

protected:
    DECLARE_MESSAGE_MAP()

    void PreSubclassWindow() override;
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    afx_msg void OnBnClickedAdd();
    afx_msg void OnBnClickedDelete();
    afx_msg void OnBnClickedUp();
    afx_msg void OnBnClickedDown();

protected:
    void ListItemSelect(int index);
    void ListItemsSwap(int source, int destination);
    _NODISCARD std::list<int> ListSelectedItems() const;
    void EnsureButtonsEnabled();

protected:
    AddCallback m_addCallback;
    OnRemoveCallback m_removeCallback;
    OnChangePositionCallback m_changePositionCallback;

    CListCtrl* m_list;
    std::array<CMFCButton, toolbar::ButtonsCount> m_buttons;
};

} // namespace controls
