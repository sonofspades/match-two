#pragma once

struct card
{
    float angle { };
    bool turning { };
    bool turned  { };
    bool flipped { };

    int32_t type { };

    glm::vec3 color { 1.0f };
};