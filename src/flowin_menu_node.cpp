#include "pch.h"
#include "flowin_menu_node.h"
#include "flowin_core.h"
#include "flowin_config.h"
#include "flowin_vars.h"

using namespace Flowin;
using cfg_t = CfgFlowinHost::Ptr;

inline bool IsFlowinAlive(const cfg_t& config)
{
    return config && FlowinCore::Get()->IsFlowinAlive(config->guid);
}

inline void NotifyFlowin(const cfg_t& config, uint32_t msg, WPARAM wp = 0, LPARAM lp = 0)
{
    if (config != nullptr)
        FlowinCore::Get()->PostFlowinMessage(config->guid, msg, wp, lp);
}

inline void NotifyFlowinCommand(const cfg_t& config, MenuCommands cmd, LPARAM lp = 0)
{
    NotifyFlowin(config, UWM_FLOWIN_COMMAND, (WPARAM)cmd, lp);
}

// clang-format off
#define flags_disable(cond) { if (cond) flags |= mainmenu_commands::flag_disabled; }
#define flags_check(cond) { if (cond) flags |= mainmenu_commands::flag_checked; }
#define flags_require_config() { if (config == nullptr) flags |= mainmenu_commands::flag_disabled; }
#define flags_default_hidden() { flags |= mainmenu_commands::flag_defaulthidden; }
// clang-format on

FlowinMenuGroup::Ptr BuildFlowinMenuNodes()
{
    static FlowinMenuGroup::Ptr shared_nodes;
    if (shared_nodes != nullptr)
        return shared_nodes;

    if (auto group = FlowinMenuGroup::NewGroup(0 /*not used*/))
    {
        if (auto node = group->NewNode(MenuCommands::Show, "显示", FlowinMenuShowOnFlowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = FlowinCore::Get();
                    if (core->IsFlowinAlive(config->guid))
                        core->PostFlowinMessage(config->guid, WM_CLOSE);
                    else
                        core->CreateFlowin(config->guid);
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::ShowOnStartup, "启动时显示",
                                        FlowinMenuShowOnFlowin | FlowinMenuShowOnSystemMenu))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                    config->show_on_startup = !config->show_on_startup;
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->show_on_startup);
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::AlwaysOnTop, "总在最上面", FlowinMenuShowOnAll))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                {
                    if (IsFlowinAlive(config))
                        NotifyFlowinCommand(config, id);
                    else
                        config->always_on_top = !config->always_on_top;
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->always_on_top);
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::BringToTop, "放置最上面", FlowinMenuShowOnMainMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::NoFrame, "无窗口边框", FlowinMenuShowOnAll))
        {
            node->action = [id = node->id](cfg_t& config)
            {
                if (config != nullptr)
                {
                    if (IsFlowinAlive(config))
                        NotifyFlowinCommand(config, id);
                    else
                        config->show_caption = !config->show_caption;
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && !config->show_caption);
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::NoFrameSilent, "无窗口边框 (安静)",
                                        FlowinMenuShowOnMainMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                flags_default_hidden();
                return flags;
            };
        }

        // snap group
        if (auto snap_group_node = group->NewNode(MenuCommands::Invalid, "", FlowinMenuShowOnAll))
        {
            auto snap_group = FlowinMenuGroup::NewGroup(FlowinMenuGroupSubmenu, "吸附");
            snap_group_node->child_group = snap_group;

            if (auto node =
                    snap_group->NewNode(MenuCommands::SnapToEdge, "吸附屏幕边缘", FlowinMenuShowOnAll))
            {
                node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

                node->get_flags = [](const cfg_t& config)
                {
                    uint32_t flags = 0;
                    flags_require_config();
                    flags_check(config && config->snap_to_edge);
                    flags_disable(!IsFlowinAlive(config));
                    return flags;
                };
            }

            if (auto node = snap_group->NewNode(MenuCommands::AutoHideWhenSnapped, "吸附时自动隐藏",
                                                 FlowinMenuShowOnAll))
            {
                node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

                node->get_flags = [](const cfg_t& config)
                {
                    uint32_t flags = 0;
                    flags_require_config();
                    flags_check(config && config->auto_hide_when_snapped);
                    flags_disable(!config || !config->snap_to_edge || !IsFlowinAlive(config));
                    return flags;
                };
            }

            if (auto node = snap_group->NewNode(MenuCommands::SnapHide, "隐藏", FlowinMenuShowOnAll))
            {
                node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

                node->get_flags = [](const cfg_t& config)
                {
                    uint32_t flags = 0;
                    flags_require_config();
                    flags_disable(!IsFlowinAlive(config) || config->auto_hide_when_snapped);
                    // flags_default_hidden();
                    return flags;
                };
            }

            if (auto node = snap_group->NewNode(MenuCommands::SnapShow, "显示", FlowinMenuShowOnAll))
            {
                node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

                node->get_flags = [](const cfg_t& config)
                {
                    uint32_t flags = 0;
                    flags_require_config();
                    flags_disable(!IsFlowinAlive(config) || config->auto_hide_when_snapped);
                    // flags_default_hidden();
                    return flags;
                };
            }
        }

        if (auto node = group->NewNode(MenuCommands::ResetPosition, "复位位置", FlowinMenuShowOnMainMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::EditMode, "编辑模式", FlowinMenuShowOnAll))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->edit_mode);
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::ShowOnTaskbar, "在任务栏单独显示", FlowinMenuShowOnSystemMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->show_in_taskbar);
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::CustomTitle, "自定义标题", FlowinMenuShowOnSystemMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };
        }

        if (auto node = group->NewNode(MenuCommands::AutoHideWhenHovered, "鼠标悬停时自动隐藏", FlowinMenuShowOnAll))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_check(config && config->auto_hide_when_hovered);
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::Transparency, "透明度", FlowinMenuShowOnSystemMenu))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };
        }

        if (auto node = group->NewNode(MenuCommands::ShowInfo, "信息", FlowinMenuShowOnFlowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    pfc::string8 msg;
                    msg << "GUID: " << pfc::print_guid(config->guid);
                    popup_message_v2::g_show(core_api::get_main_window(), msg);
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::DestroyFlowin, "删除", FlowinMenuShowOnAll))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        // Export/Import config (Shift+Right-click only)
        if (auto node = group->NewNode(MenuCommands::ExportConfig, "导出配置",
                                        FlowinMenuShowOnSystemMenu | FlowinMenuShowShiftOnly))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::ImportConfig, "导入配置",
                                        FlowinMenuShowOnSystemMenu | FlowinMenuShowShiftOnly))
        {
            node->action = [id = node->id](cfg_t& config) { NotifyFlowinCommand(config, id); };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::ShowAndHideMainWindow, "显示浮窗并隐藏主窗口",
                                        FlowinMenuShowOnFlowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = FlowinCore::Get();
                    if (!core->IsFlowinAlive(config->guid))
                        core->CreateFlowin(config->guid);
                    ui_control::get()->hide();
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                flags_disable(IsFlowinAlive(config));
                return flags;
            };
        }

        if (auto node = group->NewNode(MenuCommands::CloseAndActivateMainWindow,
                                        "关闭浮窗并激活主窗口", FlowinMenuShowOnFlowin))
        {
            node->action = [](cfg_t& config)
            {
                if (config != nullptr)
                {
                    auto core = FlowinCore::Get();
                    if (core->IsFlowinAlive(config->guid))
                        core->PostFlowinMessage(config->guid, WM_CLOSE);
                    ui_control::get()->activate();
                }
            };

            node->get_flags = [](const cfg_t& config)
            {
                uint32_t flags = 0;
                flags_require_config();
                flags_default_hidden();
                flags_disable(!IsFlowinAlive(config));
                return flags;
            };
        }

        shared_nodes = group;
    }

    return shared_nodes;
}

