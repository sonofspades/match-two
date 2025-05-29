#include <core/file.hpp>

#include <opengl/commands.hpp>
#include <opengl/functions.hpp>
#include <opengl/shader.hpp>

#include <opengl/constants/commands.hpp>
#include <opengl/constants/shader.hpp>

#include <shaders/converter.hpp>

auto main() -> int32_t
{
    shaders::Converter::convert("../../resources/shaders", "./");

    constexpr auto window_width  = 1000;
    constexpr auto window_height = 1000;
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

    opengl::Commands::clear(0.5f, 0.5f, 0.5f);

    while (!window_closed)
    {
        glfwPollEvents();

        opengl::Commands::clear(opengl::constants::color_buffer | opengl::constants::depth_buffer);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}