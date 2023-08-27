#include "Game/SettingMenu.hpp"

#include "Engine/FileExplorer.hpp"
#include "Engine/InteractionSystem.hpp"
#include "Engine/StylePanel.hpp"
#include "Engine/Settings.hpp"

#include "Game/Pet.hpp"
#include "imgui.h"

SettingMenu::SettingMenu(GameData& inDatas, Pet& inPet, Vec2 inPosition)
    : datas{inDatas}, pet{inPet}, interactionComponent{*this}
{
    datas.window->addElement(*this);
    datas.interactionSystem->addComponent(interactionComponent);
    m_size     = {400.f * datas.scale, 400.f * datas.scale};
    m_position = inPosition;
    onChange();

    shouldInitPosition = true;
}

SettingMenu::~SettingMenu()
{
    datas.interactionSystem->removeComponent(interactionComponent);
    datas.window->removeElement(*this);
}

void SettingMenu::update(double deltaTime)
{
    if (shouldInitPosition)
    {
        ImGui::SetNextWindowPos(
            ImVec2(m_position.x - datas.window->getPosition().x, m_position.y - datas.window->getPosition().y));
        shouldInitPosition = false;
    }

    // ImGui::SetNextWindowSize(ImVec2(m_size.x, m_size.y));

    bool isWindowOpen = true;
    ImGui::Begin("Settings", &isWindowOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    shouldClose = !isWindowOpen;

    if (datas.leftButtonEvent == GLFW_PRESS && !interactionComponent.isLeftSelected)
        shouldClose = true;

    if (ImGui::BeginTabBar("##settingMenuTabs", ImGuiTabBarFlags_None))
    {
        if (ImGui::BeginTabItem("Game"))
        {
            if (ImGui::BeginCombo("Style", datas.styleName.c_str()))
            {
                for (int i = 0; i < datas.stylesPath.size(); i++)
                {
                    auto& path = datas.stylesPath[i];
                    auto  fileName = path.stem();
                    if (ImGui::Selectable(fileName.string().c_str(), fileName == datas.styleName))
                    {
                        ImGuiLoadStyle(path.string().c_str(), ImGui::GetStyle());
                        datas.styleName = fileName.string();
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Content"))
        {
            if (ImGui::Button("Open content folder"))
                SystemOpen(RESOURCE_PATH);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Style"))
        {
            ShowStyleEditor();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Accessibility"))
        {
            ImGui::DragInt("Global scale", &datas.scale, 0.05f, 1, 10);
            ImGui::DragFloat("Font scale", &ImGui::GetFont()->Scale, 0.005f, 0.3f, 2.0f, "%.1f");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    const char* applyTxt   = "Apply";
    ImVec2      buttonSize = ImGui::CalcTextSize(applyTxt, NULL, true) + ImGui::GetStyle().FramePadding * 2.0f;
    ImGui::SetCursorPos(ImGui::GetWindowContentRegionMax() - buttonSize);
    if (ImGui::Button(applyTxt))
    {
        Setting::instance().exportFile(RESOURCE_PATH "/setting/setting.yaml", datas);
    }

    ImVec2 contentSize = ImGui::GetWindowSize();
    ImVec2 contentPos  = ImGui::GetWindowPos();
    ImGui::End();

    setPositionSize({datas.window->getPosition().x + contentPos.x, datas.window->getPosition().y + contentPos.y},
                    {contentSize.x, contentSize.y});
}