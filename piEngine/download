#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

namespace Pillar {

// ── Pillar VPK format ─────────────────────────────────────────────────────────
// Header (16 bytes):
//   magic    : uint32  = 0x504C5056  ('PVLP')
//   version  : uint32  = 1
//   dirSize  : uint32  (bytes of directory block)
//   dataSize : uint32  (bytes of data block)
//
// Directory block (JSON, compressed):
//   { "entries": [ { "path":"...", "offset":N, "size":N }, ... ] }
//
// Data block: raw concatenated file data.
// ─────────────────────────────────────────────────────────────────────────────

struct VPKEntry {
    std::string path;
    uint64_t    offset = 0;
    uint64_t    size   = 0;
};

class VPKArchive {
public:
    // Pack a folder into a VPK file
    static bool Pack(const std::string& sourceDir, const std::string& outVPK);

    // Extract all or specific file from VPK
    static bool Extract(const std::string& vpkPath, const std::string& outDir);
    static bool ExtractFile(const std::string& vpkPath, const std::string& entryPath,
                             const std::string& outFile);

    // Open for runtime reading
    bool Open(const std::string& vpkPath);
    void Close();

    bool HasFile(const std::string& path) const;
    std::vector<uint8_t> ReadFile(const std::string& path) const;
    std::string          ReadText(const std::string& path) const;

    const std::vector<VPKEntry>& GetEntries() const { return m_Entries; }

private:
    std::string           m_Path;
    std::vector<VPKEntry> m_Entries;
    std::unordered_map<std::string, const VPKEntry*> m_Index;
    uint64_t              m_DataOffset = 0;  // offset where data block starts

    static constexpr uint32_t MAGIC = 0x504C5056u;
};

} // namespace Pillar
