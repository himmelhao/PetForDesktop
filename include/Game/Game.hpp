#pragma once

#include "Game/GameData.hpp"
#include "Game/Pet.hpp"

#include "Engine/Framebuffer.hpp"
#include "Engine/Log.hpp"
#include "Engine/PhysicSystem.hpp"
#include "Engine/ScreenSpaceQuad.hpp"
#include "Engine/Settings.hpp"
#include "Engine/Shader.hpp"
#include "Engine/SpriteSheet.hpp"
#include "Engine/Texture.hpp"
#include "Engine/TimeManager.hpp"
#include "Engine/Utilities.hpp"
#include "Engine/Vector2.hpp"
#include "Engine/Window.hpp"
#include "Engine/Updater.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <functional>

class Game
{
protected:
    Updater      updater;
    GameData     datas;
    Setting      setting;
    TimeManager  mainLoop;
    PhysicSystem physicSystem;

protected:
    void initWindow()
    {
        // initialize the library
        if (!glfwInit())
            errorAndExit("glfw initialization error");

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
#ifdef _DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, !datas.showFrameBufferBackground);
        glfwWindowHint(GLFW_VISIBLE, datas.showFrameBufferBackground);
        glfwWindowHint(GLFW_FLOATING, datas.useForwardWindow);

        // Disable depth and stencil buffers
        glfwWindowHint(GLFW_DEPTH_BITS, 0);
        glfwWindowHint(GLFW_STENCIL_BITS, 0);

        glfwSetMonitorCallback(setMonitorCallback);
        datas.monitors.init();
        Vec2i monitorSize = datas.monitors.getMonitorsSize();
        Vec2i monitorsSizeMM = datas.monitors.getMonitorPhysicalSize();

        datas.petPosLimit = monitorSize;
        datas.windowSize  = {1, 1};

        // Evaluate pixel distance based on dpi and monitor size
        datas.pixelPerMeter = {(float)monitorSize.x / (monitorsSizeMM.x * 0.001f),
                               (float)monitorSize.y / (monitorsSizeMM.y * 0.001f)};

        datas.window = glfwCreateWindow(datas.windowSize.x, datas.windowSize.y, PROJECT_NAME, NULL, NULL);
        if (!datas.window)
        {
            glfwTerminate();
            errorAndExit("Create Window error");
        }

        glfwMakeContextCurrent(datas.window);

        glfwSetWindowAttrib(datas.window, GLFW_DECORATED, datas.showWindow);
        glfwSetWindowAttrib(datas.window, GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
        glfwDefaultWindowHints();
    }

    void initOpenGL()
    {
        // glad: load all OpenGL function pointers
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
            errorAndExit("Failed to initialize OpenGL (GLAD)");
    }

    void createResources()
    {
        datas.pFramebuffer = std::make_unique<Framebuffer>();
        datas.edgeDetectionShaders.emplace_back(RESOURCE_PATH "shader/image.vs",
                                                RESOURCE_PATH "shader/dFdxEdgeDetection.fs");

        datas.pImageShader = std::make_unique<Shader>(RESOURCE_PATH "shader/image.vs", RESOURCE_PATH "shader/image.fs");

        if (datas.debugEdgeDetection)
            datas.pImageGreyScale =
                std::make_unique<Shader>(RESOURCE_PATH "shader/image.vs", RESOURCE_PATH "shader/imageGreyScale.fs");

        datas.pSpriteSheetShader =
            std::make_unique<Shader>(RESOURCE_PATH "shader/spriteSheet.vs", RESOURCE_PATH "shader/image.fs");

        datas.pUnitFullScreenQuad = std::make_unique<ScreenSpaceQuad>(0.f, 1.f);
        datas.pFullScreenQuad     = std::make_unique<ScreenSpaceQuad>(-1.f, 1.f);

#ifdef _DEBUG
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(GLDebugMessageCallback, NULL);
#endif
    }

public:
    Game() : setting(RESOURCE_PATH "setting/setting.yaml", datas), mainLoop(datas), physicSystem(datas)
    {
        logf("%s %s\n", PROJECT_NAME, PROJECT_VERSION);
        initWindow();
        initOpenGL();

        createResources();

        Vec2i mainMonitorPosition;
        Vec2i mainMonitorSize;
        datas.monitors.getMainMonitorWorkingArea(mainMonitorPosition, mainMonitorSize);

        datas.windowPos = mainMonitorPosition + mainMonitorSize / 2;
        datas.petPos = datas.windowPos;
        glfwSetWindowPos(datas.window, datas.windowPos.x, datas.windowPos.y);

        glfwShowWindow(datas.window);

        glfwSetWindowUserPointer(datas.window, &datas);

        glfwSetMouseButtonCallback(datas.window, mousButtonCallBack);
        glfwSetCursorPosCallback(datas.window, cursorPositionCallback);

        srand(datas.randomSeed == -1 ? (unsigned)time(nullptr) : datas.randomSeed);
    }

    void initDrawContext()
    {
        Framebuffer::bindScreen();
        glViewport(0, 0, datas.windowSize.x, datas.windowSize.y);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_CULL_FACE);
    }

    ~Game()
    {
        glfwTerminate();
    }

    void run()
    {
        Pet pet(datas);

        const std::function<void(double)> unlimitedUpdate{[&](double deltaTime) {
            processInput(datas.window);

            if (datas.useMousePassThoughWindow)
                glfwSetWindowAttrib(datas.window, GLFW_MOUSE_PASSTHROUGH, !pet.isMouseOver());

            // poll for and process events
            glfwPollEvents();
        }};
        const std::function<void(double)> limitedUpdate{[&](double deltaTime) {
            pet.update(deltaTime);

            if (datas.shouldUpdateFrame)
            {
                // render
                initDrawContext();

                pet.draw();

                // swap front and back buffers
                glfwSwapBuffers(datas.window);
                datas.shouldUpdateFrame = false;
            }
        }};

        const std::function<void(double)> limitedUpdateDebugCollision{[&](double deltaTime) {
            // fullscreen
            Vec2i monitorSize  = datas.monitors.getMonitorsSize();
            datas.windowSize  = monitorSize;
            datas.petPosLimit  = {monitorSize.x - datas.windowSize.x, monitorSize.y - datas.windowSize.y};
            glfwSetWindowSize(datas.window, datas.windowSize.x, datas.windowSize.y);

            // render
            initDrawContext();

            if (datas.pImageGreyScale && datas.pEdgeDetectionTexture && datas.pFullScreenQuad)
            {
                datas.pImageGreyScale->use();
                datas.pImageGreyScale->setInt("uTexture", 0);
                datas.pFullScreenQuad->use();
                datas.pEdgeDetectionTexture->use();
                datas.pFullScreenQuad->draw();
            }

            // swap front and back buffers
            glfwSwapBuffers(datas.window);
        }};

        mainLoop.emplaceTimer([&]() { physicSystem.update(1.f / datas.physicFrameRate); }, 1.f / datas.physicFrameRate,
                              true);

        mainLoop.start();
        while (!glfwWindowShouldClose(datas.window))
        {
            mainLoop.update(unlimitedUpdate, datas.debugEdgeDetection ? limitedUpdateDebugCollision : limitedUpdate);
        }
    }
};