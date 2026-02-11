#pragma once

#include <afx.h>
#include <functional>
#include <map>
#include <type_traits>
#include <ext/core/tracer.h>

namespace controls::list {

/*
   Storage for list cells data. Allow to set data to cells and don`t care about list state

   Important: GetItemData must contain the index of the item when it was added
*/
template <typename Struct>
class SubItemsInfo
{
public:
    template <typename... Args>
    Struct* Assign(int item, int subItem, Args&&... args)
    {
        auto res = m_subItemsInfos[item].insert_or_assign(subItem, Struct(std::forward<Args>(args)...));
        return &res.first->second;
    }
    const Struct* Get(int item, int subItem) const
    {
        const auto itemIt = m_subItemsInfos.find(item);
        if (itemIt == m_subItemsInfos.end())
        {
            return nullptr;
        }
        const auto subItemIt = itemIt->second.find(subItem);
        if (subItemIt == itemIt->second.end())
        {
            return nullptr;
        }
        return &subItemIt->second;
    }
    Struct* Get(int item, int subItem)
    {
        const auto itemIt = m_subItemsInfos.find(item);
        if (itemIt == m_subItemsInfos.end())
        {
            return nullptr;
        }
        const auto subItemIt = itemIt->second.find(subItem);
        if (subItemIt == itemIt->second.end())
        {
            return nullptr;
        }
        return &subItemIt->second;
    }
    void Remove(int item, int subItem)
    {
        const auto itemIt = m_subItemsInfos.find(item);
        if (itemIt == m_subItemsInfos.end())
            return;
        const auto subItemIt = itemIt->second.find(subItem);
        if (subItemIt == itemIt->second.end())
            return;

        itemIt->second.erase(subItemIt);
        if (itemIt->second.empty())
            m_subItemsInfos.erase(itemIt);
    }
    typedef std::function<void(Struct& subItemInfo, int newItemIndex, int newSubItemIndex)> OnIndexChanged;
    void OnNewPos(OnIndexChanged&& function)
    {
        m_onIndexChangedCallback = std::move(function);
    }

    void ProcessWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam, const std::function<int(int)>& getItemData)
    {
        switch (uMsg)
        {
        case LVM_INSERTITEM:
            {
                const auto* pItem = reinterpret_cast<const LVITEM*>(lParam);

                if (!m_subItemsInfos.empty())
                {
                    for (auto movingDownIt = std::prev(m_subItemsInfos.end());
                         movingDownIt != m_subItemsInfos.end() && movingDownIt->first >= pItem->iItem;
                         --movingDownIt)
                    {
                        if (m_onIndexChangedCallback)
                        {
                            for (auto& it : movingDownIt->second)
                            {
                                m_onIndexChangedCallback(it.second, movingDownIt->first + 1, it.first);
                            }
                        }

                        m_subItemsInfos[movingDownIt->first + 1] = std::move(movingDownIt->second);
                        movingDownIt = m_subItemsInfos.erase(movingDownIt);
                    }
                }
            }
            break;
        case LVM_DELETEITEM:
            {
                const int iItem = getItemData((int)wParam);
                if (auto erasingLine = m_subItemsInfos.find(iItem); erasingLine != m_subItemsInfos.end())
                {
                    for (auto it = std::next(erasingLine); it != m_subItemsInfos.end();)
                    {
                        if (m_onIndexChangedCallback)
                        {
                            for (auto& subItems : it->second)
                            {
                                m_onIndexChangedCallback(subItems.second, it->first - 1, subItems.first);
                            }
                        }

                        m_subItemsInfos[it->first - 1] = std::move(it->second);
                        it = m_subItemsInfos.erase(it);
                    }

                    m_subItemsInfos.erase(erasingLine);
                }
            }
            break;
        case LVM_DELETEALLITEMS:
            m_subItemsInfos.clear();
            break;
        case LVM_INSERTCOLUMN:
            {
                const int iSubItem = (int)wParam;
                for (auto& controls : m_subItemsInfos)
                {
                    for (auto controlIt = std::prev(controls.second.end());
                         controlIt != controls.second.end() && controlIt->first >= iSubItem;
                         --controlIt)
                    {
                        if (m_onIndexChangedCallback)
                        {
                            m_onIndexChangedCallback(controlIt->second, controls.first, controlIt->first + 1);
                        }

                        controls.second[controlIt->first + 1] = std::move(controlIt->second);
                        controlIt = controls.second.erase(controlIt);
                    }
                }
            }
            break;
        case LVM_DELETECOLUMN:
            {
                const int iSubItem = (int)wParam;
                for (auto& controls : m_subItemsInfos)
                {
                    for (auto movingLeftIt = controls.second.upper_bound(iSubItem); movingLeftIt != controls.second.end();)
                    {
                        if (m_onIndexChangedCallback)
                        {
                            m_onIndexChangedCallback(movingLeftIt->second, controls.first, movingLeftIt->first + 1);
                        }

                        controls.second[movingLeftIt->first] = std::move(movingLeftIt->second);
                        movingLeftIt = controls.second.erase(movingLeftIt);
                    }
                }
            }
            break;
        default:
            break;
        }
    }

public:
    typedef std::map<int, Struct> ControlsByColumns;
    typedef std::map<int, ControlsByColumns> ColumnsByLines;

    ColumnsByLines m_subItemsInfos;

private:
    OnIndexChanged m_onIndexChangedCallback;
};

} // namespace controls::list
