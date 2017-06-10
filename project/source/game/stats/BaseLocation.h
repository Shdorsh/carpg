// bazowe typy podziemi i krypt
#pragma once

//-----------------------------------------------------------------------------
#include "SpawnGroup.h"

//-----------------------------------------------------------------------------
// fort ludzi, 2-3 poziom�w, bez pu�apek czy ukrytych przej��
#define HUMAN_FORT 0
// fort krasnolud�w 3-5 poziom�w, pu�apki, tajne przej�cia, na dole skarby
#define DWARF_FORT 1
// wie�a mag�w, 4-6 okr�g�ych ma�ych poziom�w
// 50% szansy na mag�w, 25% na mag�w i golemy
#define MAGE_TOWER 2
// kryj�wka bandyt�w, 1 poziom, pu�apki przy wej�ciu
// 75% szansy na bandyt�w
#define BANDITS_HIDEOUT 3
// krypta w kt�rej pochowano jak�� znan� osob�, 2-3 poziom�w, na pocz�tku ostrze�enia, na ostatnim poziomie du�o pu�apek i centralna sala ze zw�okami i skarbami
// 50% szansy na nieumar�ych, 25% nekro
#define HERO_CRYPT 4
// uwi�ziono tu jakiego� z�ego potwora, 2-3 poziom�w, jak wy�ej
// 75% szansy na nieumar�ych
#define MONSTER_CRYPT 5
// stara �wi�tynia, 1-3 poziom�w, mo�e by� oczyszczona lub nie ze z�ych kap�an�w/nekromant�w
// 25% nieumarli, 25% nekro, 25% �li
#define OLD_TEMPLE 6
// ukryta skrytka, 1 poziom, wygl�da jak zwyk�y poziom ale s� ukryte przej�cia i pu�apki, na ko�cu skarb
// 25% nic, 25% bandyci
#define VAULT 7
// baza nekromant�w
// 50% nekro, 25% z�o
#define NECROMANCER_BASE 8
// labirynt
#define LABIRYNTH 9
// jaskina
#define CAVE 10
// ko�cowy poziom questu kopalnia
#define KOPALNIA_POZIOM 11
// jak HUMAN_FORT ale 100% szansy na drzwi
#define TUTORIAL_FORT 12
// jak HUMAN_FORT ale z sal� tronow�
#define THRONE_FORT 13
// jak VAULT ale z sal� tronow�
#define THRONE_VAULT 14
// druga tekstura krypty
#define CRYPT_2_TEXTURE 15

//-----------------------------------------------------------------------------
// Location flags
enum BaseLocationOptions
{
	BLO_ROUND = 1<<0,
	BLO_LABIRYNTH = 1<<1,
	BLO_MAGIC_LIGHT = 1<<2,
	BLO_LESS_FOOD = 1<<3,
	BLO_NO_TEX_WRAP = 1<<4,
};

//-----------------------------------------------------------------------------
struct RoomType;

//-----------------------------------------------------------------------------
struct RoomStr
{
	cstring id;
	RoomType* room;

	explicit RoomStr(cstring _id) : id(_id), room(nullptr)
	{

	}
};

//-----------------------------------------------------------------------------
struct RoomStrChance
{
	cstring id;
	RoomType* room;
	int chance;

	RoomStrChance(cstring _id, int _chance) : id(_id), chance(_chance), room(nullptr)
	{

	}
};

//-----------------------------------------------------------------------------
// Rodzaj pu�apek w lokacji
#define TRAPS_NORMAL (1<<0)
#define TRAPS_MAGIC (1<<1)
#define TRAPS_NEAR_ENTRANCE (1<<2)
#define TRAPS_NEAR_END (1<<3)

//-----------------------------------------------------------------------------
// Bazowa lokalizacja, rodzaj podziemi/krypty
struct BaseLocation
{
	cstring name;
	INT2 levels;
	int size, size_lvl, join_room, join_corridor, corridor_chance;
	INT2 corridor_size, room_size;
	int options;
	RoomStr schody, wymagany;
	VEC3 fog_color, fog_color_lvl, ambient_color, ambient_color_lvl;
	VEC2 fog_range, fog_range_lvl;
	float draw_range, draw_range_lvl;
	RoomStrChance* rooms;
	int room_count, room_total;
	cstring tex_floor, tex_wall, tex_ceil;
	int door_chance, door_open, bars_chance;
	SPAWN_GROUP sg1, sg2, sg3;
	int schance1, schance2, schance3;
	int traps, tex2;
	cstring tex_floor_nrm, tex_wall_nrm, tex_ceil_nrm, tex_floor_spec, tex_wall_spec, tex_ceil_spec;

	RoomType* GetRandomRoomType() const;
};
extern BaseLocation g_base_locations[];
extern const uint n_base_locations;
