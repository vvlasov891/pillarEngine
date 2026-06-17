#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <random>

namespace Pillar {

class UUID {
public:
    UUID();
    explicit UUID(uint64_t id) : m_ID(id) {}
    operator uint64_t() const { return m_ID; }
    bool operator==(const UUID& o) const { return m_ID == o.m_ID; }
    bool operator!=(const UUID& o) const { return m_ID != o.m_ID; }
    std::string ToString() const { return std::to_string(m_ID); }

    static UUID Invalid() { return UUID(0); }
    bool IsValid() const  { return m_ID != 0; }

private:
    uint64_t m_ID;
};

} // namespace Pillar

namespace std {
    template<> struct hash<Pillar::UUID> {
        size_t operator()(const Pillar::UUID& id) const {
            return hash<uint64_t>()((uint64_t)id);
        }
    };
}
