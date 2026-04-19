#include "Entity.h"
#include "Scene.h"

namespace Pillar {

entt::registry& Entity::GetRegistry() {
    return m_Scene->GetRegistry();
}

} // namespace Pillar
