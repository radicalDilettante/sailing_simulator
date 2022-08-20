#include "yacht.h"
#include <iostream>
#include <cstring>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void Yacht::createYacht()
{
    createMesh(&_vertices[0], &_indices[0], _vertices.size(), _indices.size());
}

glm::mat4
Yacht::getModelMatrix(float deltaTime, glm::vec2 worldWind)
{
    glm::vec2 velocity =
        _prevVelocity + getPoweredPropulsion(deltaTime) + getWindPropulsion(worldWind, deltaTime) - getDrag(deltaTime);

    if (glm::length(velocity) >= MAX_SPEED)
    {
        velocity = glm::normalize(velocity) * MAX_SPEED;
    }

    float x = _curPos.x + velocity.x * deltaTime;
    float y = _curPos.y + velocity.y * deltaTime;

    // std::cout << glm::length(velocity) << std::endl;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(x, y, 0.0f));
    model = glm::rotate(model, getCurDir(), glm::vec3(0.0f, 0.0f, 1.0f));

    _prevVelocity = velocity;
    _curPos = {x, y};

    return model;
}

void Yacht::turnToPort()
{
    _curDirVec = glm::rotate(_curDirVec, 0.01f);
}

void Yacht::turnToStarboard()
{
    _curDirVec = glm::rotate(_curDirVec, -0.01f);
}

glm::vec2 Yacht::getDrag(float deltaTime)
{
    // std::cout << _prevX << " , " << _prevY << std::endl;
    glm::vec2 drag;

    if (glm::length(_prevVelocity) > 0)
    {
        glm::vec2 froudeNumber = _prevVelocity * 0.6f;
        drag = froudeNumber * deltaTime;
    }
    else
    {
        drag = {0.0f, 0.0f};
    }

    return drag;
}

glm::vec2 Yacht::getPoweredPropulsion(float deltaTime)

{
    glm::vec2 propulsion;
    if (_isEngineOn)
    {
        float engineSpeed = 0.5f * deltaTime;
        propulsion = {0.0f, engineSpeed};
        propulsion = glm::rotate(propulsion, getCurDir());
    }
    else
    {
        propulsion = {0.0f, 0.0f};
    }

    return propulsion;
}

glm::vec2 Yacht::getWindPropulsion(glm::vec2 worldWind, float deltaTime)
{
    glm::vec2 appWind = worldWind + _prevVelocity;
    float appWindAngle = glm::degrees(glm::orientedAngle(glm::normalize(appWind), _curDirVec));

    glm::vec2 propulsion = {0.0f, 0.0f};

    if (appWindAngle >= -40.0f && appWindAngle <= 40.0f)
        propulsion.y = 5.0f;
    else if (appWindAngle >= -90.0f && appWindAngle <= 90.0f)
        propulsion.y = 3.0f;
    else if (appWindAngle >= -130.0f && appWindAngle <= 130.0f)
        propulsion.y = 2.0f;
    else if (appWindAngle >= -150.0f && appWindAngle <= 150.0f)
        propulsion.y = 1.0f;
    else
        propulsion.y = 0.0f;

    _prevAppWindAngle = appWindAngle;

    propulsion = glm::rotate(propulsion, getCurDir());
    propulsion = propulsion * deltaTime;

    return propulsion;
}

float Yacht::getCurDir()
{
    return glm::orientedAngle(ZERO, _curDirVec);
}

float Yacht::getMastAngle(glm::vec2 worldWind)
{
    return glm::degrees(glm::orientedAngle(glm::normalize(worldWind), _curDirVec));
}

void Yacht::reset(glm::vec2 pos, float dirAngle)
{
    _curPos = pos;
    _prevAppWindAngle = 0.0f;
    _prevVelocity = {0.0f, 0.0f};
    _curDirVec = {0.0f, 1.0f};
    _curDirVec = glm::rotate(_curDirVec, glm::radians(dirAngle));
}

void Yacht::testCollision(glm::vec2 boxCenter, glm::vec2 boxLength)
{
    // Detect collision between obstacles(AABB) and yacht(Circle)
    glm::vec2 circlePos = {_curPos.x, _curPos.y};
    glm::vec2 boxPoint;

    glm::vec2 normal;
    glm::vec2 translateFactor;

    if (circlePos.x < boxCenter.x - boxLength.x / 2.0f)
    {
        boxPoint.x = boxCenter.x - boxLength.x / 2.0f;
        translateFactor.x = -1.0f;
    }
    else if (circlePos.x > boxCenter.x + boxLength.x / 2.0f)
    {
        boxPoint.x = boxCenter.x + boxLength.x / 2.0f;
        normal.x = -1.0f;
        translateFactor.x = 1.0f;
    }
    else
    {
        boxPoint.x = circlePos.x;
        normal.x = 1.0f;
        translateFactor.x = 0.0f;
    }

    if (circlePos.y < boxCenter.y - boxLength.y / 2.0f)
    {
        boxPoint.y = boxCenter.y - boxLength.y / 2.0f;
        normal.y = -1.0f;
        translateFactor.y = -1.0f;
    }
    else if (circlePos.y > boxCenter.y + boxLength.y / 2.0f)
    {
        boxPoint.y = boxCenter.y + boxLength.y / 2.0f;
        normal.y = -1.0f;
        translateFactor.y = 1.0f;
    }
    else
    {
        boxPoint.y = circlePos.y;
        normal.y = 1.0f;
        translateFactor.y = 0.0f;
    }

    glm::vec2 dist = circlePos - boxPoint;

    if (glm::length(dist) < CIRCLE_RADIUS)
    {
        _prevVelocity = glm::vec2{_prevVelocity.x * normal.x, _prevVelocity.y * normal.y};

        if (translateFactor.x != 0.0f && translateFactor.y != 0.0f)
        {
            translateFactor = translateFactor * 0.71f; // cos 45;
        }

        circlePos = boxPoint + CIRCLE_RADIUS * translateFactor;
        _curPos = {circlePos.x, circlePos.y};
        std::cout << "crash!!" << std::endl;
    }
}

bool Yacht::testGoal(glm::vec2 boxCenter, glm::vec2 boxLength)
{
    // Detect collision between obstacles(AABB) and yacht(Circle)
    glm::vec2 circlePos = {_curPos.x, _curPos.y + 2.0f};
    glm::vec2 boxPoint;

    if (circlePos.x < boxCenter.x - boxLength.x / 2.0f)
        boxPoint.x = boxCenter.x - boxLength.x / 2.0f;
    else if (circlePos.x > boxCenter.x + boxLength.x / 2.0f)
        boxPoint.x = boxCenter.x + boxLength.x / 2.0f;
    else
        boxPoint.x = circlePos.x;

    if (circlePos.y < boxCenter.y - boxLength.y / 2.0f)
        boxPoint.y = boxCenter.y - boxLength.y / 2.0f;
    else if (circlePos.y > boxCenter.y + boxLength.y / 2.0f)
        boxPoint.y = boxCenter.y + boxLength.y / 2.0f;
    else
        boxPoint.y = circlePos.y;

    glm::vec2 dist = circlePos - boxPoint;

    return (glm::length(dist) <= CIRCLE_RADIUS);
}