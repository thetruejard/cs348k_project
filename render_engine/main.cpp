#if true

#include "assets/assets.h"
#include "components/motion.h"
#include "components/keyboardcontroller.h"
#include "components/mouserotation.h"
#include "core/renderengine.h"
#include "geometry/sphere.h"
#include "objects/go_camera.h"
#include "objects/go_mesh.h"
#include "utils/printutils.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;


#include <iostream>
#include <vector>
#include <string>
#include <fstream>


RenderEngine engine;


void setupDemoScene(Scene* scene) {

    scene->backgroundColor = 0.1f * glm::vec3(0.5f, 0.6f, 1.0f);

    std::cout << "Loading scene\n";
    Ref<GameObject> object = Assets::importObject(engine, "./samples/assets/Sponza/Sponza_smol.gltf");
    if (!object) {
        std::cout << "Failed to load scene\n";
        exit(0);
    }
    std::cout << "Done loading scene\n";
    scene->addObject(object);


    // Prepare the camera & controls.
    Ref<GameObject> controller = engine.createObject<GameObject>();
    controller->addComponent<Motion>();
    controller->addComponent<KeyboardController>();
    controller->addComponent<MouseRotation>();
    controller->addComponent<ApplyMotion>();

    // Make the camera appear third-person-ish by moving it backwards relative to the controller.
    Ref<GO_Camera> camera = engine.createObject<GO_Camera>();
    camera->setPosition(0.0f, 0.0f, 1.0f);
    camera->setPerspective(glm::radians(70.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    camera->setParent(controller, false);
    scene->addObject(controller);
    scene->setActiveCamera(camera);


    std::cout << "Dimming the lights\n";
    // Dim the lights, since blender exports them super bright.
    // Also give random colors, just for fun.
    auto random = []() { return float(rand()) / float(RAND_MAX); };
    std::function<void(Ref<GameObject>)> dim_the_lights = [&dim_the_lights, &random](Ref<GameObject> root) {
        if (root->getTypeName() == "Light") {
            constexpr float brightness = 0.0001f;
            auto L = root.cast<GO_Light>();
            L->color *= brightness;
            if (L->type == GO_Light::Type::Directional) {
                L->color *= 0.0f;
            }
            std::cout << "LIGHT: " << root->getName() << " | ";
            Utils::Print::vec3(root.cast<GO_Light>()->color);
        }
        for (auto child : root->getChildren()) {
            dim_the_lights(child);
        }
    };
    dim_the_lights(object);


    size_t num_lights = 50;
    Ref<Mesh> sphere = engine.createMesh();
    Sphere(0.1f).toMesh(sphere, 12, 8);
    sphere->uploadMesh();
    for (size_t i = 0; i < num_lights; i++) {
        Ref<GO_Light> light = engine.createObject<GO_Light>();
        Ref<GO_Mesh> mesh = engine.createObject<GO_Mesh>();
        mesh->assignMesh(sphere);
        mesh->setParent(light, false);
        glm::vec3 pos = glm::vec3(20.0f * (2.0f * random() - 1.0f), 10.0f * random(), 20.0f * (2.0f * random() - 1.0f));
        light->setPosition(pos);
        glm::vec3 color = glm::normalize(glm::vec3(random(), random(), random()));
        light->color = 0.5f * color;
        scene->addObject(light);
    }


    std::cout << "Scene graph:\n";
    Utils::Print::objectTree(scene->getRoot().get());

}

void argsError() {
    std::cout << "Args error\n";
    exit(1);
}

int main(int argc, char* argv[]) {

    RenderPipelineType pipeline = RenderPipelineType::Deferred;
    std::string pipeline_name = "deferred-none";
    size_t lights = 100;
    std::filesystem::path log_file;
    bool interactive = false;

    // Parse arguments
    std::vector<std::string> args;
    args.reserve(argc);
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    for (int i = 0; i < args.size(); i++) {
        if (args[i] == "--lights") {
            if (i == args.size()-1)
                argsError();
            lights = std::stoi(args[++i]);
        }
        else if (args[i] == "--pipeline") {
            if (++i == args.size())
                argsError();
            if (args[i] == "deferred-none")
                pipeline = RenderPipelineType::Deferred;
            else if (args[i] == "deferred-sphere")
                pipeline = RenderPipelineType::Deferred;
            else if (args[i] == "forward-none")
                pipeline = RenderPipelineType::Deferred;
            else if (args[i] == "forward-tile")
                pipeline = RenderPipelineType::Deferred;
            else if (args[i] == "forward-cluster")
                pipeline = RenderPipelineType::Deferred;
            else argsError();
            pipeline_name = args[i];
        }
        else if (args[i] == "--log-file") {
            if (++i == args.size())
                argsError();
            log_file = args[i];
        }
        else if (args[i] == "--interactive" || args[i] == "-I") {
            interactive = true;
        }
        else {
            argsError();
        }
    }

    std::cout << "lights: " << lights << "\n";
    std::cout << "pipeline: " << pipeline_name << "\n";


    engine.getGraphics()->setRenderPipeline(pipeline);
    Ref<Scene> scene = engine.createScene();
    setupDemoScene(scene.get());
    engine.setActiveScene(scene);

    if (interactive) {
        engine.launch("test", 1920, 1080, false);
    }
    else {
        // Load camera path
        std::ifstream campath_file("samples/assets/ian/cam_trajectory.json");
        json campath = json::parse(campath_file);
        std::vector<float> cam_mats;
        size_t num_cam_mats = campath.size();
        cam_mats.reserve(16 * campath.size());
        for (int i = 0; i < campath.size(); i++) {
            auto& m = campath[i];
            if (m.size() != 16) {
                std::cout << "TRAJECTORY MATRIX SIZE NOT 16\n";
                exit(1);
            }
            cam_mats.insert(cam_mats.end(), m.begin(), m.end());
        }

        json result = engine.launch_eval("Eval", 1280, 720, false, (glm::mat4*)cam_mats.data(), num_cam_mats, !log_file.empty());

        if (!log_file.empty()) {
            std::ofstream log_out(log_file);
            //    log_out << result.dump();
        }
    }

    
    return 0;
}


#endif
