#include "physics_debug.hpp"

void PhysicsDebug::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
    _debug_vertices.emplace_back(from.x(), from.y(), from.z());
    _debug_vertices.emplace_back(to.x(), to.y(), to.z());
}

void PhysicsDebug::clearLines()
{
    _debug_vertices.clear();
}

auto PhysicsDebug::debug_vertices() const -> const std::vector<glm::vec3>&
{
    return _debug_vertices;
}