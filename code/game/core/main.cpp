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

#include "card.hpp"

btCollisionWorld* world;

glm::mat4 view;
glm::mat4 proj;

card cards[4][13] { }; // TODO have a board class

card* last_card { };

static auto card_is_matching  = false;
static auto card_is_turning   = false;
static auto card_is_reversing = false;

static constexpr auto card_columns_count = 13;

auto main() -> int32_t
{
    shaders::Converter::convert("../../resources/shaders", "./");

    constexpr auto window_width  = 1920;
    constexpr auto window_height = 980;
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

    glfwSetKeyCallback(window, [](const int key, const int, const int action, const int) -> void
    {
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            for (auto row = 0; row < 4; row++)
            {
                for (auto col = 0; col < card_columns_count; col++) // TODO reset the cards in the board class
                {
                    auto& card = cards[row][col];

                    card.turned    = false;
                    card.turning   = false;
                    card.reversing = false;
                    card.flipped   = false;
                    card.angle     = 0.0f;

                    card_is_turning   = false;
                    card_is_reversing = false;
                    card_is_matching  = false;
                    last_card         = nullptr;
                }
            }
        }
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

                auto& card = cards[row][col];

                if (card_is_turning || card_is_reversing || card.turned || card.turning || card.flipped) // TODO have less checks with the current card states - maybe without the turned/turning
                {
                    return;
                }

                // TODO refactor from here

                card.turning    = true;
                card_is_turning = true;

                if (last_card != nullptr)
                {
                    if (last_card->type == cards[row][col].type)
                    {
                        card_is_matching = true;
                    }
                    else
                    {
                        card_is_matching = false;
                        card_is_reversing = true;

                              card.reversing = true;
                        last_card->reversing = true;
                    }
                }
                else
                {
                    last_card = &cards[row][col];
                }

                // TODO refactor to here
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

    Assimp::Importer card_importer; // TODO replace assimp with an obj loader

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

    opengl::Buffer card_vbo; // TODO replace with a Mesh class
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

    proj = glm::ortho(0.0f,  static_cast<float>(window_width), 0.0f, static_cast<float>(window_height), -1.0f, 1.0f);
    view = glm::mat4(1.0f);

    const std::vector camera_uniforms
    {
        view, proj
    };

    opengl::Buffer transform_ubo; // TODO have like an uniform buffer manager
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

    opengl::Commands::clear(0.08627450980392157f, 0.3803921568627451f, 0.05490196f);

    glm::vec3 card_background_color { 0.97647058f, 0.47843137254901963f, 0.0f };

    std::vector<glm::vec3> card_colors; // TODO colors should not be that random

    for (auto i = 0; i < 26; i++)
    {
        card_colors.emplace_back(glm::linearRand(glm::vec3(0.0f), glm::vec3(1.0f)));
    }

    constexpr auto tile_width_size  = 145.5f;
    constexpr auto tile_height_size = 212.0f;

    const auto bt_world_configuration = new btDefaultCollisionConfiguration;

    world = new btCollisionWorld(new btCollisionDispatcher(bt_world_configuration), new btDbvtBroadphase(), bt_world_configuration);

    auto card_shape = new btBoxShape(btVector3(65.0f, 97.0f, 0.2f));

    auto card_pairing = false;
    auto card_type    = 0;

    for (auto row = 0; row < 4; row++) // TODO change this with a new collision system
    {
        for (auto col = 0; col < card_columns_count; col++)
        {
            const auto x = tile_width_size  * col - 6.0f * tile_width_size  + static_cast<float>(window_width)  / 2.0f;
            const auto y = tile_height_size * row - 1.5f * tile_height_size + static_cast<float>(window_height) / 2.0f;

            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(btVector3(x, y, 0.0f));

            auto card_object = new btCollisionObject();
            card_object->setCollisionShape(card_shape);
            card_object->setWorldTransform(transform);

            card_object->setUserIndex(row);
            card_object->setUserIndex2(col);

            world->addCollisionObject(card_object);

            cards[row][col].type = card_type;

            if (card_pairing)
            {
                card_pairing = false;
                card_type++;
            }
            else
            {
                card_pairing = true;
            }
        }
    }

    // TODO merge this in to in board class from here

    for (auto row = 0; row < 4; row++)
    {
        for (auto col = 0; col < card_columns_count; col++)
        {
            auto new_row = glm::linearRand(0, 3);
            auto new_col = glm::linearRand(0, card_columns_count - 1);

            auto&     card = cards[row][col];
            auto& new_card = cards[new_row][new_col];

            const auto type = card.type;
                card.type = new_card.type;
            new_card.type = type;
        }
    }

    for (auto row = 0; row < 4; row++)
    {
        for (auto col = 0; col < card_columns_count; col++)
        {
            auto& card = cards[row][col];

            card.color = card_colors[card.type];
        }
    }

    // TODO merge this in to in board class to here

    constexpr auto card_rotation_speed     = 180.0f;
    constexpr auto card_rotation_max_angle = 180.0f;

    constexpr auto card_scale = 130.0f;

    auto starting_time = glfwGetTime();

    while (!window_closed)
    {
        glfwPollEvents();

        auto current_time = glfwGetTime(); // TODO use time Time.delta_time

        auto delta_time = current_time - starting_time;
          starting_time = current_time;

        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

        base_shader.bind();

        card_vao.bind();

        for (auto row = 0; row < 4; row++)
        {
            for (auto col = 0; col < card_columns_count; col++)
            {
                auto& card = cards[row][col];

                if (card.flipped)
                {
                    continue;
                }

                material_ubo.update(core::buffer::make_data(&card_background_color));

                const auto x = tile_width_size  * col - 6.0f * tile_width_size  + static_cast<float>(window_width)  / 2.0f;
                const auto y = tile_height_size * row - 1.5f * tile_height_size + static_cast<float>(window_height) / 2.0f;

                model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
                model = glm::scale(model, glm::vec3(card_scale, card_scale, 1.0f));

                // TODO clean the states
                // TODO update the color in a more simple way

                if (card.turning)
                {
                    card.angle += delta_time * card_rotation_speed;

                    auto a = glm::smoothstep(0.0f, card_rotation_max_angle, card.angle);

                    if (a >= 0.5f)
                    {
                        material_ubo.update(core::buffer::make_data(&card.color));
                    }

                    model = glm::rotate(model, glm::radians(a * card_rotation_max_angle), glm::vec3(0.0f, 1.0f, 0.0f));

                    if (card.angle >= card_rotation_max_angle)
                    {
                        card.turned  = true;
                        card.turning = false;

                        card_is_turning = false;

                        if (card_is_matching)
                        {
                            card_is_matching   = false;

                            last_card->flipped = true;
                            card.flipped       = true;

                            last_card = nullptr;
                        }
                    }
                }
                else if (card.reversing && !card_is_turning)
                {
                    card.angle -= delta_time * card_rotation_speed;

                    auto a = glm::smoothstep(0.0f, card_rotation_max_angle, card.angle);

                    if (a <= 0.5f)
                    {
                        material_ubo.update(core::buffer::make_data(&card_background_color));
                    }
                    else
                    {
                        material_ubo.update(core::buffer::make_data(&card.color));
                    }

                    model = glm::rotate(model, glm::radians(a * card_rotation_max_angle), glm::vec3(0.0f, 1.0f, 0.0f));

                    if (card.angle <= 0.0f)
                    {
                        card.turned    = false;
                        card.reversing = false;

                        if (last_card != nullptr)
                        {
                            last_card->turned    = false;
                            last_card->reversing = false;

                            last_card = nullptr;
                        }

                        card_is_reversing = false;
                    }
                }
                else if (card.turned)
                {
                    material_ubo.update(core::buffer::make_data(&card.color));
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