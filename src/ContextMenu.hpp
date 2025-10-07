#pragma once
#include <string>
#include <vector>
#include <functional>

// 菜单项的数据结构，保持不变
struct MenuItem {
    std::string Name;
    std::function<void()> Action;
    std::vector<MenuItem> SubItems; // <<< 新增！支持无限层级的子菜单！
};

// 一个独立的、可复用的右键菜单类
class ContextMenu
{
public:
    ContextMenu();
    ~ContextMenu();

    // 打开菜单。主程序只需要调用这个函数
    void Open();

    // 渲染菜单。在ImGui的帧循环里调用
    void Render();
    
    // 清空所有菜单项
    void Clear();

    // 添加一个主菜单项
    void AddItem(const MenuItem& item);
    
private:
    // 一个递归函数，用来画出菜单和所有子菜单
    void RenderMenuItems(const std::vector<MenuItem>& items);

    bool _shouldOpen; // 标记下一帧是否要打开菜单
    std::vector<MenuItem> _items; // 存储所有顶级菜单项
};