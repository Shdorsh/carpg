#pragma once

struct Location;
struct PlayerController;
struct Unit;

struct ScriptContext
{
	Unit* talker;
	PlayerController* local_player;
	PlayerController* player;
	Location* location;
};
