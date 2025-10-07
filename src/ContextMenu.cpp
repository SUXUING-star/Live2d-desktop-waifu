#include "ContextMenu.hpp"
#include "imgui.h"

ContextMenu::ContextMenu() : _shouldOpen(false) {}

ContextMenu::~ContextMenu() {}

void ContextMenu::Open()
{
    _shouldOpen = true;
}

void ContextMenu::Clear()
{
    _items.clear();
}

void ContextMenu::AddItem(const MenuItem& item)
{
    _items.push_back(item);
}

void ContextMenu::Render()
{
    // 如果上一帧发出了"打开"信号，就在这一帧真正打开它
    if (_shouldOpen)
    {
        ImGui::OpenPopup("context_menu_popup");
        _shouldOpen = false; // 重置信号
    }

    // `BeginPopupContextWindow` 是一个更牛逼的方式，它能自动检测窗口内的右键点击
    // 但为了逻辑清晰，我们还是用手动Open的方式
    if (ImGui::BeginPopup("context_menu_popup"))
    {
        // 从顶级菜单项开始，递归渲染
        RenderMenuItems(_items);
        ImGui::EndPopup();
    }
}

// 递归渲染函数
void ContextMenu::RenderMenuItems(const std::vector<MenuItem>& items)
{
    for (const auto& item : items)
    {
        // 如果这个菜单项有子菜单
        if (!item.SubItems.empty())
        {
            if (ImGui::BeginMenu(item.Name.c_str()))
            {
                // 递归调用，渲染它的子菜单
                RenderMenuItems(item.SubItems);
                ImGui::EndMenu();
            }
        }
        // 如果它是个普通的菜单项
        else
        {
            if (ImGui::MenuItem(item.Name.c_str()))
            {
                // 如果有动作，就执行它
                if (item.Action)
                {
                    item.Action();
                }
            }
        }
    }
}