#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "Buffer.h"
#include "ForgottenGame.h"
#include "MergeJoin.h"
#include "Box2DStep.h"
#include "Debug.h"
#include "SDLRender.h"

#include <tchar.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* window;
SDL_Surface* screenSurface;
SDL_Surface* blobSurface;

ProcessHost host;

void init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fatal_error(FORMAT("Could not initialize SDL: " << SDL_GetError()));
    }
    window = SDL_CreateWindow(
        "Forgotten",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (!window) {
        fatal_error(FORMAT("Could not create window: " << SDL_GetError()));
    }
    screenSurface = SDL_GetWindowSurface(window);
}

void loadAssets()
{
    blobSurface = SDL_LoadBMP("sprite_default.bmp");
    if (!blobSurface) {
        fatal_error("Could not load an asset.");
    }
}

typedef long int Eid;
typedef int Tid;

BUILD_COLUMN(EidColumn, Eid, eid)
BUILD_COLUMN(TidColumn, Tid, tid)
BUILD_COLUMN(PositionColumn, vec2, position)
BUILD_COLUMN(BodyColumn, b2Body*, body)
BUILD_COLUMN(ForceColumn, vec2, force)
BUILD_COLUMN(ContactColumn, (pair<b2Fixture*, b2Fixture*>), contact)
BUILD_COLUMN(SDLTextureColumn, SDL_Surface*, sdl_texture)

template<typename... TDataColumns>
using Aspect = Mapping<EidColumn, TDataColumns...>;

using ForcesChannel = TransientChannel<Record<BodyColumn, ForceColumn>, Vector>;
using BodiesChannel = PersistentChannel<Aspect<BodyColumn>, Set>;
using PositionsChannel = Buffer<2, TransientChannel<Aspect<PositionColumn>, Set>>;
using ContactsChannel = PersistentChannel<Record<ContactColumn>, Vector>;
using TexturesChannel = PersistentChannel<Mapping<TidColumn, SDLTextureColumn>, Set>;
using TextureAspectChannel = PersistentChannel<Aspect<TidColumn>, Set>;
using TexturePositionChannel = TransientChannel<Mapping<TidColumn, PositionColumn>, Set>;
using RenderablesChannel = TransientChannel<Record<SDLTextureColumn, PositionColumn>, Vector>;

unique_ptr<ForgottenGame> createGame()
{
    auto game = std::make_unique<ForgottenGame>();
    auto& forces = game->simulation.makeChannel<ForcesChannel>();
    auto& positions = game->simulation.makeChannel<PositionsChannel>();
    auto& bodies = game->simulation.makeChannel<BodiesChannel>();
    auto& contacts = game->simulation.makeChannel<ContactsChannel>();
    auto& textureAspects = game->simulation.makeChannel<TextureAspectChannel>();
    auto& textures = game->simulation.makeChannel<TexturesChannel>();

    auto& texturePositions = game->output.makeChannel<TexturePositionChannel>();
    auto& renderables = game->output.makeChannel<RenderablesChannel>();

    game->simulation.makeProcess<Box2DStep<ForcesChannel, BodiesChannel, PositionsChannel, ContactsChannel>>(
        forces, bodies, positions, contacts, &(game->world), 8, 3);

    game->output.makeProcess<MergeJoin<PositionsChannel, TextureAspectChannel, TexturePositionChannel>>(
        positions, textureAspects, texturePositions);

    game->output.makeProcess<MergeJoin<TexturePositionChannel, TexturesChannel, RenderablesChannel>>(
        texturePositions, textures, renderables);

    game->output.makeProcess<SDLRender<RenderablesChannel>>(renderables);

    game->output.makeProcess<Debug<PositionsChannel>>(positions);

    Eid player = 1;

    // Add one body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0.0f, 4.0f);
    b2Body* body = game->world.CreateBody(&bodyDef);
    bodies.write().put(Aspect<BodyColumn>({ player }, { body }));

    // Add a texture
    textures.write().put(Mapping<TidColumn, SDLTextureColumn>({ 0 }, { blobSurface }));

    // Apply texture to the body
    textureAspects.write().put(Aspect<TidColumn>({ player }, { 0 }));

    return game;
}

void close()
{
    if (blobSurface) {
        SDL_FreeSurface(blobSurface);
        blobSurface = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

int _tmain(int argc, _TCHAR* argv[])
{
    init();
    loadAssets();

    auto game = createGame();
    game->run();

    close();
    return 0;
}

void fatal_error(string message, string title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), window);
    close();
    exit(-1);
}