#include "stdafx.h"
#include "Resources.h"
#include "Forgotten.h"

std::string getFileContents(fs::path file_path)
{
    if (!fs::exists(file_path)) {
        fatalError(FORMAT("Required file missing: " << file_path));
    }
    std::ifstream in(file_path.string(), std::ios::in | std::ios::binary);
    if (in) {
        std::string contents;
        in.seekg(0, std::ios::end);
        contents.resize((unsigned int)in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&contents[0], contents.size());
        in.close();
        return contents;
    } else {
        fatalError(FORMAT("Could not read file " << file_path));
    }
}

fs::path getDataPath()
{
    return fs::path(SDL_GetBasePath()) / "data";
}