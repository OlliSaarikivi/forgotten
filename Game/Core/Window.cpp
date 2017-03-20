#include <Core/stdafx.h>
#include "State.h"
#include "Window.h"
#include "Game.h"

static vector<Vec2i> mouseMoves;
static vector<sf::Event::KeyEvent> keyPresses;

void handleEvents(sf::Window& window) {
	sf::Event event;
	while (window.pollEvent(event)) {
		switch (event.type) {
		case sf::Event::Closed:
			exitGame();
		case sf::Event::MouseMoved:
			mouseMoves.emplace_back(event.mouseMove.x, event.mouseMove.y);
		case sf::Event::MouseLeft:
			mouseMoves.emplace_back(MouseOutsideWindow);
		case sf::Event::KeyPressed:
			keyPresses.emplace_back(event.key);
		}
	}
}