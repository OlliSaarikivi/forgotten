#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* window;
SDL_Surface* screenSurface;
SDL_Surface* blobSurface;

ProcessHost host;

void init() {
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

void loadAssets() {
	blobSurface = SDL_LoadBMP("sprite_default.bmp");
	if (!blobSurface) {
		fatal_error("Could not load an asset.");
	}
}

void createProcesses()
{
	/* Channels */
	//PositionsChannel_t monsters;
	/* Processes */
	//SelectAnyTarget<PositionsChannel_t, PositionsChannel_t, TargetingsChannel_t> monster_targeting(monsters, players, monster_targetings);
	//MoveTowardsTarget<TargetingsChannel_t, PositionsChannel_t> monster_movement(monster_targetings, monsters);

    auto r1 = Row<Eid, PositionColumn>();
    auto r2 = Row<Eid, BodyColumn>();
    auto r3 = Row<Eid, PositionColumn, BodyColumn>();
    r3.setAll(r2);
}

void close() {
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

int main(int argc, char* args[]) {
	init();
	loadAssets();

	SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 148, 237, 100));
	SDL_Rect target = { 0, 0, 0, 0 };
	SDL_BlitSurface(blobSurface, NULL, screenSurface, &target);
	SDL_UpdateWindowSurface(window);

	SDL_Delay(2000);
	close();
	return 0;
}

void fatal_error(string message, string title) {
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), window);
	close();
	exit(-1);
}