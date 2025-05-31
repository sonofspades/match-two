#pragma once

class PhysicsDebug final : public btIDebugDraw
{
public:
    auto drawLine(const btVector3& from, const btVector3& to, const btVector3& color) -> void override;
    auto clearLines() -> void override;

    auto drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) -> void override { }
    auto reportErrorWarning(const char* warningString) -> void override { }
    auto setDebugMode(int debugMode) -> void override { }

    auto getDebugMode()   const -> int override { return DBG_DrawWireframe; }
    auto debug_vertices() const -> const std::vector<glm::vec3>&;

private:
    std::vector<glm::vec3> _debug_vertices;
};