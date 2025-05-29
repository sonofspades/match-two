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

auto main() -> int32_t
{
    shaders::Converter::convert("../../resources/shaders", "./");

    constexpr auto window_width  = 1920;
    constexpr auto window_height = 1080;
    static    auto window_closed = false;

    if (glfwInit() != GLFW_TRUE)
    {
        return -1;
    }

    const auto window = glfwCreateWindow(window_width, window_height, "Match Two", nullptr);

    glfwSetWindowCloseCallback(window, []
    {
        window_closed = true;
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

    auto proj = glm::perspective(glm::radians(60.0f), static_cast<float>(window_width) / static_cast<float>(window_height), 0.1f, 100.0f);
    auto view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -8.0f));

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

    while (!window_closed)
    {
        glfwPollEvents();

        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

        base_shader.bind();

        card_vao.bind();

        for (auto row = 0; row < 13; row++)
        {
            constexpr auto tile_size = 1.25f;

            const auto x = tile_size * row - 6.0f * tile_size;
            const auto y = 0.0f;

            model = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));

            transform_ubo.update(core::buffer::make_data(&model));

            opengl::Commands::draw_elements(opengl::constants::triangles, card_elements.size());
        }

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}