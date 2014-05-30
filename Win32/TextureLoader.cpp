#include "stdafx.h"
#include "TextureLoader.h"
#include "Resources.h"
#include "Forgotten.h"

const string TEXTURES_DIR = "textures";

TextureLoader::TextureLoader(Game& game) : game(game)
{
}

gl::images::PNGImage TextureLoader::loadPNG(string qualified_name)
{
    std::ifstream in((getDataDir() / TEXTURES_DIR / FORMAT(qualified_name << ".png")).string(), std::ios::in | std::ios::binary);
    return gl::images::PNGImage(in);
}

void setTextureAttributes(string type, gl::Bound<gl::TextureOps>& binding)
{
    if ("diffuse" == type) {
        binding.MinFilter(gl::TextureMinFilter::Nearest)
            .MagFilter(gl::TextureMagFilter::Nearest);
    } else {
        fatalError(FORMAT("Unrecognized texture type: " << type));
    }
}

TextureHandle TextureLoader::loadSprite(string type, string name)
{
    auto texture = std::make_unique<gl::Texture>();
    auto image = loadPNG(FORMAT(type << "/" << name));
    auto binding = gl.Bound(gl::Texture::Target::_2D, *texture);
    binding.Image2D(image);
    setTextureAttributes(type, binding);
    return game.textures.putResource(std::move(texture));
}

template<typename T>
long parseSize(const T* str);
template<>
long parseSize(const char* str)
{
    return strtol(str, nullptr, 10);
}
template<>
long parseSize(const wchar_t* str)
{
    return wcstol(str, nullptr, 10);
}

void TextureLoader::loadSpriteArrays()
{
    auto textures_dir = getDataDir() / TEXTURES_DIR;
    if (!fs::exists(textures_dir)) {
        fatalError(FORMAT("Missing directory: " << textures_dir));
    }
    if (!fs::is_directory(textures_dir)) {
        fatalError(FORMAT("Textures directory is a file: " << textures_dir));
    }
    for (auto tex_dir_iter = fs::directory_iterator(textures_dir); tex_dir_iter != fs::directory_iterator(); ++tex_dir_iter) {
        auto size_dir_candidate = *tex_dir_iter;
        if (!fs::is_directory(size_dir_candidate)) {
            continue;
        }
        long size = parseSize(size_dir_candidate.path().filename().c_str());
        if (size <= 0) {
            continue;
        }
        if ((size & (size - 1)) != 0) {
            fatalError(FORMAT("Encountered non-power-of-two texture directory: " << size_dir_candidate));
        }
        // Now we have found a directory for textures of a particular size
        for (auto size_dir_iter = fs::directory_iterator(size_dir_candidate); size_dir_iter != fs::directory_iterator(); ++size_dir_iter) {
            auto type_dir_candidate = *size_dir_iter;
            if (!fs::is_directory(type_dir_candidate)) {
                continue;
            }
            string type = type_dir_candidate.path().filename().string();

            vector<string> names;
            for (auto type_dir_iter = fs::directory_iterator(type_dir_candidate); type_dir_iter != fs::directory_iterator(); ++type_dir_iter) {
                auto sprite_path = *type_dir_iter;
                names.emplace_back(sprite_path.path().stem().string());
            }
            TextureHandle handle = loadSpriteArray(size, type, names);

            for (int i = 0; i < names.size(); ++i) {
                game.sprite_textures.put({ { size }, { type }, { names[i] }, handle, { i } });
            }
        }
    }
}

TextureHandle TextureLoader::loadSpriteArray(unsigned int size, string type, const vector<string>& names)
{
    if (size == 0 || (size & (size - 1)) != 0) {
        fatalError(FORMAT("Texture size must be a power of two"));
    }
    string relative_dir = FORMAT(size << "/" << type << "/");
    auto texture = std::make_unique<gl::Texture>();
    auto binding = gl.Bound(gl::Texture::Target::_2DArray, *texture);
    for (int i = 0; i < names.size(); ++i) {
        auto image = loadPNG(FORMAT(relative_dir << names[i]));
        if (i == 0) {
            binding.Image3D(0, gl::PixelDataInternalFormat::RGBA, size, size, names.size(), 0, image.Format(), image.Type(), nullptr);
        }
        binding.SubImage3D(image, 0, 0, i);
    }
    setTextureAttributes(type, binding);
    return game.textures.putResource(std::move(texture));
}