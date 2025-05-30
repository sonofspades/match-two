#include <core/file.hpp>

#include <opengl/buffer.hpp>
#include <opengl/commands.hpp>
#include <opengl/functions.hpp>
#include <opengl/pipeline.hpp>
#include <opengl/shader.hpp>
#include <opengl/vertex_array.hpp>

#include <opengl/constants/buffer.hpp>
#include <opengl/constants/commands.hpp>
#include <opengl/constants/pipeline.hpp>
#include <opengl/constants/shader.hpp>

#include <shaders/converter.hpp>

#include "physics_debug.hpp"

btCollisionWorld* world;

glm::mat4 view;
glm::mat4 proj;

uint32_t cards[4][13] { };

auto main() -> int32_t
{
    shaders::Converter::convert("../../resources/shaders", "./");

    constexpr auto window_width  = 1920;
    constexpr auto window_height = 1080;
    static    auto window_closed = false;

    static auto cursor_x = 0.0f;
    static auto cursor_y = 0.0f;

    if (glfwInit() != GLFW_TRUE)
    {
        return -1;
    }

    const auto window = glfwCreateWindow(window_width, window_height, "Match Two", nullptr);

    glfwSetWindowCloseCallback(window, []
    {
        window_closed = true;
    });

    glfwSetCursorPosCallback(window, [](const double x, const double y) -> void
    {
        cursor_x = x;
        cursor_y = y;
    });

    glfwSetMouseButtonCallback(window, [](const int button, const int action, const int) -> void
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            constexpr glm::vec4 viewport { 0.0f, 0.0f, window_width, window_height };

            auto start = glm::unProject(glm::vec3(cursor_x, window_height - cursor_y, -1.0f), view, proj, viewport);
            auto end   = glm::unProject(glm::vec3(cursor_x, window_height - cursor_y,  1.0f), view, proj, viewport);
                 end   = start + glm::normalize(end - start) * 1000.0f;

            const btVector3 from(start.x, start.y, start.z);
            const btVector3   to(  end.x,   end.y,   end.z);

            btCollisionWorld::ClosestRayResultCallback result(from, to);
                                               world->rayTest(from, to, result);

            if (result.hasHit())
            {
                const auto row = result.m_collisionObject->getUserIndex();
                const auto col = result.m_collisionObject->getUserIndex2();

                cards[row][col] = 1;
            }
        }
    });

    glfwMakeContextCurrent(window);

    opengl::Functions::init();

    opengl::ShaderStage base_shader_vert;
    base_shader_vert.type(opengl::constants::vertex_shader);
    base_shader_vert.create();
    base_shader_vert.source(core::File::read("default_base_shader.vert", std::ios::binary));

    opengl::ShaderStage base_shader_frag;
    base_shader_frag.type(opengl::constants::fragment_shader);
    base_shader_frag.create();
    base_shader_frag.source(core::File::read("default_base_shader.frag", std::ios::binary));

    opengl::Shader base_shader;
    base_shader.create();
    base_shader.attach(base_shader_vert);
    base_shader.attach(base_shader_frag);
    base_shader.link();

    Assimp::Importer card_importer;

    std::vector<glm::vec3> card_vertices;
    std::vector<uint32_t>  card_elements;

    const auto card_scene = card_importer.ReadFile("card.obj", 0);
    const auto card_mesh  = card_scene->mMeshes[0];

    for (auto i = 0; i < card_mesh->mNumVertices; i++)
    {
        const auto& vertex = card_mesh->mVertices[i];

        card_vertices.emplace_back(vertex.x, vertex.y, vertex.z);
    }

    for (auto i = 0; i < card_mesh->mNumFaces; i++)
    {
        const auto& face = card_mesh->mFaces[i];

        for (auto j = 0; j < face.mNumIndices; j++)
        {
            card_elements.emplace_back(face.mIndices[j]);
        }
    }

    constexpr core::vertex_array::attribute position_attribute { 0, 3, opengl::constants::float_type, 0 };

    opengl::Buffer card_vbo;
    card_vbo.create();
    card_vbo.storage(core::buffer::make_data(card_vertices));

    opengl::Buffer card_ebo;
    card_ebo.create();
    card_ebo.storage(core::buffer::make_data(card_elements));

    opengl::VertexArray card_vao;
    card_vao.create();
    card_vao.attach_vertices(card_vbo, sizeof(glm::vec3));
    card_vao.attach_elements(card_ebo);

    card_vao.attribute(position_attribute);

    glm::mat4 model { 1.0f };

    glm::vec3 material_albedo { 1.0f, 0.0f, 0.0f };

    proj = glm::perspective(glm::radians(60.0f), static_cast<float>(window_width) / static_cast<float>(window_height), 0.1f, 100.0f);
    view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -8.0f));

    const std::vector camera_uniforms
    {
        view, proj
    };

    opengl::Buffer transform_ubo;
    transform_ubo.create();
    transform_ubo.storage(core::buffer::make_data(&model), opengl::constants::dynamic_draw);
    transform_ubo.bind_base(opengl::constants::uniform_buffer, core::buffer::transform);

    opengl::Buffer camera_ubo;
    camera_ubo.create();
    camera_ubo.storage(core::buffer::make_data(camera_uniforms), opengl::constants::dynamic_draw);
    camera_ubo.bind_base(opengl::constants::uniform_buffer, core::buffer::camera);

    opengl::Buffer material_ubo;
    material_ubo.create();
    material_ubo.storage(core::buffer::make_data(&material_albedo), opengl::constants::dynamic_draw);
    material_ubo.bind_base(opengl::constants::uniform_buffer, core::buffer::material);

    opengl::Pipeline::enable(opengl::constants::depth_test);
    opengl::Pipeline::enable(opengl::constants::cull_face);

    opengl::Commands::clear(0.5f, 0.5f, 0.5f);

    constexpr auto tile_width_size  = 1.25f;
    constexpr auto tile_height_size = 1.75f;

    auto physics_debug = new PhysicsDebug;
    const auto bt_world_configuration = new btDefaultCollisionConfiguration;

    world = new btCollisionWorld(new btCollisionDispatcher(bt_world_configuration), new btDbvtBroadphase(), bt_world_configuration);
    world->setDebugDrawer(physics_debug);

    auto card_shape = new btBoxShape(btVector3(0.5f, 0.76f, 0.02f));

    for (auto row = 0; row < 4; row++)
    {
        for (auto col = 0; col < 13; col++)
        {
            const auto x = tile_width_size  * col - 6.0f * tile_width_size;
            const auto y = tile_height_size * row - 1.5f * tile_height_size;

            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(x, y, 0.0f));

            auto card_object = new btCollisionObject();
            card_object->setCollisionShape(card_shape);
            card_object->setWorldTransform(transform);

            card_object->setUserIndex(row);
            card_object->setUserIndex2(col);

            world->addCollisionObject(card_object);
        }
    }

    world->debugDrawWorld();

    opengl::Buffer debug_vbo;
    debug_vbo.create();
    debug_vbo.storage(core::buffer::make_data(physics_debug->debug_vertices()));

    opengl::VertexArray debug_vao;
    debug_vao.create();
    debug_vao.attach_vertices(debug_vbo, sizeof(glm::vec3));
    debug_vao.attribute(position_attribute);

    glm::vec3 debug_color { 0.0f, 1.0f, 0.0f };

    while (!window_closed)
    {
        glfwPollEvents();

        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

        base_shader.bind();

        model = glm::mat4(1.0f);

        transform_ubo.update(core::buffer::make_data(&model));
         material_ubo.update(core::buffer::make_data(&debug_color));

        debug_vao.bind();

        opengl::Commands::draw_vertices(opengl::constants::lines, physics_debug->debug_vertices().size());

        material_ubo.update(core::buffer::make_data(&material_albedo));

        card_vao.bind();

        for (auto row = 0; row < 4; row++)
        {
            for (auto col = 0; col < 13; col++)
            {
                const auto x = tile_width_size  * col - 6.0f * tile_width_size;
                const auto y = tile_height_size * row - 1.5f * tile_height_size;

                model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));

                if (cards[row][col] == 1)
                {
                    model = glm::rotate(model, glm::radians(static_cast<float>(glfwGetTime()) * 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                }

                transform_ubo.update(core::buffer::make_data(&model));

                opengl::Commands::draw_elements(opengl::constants::triangles, card_elements.size());
            }
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}