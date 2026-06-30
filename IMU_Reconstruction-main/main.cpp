#include <GLFW/glfw3.h>
#include <thread>
#include <mutex>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "udp_receiver.h"
#include "sensor_manager.h"
#include "renderer.h"
#include "input_handler.h"
#include "csv_logger.h"
#include "gltf_model.h"

static GLFWwindow* g_window = nullptr;

// q_local_child = inverse(q_world_parent) * q_world_child
static glm::quat localRelativeTo(const glm::quat& worldParent, const glm::quat& worldChild)
{
    return glm::normalize(glm::inverse(worldParent) * worldChild);
}

int main()
{
    SensorManager sensorManager;
    std::thread receiver(udpReceiver, std::ref(sensorManager));

    if(!glfwInit())
        return -1;

    GLFWwindow* window = glfwCreateWindow(1400, 900,
        "IMU Visualizer - L_FA, R_FA, L_UA, R_UA, L_TH, L_SH, R_TH, R_SH", nullptr, nullptr);

    if(!window)
        return -1;

    g_window = window;
    glfwMakeContextCurrent(window);

    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    glfwSetFramebufferSizeCallback(
        window,
        [](GLFWwindow*, int w, int h)
        {
            glViewport(0, 0, w, h);
        }
    );

    Renderer renderer;
    renderer.initialize();

    CsvLogger csvLogger;
    csvLogger.open();

    InputHandler inputHandler(sensorManager, csvLogger, renderer);
    glfwSetKeyCallback(window, keyCallbackDispatcher);
    glfwSetWindowUserPointer(window, &inputHandler);

    while(!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
            renderer.setCameraView(CameraView::FRONT);
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
            renderer.setCameraView(CameraView::BACK);
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
            renderer.setCameraView(CameraView::SIDE);

        // ── Raw calibrated world‑space quaternions (only used internally) ──
        glm::quat correctedLFA   = sensorManager.getCorrectedLFAQuat();
        glm::quat correctedRFA   = sensorManager.getCorrectedRFAQuat();
        glm::quat correctedLUA   = sensorManager.getCorrectedLUAQuat();
        glm::quat correctedRUA   = sensorManager.getCorrectedRUAQuat();
        glm::quat correctedLTH   = sensorManager.getCorrectedLTHQuat();
        glm::quat correctedLSH   = sensorManager.getCorrectedLSHQuat();
        glm::quat correctedRTH   = sensorManager.getCorrectedRTHQuat();
        glm::quat correctedRSH   = sensorManager.getCorrectedRSHQuat();
        glm::quat correctedHips  = sensorManager.getCorrectedHipsQuat();
        glm::quat correctedChest = sensorManager.getCorrectedChestQuat();

        // ── Hierarchical (parent‑relative) quaternions ──
        // Roots (world‑space corrected quats, identity = neutral pose)
        glm::quat hipsWorld  = correctedHips;
        glm::quat chestWorld = correctedChest;

        // Legs are driven against a yaw-only hips reference (pelvis pitch/roll should not swing the legs — only twisting at the waist should).
        glm::quat hipsYawOnlyForLegs = yawOnly(hipsWorld);

        glm::quat localLUA = sensorManager.isLUAActive()
            ? localRelativeTo(chestWorld, correctedLUA)
            : glm::quat(1,0,0,0);

        glm::quat localRUA = sensorManager.isRUAActive()
            ? localRelativeTo(chestWorld, correctedRUA)
            : glm::quat(1,0,0,0);

        glm::quat localLFA = sensorManager.isLFAActive()
            ? localRelativeTo(correctedLUA, correctedLFA)
            : glm::quat(1,0,0,0);

        glm::quat localRFA = sensorManager.isRFAActive()
            ? localRelativeTo(correctedRUA, correctedRFA)
            : glm::quat(1,0,0,0);

        glm::quat localLTH = sensorManager.isLTHActive()
            ? localRelativeTo(hipsYawOnlyForLegs, correctedLTH)
            : glm::quat(1,0,0,0);

        glm::quat localRTH = sensorManager.isRTHActive()
            ? localRelativeTo(hipsYawOnlyForLegs, correctedRTH)
            : glm::quat(1,0,0,0);

        glm::quat localLSH = sensorManager.isLSHActive()
            ? localRelativeTo(correctedLTH, correctedLSH)
            : glm::quat(1,0,0,0);

        glm::quat localRSH = sensorManager.isRSHActive()
            ? localRelativeTo(correctedRTH, correctedRSH)
            : glm::quat(1,0,0,0);

        // Render with hierarchical joint rotations
        renderer.render(localLFA, localRFA, localLUA, localRUA,
                        localLTH, localRTH, localLSH, localRSH,
                        chestWorld, hipsWorld,
                        sensorManager.isPlacementGuideMode());

        // CSV now logs parent‑relative joint quaternions
        std::vector<SensorSample> samples = {
            { "L_FA",  localLFA },
            { "R_FA",  localRFA },
            { "L_UA",  localLUA },
            { "R_UA",  localRUA },
            { "L_TH",  localLTH },
            { "L_SH",  localLSH },
            { "R_TH",  localRTH },
            { "R_SH",  localRSH },
            { "HIPS",  hipsWorld },
            { "CHEST", chestWorld },
        };
        std::vector<bool> active = {
            sensorManager.isLFAActive(),
            sensorManager.isRFAActive(),
            sensorManager.isLUAActive(),
            sensorManager.isRUAActive(),
            sensorManager.isLTHActive(),
            sensorManager.isLSHActive(),
            sensorManager.isRTHActive(),
            sensorManager.isRSHActive(),
            sensorManager.isHipsActive(),
            sensorManager.isChestActive(),
        };
        csvLogger.log(samples, active);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    csvLogger.close();
    receiver.detach();
    glfwTerminate();
    return 0;
}