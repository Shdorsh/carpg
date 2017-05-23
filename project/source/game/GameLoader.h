#pragma once

struct GameLoader
{
	string system_dir;
	uint errors, warnings;
};

extern GameLoader game_loader;
