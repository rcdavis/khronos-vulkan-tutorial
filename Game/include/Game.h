#pragma once

class Game {
public:
	Game() = default;
	~Game();

	void Run();

private:
	bool Init();
	void Shutdown();

	void Render();

private:
	bool mIsRunning = false;
};
