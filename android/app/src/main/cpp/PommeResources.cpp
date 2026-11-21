// Android filesystem bridge for Nanosaur II's original Pomme game resources.
// Android APK assets aren't ordinary desktop files. Extract the full Data tree
// once into application-private storage, then give Pomme real filesystem paths.
// The original game still needs OpenGL compatibility and an Android entry point.
#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <Pomme.h>
#include <PommeFiles.h>
#include <PommeInit.h>

namespace
{
namespace nativefs = std::filesystem;
constexpr const char* kManifest = "nanosaur-data-files.txt";
constexpr const char* kInstalledManifest = ".nanosaur-data-manifest";

std::string ReadDiskText(const nativefs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) return {};
    return std::string(std::istreambuf_iterator<char>(input), {});
}

bool SafeAssetPath(const std::string& path)
{
    if (path.empty() || path.find('\\') != std::string::npos ||
        path.front() == '/' || path.find(':') != std::string::npos)
        return false;

    const nativefs::path parsed(path);
    if (parsed.is_absolute()) return false;
    for (const auto& part : parsed)
        if (part == ".." || part == ".") return false;
    return true;
}
}

// Called from the SDL native app thread, not the Android UI thread.
// Returns true when all original resources and Pomme's host-volume mapping
// have been verified, false for preview-only or failed extraction.
bool InitializePommeResources()
{
    size_t manifestSize = 0;
    void* manifestRaw = SDL_LoadFile(kManifest, &manifestSize);
    if (!manifestRaw)
    {
        SDL_Log("Nanosaur full-resource manifest not present: running preview-only mode");
        return false;
    }

    const std::string manifest(static_cast<const char*>(manifestRaw), manifestSize);
    SDL_free(manifestRaw);
    if (manifest.empty())
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Empty original resource manifest");
        return false;
    }

    const char* internalDir = SDL_GetAndroidInternalStoragePath();
    if (!internalDir || !*internalDir)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Cannot get Android app-private storage: %s", SDL_GetError());
        return false;
    }

    const nativefs::path root(internalDir);
    const nativefs::path dataRoot = root / "Data";
    const nativefs::path installedManifest = root / kInstalledManifest;

    try
    {
        if (ReadDiskText(installedManifest) != manifest ||
            !nativefs::is_regular_file(dataRoot / "System" / "gamecontrollerdb.txt"))
        {
            std::istringstream entries(manifest);
            std::string relative;
            size_t extracted = 0;
            size_t totalBytes = 0;

            while (std::getline(entries, relative))
            {
                if (!relative.empty() && relative.back() == '\r')
                    relative.pop_back();
                if (relative.empty()) continue;
                if (!SafeAssetPath(relative))
                    throw std::runtime_error("Unsafe game asset path: " + relative);

                const std::string packagedPath = "Data/" + relative;
                size_t size = 0;
                void* bytes = SDL_LoadFile(packagedPath.c_str(), &size);
                if (!bytes)
                    throw std::runtime_error("Missing APK asset: " + packagedPath);

                const nativefs::path output = dataRoot / relative;
                std::error_code ec;
                nativefs::create_directories(output.parent_path(), ec);
                if (ec)
                {
                    SDL_free(bytes);
                    throw std::runtime_error("Cannot create game resource directory: " + ec.message());
                }

                std::ofstream file(output, std::ios::binary | std::ios::trunc);
                if (!file)
                {
                    SDL_free(bytes);
                    throw std::runtime_error("Cannot create game resource: " + output.string());
                }
                file.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(size));
                SDL_free(bytes);
                if (!file)
                    throw std::runtime_error("Failed writing game asset: " + output.string());
                file.close();

                ++extracted;
                totalBytes += size;
            }

            // Write the marker only after every file has been successfully extracted.
            std::ofstream marker(installedManifest, std::ios::binary | std::ios::trunc);
            marker.write(manifest.data(), static_cast<std::streamsize>(manifest.size()));
            if (!marker) throw std::runtime_error("Failed to save game resource manifest");
            SDL_Log("Nanosaur extracted %zu original game resources (%zu bytes)", extracted, totalBytes);
        }
        else
        {
            SDL_Log("Nanosaur game assets already installed; extraction skipped");
        }

        // Pomme's Macintosh FSSpec emulation needs real filesystem paths.
        // Initialize only the configured filesystem core (no sound/graphics).
        Pomme::Init();
        const auto spec = Pomme::Files::HostPathToFSSpec(dataRoot / "System");
        FSSpec gamepadDatabase = {};
        const OSErr result = FSMakeFSSpec(spec.vRefNum, spec.parID,
                                         ":System:gamecontrollerdb.txt", &gamepadDatabase);
        if (result != noErr)
        {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                         "Pomme FSSpec lookup failed: %d", static_cast<int>(result));
            Pomme::Shutdown();
            return false;
        }

        SDL_Log("Nanosaur Pomme filesystem ready: %s", dataRoot.string().c_str());
        Pomme::Shutdown();  // The full game boot will own the persistent runtime later.
        return true;
    }
    catch (const std::exception& exception)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "Nanosaur resource/Pomme initialization failed: %s", exception.what());
        return false;
    }
}
