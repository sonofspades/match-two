#pragma once

class PhysicsDebug final : public btIDebugDraw
{
public:
    void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;
    void clearLines() override;

    void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override { }
    void reportErrorWarning(const char* warningString) override { }
    void setDebugMode(int debugMode) override { }

    int getDebugMode() const override { return DBG_DrawWireframe; }

    auto debug_vertices() const -> const std::vector<glm::vec3>&;

private:
    std::vector<glm::vec3> _debug_vertices;
};