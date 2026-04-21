#include "my_application.h"
#include <GLFW/glfw3.h>

// Render factories
#include "my_simple_render_factory.h"
#include "my_point_curve_render_factory.h"
#include "my_keyboard_controller.h"

// use radian rather degree for angle
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// Std
#include <stdexcept>
#include <array>
#include <chrono>
#include <iostream>
#include <algorithm>
#include <limits>

struct MyGlobalUBO 
{
    alignas(16) glm::mat4 projectionView{ 1.0f };
    alignas(16) glm::vec4 ambientLightColor{ 1.0f, 1.0f, 1.0f, 0.03f }; // w is intensity
    alignas(16) glm::vec3 lightPosition{ 1.0f, 1.0f, 2.0 };
    alignas(16) glm::vec4 lightColor{ 1.0f, 1.0f, 1.0f, 1.0f };         // w is light intensity
};

MyApplication::MyApplication() :
    m_bPerspectiveProjection(true)
{
    m_pMyGlobalPool =
        MyDescriptorPool::Builder(m_myDevice)
        .setMaxSets(MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MySwapChain::MAX_FRAMES_IN_FLIGHT)
        .build();

    _loadGameObjects();
}

void MyApplication::run() 
{
    std::vector<std::unique_ptr<MyBuffer>> uboBuffers(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < (int)uboBuffers.size(); i++) 
    {
        uboBuffers[i] = std::make_unique<MyBuffer>(
            m_myDevice,
            sizeof(MyGlobalUBO),
            1,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        uboBuffers[i]->map();
    }

    auto globalSetLayout =
        MyDescriptorSetLayout::Builder(m_myDevice)
        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
        .build();

    std::vector<VkDescriptorSet> globalDescriptorSets(MySwapChain::MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < (int)globalDescriptorSets.size(); i++)
	{
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        MyDescriptorWriter(*globalSetLayout, *m_pMyGlobalPool)
            .writeBuffer(0, &bufferInfo)
            .build(globalDescriptorSets[i]);
    }

    MySimpleRenderFactory     simpleRenderFactory{ m_myDevice, m_myRenderer.swapChainRenderPass(), globalSetLayout->descriptorSetLayout() };
    MyPointCurveRenderFactory pointRenderFactory { m_myDevice, m_myRenderer.swapChainRenderPass(), VK_PRIMITIVE_TOPOLOGY_POINT_LIST };
    MyPointCurveRenderFactory lineRenderFactory  { m_myDevice, m_myRenderer.swapChainRenderPass(), VK_PRIMITIVE_TOPOLOGY_LINE_STRIP };
    MyPointCurveRenderFactory normalRenderFactory{ m_myDevice, m_myRenderer.swapChainRenderPass(), VK_PRIMITIVE_TOPOLOGY_LINE_LIST };

    m_myWindow.bindMyApplication(this);

    // Start in edit mode (orthographic, click-to-add-point)
    switchEditMode();

    auto viewerObject = MyGameObject::createGameObject("camera");
    MyKeyboardController cameraController{};
    auto currentTime  = std::chrono::high_resolution_clock::now();

    while (!m_myWindow.shouldClose()) 
    {
        m_myWindow.pollEvents();

        auto  newTime   = std::chrono::high_resolution_clock::now();
        float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
        currentTime     = newTime;


        // -------------------------------------------------------
        // Keyboard camera navigation (works without a mouse)
        // Arrow keys / numpad: rotate  |  Shift+arrows: pan
        // = or KP_ADD: zoom in  |  - or KP_SUBTRACT: zoom out
        // -------------------------------------------------------
        if (!m_bCreateModel)
        {
            GLFWwindow* win = m_myWindow.glfwWindow();
            const float rotSpeed  = frameTime * 1.2f;   // rad/frame
            const float zoomSpeed = frameTime * 0.8f;   // fraction/frame
            const float panSpeed  = frameTime * 0.4f;   // fraction/frame

            bool shiftHeld = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS ||
                              glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

            float rotH = 0.f, rotV = 0.f, zoom = 0.f, panH = 0.f, panV = 0.f;

            // Horizontal rotation or pan
            bool leftHeld  = glfwGetKey(win, GLFW_KEY_LEFT)  == GLFW_PRESS ||
                             glfwGetKey(win, GLFW_KEY_KP_4)  == GLFW_PRESS;
            bool rightHeld = glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS ||
                             glfwGetKey(win, GLFW_KEY_KP_6)  == GLFW_PRESS;
            // Vertical rotation or pan
            bool upHeld    = glfwGetKey(win, GLFW_KEY_UP)    == GLFW_PRESS ||
                             glfwGetKey(win, GLFW_KEY_KP_8)  == GLFW_PRESS;
            bool downHeld  = glfwGetKey(win, GLFW_KEY_DOWN)  == GLFW_PRESS ||
                             glfwGetKey(win, GLFW_KEY_KP_2)  == GLFW_PRESS;
            // Zoom
            bool zoomInHeld  = glfwGetKey(win, GLFW_KEY_EQUAL)       == GLFW_PRESS ||
                               glfwGetKey(win, GLFW_KEY_KP_ADD)       == GLFW_PRESS;
            bool zoomOutHeld = glfwGetKey(win, GLFW_KEY_MINUS)        == GLFW_PRESS ||
                               glfwGetKey(win, GLFW_KEY_KP_SUBTRACT)  == GLFW_PRESS;

            if (shiftHeld)
            {
                if (leftHeld)  panH -= panSpeed;
                if (rightHeld) panH += panSpeed;
                if (upHeld)    panV += panSpeed;
                if (downHeld)  panV -= panSpeed;
            }
            else
            {
                if (leftHeld)  rotH -= rotSpeed;
                if (rightHeld) rotH += rotSpeed;
                if (upHeld)    rotV += rotSpeed;
                if (downHeld)  rotV -= rotSpeed;
            }

            if (zoomInHeld)  zoom += zoomSpeed;
            if (zoomOutHeld) zoom -= zoomSpeed;

            if (rotH || rotV || zoom || panH || panV)
                m_myCamera.keyStep(rotH, rotV, zoom, panH, panV);
        }

        float aspectRatio = m_myRenderer.aspectRatio();

        if (m_bPerspectiveProjection)
            m_myCamera.setPerspectiveProjection(glm::radians(50.f), aspectRatio, 0.1f, 100.f);
        else
            m_myCamera.setOrthographicProjection(-aspectRatio, aspectRatio, -1.0f, 1.0f, -100.0f, 100.0f);

        if (auto commandBuffer = m_myRenderer.beginFrame())
		{
            int frameIndex = m_myRenderer.frameIndex();
            MyFrameInfo frameInfo{
                frameIndex, frameTime, commandBuffer,
                m_myCamera, globalDescriptorSets[frameIndex],
                glm::vec3(1.0f, 1.0f, 1.0f) };
			
            MyGlobalUBO ubo{};
            ubo.projectionView = m_myCamera.projectionMatrix() * m_myCamera.viewMatrix();
            uboBuffers[frameIndex]->writeToBuffer(&ubo);
            uboBuffers[frameIndex]->flush();

            m_myRenderer.beginSwapChainRenderPass(commandBuffer);

            // Render revolution surface (lit, with color)
            simpleRenderFactory.renderGameObjects(frameInfo, m_vMyGameObjects);

            // Control points (red dots)
            frameInfo.color = glm::vec3(1.0f, 0.0f, 0.0f);
            pointRenderFactory.renderControlPoints(frameInfo, m_vMyGameObjects);

            // Control polygon (yellow line strip)
            frameInfo.color = glm::vec3(1.0f, 1.0f, 0.0f);
            lineRenderFactory.renderControlPoints(frameInfo, m_vMyGameObjects);

            // Center / axis of revolution (blue)
            frameInfo.color = glm::vec3(0.0f, 0.0f, 1.0f);
            lineRenderFactory.renderCenterLine(frameInfo, m_vMyGameObjects);

            // Bezier profile curve (white)
            frameInfo.color = glm::vec3(1.0f, 1.0f, 1.0f);
            lineRenderFactory.renderBezierCurve(frameInfo, m_vMyGameObjects);

            // Phase 3: normal vectors (green) — toggle with N
            if (m_bShowNormals)
			{
                frameInfo.color = glm::vec3(0.0f, 1.0f, 0.0f);
                normalRenderFactory.renderNormals(frameInfo, m_vMyGameObjects);
            }

            m_myRenderer.endSwapChainRenderPass(commandBuffer);
            m_myRenderer.endFrame();
        }
    }

    vkDeviceWaitIdle(m_myDevice.device());
}

// --------------------------------------------------------------------------
// Phase 1 helpers: add control points on mouse click
// --------------------------------------------------------------------------

void MyApplication::switchProjectionMatrix()
{
    m_bPerspectiveProjection = !m_bPerspectiveProjection;
}

void MyApplication::_loadGameObjects()
{
    m_pMyBezier = std::make_shared<MyBezier>();

    // Rough scene bounds (updated after surface generation)
    glm::vec3 min{ -1.0f, -1.0f, -1.0f }, max{ 1.0f, 1.0f, 1.0f };
    m_myCamera.setSceneMinMax(min, max);

    // --- control_points: up to 100 click-placed points ---
    auto mypontLine = std::make_shared<MyModel>(m_myDevice, 100);
    auto mygameobj1 = MyGameObject::createGameObject("control_points");
    mygameobj1.model = mypontLine;
    mygameobj1.color = glm::vec3(1.0f, 0.0f, 0.0f);
    m_vMyGameObjects.push_back(std::move(mygameobj1));

    // --- center_line: X axis indicator (2 points) ---
    auto mycenterLine = std::make_shared<MyModel>(m_myDevice, 2);
    auto mygameobj2   = MyGameObject::createGameObject("center_line");
    mygameobj2.model  = mycenterLine;
    mygameobj2.color  = glm::vec3(0.0f, 0.0f, 1.0f);

    std::vector<MyModel::PointCurve> centerLine(2);
    centerLine[0].position = glm::vec3(-1.0f, 0.0f, 0.0f);
    centerLine[0].color    = glm::vec3(0.0f, 0.0f, 1.0f);
    centerLine[1].position = glm::vec3( 1.0f, 0.0f, 0.0f);
    centerLine[1].color    = glm::vec3(0.0f, 0.0f, 1.0f);
    mycenterLine->updatePointCurves(centerLine);
    m_vMyGameObjects.push_back(std::move(mygameobj2));

    // --- bezier_curve: evaluated curve (up to 1000 points) ---
    auto mybezierCurve = std::make_shared<MyModel>(m_myDevice, 1000);
    auto mygameobj3    = MyGameObject::createGameObject("bezier_curve");
    mygameobj3.model   = mybezierCurve;
    mygameobj3.color   = glm::vec3(1.0f, 1.0f, 1.0f);
    m_vMyGameObjects.push_back(std::move(mygameobj3));

    // --- bezier_surface: created on demand ---
    auto mygameobj4 = MyGameObject::createGameObject("bezier_surface");
    mygameobj4.color = glm::vec3(1.0f, 1.0f, 1.0f);
    m_vMyGameObjects.push_back(std::move(mygameobj4));

    // --- surface_normals: created on demand ---
    auto mygameobj5 = MyGameObject::createGameObject("surface_normals");
    mygameobj5.color = glm::vec3(0.0f, 1.0f, 0.0f);
    m_vMyGameObjects.push_back(std::move(mygameobj5));
}

// --------------------------------------------------------------------------
// Phase 1: Mouse events — add control points in edit mode
// --------------------------------------------------------------------------

void MyApplication::mouseButtonEvent(bool bMouseDown, float posx, float posy)
{
    if (m_bCreateModel && bMouseDown)
    {
        // Reject clicks below the axis (lower half)
        if (posy >= 0.5f)
            return;

        // Convert (0,1) window coords -> (-1,1) NDC, flip Y
        float x =  2.0f * posx - 1.0f;
        float y = -2.0f * posy + 1.0f;

        std::cout << "Add control point ( " << x << " , " << y << " )" << std::endl;

        m_pMyBezier->addControlPoint(x, y);

        MyModel::PointCurve cp;
        cp.position = glm::vec3(x, y, 0.0f);
        cp.color    = glm::vec3(1.0f, 0.0f, 1.0f);
        m_vControlPointVertices.push_back(cp);

        if (m_vControlPointVertices.size() >= 100)
            return;

        // Evaluate Bezier once we have at least 3 points
        if (m_vControlPointVertices.size() >= 2)
            m_pMyBezier->createBezierCurve(200);

        // Update GPU buffers
        for (auto& obj : m_vMyGameObjects)
        {
            if (obj.name() == "control_points")
                obj.model->updatePointCurves(m_vControlPointVertices);
            else if (obj.name() == "bezier_curve")
                obj.model->updatePointCurves(m_pMyBezier->m_vCurve);
        }
    }
    else // camera navigation mode
    {
        m_bMouseButtonPress = bMouseDown;
        m_myCamera.setButton(m_bMouseButtonPress, posx, posy);
    }
}

void MyApplication::mouseMotionEvent(float posx, float posy)
{
    if (!m_bCreateModel)
        m_myCamera.setMotion(m_bMouseButtonPress, posx, posy);
}

void MyApplication::setCameraNavigationMode(MyCamera::MyCameraMode mode)
{
    m_myCamera.setMode(mode);   
}

// --------------------------------------------------------------------------
// Phase 4: Switch between edit mode and camera navigation mode
// --------------------------------------------------------------------------

void MyApplication::switchEditMode()
{
    m_bCreateModel       = !m_bCreateModel;
    m_bPerspectiveProjection = !m_bCreateModel;

    if (m_bPerspectiveProjection)
    {
        std::cout << "Switch to Navigation Mode  [R=rotate  P=pan  Z=zoom  F=fit-all]" << std::endl;
        setCameraNavigationMode(MyCamera::MYCAMERA_FITALL);
    }
    else
    {
        std::cout << "Switch to Edit Mode  [left-click to add control points]" << std::endl;
        m_myCamera.setViewYXZ(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    }
}

// --------------------------------------------------------------------------
// Phase 4: Reset — clear curve, surface, and all GPU data
// --------------------------------------------------------------------------

void MyApplication::resetSurface()
{
    std::cout << "Reset: clearing curve and surface." << std::endl;

    // Wait for GPU to be idle before releasing GPU memory
    m_myDevice.waitIdle();

    // Reset CPU data
    m_pMyBezier = std::make_shared<MyBezier>();
    m_vControlPointVertices.clear();
    m_vNormalVectors.clear();
    m_bSurfaceExists = false;

    // Clear GPU buffers for points/curve (set vertex count to 0)
    std::vector<MyModel::PointCurve> empty;
    for (auto& obj : m_vMyGameObjects)
    {
        if (obj.name() == "control_points" || obj.name() == "bezier_curve")
        {
            obj.model->updatePointCurves(empty);
        }
        else if (obj.name() == "bezier_surface" || obj.name() == "surface_normals")
        {
            obj.model = nullptr; // render factory will skip nullptr
        }
    }
}

// --------------------------------------------------------------------------
// Phase 3: Toggle normal vector visibility
// --------------------------------------------------------------------------

void MyApplication::showHideNormalVectors()
{
    m_bShowNormals = !m_bShowNormals;
    std::cout << (m_bShowNormals ? "Show" : "Hide") << " Normal Vectors" << std::endl;
}

// --------------------------------------------------------------------------
// Phase 2 & 3: Build the revolution surface and upload to GPU
// --------------------------------------------------------------------------

void MyApplication::createBezierRevolutionSurface()
{
    if (m_pMyBezier->numberOfControlPoints() < 2)
    {
        std::cout << "Need at least 2 control points to build a surface." << std::endl;
        return;
    }

    std::cout << "Creating revolution surface ("
              << m_iXResolution << " x " << m_iRResolution << ")..." << std::endl;

    m_vNormalVectors.clear();

    // Step 1: Generate surface vertices, normals, and indices
    m_pMyBezier->createRevolutionSurface(m_iXResolution, m_iRResolution);

    // Step 2: Build the surface GPU model
    MyModel::Builder builder;
    builder.indices  = m_pMyBezier->m_vIndices;
    builder.vertices = m_pMyBezier->m_vSurface;

    std::shared_ptr<MyModel> mysurface = std::make_shared<MyModel>(m_myDevice, builder);

    // Step 3: Build the normal-vector line model (two PointCurve verts per surface vertex)
    int nvertices = (int)m_pMyBezier->m_vSurface.size();
    m_vNormalVectors.resize(nvertices * 2);

    const float normalScale = 0.05f; // length of displayed normal arrow

    for (int ii = 0; ii < nvertices; ii++)
    {
        const auto& sv = m_pMyBezier->m_vSurface[ii];
        m_vNormalVectors[ii * 2 + 0].position = sv.position;
        m_vNormalVectors[ii * 2 + 0].color    = glm::vec3(0.0f, 1.0f, 0.0f);
        m_vNormalVectors[ii * 2 + 1].position = sv.position + sv.normal * normalScale;
        m_vNormalVectors[ii * 2 + 1].color    = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    auto mynormals = std::make_shared<MyModel>(m_myDevice, nvertices * 2);
    mynormals->updatePointCurves(m_vNormalVectors);

    // Update game objects
    for (auto& obj : m_vMyGameObjects)
    {
        if (obj.name() == "bezier_surface")
            obj.model = mysurface;
        else if (obj.name() == "surface_normals")
            obj.model = mynormals;
    }

    m_bSurfaceExists = true;

    // Step 4: Update camera scene bounds from surface extents
    glm::vec3 bmin( std::numeric_limits<float>::max());
    glm::vec3 bmax(-std::numeric_limits<float>::max());
    for (const auto& v : m_pMyBezier->m_vSurface)
    {
        bmin = glm::min(bmin, v.position);
        bmax = glm::max(bmax, v.position);
    }
    // Pad slightly
    glm::vec3 pad = (bmax - bmin) * 0.1f + glm::vec3(0.01f);
    m_myCamera.setSceneMinMax(bmin - pad, bmax + pad);

    std::cout << "Surface created: " << nvertices << " vertices, "
              << m_pMyBezier->m_vIndices.size() / 3 << " triangles." << std::endl;
}

// --------------------------------------------------------------------------
// Phase 5: Creativity — Twist toggle and Color Gradient toggle
// --------------------------------------------------------------------------

void MyApplication::toggleTwist()
{
    // Toggle between no twist (0) and a full 360-degree helical twist (2*PI)
    float current = m_pMyBezier->getTwistAngle();
    float next    = (current < 0.01f) ? glm::two_pi<float>() : 0.0f;
    m_pMyBezier->setTwistAngle(next);

    std::cout << "Twist " << (next > 0.0f ? "ON (360 deg)" : "OFF") << std::endl;

    // Immediately rebuild if a surface already exists
    if (m_bSurfaceExists)
        createBezierRevolutionSurface();
}

void MyApplication::toggleColorGradient()
{
    bool next = !m_pMyBezier->getColorGradient();
    m_pMyBezier->setColorGradient(next);

    std::cout << "Color gradient " << (next ? "ON" : "OFF") << std::endl;

    if (m_bSurfaceExists)
        createBezierRevolutionSurface();
}

void MyApplication::_rebuildSurface()
{
    if (m_bSurfaceExists)
        createBezierRevolutionSurface();
}
