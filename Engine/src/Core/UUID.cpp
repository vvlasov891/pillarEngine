#include "UUID.h"

namespace Pillar {

static std::random_device               s_RD;
static std::mt19937_64                  s_Engine(s_RD());
static std::uniform_int_distribution<uint64_t> s_Dist(1, UINT64_MAX);

UUID::UUID() : m_ID(s_Dist(s_Engine)) {}

} // namespace Pillar
