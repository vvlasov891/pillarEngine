#include "Components.h"
#include <glm/gtc/matrix_transform.hpp>

namespace Pillar {

glm::mat4 TransformComponent::GetMatrix() const {
    glm::mat4 m = glm::mat4(1.0f);
    m = glm::translate(m, Position);
    m = glm::rotate(m, glm::radians(Rotation.y), {0,1,0});
    m = glm::rotate(m, glm::radians(Rotation.x), {1,0,0});
    m = glm::rotate(m, glm::radians(Rotation.z), {0,0,1});
    m = glm::scale(m, Scale);
    return m;
}

} // namespace Pillar
