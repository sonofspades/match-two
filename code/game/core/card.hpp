#pragma once

struct card
{
    float angle { };
    bool turning { }; // TODO get rid of this multiple states - use the card_state enum
    bool turned  { };
    bool flipped { };
    bool reversing { };

    int32_t type { };

    glm::vec3 color { 1.0f };
};