#include "Game.h"

#include "Utils/Log.h"

Game::~Game() {
	Shutdown();
}

void Game::Run() {
	if (!Init()) {
		LOG_ERROR("Failed to initialize game.");
		return;
	}

	while (mIsRunning) {
		Render();
	}
}

bool Game::Init() {
	LOG_INFO("Initialized game.");
	return true;
}

void Game::Shutdown() {

}

void Game::Render() {

}
