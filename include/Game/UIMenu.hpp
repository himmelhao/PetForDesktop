#pragma once

#include "Engine/ClassUtility.hpp"
#include "Engine/InteractionComponent.hpp"
#include "Engine/Rect.hpp"

#include "Game/GameData.hpp"

#include <string>

class UIMenu : public Rect
{
protected:
    GameData& datas;
    bool      shouldClose        = false;
    bool      shouldInitPosition = false;

    InteractionComponent interactionComponent;
    Vec2                 prevPos;

protected:
    void windowBegin();

    void windowEnd();

    void textCentered(std::string text);

public:
    GETTER_BY_VALUE(ShouldClose, shouldClose)

    UIMenu(GameData& inDatas, Vec2 inPosition, Vec2 inSize);

    virtual ~UIMenu();

};
