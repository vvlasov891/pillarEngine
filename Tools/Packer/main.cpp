#include <VPK/VPKArchive.h>
#include <Core/Log.h>
#include <iostream>
#include <string>

// ─────────────────────────────────────────────────────────────────────────────
//  PillarPacker — command-line tool
//
//  Usage:
//    PillarPacker pack   <source_dir> <output.vpk>
//    PillarPacker unpack <input.vpk>  <output_dir>
//    PillarPacker list   <input.vpk>
//    PillarPacker build  <scene.pilevel>
//       Packs assets/ into game.vpk and copies the .exe alongside it.
// ─────────────────────────────────────────────────────────────────────────────

static void PrintUsage() {
    std::cout <<
        "PillarPacker v1.0\n"
        "Usage:\n"
        "  pack   <source_dir> <output.vpk>   Pack a folder into a VPK\n"
        "  unpack <input.vpk>  <output_dir>   Extract all files from a VPK\n"
        "  list   <input.vpk>                 List contents of a VPK\n"
        "  build  <assets_dir> <output.vpk>   Pack and prepare game distribution\n";
}

int main(int argc, char** argv) {
    Pillar::Log::Init("packer.log");

    if (argc < 3) { PrintUsage(); return 1; }

    std::string cmd = argv[1];

    if (cmd == "pack" && argc >= 4) {
        std::string src = argv[2];
        std::string out = argv[3];
        std::cout << "Packing '" << src << "' -> '" << out << "' ...\n";
        bool ok = Pillar::VPKArchive::Pack(src, out);
        std::cout << (ok ? "Done.\n" : "Failed.\n");
        return ok ? 0 : 1;
    }

    if (cmd == "unpack" && argc >= 4) {
        std::string vpk = argv[2];
        std::string dir = argv[3];
        std::cout << "Extracting '" << vpk << "' -> '" << dir << "' ...\n";
        bool ok = Pillar::VPKArchive::Extract(vpk, dir);
        std::cout << (ok ? "Done.\n" : "Failed.\n");
        return ok ? 0 : 1;
    }

    if (cmd == "list" && argc >= 3) {
        Pillar::VPKArchive ar;
        if (!ar.Open(argv[2])) return 1;
        std::cout << "Contents of '" << argv[2] << "':\n";
        for (auto& e : ar.GetEntries())
            std::cout << "  " << e.path
                      << "  (" << e.size << " bytes)\n";
        std::cout << ar.GetEntries().size() << " file(s) total.\n";
        return 0;
    }

    if (cmd == "build" && argc >= 4) {
        std::string assetsDir = argv[2];
        std::string outVPK    = argv[3];
        std::cout << "Building game package...\n";
        std::cout << "  Packing assets: " << assetsDir << " -> " << outVPK << "\n";
        bool ok = Pillar::VPKArchive::Pack(assetsDir, outVPK);
        if (ok) std::cout << "Build succeeded: " << outVPK << "\n";
        else    std::cout << "Build FAILED.\n";
        return ok ? 0 : 1;
    }

    PrintUsage();
    return 1;
}
