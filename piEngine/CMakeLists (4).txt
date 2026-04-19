#include "VPKArchive.h"
#include "Core/Log.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <cstring>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace Pillar {

// ── Pack ──────────────────────────────────────────────────────────────────────
bool VPKArchive::Pack(const std::string& sourceDir, const std::string& outVPK) {
    if (!fs::exists(sourceDir)) {
        PL_ERROR("VPK Pack: source dir not found: {}", sourceDir);
        return false;
    }

    // Collect files
    std::vector<VPKEntry> entries;
    std::vector<std::vector<uint8_t>> fileData;
    uint64_t offset = 0;

    for (auto& p : fs::recursive_directory_iterator(sourceDir)) {
        if (!p.is_regular_file()) continue;
        std::string rel = fs::relative(p.path(), sourceDir).generic_string();
        std::ifstream fin(p.path(), std::ios::binary);
        if (!fin) continue;
        std::vector<uint8_t> data((std::istreambuf_iterator<char>(fin)),
                                   std::istreambuf_iterator<char>());
        VPKEntry e; e.path = rel; e.offset = offset; e.size = data.size();
        entries.push_back(e);
        fileData.push_back(std::move(data));
        offset += e.size;
    }

    // Build directory JSON
    json dir;
    dir["entries"] = json::array();
    for (auto& e : entries) {
        dir["entries"].push_back({
            {"path",   e.path},
            {"offset", e.offset},
            {"size",   e.size}
        });
    }
    std::string dirStr = dir.dump();
    uint32_t dirSize  = (uint32_t)dirStr.size();
    uint32_t dataSize = (uint32_t)offset;

    // Write VPK
    std::ofstream fout(outVPK, std::ios::binary);
    if (!fout) { PL_ERROR("VPK Pack: cannot write: {}", outVPK); return false; }

    // Header
    uint32_t magic = MAGIC, version = 1;
    fout.write((char*)&magic,   4);
    fout.write((char*)&version, 4);
    fout.write((char*)&dirSize, 4);
    fout.write((char*)&dataSize,4);

    // Directory
    fout.write(dirStr.data(), dirStr.size());

    // Data
    for (auto& d : fileData)
        fout.write((char*)d.data(), d.size());

    PL_INFO("VPK packed {} files -> {} ({} bytes)", entries.size(), outVPK,
            16 + dirSize + dataSize);
    return true;
}

// ── Extract ───────────────────────────────────────────────────────────────────
bool VPKArchive::Extract(const std::string& vpkPath, const std::string& outDir) {
    VPKArchive ar;
    if (!ar.Open(vpkPath)) return false;
    for (auto& e : ar.GetEntries()) {
        auto data = ar.ReadFile(e.path);
        auto dest = fs::path(outDir) / e.path;
        fs::create_directories(dest.parent_path());
        std::ofstream f(dest, std::ios::binary);
        f.write((char*)data.data(), data.size());
    }
    PL_INFO("VPK extracted {} files to {}", ar.GetEntries().size(), outDir);
    return true;
}

bool VPKArchive::ExtractFile(const std::string& vpkPath, const std::string& entryPath,
                              const std::string& outFile) {
    VPKArchive ar;
    if (!ar.Open(vpkPath)) return false;
    auto data = ar.ReadFile(entryPath);
    if (data.empty()) return false;
    std::ofstream f(outFile, std::ios::binary);
    f.write((char*)data.data(), data.size());
    return true;
}

// ── Open / Read ───────────────────────────────────────────────────────────────
bool VPKArchive::Open(const std::string& vpkPath) {
    std::ifstream f(vpkPath, std::ios::binary);
    if (!f) { PL_ERROR("VPK: cannot open {}", vpkPath); return false; }

    uint32_t magic, version, dirSize, dataSize;
    f.read((char*)&magic,   4);
    f.read((char*)&version, 4);
    f.read((char*)&dirSize, 4);
    f.read((char*)&dataSize,4);

    if (magic != MAGIC) { PL_ERROR("VPK: bad magic in {}", vpkPath); return false; }

    std::string dirStr(dirSize, '\0');
    f.read(dirStr.data(), dirSize);
    m_DataOffset = 16 + dirSize;

    auto dir = json::parse(dirStr);
    m_Entries.clear(); m_Index.clear();
    for (auto& je : dir["entries"]) {
        VPKEntry e;
        e.path   = je["path"].get<std::string>();
        e.offset = je["offset"].get<uint64_t>();
        e.size   = je["size"].get<uint64_t>();
        m_Entries.push_back(e);
    }
    for (auto& e : m_Entries) m_Index[e.path] = &e;
    m_Path = vpkPath;
    return true;
}

void VPKArchive::Close() {
    m_Entries.clear(); m_Index.clear(); m_Path.clear();
}

bool VPKArchive::HasFile(const std::string& path) const {
    return m_Index.count(path) > 0;
}

std::vector<uint8_t> VPKArchive::ReadFile(const std::string& path) const {
    auto it = m_Index.find(path);
    if (it == m_Index.end()) { PL_WARN("VPK: file not found: {}", path); return {}; }
    const VPKEntry* e = it->second;
    std::ifstream f(m_Path, std::ios::binary);
    f.seekg((std::streampos)(m_DataOffset + e->offset));
    std::vector<uint8_t> data(e->size);
    f.read((char*)data.data(), e->size);
    return data;
}

std::string VPKArchive::ReadText(const std::string& path) const {
    auto data = ReadFile(path);
    return std::string(data.begin(), data.end());
}

} // namespace Pillar
