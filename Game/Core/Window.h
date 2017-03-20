#pragma once

#include <Database\Columnar.h>
#include <Database\Table.h>


// Mouse
const Vec2i MouseOutsideWindow;
extern vector<Vec2i> mouseMoves;
// Keyboard
extern vector<sf::Event::KeyEvent> keyPresses;

void handleEvents(sf::Window& window);