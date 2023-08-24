#include "Game/ContextualMenu.hpp"

#include "Game/Pet.hpp"
#include "imgui.h"

#include <cpr/cpr.h>
#include <format>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#define SystemOpenURL(url) system("start " url);
#elif __APPLE__
#define SystemOpenURL(url) system("open " url);
#elif __linux__
#define SystemOpenURL(url) system("xdg-open" url);
#else
#error "Unknown compiler"
#endif

ContextualMenu::ContextualMenu(GameData& data, Vec2 position) : datas{data}, interactionComponent{*this}
{
    data.window->addElement(*this);
    data.interactionSystem->addComponent(interactionComponent);
    m_size = {75.f * data.scale, 150.f * data.scale};
    m_position = position - m_size;
    onChange();
}

ContextualMenu ::~ContextualMenu()
{
    datas.interactionSystem->removeComponent(interactionComponent);
    datas.window->removeElement(*this);
}


void ContextualMenu::update(double deltaTime)
{
    ImGui::SetNextWindowPos(
        ImVec2(m_position.x - datas.window->getPosition().x, m_position.y - datas.window->getPosition().y));
    ImGui::SetNextWindowSize(ImVec2(m_size.x, m_size.y));
    ImGui::GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);
    ImGui::Begin("Contextual menu", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove);

    ImVec2 sizeCenter = ImVec2(ImGui::GetContentRegionAvail().x, 0.0f);

    if (ImGui::Button("Spawn pet", sizeCenter))
    {
        datas.pets.emplace_back(std::make_shared<Pet>(datas));
        shouldClose = true;
    }

    if (ImGui::Button("Sleep", sizeCenter))
    {
        shouldClose = true;
    }

    if (ImGui::Button("Settings", sizeCenter))
    {
        shouldClose = true;
    }

    ImGui::Separator();

    if (ImGui::Button("Join us!", sizeCenter))
    {
        SystemOpenURL("https://discord.gg/gjdQmHAp7e")
        shouldClose = true;
    }

    if (ImGui::Button("Support this project", sizeCenter))
    {
        SystemOpenURL("https://www.patreon.com/PetForDesktop")
        shouldClose = true;
    }

    if (ImGui::Button("Bug report", sizeCenter))
    {
        SystemOpenURL("https://github.com/Renardjojo/PetForDesktop/issues/new/choose")
        shouldClose = true;
    }

    textCentered(std::format("{:.1f} FPS", ImGui::GetIO().Framerate));
    ImVec2 contentSize = ImGui::GetWindowSize();
    ImGui::End();
    setSize({contentSize.x, contentSize.y});
}

void ContextualMenu::textCentered(std::string text)
{
    auto windowWidth = ImGui::GetWindowSize().x;
    auto textWidth   = ImGui::CalcTextSize(text.c_str()).x;

    ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
    ImGui::Text(text.c_str());
}