FlowinMenuGroupList BuildFlowinMenuGroups()
{
    FlowinMenuGroupList groups;
    // root
    if (auto root = FlowinMenuGroup::NewGroup(FlowinMenuGroupRoot))
    {
        if (auto node = root->NewNode(MenuCommands::NewFlowin, "新建浮窗"))
        {
            node->action = [](cfg_t&) { FlowinCore::Get()->CreateFlowin(); };
        }

        if (auto node = root->NewNode(MenuCommands::ShowAll, "显示全部"))
        {
            node->action = [](cfg_t&)
            {
                Configuration::ForEach(
                    [](const CfgFlowinHost::Ptr& config)
                    {
                        auto core = FlowinCore::Get();
                        if (core->IsFlowinAlive(config->guid))
                            core->PostFlowinMessage(config->guid, UWM_FLOWIN_ACTIVE);
                        else
                            core->CreateFlowin(config->guid);
                    });
            };

            node->get_flags = [](const cfg_t&)
            {
                uint32_t flags = 0;
                const size_t flowin_count = Configuration::GetCount();
                flags_disable(flowin_count == 0);
                return flags;
            };
        }

        if (auto node = root->NewNode(MenuCommands::CloseAll, "全部关闭"))
        {
            node->action = [](cfg_t&)
            { Configuration::ForEach([](const CfgFlowinHost::Ptr& config) { NotifyFlowin(config, WM_CLOSE); }); };

            node->get_flags = [](const cfg_t&)
            {
                uint32_t flags = 0;
                const size_t flowin_count = Configuration::GetCount();
                flags_disable(flowin_count == 0);
                return flags;
            };

            groups.push_back(root);
        }

        if (auto& shared_group = BuildFlowinMenuNodes())
        {
            auto& shared_nodes = shared_group->nodes;
            // active
            if (auto active = FlowinMenuGroup::NewGroup(FlowinMenuGroupActive))
            {
                // identify
                active->NewNode(MenuCommands::Identify, "", FlowinMenuShowOnActive);
                // separator
                active->NewNode(MenuCommands::Separator, "", FlowinMenuShowOnActive);
                // shared nodes
                for (auto& node : shared_nodes)
                {
                    if (node->show_flags & FlowinMenuShowOnActive)
                        active->nodes.push_back(node);
                }

                groups.push_back(active);
            }
            // available flowins
            Configuration::ForEach(
                [&groups, &shared_nodes](CfgFlowinHost::Ptr& config)
                {
                    if (auto group = FlowinMenuGroup::NewGroup(FlowinMenuGroupLive))
                    {
                        group->config = config;
                        for (auto& node : shared_nodes)
                        {
                            if (node->show_flags & FlowinMenuShowOnFlowin)
                                group->nodes.push_back(node);
                        }

                        groups.push_back(group);
                    }
                });
        }
    }

    return groups;
}
