#pragma once

#include "Engine.h"
#include "Const.h"
#include "GameCommon.h"
#include "BitStreamFunc.h"
#include "Object.h"
#include "ConsoleCommands.h"
#include "Quest.h"
#include "Net.h"
#include "Building.h"
#include "Dialog.h"
#include "BaseTrap.h"
#include "SpawnGroup.h"
#include "BaseLocation.h"
#include "Useable.h"
#include "Spell.h"
#include "Door.h"
#include "Bullet.h"
#include "SpellEffects.h"
#include "GroundItem.h"
#include "Trap.h"
#include "ParticleSystem.h"
#include "GameKeys.h"
#include "SceneNode.h"
#include "QuadTree.h"
#include "Music.h"
#include "PlayerInfo.h"

// gui
#include "MainMenu.h"
#include "Inventory.h"
#include "StatsPanel.h"
#include "TeamPanel.h"
#include "Dialog2.h"
#include "Console.h"
#include "GameMenu.h"
#include "Options.h"
#include "SaveLoadPanel.h"
#include "GetTextDialog.h"
#include "GetNumberDialog.h"
#include "Journal.h"
#include "Minimap.h"
#include "GameGui.h"
#include "WorldMapGui.h"
#include "CreateCharacterPanel.h"
#include "MultiplayerPanel.h"
#include "CreateServerPanel.h"
#include "PickServerPanel.h"
#include "ServerPanel.h"
#include "InfoBox.h"
#include "MpBox.h"
#include "LoadScreen.h"
#include "Controls.h"
#include "GameMessagesContainer.h"

// postacie
#include "Unit.h"
#include "HeroData.h"
#include "PlayerController.h"
#include "AIController.h"

// lokacje
#include "Location.h"
#include "OutsideLocation.h"
#include "City.h"
#include "Village.h"
#include "InsideLocation.h"
#include "SingleInsideLocation.h"
#include "MultiInsideLocation.h"
#include "CaveLocation.h"
#include "Camp.h"

//#define DRAW_LOCAL_PATH
#ifdef _DEBUG
#	define CHEATS_START_MODE true
#else
#	define CHEATS_START_MODE false
#endif
#ifdef DRAW_LOCAL_PATH
#	ifndef _DEBUG
#		error "DRAW_LOCAL_PATH in release!"
#	endif
#endif

struct APoint
{
	int odleglosc, koszt, suma, stan;
	INT2 prev;

	inline bool IsLower(int suma2) const
	{
		return stan == 0 || suma2 < suma;
	}
};

//-----------------------------------------------------------------------------
// Tryb szybkiego uruchamiania gry
enum QUICKSTART
{
	QUICKSTART_NONE,
	QUICKSTART_SINGLE,
	QUICKSTART_HOST,
	QUICKSTART_JOIN_LAN,
	QUICKSTART_JOIN_IP
};

//-----------------------------------------------------------------------------
// Stan gry
enum GAME_STATE
{
	GS_MAIN_MENU,
	GS_WORLDMAP,
	GS_LEVEL,
	GS_LOAD
};

//-----------------------------------------------------------------------------
// Stan mapy �wiata
enum WORLDMAP_STATE
{
	WS_MAIN,
	WS_TRAVEL,
	WS_ENCOUNTER
};

struct SpeechBubble;
struct EntityInterpolator;

enum AllowInput
{
	ALLOW_NONE =		0,	// 00	
	ALLOW_KEYBOARD =	1,	// 01
	ALLOW_MOUSE =		2,	// 10
	ALLOW_INPUT =		3	// 11
};

inline int KeyAllowState(byte k)
{
	if(k > 7)
		return ALLOW_KEYBOARD;
	else if(k != 0)
		return ALLOW_MOUSE;
	else
		return ALLOW_NONE;
}

enum COLLISION_GROUP
{
	CG_WALL = 1<<8,
	CG_UNIT = 1<<9
};

enum BeforePlayer
{
	BP_NONE,
	BP_UNIT,
	BP_CHEST,
	BP_DOOR,
	BP_ITEM,
	BP_USEABLE
};

union BeforePlayerPtr
{
	Unit* unit;
	Chest* chest;
	Door* door;
	GroundItem* item;
	Useable* useable;
	void* any;
};

extern const float ATTACK_RANGE;
extern const VEC2 alert_range;
extern const float ARROW_SPEED;
extern const float ARROW_TIMER;
extern const float MIN_H;

struct AttachedSound
{
	FMOD::Channel* channel;
	Unit* unit;
};

COMPILE_ASSERT(sizeof(time_t) == sizeof(__int64));

struct LoadTask
{
	enum Type
	{
		LoadShader,
		SetupShaders,
		LoadTex,
		LoadTex2,
		LoadMesh,
		LoadVertexData,
		LoadTrap,
		LoadSound,
		LoadObject,
		LoadTexResource,
		LoadItem,
		LoadMusic
	};

	Type type;
	cstring filename;
	union
	{
		ID3DXEffect** effect;
		TEX* tex;
		Texture* tex2;
		Animesh** mesh;
		VertexData** vd;
		BaseTrap* trap;
		SOUND* sound;
		Obj* obj;
		Resource** tex_res;
		Item* item;
		Music* music;
	};

	LoadTask(cstring _filename, ID3DXEffect** _effect) : type(LoadShader), filename(_filename), effect(_effect)
	{

	}
	LoadTask(Type _type) : type(_type)
	{

	}
	LoadTask(cstring _filename, TEX* _tex) : type(LoadTex), filename(_filename), tex(_tex)
	{

	}
	LoadTask(cstring _filename, Texture* _tex2) : type(LoadTex2), filename(_filename), tex2(_tex2)
	{

	}
	LoadTask(cstring _filename, Animesh** _mesh) : type(LoadMesh), filename(_filename), mesh(_mesh)
	{

	}
	LoadTask(cstring _filename, VertexData** _vd) : type(LoadVertexData), filename(_filename), vd(_vd)
	{

	}
	LoadTask(cstring _filename, BaseTrap* _trap) : type(LoadTrap), filename(_filename), trap(_trap)
	{

	}
	LoadTask(cstring _filename, SOUND* _sound) : type(LoadSound), filename(_filename), sound(_sound)
	{

	}
	LoadTask(cstring _filename, Obj* _obj) : type(LoadObject), filename(_filename), obj(_obj)
	{

	}
	LoadTask(cstring _filename, Resource** _tex_res) : type(LoadTexResource), filename(_filename), tex_res(_tex_res)
	{

	}
	LoadTask(cstring _filename, Item* _item) : type(LoadItem), filename(_filename), item(_item)
	{

	}
	LoadTask(Music* _music) : type(LoadMusic), filename(_music->file), music(_music)
	{

	}
};

struct UnitView
{
	Unit* unit;
	VEC3 last_pos;
	float time;
	bool valid;
};

enum GMS
{
	GMS_NEED_WEAPON = 1,
	GMS_NEED_KEY,
	GMS_NEED_LADLE,
	GMS_NEED_HAMMER,
	GMS_DONT_LOOT_FOLLOWER,
	GMS_JOURNAL_UPDATED,
	GMS_GATHER_TEAM,
	GMS_ADDED_RUMOR,
	GMS_UNLOCK_DOOR,
	GMS_CANT_DO,
	GMS_UNIT_BUSY,
	GMS_NOT_NOW,
	GMS_NOT_IN_COMBAT,
	GMS_IS_LOOTED,
	GMS_USED,
	GMS_DONT_LOOT_ARENA,
	GMS_NOT_LEADER,
	GMS_ONLY_LEADER_CAN_TRAVEL,
	GMS_NO_POTION,
	GMS_GAME_SAVED,
	GMS_NEED_PICKAXE,
	GMS_PICK_CHARACTER,
	GMS_ADDED_ITEM
};

struct UnitWarpData
{
	Unit* unit;
	int where;
};

#define FALLBACK_TRAIN 0
#define FALLBACK_REST 1
#define FALLBACK_ARENA 2
#define FALLBACK_ENTER 3
#define FALLBACK_EXIT 4
#define FALLBACK_CHANGE_LEVEL 5
#define FALLBACK_NONE 6
#define FALLBACK_ARENA_EXIT 7
#define FALLBACK_USE_PORTAL 8
#define FALLBACK_WAIT_FOR_WARP 9
#define FALLBACK_ARENA2 10
#define FALLBACK_CLIENT 11
#define FALLBACK_CLIENT2 12

enum InventoryMode
{
	I_NONE,
	I_INVENTORY,
	I_LOOT_BODY,
	I_LOOT_CHEST,
	I_TRADE,
	I_SHARE,
	I_GIVE
};

inline PlayerController::Action InventoryModeToActionRequired(InventoryMode imode)
{
	switch(imode)
	{
	case I_NONE:
	case I_INVENTORY:
		return PlayerController::Action_None;
	case I_LOOT_BODY:
		return PlayerController::Action_LootUnit;
	case I_LOOT_CHEST:
		return PlayerController::Action_LootChest;
	case I_TRADE:
		return PlayerController::Action_Trade;
	case I_SHARE:
		return PlayerController::Action_ShareItems;
	case I_GIVE:
		return PlayerController::Action_GiveItems;
	default:
		assert(0);
		return PlayerController::Action_None;
	}
}

struct TeamShareItem
{
	Unit* from, *to;
	const Item* item;
	int index, value;
};

typedef bool (*BoolFunc)();

struct Encounter
{
	VEC2 pos;
	int szansa;
	float zasieg;
	bool dont_attack, timed;
	DialogEntry* dialog;
	SPAWN_GROUP grupa;
	cstring text;
	Quest_Encounter* quest; // tak naprawd� nie musi to by� Quest_Encounter, mo�e by� zwyk�y Quest, chyba �e jest to czasowy encounter!
	LocationEventHandler* location_event_handler;
	// nowe pola
	BoolFunc check_func;

	// dla kompatybilno�ci ze starym kodem, ustawia tylko nowe pola
	Encounter() : check_func(NULL)
	{

	}
};

extern const VEC2 POISSON_DISC_2D[];
extern const int poisson_disc_count;

struct QuestItemRequest
{
	const Item** item;
	string name;
	int quest_refid;
	vector<ItemSlot>* items;
	Unit* unit;
};

enum PLOTKA_QUESTOWA
{
	P_TARTAK,
	P_KOPALNIA,
	P_ZAWODY_W_PICIU,
	P_BANDYCI,
	P_MAGOWIE,
	P_MAGOWIE2,
	P_ORKOWIE,
	P_GOBLINY,
	P_ZLO,
	P_MAX
};

enum DRAW_FLAGS
{
	DF_TERRAIN = 1<<0,
	DF_OBJECTS = 1<<1,
	DF_UNITS = 1<<2,
	DF_PARTICLES = 1<<3,
	DF_SKYBOX =  1<<4,
	DF_BULLETS = 1<<5,
	DF_BLOOD = 1<<6,
	DF_ITEMS = 1<<7,
	DF_USEABLES = 1<<8,
	DF_TRAPS = 1<<9,
	DF_AREA = 1<<10,
	DF_EXPLOS = 1<<11,
	DF_LIGHTINGS = 1<<12,
	DF_PORTALS = 1<<13,
	DF_GUI = 1<<14,
	DF_MENU = 1<<15,
};

struct TutorialText
{
	cstring text;
	VEC3 pos;
	int state; // 0 - nie aktywny, 1 - aktywny, 2 - uruchomiony
	int id;
};

typedef fastdelegate::FastDelegate1<cstring> PrintMsgFunc;

cstring PacketToString(Packet* packet);

struct EntityInterpolator
{
	static const int MAX_ENTRIES = 4;
	struct Entry
	{
		VEC3 pos;
		float rot;
		float timer;

		inline void operator = (const Entry& e)
		{
			pos = e.pos;
			rot = e.rot;
			timer = e.timer;
		}
	} entries[MAX_ENTRIES];
	int valid_entries;

	void Reset(const VEC3& pos, float rot);
};

struct TimedUnit
{
	Unit* unit;
	int location;
	int days;
};

const float UNIT_VIEW_A = 0.2f;
const float UNIT_VIEW_B = 0.4f;
const int UNIT_VIEW_MUL = 5;

struct Terrain;

struct PostEffect
{
	int id;
	D3DXHANDLE tech;
	float power;
	VEC4 skill;
};

enum SuperShaderSwitches
{
	SSS_ANIMATED,
	SSS_HAVE_BINORMALS,
	SSS_FOG,
	SSS_SPECULAR,
	SSS_NORMAL,
	SSS_POINT_LIGHT,
	SSS_DIR_LIGHT
};

struct SuperShader
{
	ID3DXEffect* e;
	uint id;
};

class CityGenerator;

enum class OpenPanel
{
	None,
	Stats,
	Inventory,
	Team,
	Journal,
	Minimap,
	Action,
	Trade,
	Unknown
};

struct Game : public Engine, public UnitEventHandler
{
	Game();
	~Game();

	void InitGame();
	void OnCleanup();
	void OnDraw();
	void OnDraw(bool normal=true);
	void OnTick(float dt);
	void OnChar(char c);
	void OnReload();
	void OnReset();
	void OnResize();
	void OnFocus(bool focus);

	bool Start0(bool fullscreen, int w, int h);
	bool GetTitle(LocalString& s);
	void ChangeTitle();
	Texture LoadTex2(cstring name);
	void LoadData();
	void ClearPointers();
	void CreateTextures();
	void PreloadData();
	void SetMeshSpecular();

	void InitGameText();
	void LoadUnitsText();
	void LoadStatsText();
	void LoadNames();

	QUICKSTART quickstart;
	HANDLE mutex;

	// supershader
	string sshader_code;
	FILETIME sshader_edit_time;
	ID3DXEffectPool* sshader_pool;
	vector<SuperShader> sshaders;
	D3DXHANDLE hSMatCombined, hSMatWorld, hSMatBones, hSTint, hSAmbientColor, hSFogColor, hSFogParams, hSLightDir, hSLightColor, hSLights, hSSpecularColor, hSSpecularIntensity,
		hSSpecularHardness, hSCameraPos, hSTexDiffuse, hSTexNormal, hSTexSpecular;
	void InitSuperShader();
	SuperShader* GetSuperShader(uint id);
	SuperShader* CompileSuperShader(uint id);
	inline uint GetSuperShaderId(bool animated, bool have_binormals, bool fog, bool specular, bool normal, bool point_light, bool dir_light) const
	{
		uint id = 0;
		if(animated)
			id |= (1<<SSS_ANIMATED);
		if(have_binormals)
			id |= (1<<SSS_HAVE_BINORMALS);
		if(fog)
			id |= (1<<SSS_FOG);
		if(specular)
			id |= (1<<SSS_SPECULAR);
		if(normal)
			id |= (1<<SSS_NORMAL);
		if(point_light)
			id |= (1<<SSS_POINT_LIGHT);
		if(dir_light)
			id |= (1<<SSS_DIR_LIGHT);
		return id;
	}

	float light_angle;
	bool dungeon_tex_wrap;

	void SetupSuperShader();
	void ReloadShaders();
	void ReleaseShaders();
	void ShaderVersionChanged();

	// scene
	bool cl_normalmap, cl_specularmap, cl_glow;
	bool r_alphatest, r_nozwrite, r_nocull, r_alphablend;
	DrawBatch draw_batch;
	VDefault blood_v[4];
	VParticle billboard_v[4];
	VEC3 billboard_ext[4];
	VParticle portal_v[4];
	IDirect3DVertexDeclaration9* vertex_decl[VDI_MAX];
	int uv_mod;
	QuadTree quadtree;
	float grass_range;
	LevelParts level_parts;
	VB vbInstancing;
	uint vb_instancing_max;
	vector< const vector<MATRIX>* > grass_patches[2];
	uint grass_count[2];

	void InitScene();
	void CreateVertexDeclarations();
	void BuildDungeon();
	void ChangeDungeonTexWrap();
	void FillDungeonPart(INT2* dungeon_part, word* faces, int& index, word offset);
	void CleanScene();
	void ListDrawObjects(LevelContext& ctx, FrustumPlanes& frustum, bool outside);
	void ListDrawObjectsUnit(LevelContext* ctx, FrustumPlanes& frustum, bool outside, Unit& u);
	void FillDrawBatchDungeonParts(FrustumPlanes& frustum);
	void AddOrSplitSceneNode(SceneNode* node, int exclude_subs=0);
	int GatherDrawBatchLights(LevelContext& ctx, SceneNode* node, float x, float z, float radius, int sub=0);
	void DrawScene(bool outside);
	void DrawGlowingNodes(bool use_postfx);
	void DrawSkybox();
	void DrawTerrain(const vector<uint>& parts);
	void DrawDungeon(const vector<DungeonPart>& parts, const vector<Lights>& lights, const vector<NodeMatrix>& matrices);
	void DrawSceneNodes(const vector<SceneNode*>& nodes, const vector<Lights>& lights, bool outside);
	void DrawDebugNodes(const vector<DebugSceneNode*>& nodes);
	void DrawBloods(bool outside, const vector<Blood*>& bloods);
	void DrawBillboards(const vector<Billboard>& billboards);
	void DrawExplosions(const vector<Explo*>& explos);
	void DrawParticles(const vector<ParticleEmitter*>& pes);
	void DrawTrailParticles(const vector<TrailParticleEmitter*>& tpes);
	void DrawLightings(const vector<Electro*>& electros);
	void DrawAreas(const vector<Area>& areas, float range);
	void DrawPortals(const vector<Portal*>& portals);
	inline void SetAlphaTest(bool use_alphatest)
	{
		if(use_alphatest != r_alphatest)
		{
			r_alphatest = use_alphatest;
			V( device->SetRenderState(D3DRS_ALPHATESTENABLE, r_alphatest ? TRUE : FALSE) );
		}
	}
	inline void SetNoZWrite(bool use_nozwrite)
	{
		if(use_nozwrite != r_nozwrite)
		{
			r_nozwrite = use_nozwrite;
			V( device->SetRenderState(D3DRS_ZWRITEENABLE, r_nozwrite ? FALSE : TRUE) );
		}
	}
	inline void SetNoCulling(bool use_nocull)
	{
		if(use_nocull != r_nocull)
		{
			r_nocull = use_nocull;
			V( device->SetRenderState(D3DRS_CULLMODE, r_nocull ? D3DCULL_NONE : D3DCULL_CCW) );
		}
	}
	inline void SetAlphaBlend(bool use_alphablend)
	{
		if(use_alphablend != r_alphablend)
		{
			r_alphablend = use_alphablend;
			V( device->SetRenderState(D3DRS_ALPHABLENDENABLE, r_alphablend ? TRUE : FALSE) );
		}
	}
	void UvModChanged();
	void InitQuadTree();
	void DrawGrass();
	void ListGrass();
	void SetTerrainTextures();
	void ClearQuadtree();
	void ClearGrass();

	// profiler
	int profiler_mode;

	//-----------------------------------------------------------------
	// ZASOBY
	//-----------------------------------------------------------------
	Animesh* aHumanBase, *aHair[5], *aBeard[5], *aMustache[2], *aEyebrows;
	Animesh* aBox, *aCylinder, *aSphere, *aCapsule;
	Animesh* aArrow, *aSkybox, *aWorek, *aSkrzynia, *aKratka, *aNaDrzwi, *aNaDrzwi2, *aSchodyDol, *aSchodyGora, *aSchodyDol2, *aSpellball, *aPrzycisk, *aBeczka, *aDrzwi, *aDrzwi2;
	VertexData* vdSchodyGora, *vdSchodyDol, *vdNaDrzwi;
	TEX tItemRegion, tMinimap, tChar, tSave;
	TEX tMapBg, tWorldMap, tMapIcon[L_MAX], tEnc, tSelected[2], tMover, tCzern, tEmerytura, tPortal, tLightingLine, tKlasaCecha, tPaczka, tRip, tCelownik, tObwodkaBolu, tEquipped,
		tDialogUp, tDialogDown, tBubble, tMiniunit, tMiniunit2, tSchodyDol, tSchodyGora, tList, tListGonczy, tIcoHaslo, tIcoZapis, tGotowy, tNieGotowy, tTrawa, tTrawa2, tTrawa3, tZiemia,
		tDroga, tMiniSave, tWczytywanie[2], tMiniunit3, tMiniunit4, tMiniunit5, tMinibag, tMinibag2, tMiniportal, tBuffPoison, tBuffAlcohol, tBuffRegeneration, tBuffFood, tBuffNatural, tPole;
	Texture tKrew[BLOOD_MAX], tKrewSlad[BLOOD_MAX], tFlare, tFlare2, tIskra, tWoda;
	TexturePack tFloor[2], tWall[2], tCeil[2], tFloorBase, tWallBase, tCeilBase;
	ID3DXEffect* eMesh, *eParticle, *eSkybox, *eTerrain, *eArea, *eGui, *ePostFx, *eGlow, *eGrass;
	D3DXHANDLE techAnim, techHair, techAnimDir, techHairDir, techMesh, techMeshDir, techMeshSimple, techMeshSimple2, techMeshExplo, techParticle, techSkybox, techTerrain, techArea, techTrail,
		techGui, techGlowMesh, techGlowAni, techGrass;
	D3DXHANDLE hAniCombined, hAniWorld, hAniBones, hAniTex, hAniFogColor, hAniFogParam, hAniTint, hAniHairColor, hAniAmbientColor, hAniLightDir, hAniLightColor, hAniLights,
		hMeshCombined, hMeshWorld, hMeshTex, hMeshFogColor, hMeshFogParam, hMeshTint, hMeshAmbientColor, hMeshLightDir, hMeshLightColor, hMeshLights,
		hParticleCombined, hParticleTex, hSkyboxCombined, hSkyboxTex, hAreaCombined, hAreaColor, hAreaPlayerPos, hAreaRange,
		hTerrainCombined, hTerrainWorld, hTerrainTexBlend, hTerrainTex[5], hTerrainColorAmbient, hTerrainColorDiffuse, hTerrainLightDir, hTerrainFogColor, hTerrainFogParam,
		hGuiSize, hGuiTex, hPostTex, hPostPower, hPostSkill, hGlowCombined, hGlowBones, hGlowColor, hGrassViewProj, hGrassTex, hGrassFogColor, hGrassFogParams, hGrassAmbientColor;
	SOUND sGulp, sMoneta, sBow[2], sDoor[3], sDoorClosed, sDoorClose, sItem[8], sTalk[4], sChestOpen, sChestClose, sDoorBudge, sRock, sWood, sCrystal, sMetal, sBody[5], sBone, sSkin, sArenaFight,
		sArenaWygrana, sArenaPrzegrana, sUnlock, sEvil, sXarTalk, sOrcTalk, sGoblinTalk, sGolemTalk, sEat;
	VB vbParticle;
	SURFACE sChar, sSave, sItemRegion;
	static cstring txGoldPlus, txQuestCompletedGold;
	cstring txAiNoHpPot[2], txAiJoinTour[4], txAiCity[2], txAiVillage[2], txAiMoonwell, txAiForest, txAiCampEmpty, txAiCampFull, txAiFort, txAiDwarfFort, txAiTower, txAiArmory, txAiHideout,
		txAiVault, txAiCrypt, txAiTemple, txAiNecromancerBase, txAiLabirynth, txAiNoEnemies, txAiNearEnemies, txAiCave, txAiInsaneText[11], txAiDefaultText[9], txAiOutsideText[3],
		txAiInsideText[2], txAiHumanText[2], txAiOrcText[7], txAiGoblinText[5], txAiMageText[4], txAiSecretText[3], txAiHeroDungeonText[4], txAiHeroCityText[5], txAiBanditText[6],
		txAiHeroOutsideText[2], txAiDrunkMageText[3], txAiDrunkText[5], txAiDrunkmanText[4];
	cstring txRandomEncounter, txCamp;
	cstring txEnteringLocation, txGeneratingMap, txGeneratingBuildings, txGeneratingObjects, txGeneratingUnits, txGeneratingItems, txGeneratingPhysics, txRecreatingObjects, txGeneratingMinimap,
		txLoadingComplete, txWaitingForPlayers, txGeneratingTerrain;
	cstring txContestNoWinner, txContestStart, txContestTalk[14], txContestWin, txContestWinNews, txContestDraw, txContestPrize, txContestNoPeople;
	cstring txTut[10], txTutNote, txTutLoc, txTour[23], txTutPlay, txTutTick;
	cstring txCantSaveGame, txSaveFailed, txSavedGameN, txLoadFailed, txQuickSave, txGameSaved, txLoadingLocations, txLoadingData, txLoadingQuests, txEndOfLoading, txCantSaveNow, txCantLoadGame,
		txLoadSignature, txLoadVersion, txLoadSaveVersionNew, txLoadSaveVersionOld, txLoadMP, txLoadSP, txLoadError, txLoadOpenError;
	cstring txPvpRefuse, txSsFailed, txSsDone, txLoadingResources, txLoadingShader, txConfiguringShaders, txLoadingTexture, txLoadingMesh, txLoadingMeshVertex, txLoadingSound, txLoadingMusic,
		txWin, txWinMp, txINeedWeapon, txNoHpp, txCantDo, txDontLootFollower, txDontLootArena, txUnlockedDoor, txNeedKey, txLevelUp, txLevelDown, txLocationText, txLocationTextMap,
		txRegeneratingLevel, txGmsLooted, txGmsRumor, txGmsJournalUpdated, txGmsUsed, txGmsUnitBusy, txGmsGatherTeam, txGmsNotLeader, txGmsNotInCombat, txGainStr, txGainDex, txGainEnd,
		txGainOneHanded, txGainBow, txGainShield, txGainLightArmor, txGainHeavyArmor, txGainText, txNeedLadle, txNeedPickaxe, txNeedHammer, txNeedUnk, txRealyQuit, txSecretAppear, txGmsAddedItem,
		txGmsAddedItems;
	cstring txRumor[28], txRumorD[7];
	cstring txMayorQFailed[3], txQuestAlreadyGiven[2], txMayorNoQ[2], txCaptainQFailed[2], txCaptainNoQ[2], txLocationDiscovered[2], txAllDiscovered[2], txCampDiscovered[2],
		txAllCampDiscovered[2], txNoQRumors[2], txRumorQ[9], txNeedMoreGold, txNoNearLoc, txNearLoc, txNearLocEmpty[2], txNearLocCleared, txNearLocEnemy[2], txNoNews[2], txAllNews[2], txPvpTooFar,
		txPvp, txPvpWith, txNewsCampCleared, txNewsLocCleared, txArenaText[3], txArenaTextU[5];
	cstring txNear, txFar, txVeryFar, txELvlVeryWeak[2], txELvlWeak[2], txELvlAvarage[2], txELvlQuiteStrong[2], txELvlStrong[2];
	cstring txSGOOrcs, txSGOGoblins, txSGOBandits, txSGOEnemies, txSGOUndeads, txSGOMages, txSGOGolems, txSGOMagesAndGolems, txSGOUnk, txSGOPowerfull;
	cstring txArthur, txMineBuilt, txAncientArmory, txPortalClosed, txPortalClosedNews, txHiddenPlace, txOrcCamp, txPortalClose, txPortalCloseLevel, txXarDanger, txGorushDanger, txGorushCombat,
		txMageHere, txMageEnter, txMageFinal, txQuest[267], txForMayor, txForSoltys;
	cstring txEnterIp, txConnecting, txInvalidIp, txWaitingForPswd, txEnterPswd, txConnectingTo, txConnectTimeout, txConnectInvalid, txConnectVersion, txConnectRaknet, txCantJoin, txLostConnection,
		txInvalidPswd, txCantJoin2, txServerFull, txInvalidData, txNickUsed, txInvalidVersion, txInvalidVersion2, txInvalidNick, txGeneratingWorld, txLoadedWorld, txWorldDataError, txLoadedPlayer,
		txPlayerDataError, txGeneratingLocation, txLoadingLocation, txLoadingLocationError, txLoadingChars, txLoadingCharsError, txSendingWorld, txMpNPCLeft, txLoadingLevel, txDisconnecting,
		txLost, txLeft, txLost2, txUnconnected, txDisconnected, txClosing, txKicked, txUnknown, txUnknown2, txWaitingForServer, txStartingGame;
	cstring txCreateServerFailed, txInitConnectionFailed, txServer, txPlayerKicked, txYouAreLeader, txRolledNumber, txPcIsLeader, txReceivedGold, txYouDisconnected, txYouKicked, txPcWasKicked,
		txPcLeftGame, txGamePaused, txGameResumed, txCanUseCheats, txCantUseCheats, txPlayerLeft;
	cstring txDialog[1302], txYell[3];

	static Game* _game;
	static Game& Get()
	{
		return *_game;
	}

	//-----------------------------------------------------------------
	// GAME
	//---------------------------------
	// KAMERA
	VEC3 prev_from, prev_to, camera_center; // poprzednia pozycja kamery
	MATRIX tmp_matViewProj, tmp_matViewInv;
	bool first_frame; // resetuje pozycje kamery
	FrustumPlanes camera_frustum, camera_frustum2;
	float rotY, cam_dist, draw_range, rot_buf;

	//---------------------------------
	// GUI / HANDEL
	InventoryMode inventory_mode;
	vector<ItemSlot> chest_merchant, chest_blacksmith, chest_alchemist, chest_innkeeper, chest_food_seller, chest_trade;
	bool* trader_buy;

	//---------------------------------
	// RYSOWANIE
	MATRIX mat;
	int particle_count;
	Terrain* terrain;
	VB vbDungeon;
	IB ibDungeon;
	INT2 dungeon_part[16], dungeon_part2[16], dungeon_part3[16], dungeon_part4[16];
	vector<ParticleEmitter*> pes2;
	VEC4 fog_color, fog_params, ambient_color;
	int alpha_test_state;
	bool cl_fog, cl_lighting, draw_particle_sphere, draw_unit_radius, draw_hitbox, draw_phy, draw_col;
	Obj obj_alpha;
	float portal_anim, drunk_anim;
	// post effect u�ywa 3 tekstur lub je�li jest w��czony multisampling 3 surface i 1 tekstury
	SURFACE sPostEffect[3];
	TEX tPostEffect[3];
	VB vbFullscreen;
	vector<PostEffect> post_effects;
	SURFACE sCustom;

	//---------------------------------
	// KONSOLA I KOMENDY
	bool have_console, console_open, inactive_update, nosound, noai, cheats, used_cheats, debug_info, debug_info2, dont_wander, nomusic;
	string cfg_file;
	vector<ConsoleCommand> cmds;
	int sound_volume, music_volume, mouse_sensitivity;

	//---------------------------------
	// GRA
	GAME_STATE game_state, prev_game_state;
	PlayerController* pc;
	AllowInput allow_input;
	bool testing, force_seed_all, koniec_gry, local_ctx_valid, target_loc_is_camp, exit_mode, exit_to_menu;
	int death_screen, dungeon_level;
	bool death_solo;
	float death_fade, speed;
	vector<AnimeshInstance*> bow_instances;
	Pak* pak;
	Unit* selected_unit, *selected_target;
	vector<AIController*> ais;
	Item gold_item;
	BeforePlayer before_player;
	BeforePlayerPtr before_player_ptr;
	uint force_seed, next_seed;
	bool next_seed_extra;
	int next_seed_val[3];
	vector<AttachedSound> attached_sounds;
	SaveSlot single_saves[MAX_SAVE_SLOTS], multi_saves[MAX_SAVE_SLOTS];
	vector<UnitView> unit_views;
	vector<UnitWarpData> unit_warp_data;
	LevelContext local_ctx;
	ObjectPool<TmpLevelContext> tmp_ctx_pool;
	City* city_ctx; // je�eli jest w mie�cie/wiosce to ten wska�nik jest ok, takto NULL
	vector<Unit*> to_remove;
	CityGenerator* gen;

	//---------------------------------
	// SCREENSHOT
	time_t last_screenshot;
	uint screenshot_count;
	D3DXIMAGE_FILEFORMAT screenshot_format;

	//---------------------------------
	// DIALOGI
	DialogContext dialog_context;
	DialogContext* current_dialog;
	INT2 dialog_cursor_pos; // ostatnia pozycja myszki przy wyborze dialogu
	vector<string> dialog_choices; // u�ywane w MP u klienta
	string predialog;

	//---------------------------------
	// PATHFINDING
	vector<APoint> a_map;
#ifdef DRAW_LOCAL_PATH
	vector<std::pair<VEC2, int> > test_pf;
	Unit* marked;
	bool test_pf_outside;
#endif
	vector<bool> local_pfmap;

	//---------------------------------
	// FIZYKA
	btCollisionShape* shape_wall, *shape_low_celling, *shape_arrow, *shape_celling, *shape_floor, *shape_door, *shape_block, *shape_schody, *shape_schody_c[2];
	btHeightfieldTerrainShape* terrain_shape;
	btCollisionObject* obj_arrow, *obj_terrain, *obj_spell;
	vector<CollisionObject> global_col; // wektor na tymczasowe obiekty, cz�sto u�ywany przy zbieraniu obiekt�w do kolizji
	vector<btCollisionShape*> shapes;
	vector<CameraCollider> cam_colliders;

	//---------------------------------
	// WIADOMO�CI / NOTATKI / PLOTKI
	vector<string> notes;
	vector<string> plotki;

	//---------------------------------
	// WCZYTYWANIE
	float load_game_progress, loading_dt;
	string load_game_text;
	vector<LoadTask> load_tasks;
	Timer loading_t;
	int loading_steps, loading_index;
	DWORD clear_color2;
	
	//---------------------------------
	// MINIMAPA
	bool minimap_opened_doors;
	vector<INT2> minimap_reveal;

	//---------------------------------
	// FALLBACK
	int fallback_co, fallback_1, fallback_2;
	float fallback_t;

	//--------------------------------------
	// ARENA
	// etapy: 0-brak, 1-walka wchodzenie, 2-walka oczekiwanie, 3-walka, 4-walka oczekiwanie po, 5-walka koniec
	// 10-pvp wchodzenie, 11-pvp oczekiwanie, 12-pvp 13-pvp oczekiwanie po, 14-pvp koniec
	enum TrybArena
	{
		Arena_Brak,
		Arena_Walka,
		Arena_Pvp,
		Arena_Zawody
	} arena_tryb;
	enum EtapAreny
	{
		Arena_OdliczanieDoPrzeniesienia,
		Arena_OdliczanieDoStartu,
		Arena_TrwaWalka,
		Arena_OdliczanieDoKonca,
		Arena_OdliczanieDoWyjscia
	} arena_etap;
	int arena_poziom; // poziom trudno�ci walki na arenie [1-3]
	int arena_wynik; // wynik walki na arenia [0 - gracz wygra�, 1 - gracz przegra�]
	vector<Unit*> at_arena; // jednostki na arenie
	float arena_t; // licznik czasu na arenie
	bool arena_free; // czy arena jest wolna
	Unit* arena_fighter; // posta� z kt�r� walczy si� na arenie w pvp
	vector<Unit*> near_players;
	vector<string> near_players_str;
	Unit* pvp_unit;
	struct PvpResponse
	{
		Unit* from, *to;
		float timer;
		bool ok;
	} pvp_response;
	PlayerController* pvp_player;
	vector<Unit*> arena_viewers;

	void CleanArena();

	//--------------------------------------
	// ZAWODY
	enum IRONFIST_STATE
	{
		IS_BRAK,
		IS_ROZPOCZYNANIE,
		IS_TRWAJA
	} zawody_stan;
	int zawody_rok, zawody_rok_miasta, zawody_miasto, zawody_stan2, zawody_stan3, zawody_runda, zawody_arena;
	vector<Unit*> zawody_ludzie;
	float zawody_czas;
	Unit* zawody_mistrz, *zawody_niewalczacy, *zawody_drugi_zawodnik, *zawody_zwyciezca;
	vector<std::pair<Unit*, Unit*> > zawody_walczacy;
	bool zawody_wygenerowano;

	void StartTournament(Unit* arena_master);
	bool IfUnitJoinTournament(Unit& u);
	void GenerateTournamentUnits();
	void UpdateTournament(float dt);
	void StartTournamentRound();
	void TournamentTalk(cstring text);
	void TournamentTrain(Unit& u);
	void CleanTournament();

	//--------------------------------------
	// DRU�YNA
	vector<Unit*> team; // wszyscy cz�onkowie dru�yny
	vector<Unit*> active_team; // cz�onkowie dru�yny kt�rzy maj� udzia� w �upach (bez questowych postaci)
	Unit* leader; // przyw�dca dru�yny
	int team_gold; // niepodzielone z�oto dru�ynowe
	int take_item_id; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	int team_share_id; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	vector<TeamShareItem> team_shares; // u�ywane przy wymianie ekwipunku ai [tymczasowe]
	bool atak_szalencow, free_recruit, bandyta;
	vector<INT2> tmp_path;

	void AddTeamMember(Unit* unit, bool active);
	void RemoveTeamMember(Unit* unit);

	//--------------------------------------
	// QUESTY
	vector<Quest*> unaccepted_quests; // niezaakceptowane questy
	vector<Quest*> quests; // zaakceptowane questy
	vector<Quest_Dungeon*> quests_timeout; // questy ograniczone czasowo [po jakim� czasie lokacja znika albo nie tworzy jednostki]
	int quest_counter; // licznik zada�
	vector<QuestItemRequest*> quest_item_requests;
	inline void AddQuestItemRequest(const Item** item, cstring name, int quest_refid, vector<ItemSlot>* items, Unit* unit=NULL)
	{
		assert(item && name && quest_refid != -1);
		QuestItemRequest* q = new QuestItemRequest;
		q->item = item;
		q->name = name;
		q->quest_refid = quest_refid;
		q->items = items;
		q->unit = unit;
		quest_item_requests.push_back(q);
	}
	int ile_plotek_questowych;
	bool plotka_questowa[P_MAX];
	int unique_quests_completed;
	bool unique_completed_show;
	int tartak_miasto, tartak_dni, tartak_stan, tartak_refid, tartak_stan2, tartak_las;
	HumanData hd_artur_drwal;
	Unit* tartak_poslaniec;
	enum KopalniaStan
	{
		KS_BRAK,
		KS_WYGENEROWANO_INWESTORA,
		KS_MASZ_UDZIALY,
		KS_MASZ_DUZE_UDZIALY
	};
	KopalniaStan kopalnia_stan; // 0 - brak, 1 - wygenerowano inwestora, 2 - masz udzia�y, 3 - masz du�e udzia�y
	enum KopalniaStan2
	{
		KS2_BRAK,
		KS2_TRWA_BUDOWA,
		KS2_WYBUDOWANO,
		KS2_MOZLIWA_ROZBUDOWA,
		KS2_TRWA_ROZBUDOWA,
		KS2_ROZBUDOWANO,
		KS2_ZNALEZIONO_PORTAL
	};
	KopalniaStan2 kopalnia_stan2; // 0 - brak, 1 - trwa budowa, 2 - wybudowano, 3 - trwa rozbudowa, 4 - rozbudowano, 5 - znaleziono portal
	enum KopalniaStan3
	{
		KS3_BRAK,
		KS3_WYGENEROWANO,
		KS3_WYGENEROWANO_W_BUDOWIE,
		KS3_WYGENEROWANO_WYBUDOWANY,
		KS3_WYGENEROWANO_ROZBUDOWANY,
		KS3_WYGENEROWANO_PORTAL
	};
	KopalniaStan3 kopalnia_stan3; // 0 - brak, 1 - wygeneruj, 2 - wygenerowano w budowie, 3 - wygenerowano wybudowany, 4 - wygenerowano rozbudowany, 5 - wygenerowano portal
	int kopalnia_miasto, kopalnia_gdzie, kopalnia_refid, kopalnia_dni, kopalnia_ile_dni, kopalnia_dni_zloto;
	Unit* kopalnia_poslaniec;
	// zawody w piciu (0 - nie by�o, 1 - by�o, 2 - dzisiaj, 3 - pocz�tek, 4 - trwa)
	int chlanie_gdzie, chlanie_stan, chlanie_stan2;
	vector<Unit*> chlanie_ludzie;
	float chlanie_czas;
	bool chlanie_wygenerowano;
	Unit* chlanie_zwyciesca;
	void UpdateContest(float dt);
	// quest bandyci
	enum BandyciStan
	{
		BS_BRAK,
		BS_WYGENEROWANO_ARTO,
		BS_WYGENERUJ_STRAZ,
		BS_WYGENEROWANO_STRAZ,
		BS_ODLICZANIE,
		BS_AGENT_PRZYSZEDL,
		BS_AGENT_POGADAL,
		BS_AGENT_POSZEDL
	};
	int bandyci_refid, bandyci_miasto, bandyci_gdzie;
	BandyciStan bandyci_stan;
	float bandyci_czas;
	Unit* bandyci_agent;
	// quest magowie
	enum MagowieStan
	{
		MS_BRAK,
		MS_WYGENEROWANO_UCZONEGO,
		MS_UCZONY_CZEKA,
		MS_ODLICZANIE,
		MS_SPOTKANIE,
		MS_SPOTKANO_GOLEMA,
		MS_POROZMAWIANO_Z_KAPITANEM,
		MS_WYGENEROWANO_STAREGO_MAGA,
		MS_STARY_MAG_DOLACZYL,
		MS_PRZYPOMNIAL_SOBIE,
		MS_KUP_MIKSTURE,
		MS_MAG_WYLECZONY,
		MS_MAG_IDZIE,
		MS_MAG_POSZEDL,
		MS_MAG_WYGENEROWANY_W_MIESCIE,
		MS_MAG_ZREKRUTOWANY,
		MS_UKONCZONO
	};
	MagowieStan magowie_stan;
	int magowie_refid, magowie_refid2, magowie_miasto, magowie_dni, magowie_gdzie;
	Unit* magowie_uczony;
	bool magowie_zaplacono;
	float magowie_czas;
	string magowie_imie, magowie_imie_dobry;
	HumanData magowie_hd;
	// quest orkowie
	enum OrkowieStan
	{
		OS_BRAK,
		OS_WYGENEROWANO_STRAZNIKA,
		OS_STRAZNIK_POGADAL,
		OS_ZAAKCEPTOWANO,
		OS_ORK_DOLACZYL,
		OS_UKONCZONO,
		OS_UKONCZONO_DOLACZYL,
		OS_POWIEDZIAL_O_OBOZIE,
		OS_OCZYSZCZONO,
		OS_WYBRAL_KLASE,
		OS_POWIEDZIAL_O_BAZIE,
		OS_GENERUJ_ORKI,
		OS_WYGENEROWANO_ORKI,
		OS_WYCZYSC,
		OS_KONIEC
	};
	enum GorushKlasa
	{
		GORUSH_BRAK,
		GORUSH_WOJ,
		GORUSH_LOWCA,
		GORUSH_SZAMAN
	};
	OrkowieStan orkowie_stan;
	int orkowie_miasto, orkowie_refid, orkowie_refid2, orkowie_dni, orkowie_gdzie;
	Unit* orkowie_straznik, *orkowie_gorush;
	GorushKlasa orkowie_klasa;
	vector<ItemSlot> orkowie_towar;
	// quest gobliny
	enum GOBLINY_STAN
	{
		GS_BRAK,
		GS_WYGENEROWANO_SZLACHCICA,
		GS_ODLICZANIE,
		GS_POSLANIEC_POGADAL,
		GS_ODDANO_LUK,
		GS_SZLACHCIC_ZNIKNAL,
		GS_WYGENEROWANO_MAGA,
		GS_MAG_POGADAL_START,
		GS_MAG_POGADAL,
		GS_MAG_POSZEDL,
		GS_POZNANO_LOKACJE
	};
	GOBLINY_STAN gobliny_stan;
	int gobliny_refid, gobliny_miasto, gobliny_dni;
	Unit* gobliny_szlachcic, *gobliny_poslaniec;
	HumanData gobliny_hd;
	// quest z�o
	enum ZLO_STAN
	{
		ZS_BRAK,
		ZS_WYGENEROWANO_KAPLANA,
		ZS_OLTARZ_STWORZONY,
		ZS_PRZYWOLANIE,
		ZS_GENERUJ_MAGA,
		ZS_WYGENEROWANO_MAGA,
		ZS_ZAMYKANIE_PORTALI,
		ZS_ZABIJ_BOSSA,
		ZS_JOZAN_CHCE_GADAC,
		ZS_JOZAN_IDZIE,
		ZS_JOZAN_POSZEDL
	};
	ZLO_STAN zlo_stan;
	int zlo_refid, zlo_miasto, zlo_gdzie, zlo_gdzie2;
	VEC3 zlo_pos;
	float zlo_czas;
	Unit* jozan;
	// quest szaleni
	enum SzaleniStan
	{
		SS_BRAK,
		SS_POGADANO_Z_SZALONYM,
		SS_WZIETO_KAMIEN,
		SS_PIERWSZY_ATAK,
		SS_POGADANO_Z_TRENEREM,
		SS_KONIEC
	} szaleni_stan;
	int szaleni_refid, szaleni_dni;
	bool szaleni_sprawdz_kamien;
	void SzaleniSprawdzKamien();
	// sekretny quest
	enum SekretStan
	{
		SS2_WYLACZONY,
		SS2_BRAK,
		SS2_WRZUCONO_KAMIEN,
		SS2_WYGENEROWANO,
		SS2_ZAMKNIETO,
		SS2_WYGENEROWANO2,
		SS2_POGADANO,
		SS2_WALKA,
		SS2_PRZEGRANO,
		SS2_WYGRANO,
		SS2_NAGRODA
	} sekret_stan;
	bool CheckMoonStone(GroundItem* item, Unit* unit);
	inline Item* GetSecretNote()
	{
		return (Item*)FindItem("sekret_kartka");
	}
	int sekret_gdzie, sekret_gdzie2;

	//
	vector<Unit*> warp_to_inn;

	// newsy w grze
	vector<News*> news;
	void AddNews(cstring text);

	bool show_mp_panel;
	int draw_flags;
	bool in_tutorial;
	int tut_state;
	vector<TutorialText> ttexts;
	VEC3 tut_manekin;
	Object* tut_tarcza, *tut_tarcza2;
	void UpdateTutorial();
	void TutEvent(int id);
	void EndOfTutorial(int);

	struct TutChestHandler : public ChestEventHandler
	{
		void HandleChestEvent(ChestEventHandler::Event event)
		{
			Game::Get().TutEvent(0);
		}
		int GetChestEventHandlerQuestRefid()
		{
			// w tutorialu nie mo�na zapisywa�
			return -1;
		}
	} tut_chest_handler;
	struct TutChestHandler2 : public ChestEventHandler
	{
		void HandleChestEvent(ChestEventHandler::Event event)
		{
			Game::Get().TutEvent(1);
		}
		int GetChestEventHandlerQuestRefid()
		{
			// w tutorialu nie mo�na zapisywa�
			return -1;
		}
	} tut_chest_handler2;
	struct TutUnitHandler : public UnitEventHandler
	{
		void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit)
		{
			Game::Get().TutEvent(2);
		}
		int GetUnitEventHandlerQuestRefid()
		{
			// w tutorialu nie mo�na zapisywa�
			return -1;
		}
	} tut_unit_handler;

	// muzyka
	MUSIC_TYPE music_type;
	SOUND last_music;
	vector<Music*> tracks;
	int track_id;
	void SetMusic();
	void SetMusic(MUSIC_TYPE type);
	void SetupTracks();
	void UpdateMusic();

	// u�ywane przy wczytywaniu gry
	vector<AIController*> ai_bow_targets, ai_cast_targets;
	vector<Location*> load_location_quest;
	vector<Unit*> load_unit_handler;
	vector<Chest*> load_chest_handler;
	vector<Unit**> load_unit_refid;

	bool hardcore_mode, hardcore_option;
	string hardcore_savename;
	bool clearup_shutdown;
	const Item* crazy_give_item; // dawany przedmiot, nie trzeba zapisywa�
	int total_kills;
	vector<TimedUnit> timed_units;
	float grayout;
	bool cl_postfx;

	void AddTimedUnit(Unit* unit, int location, int days);
	void RemoveTimedUnit(Unit* unit);
	inline bool WantAttackTeam(Unit& u)
	{
		if(IsLocal())
			return u.attack_team;
		else
			return IS_SET(u.ai_mode, 0x08);
	}
	void RemoveUnitFromLocation(Unit* unit, int location);

	// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
	// mo�e zwr�ci� questowy przedmiot je�li b�dzie wystarczaj�co tani, lub unikat!
	const Item* GetRandomItem(int max_value);

	// klawisze
	void InitGameKeys();
	void ResetGameKeys();
	void SaveGameKeys();
	void LoadGameKeys();
	inline bool KeyAllowed(byte k)
	{
		return IS_SET(allow_input, KeyAllowState(k));
	}
	inline byte KeyDoReturn(GAME_KEYS gk, KeyF f)
	{
		GameKey& k = GKey[gk];
		if(k[0])
		{
			if(KeyAllowed(k[0]) && (Key.*f)(k[0]))
				return k[0];
		}
		if(k[1])
		{
			if(KeyAllowed(k[1]) && (Key.*f)(k[1]))
				return k[1];
		}
		return VK_NONE;
	}
	inline byte KeyDoReturn(GAME_KEYS gk, KeyFC f)
	{
		return KeyDoReturn(gk, (KeyF)f);
	}
	inline bool KeyDo(GAME_KEYS gk, KeyF f)
	{
		return KeyDoReturn(gk, f) != VK_NONE;
	}
	inline bool KeyDo(GAME_KEYS gk, KeyFC f)
	{
		return KeyDo(gk, (KeyF)f);
	}
	inline bool KeyDownAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::Down);
	}
	inline bool KeyPressedReleaseAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedRelease);
	}
	inline bool KeyDownUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::DownUp);
	}
	inline bool KeyDownUp(GAME_KEYS gk)
	{
		GameKey& k = GKey[gk];
		if(k[0])
		{
			if(Key.DownUp(k[0]))
				return true;
		}
		if(k[1])
		{
			if(Key.DownUp(k[1]))
				return true;
		}
		return false;
	}
	inline bool KeyPressedUpAllowed(GAME_KEYS gk)
	{
		return KeyDo(gk, &KeyStates::PressedUp);
	}
	// Zwraca czy dany klawisze jest wyci�ni�ty, je�li nie jest to dozwolone to traktuje jak wyci�ni�ty
	inline bool KeyUpAllowed(byte key)
	{
		if(KeyAllowed(key))
			return Key.Up(key);
		else
			return true;
	}
	// przedmioty w czasie grabienia itp s� tu przechowywane indeksy
	// ujemne warto�ci odnosz� si� do slot�w (SLOT_WEAPON = -SLOT_WEAPON-1), pozytywne do zwyk�ych przedmiot�w
	vector<int> tmp_inventory[2];
	int tmp_inventory_shift[2];

	void BuildTmpInventory(int index);

	void BreakAction(Unit& u, bool fall=false);
	void BreakAction2(Unit& u, bool fall=false);
	void CreateTerrain();
	void Draw();
	void ExitToMenu();
	void DoExitToMenu();
	void GenerateImage(Item* item);
	Unit* GetFollowTarget();
	void SetupCamera(float dt);
	void SetupShaders();
	void TakeScreenshot(bool text=false, bool no_gui=false);
	void UpdateGame(float dt);
	void UpdatePlayer(LevelContext& ctx, float dt);
	void PlayerCheckObjectDistance(Unit& u, const VEC3& pos, void* ptr, float& best_dist, BeforePlayer type);

	int CheckMove(VEC3& pos, const VEC3& dir, float radius, Unit* me, bool* is_small=NULL);
	int CheckMovePhase(VEC3& pos, const VEC3& dir, float radius, Unit* me, bool* is_small=NULL);

	struct IgnoreObjects
	{
		// NULL lub tablica jednostek zako�czona NULL
		const Unit** ignored_units;
		// NULL lub tablica obiekt�w [u�ywalnych lub nie] zako�czona NULL
		const void** ignored_objects;
		// czy ignorowa� bloki
		bool ignore_blocks;
		// czy ignorowa� obiekty
		bool ignore_objects;
	};
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const VEC3& pos, float radius, const IgnoreObjects* ignore=NULL);
	void GatherCollisionObjects(LevelContext& ctx, vector<CollisionObject>& objects, const BOX2D& box, const IgnoreObjects* ignore=NULL);
	bool Collide(const vector<CollisionObject>& objects, const VEC3& pos, float radius);
	bool Collide(const vector<CollisionObject>& objects, const BOX2D& box, float margin=0.f);
	bool Collide(const vector<CollisionObject>& objects, const BOX2D& box, float margin, float rot);
	void ParseCommand(const string& str, PrintMsgFunc print_func=NULL, PARSE_SOURCE ps=PS_UNKNOWN);
	void AddCommands();
	void AddConsoleMsg(cstring msg);
	void UpdateAi(float dt);
	void StartDialog(DialogContext& ctx, Unit* talker, DialogEntry* dialog=NULL);
	void StartDialog2(PlayerController* player, Unit* talker, DialogEntry* dialog=NULL);
	void EndDialog(DialogContext& ctx);
	void UpdateGameDialog(DialogContext& ctx, float dt);
	void GenerateStockItems();
	void GenerateMerchantItems(vector<ItemSlot>& items, int price_limit);
	void ApplyToTexturePack(TexturePack& tp, cstring diffuse, cstring normal, cstring specular);
	void SetDungeonParamsAndTextures(BaseLocation& base);
	void MoveUnit(Unit& unit, bool warped=false);
	bool RayToMesh(const VEC3& ray_pos, const VEC3& ray_dir, const VEC3& obj_pos, float obj_rot, VertexData* vd, float& dist);
	bool CollideWithStairs(const CollisionObject& co, const VEC3& pos, float radius) const;
	bool CollideWithStairsRect(const CollisionObject& co, const BOX2D& box) const;
	void ValidateGameData(bool popup);
	void TestGameData(bool major);
	void TestItemScript(const int* script, string& errors, uint& count);
	void TestUnitSpells(const SpellList& spells, string& errors, uint& count);
	Unit* CreateUnit(UnitData& base, int level=-1, Human* human_data=NULL, bool create_physics=true, Unit* test_unit=NULL);
	void ParseItemScript(Unit& unit, const int* script);
	bool IsEnemy(Unit& u1, Unit& u2, bool ignore_dont_attack=false);
	bool IsFriend(Unit& u1, Unit& u2);
	bool CanSee(Unit& unit, Unit& unit2);
	// nie dzia�a dla budynk�w bo nie uwzgl�dnia obiekt�w
	bool CanSee(const VEC3& v1, const VEC3& v2);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, VEC3& hitpoint);
	bool CheckForHit(LevelContext& ctx, Unit& unit, Unit*& hitted, Animesh::Point& hitbox, Animesh::Point* bone, VEC3& hitpoint);
	void UpdateParticles(LevelContext& ctx, float dt);
	// wykonuje atak postaci
	enum ATTACK_RESULT
	{
		ATTACK_NOT_HIT, // nie trafiono nic
		ATTACK_BLOCKED, // cios zablokowano
		ATTACK_PARRIED, // atak sparowany
		ATTACK_HIT, // trafiono
		ATTACK_CLEAN_HIT // czysty cios
	};
	ATTACK_RESULT DoAttack(LevelContext& ctx, Unit& unit);
#define DMG_NO_BLOOD (1<<0)
#define DMG_MAGICAL (1<<1)
	void GiveDmg(LevelContext& ctx, Unit* giver, float dmg, Unit& taker, const VEC3* hitpoint=NULL, int dmg_flags=0, bool trained_armor=false);
	void UpdateUnits(LevelContext& ctx, float dt);
	void UpdateUnitInventory(Unit& unit);
	bool FindPath(LevelContext& ctx, const INT2& start_tile, const INT2& target_tile, vector<INT2>& path, bool can_open_doors=true, bool wedrowanie=false, vector<INT2>* blocked=NULL);
	INT2 RandomNearTile(const INT2& tile);
	bool CanLoadGame() const;
	bool CanSaveGame() const;
	bool IsAnyoneAlive() const;
	int FindLocalPath(LevelContext& ctx, vector<INT2>& path, const INT2& my_tile, const INT2& target_tile, const Unit* me, const Unit* other, const void* useable=NULL, bool is_end_point=false);
	bool DoShieldSmash(LevelContext& ctx, Unit& attacker);
	VEC4 GetFogColor();
	VEC4 GetFogParams();
	VEC4 GetAmbientColor();
	VEC4 GetLightColor();
	VEC4 GetLightDir();
	void UpdateBullets(LevelContext& ctx, float dt);
	void SpawnDungeonColliders();
	void RemoveColliders();
	void CreateCollisionShapes();
	inline bool AllowKeyboard() const { return IS_SET(allow_input, ALLOW_KEYBOARD); }
	inline bool AllowMouse() const { return IS_SET(allow_input, ALLOW_MOUSE); }
	VEC3 PredictTargetPos(const Unit& me, const Unit& target, float bullet_speed) const;
	inline bool CanShootAtLocation(const Unit& me, const Unit& target, const VEC3& pos) const
	{
		return CanShootAtLocation2(me, &target, pos);
	}
	bool CanShootAtLocation(const VEC3& from, const VEC3& to) const;
	bool CanShootAtLocation2(const Unit& me, const void* ptr, const VEC3& to) const;
	void LoadItems(Item* items, uint count, uint stride);
	void SpawnTerrainCollider();
	void GenerateDungeonObjects();
	void GenerateDungeonUnits();
	void SetUnitPointers();
	Unit* SpawnUnitInsideRoom(Pokoj& p, UnitData& unit, int level=-1, const INT2& pt=INT2(-1000,-1000), const INT2& pt2=INT2(-1000,-1000));
	Unit* SpawnUnitInsideRoomOrNear(InsideLocationLevel& lvl, Pokoj& p, UnitData& unit, int level=-1, const INT2& pt=INT2(-1000,-1000), const INT2& pt2=INT2(-1000,-1000));
	Unit* SpawnUnitNearLocation(LevelContext& ctx, const VEC3& pos, UnitData& unit, const VEC3* look_at=NULL, int level=-1, float extra_radius=2.f);
	Unit* SpawnUnitInsideArea(LevelContext& ctx, const BOX2D& area, UnitData& unit, int level=-1);
	Unit* SpawnUnitInsideInn(UnitData& unit, int level=-1, InsideBuilding* inn=NULL);
	Unit* CreateUnitWithAI(LevelContext& ctx, UnitData& unit, int level=-1, Human* human_data=NULL, const VEC3* pos=NULL, const float* rot=NULL, AIController** ai=NULL);
	void ChangeLevel(int gdzie);
	void AddPlayerTeam(const VEC3& pos, float rot, bool reenter, bool hide_weapon);
	void OpenDoorsByTeam(const INT2& pt);
	void ExitToMap();
	void SetExitWorldDir();
	void RespawnObjectColliders(bool spawn_pes=true);
	void RespawnObjectColliders(LevelContext& ctx, bool spawn_pes=true);
	void SetRoomPointers();
	SOUND GetMaterialSound(MATERIAL_TYPE m1, MATERIAL_TYPE m2);
	void PlayAttachedSound(Unit& unit, SOUND sound, float smin, float smax);
	float GetLevelDiff(Unit& player, Unit& enemy);
	ATTACK_RESULT DoGenericAttack(LevelContext& ctx, Unit& attacker, Unit& hitted, const VEC3& hitpoint, float dmg, int dmg_type, Skill weapon_skill);
	void GenerateLabirynthUnits();
	int GetDungeonLevel();
	int GetDungeonLevelChest();
	void GenerateCave(Location& l);
	void GenerateCaveObjects();
	void GenerateCaveUnits();
	void SaveGame(HANDLE file);
	void LoadGame(HANDLE file);
	void RemoveUnusedAiAndCheck();
	void CheckUnitsAi(LevelContext& ctx, int& err_count);
	void CastSpell(LevelContext& ctx, Unit& unit);
	void SpellHitEffect(LevelContext& ctx, Bullet& bullet, const VEC3& pos, Unit* hitted);
	void UpdateExplosions(LevelContext& ctx, float dt);
	void UpdateTraps(LevelContext& ctx, float dt);
	// zwraca tymczasowy wska�nik na stworzon� pu�apk� lub NULL (mo�e si� nie uda� tylko dla ARROW i POISON)
	Trap* CreateTrap(INT2 pt, TRAP_TYPE type, bool timed=false);
	bool RayTest(const VEC3& from, const VEC3& to, Unit* ignore, VEC3& hitpoint, Unit*& hitted);
	void UpdateElectros(LevelContext& ctx, float dt);
	void UpdateDrains(LevelContext& ctx, float dt);
	void AI_Shout(LevelContext& ctx, AIController& ai);
	void AI_DoAttack(AIController& ai, Unit* target, bool w_biegu=false);
	void AI_HitReaction(Unit& unit, const VEC3& pos);
	void UpdateAttachedSounds(float dt);
	void BuildRefidTables();
	bool SaveGameSlot(int slot, cstring text);
	bool LoadGameSlot(int slot);
	void LoadSaveSlots();
	void Quicksave(bool from_console);
	void Quickload(bool from_console);
	void ClearGameVarsOnNewGameOrLoad();
	void ClearGameVarsOnNewGame();
	void ClearGameVarsOnLoad();
	Quest* FindQuest(int location, int source);
	Quest* FindQuest(int refid, bool active=true);
	Quest* FindUnacceptedQuest(int location, int source);
	Quest* FindUnacceptedQuest(int refid);
	// zwraca losowe miasto lub wiosk� kt�ra nie jest this_city
	int GetRandomCityLocation(int this_city=-1);
	// zwraca losowe miasto lub wiosk� kt�ra nie jest this_city i nie ma aktywnego questa
	int GetFreeRandomCityLocation(int this_city=-1);
	// zwraca losowe miasto kt�re nie jest this_city
	int GetRandomCity(int this_city=-1);
	void LoadQuests(vector<Quest*>& v_quests, HANDLE file);
	void ClearGame();
	cstring FormatString(DialogContext& ctx, const string& str_part);
	int GetNearestLocation(const VEC2& pos, bool not_quest, bool not_city);
	int GetNearestLocation2(const VEC2& pos, int flags, bool not_quest, int flagi_cel=-1);
	inline int GetNearestCity(const VEC2& pos)
	{
		return GetNearestLocation2(pos, (1<<L_CITY)|(1<<L_VILLAGE), false);
	}
	void AddGameMsg(cstring msg, float time);
	void AddGameMsg2(cstring msg, float time, int id);
	void AddGameMsg3(GMS id);
	int CalculateQuestReward(int gold);
	void AddReward(int gold, int exp=0)
	{
		AddGold(CalculateQuestReward(gold), NULL, true, txQuestCompletedGold, 4.f, false);
	}
	INT2 CalcRect(FONT font, cstring text, int flags);
	void DoLoading();
	void CreateCityMinimap();
	void CreateDungeonMinimap();
	void RebuildMinimap();
	void UpdateDungeonMinimap(bool send);
	void DungeonReveal(const INT2& tile);
	const Item* FindQuestItem(cstring name, int refid);
	void SaveStock(HANDLE file, vector<ItemSlot>& cnt);
	void LoadStock(HANDLE file, vector<ItemSlot>& cnt);
	Door* FindDoor(LevelContext& ctx, const INT2& pt);
	void AddGroundItem(LevelContext& ctx, GroundItem* item);
	void GenerateDungeonObjects2();
	SOUND GetItemSound(const Item* item);
	cstring GetCurrentLocationText();
	void Unit_StopUsingUseable(LevelContext& ctx, Unit& unit, bool send=true);
	void OnReenterLevel(LevelContext& ctx);
	void EnterLevel(bool first, bool reenter, bool from_lower, int from_portal, bool from_outside);
	void LeaveLevel(bool clear=false);
	void LeaveLevel(LevelContext& ctx, bool clear);
	void CreateBlood(LevelContext& ctx, const Unit& unit, bool fully_created=false);
	void WarpUnit(Unit& unit, const VEC3& pos);
	void ProcessUnitWarps();
	void ProcessRemoveUnits();
	void ApplyContext(ILevel* level, LevelContext& ctx);
	void UpdateContext(LevelContext& ctx, float dt);
	inline LevelContext& GetContext(Unit& unit)
	{
		if(unit.in_building == -1)
			return local_ctx;
		else
		{
			assert(city_ctx);
			return city_ctx->inside_buildings[unit.in_building]->ctx;
		}
	}
	inline LevelContext& GetContext(const VEC3& pos)
	{
		if(!city_ctx)
			return local_ctx;
		else
		{
			INT2 offset(int((pos.x-256.f)/256.f), int((pos.z-256.f)/256.f));
			if(offset.x%2 == 1)
				++offset.x;
			if(offset.y%2 == 1)
				++offset.y;
			offset /= 2;
			for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
			{
				if((*it)->level_shift == offset)
					return (*it)->ctx;
			}
			return local_ctx;
		}
	}
	// dru�yna
	inline int GetTeamSize() // zwraca liczb� os�b w dru�ynie
	{
		return team.size();
	}
	inline int GetActiveTeamSize() // liczba os�b w dru�ynie kt�re nie s� questowe
	{
		return active_team.size();
	}
	bool HaveTeamMember();
	bool HaveTeamMemberNPC();
	bool HaveTeamMemberPC();
	bool IsTeamMember(Unit& unit);
	inline Unit* GetLeader() { return leader; }
	int GetPCShare();
	int GetPCShare(int pc, int npc);
	inline bool IsLeader(const Unit& unit)
	{
		return &unit == GetLeader();
	}
	inline bool IsLeader(const Unit* unit)
	{
		assert(unit);
		return unit == GetLeader();
	}
	inline bool IsLeader()
	{
		if(!IsOnline())
			return true;
		else
			return leader_id == my_id;
	}
	void AddGold(int ile, vector<Unit*>* to=NULL, bool show=false, cstring msg=txGoldPlus, float time=3.f, bool defmsg=true);
	void AddGoldArena(int ile);
	void CheckTeamItems();
	void ValidateTeamItems();
	void BuyTeamItems();
	bool IsTeamNotBusy();
	bool IsAnyoneTalking() const;
	// szuka przedmiotu w dru�ynie
	bool FindItemInTeam(const Item* item, int refid, Unit** unit, int* i_index, bool check_npc=true);
	bool FindQuestItem2(Unit* unit, cstring id, Quest** quest, int* i_index);
	inline bool HaveQuestItem(const Item* item, int refid=-1)
	{
		return FindItemInTeam(item, refid, NULL, NULL, true);
	}
	bool RemoveQuestItem(const Item* item, int refid=-1);
	bool RemoveItemFromWorld(const Item* item);
	bool IsBetterItem(Unit& unit, const Item* item, int* value=NULL);
	SPAWN_GROUP RandomSpawnGroup(const BaseLocation& base);
	// to by mog�o by� globalna funkcj�
	void GenerateTreasure(int level, int count, vector<ItemSlot>& items, int& gold);
	void SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count);
	void PlayHitSound(MATERIAL_TYPE mat_bron, MATERIAL_TYPE mat_cialo, const VEC3& hitpoint, float range, bool dmg);
	// wczytywanie
	void LoadingStart(int steps);
	void LoadingStep(cstring text=NULL);
	//
	void StartArenaCombat(int level);
	InsideBuilding* GetArena();
	bool WarpToArea(LevelContext& ctx, const BOX2D& area, float radius, VEC3& pos, int tries=10);
	void DeleteUnit(Unit* unit);
	void DialogTalk(DialogContext& ctx, cstring msg);
	void GenerateHeroName(HeroData& hero);
	void GenerateHeroName(Class klasa, bool szalony, string& name);
	inline bool WantExitLevel()
	{
		return !KeyDownAllowed(GK_WALK);
	}
	VEC2 GetMapPosition(Unit& unit);
	void EventTakeItem(int id);
	const Item* GetBetterItem(const Item* item);
	void CheckIfLocationCleared();
	void SpawnArenaViewers(int count);
	void RemoveArenaViewers();
	inline bool CanWander(Unit& u)
	{
		if(city_ctx && u.ai->loc_timer <= 0.f && !dont_wander && IS_SET(u.data->flagi, F_AI_CHODZI))
		{
			if(u.busy != Unit::Busy_No)
				return false;
			if(u.IsHero())
			{
				if(u.hero->team_member && u.hero->mode != HeroData::Wander)
					return false;
				else if(zawody_wygenerowano)
					return false;
				else
					return true;
			}
			else if(u.in_building == -1)
				return true;
			else
				return false;
		}
		else
			return false;
	}
	float PlayerAngleY();
	VEC3 GetExitPos(Unit& u);
	void AttackReaction(Unit& attacked, Unit& attacker);
	// czy mo�na opu�ci� lokacj� (0-tak, 1-dru�yna za daleko, 2-wrogowie w pobli�u)
	int CanLeaveLocation(Unit& unit);
	void GenerateTraps();
	void RegenerateTraps();
	void SpawnHeroesInsideDungeon();
	GroundItem* SpawnGroundItemInsideAnyRoom(InsideLocationLevel& lvl, const Item* item);
	GroundItem* SpawnGroundItemInsideRoom(Pokoj& pokoj, const Item* item);
	GroundItem* SpawnGroundItemInsideRadius(const Item* item, const VEC2& pos, float radius, bool try_exact=false);
	void InitQuests();
	void GenerateQuestUnits();
	void GenerateQuestUnits2(bool on_enter);
	void UpdateQuests(int days);
	void SaveQuestsData(HANDLE file);
	void LoadQuestsData(HANDLE file);
	void RemoveQuestUnit(UnitData* ud, bool on_leave);
	void RemoveQuestUnits(bool on_leave);
	void GenerujTartak(bool w_budowie);
	// zwraca losowe miasto/wiosk� pomijaj�c te ju� u�yte, 0-wioska/miasto, 1-miasto, 2-wioska
	int GetRandomCityLocation(const vector<int>& used, int type=0) const;
	bool GenerujKopalnie();
	void HandleUnitEvent(UnitEventHandler::TYPE event, Unit* unit);
	int GetUnitEventHandlerQuestRefid();
	void EndUniqueQuest();
	Pokoj& GetRoom(InsideLocationLevel& lvl, int cel, bool schody_dol);
	void UpdateGame2(float dt);
	inline bool IsUnitDontAttack(Unit& u)
	{
		if(IsLocal())
			return u.dont_attack;
		else
			return IS_SET(u.ai_mode, 0x01);
	}
	inline bool IsUnitAssist(Unit& u)
	{
		if(IsLocal())
			return u.assist;
		else
			return IS_SET(u.ai_mode, 0x02);
	}
	inline bool IsUnitIdle(Unit& u)
	{
		if(IsLocal())
			return u.ai->state == AIController::Idle;
		else
			return !IS_SET(u.ai_mode, 0x04);
	}
	void SetUnitWeaponState(Unit& unit, bool wyjmuje, BRON co);
	void UpdatePlayerView();
	void OnCloseInventory();
	void CloseInventory(bool do_close=true);
	void CloseAllPanels();
	bool CanShowEndScreen();
	void UpdateGameDialogClient();
	LevelContext& GetContextFromInBuilding(int in_building);
	bool Cheat_KillAll(int typ, Unit& unit, Unit* ignore);
	void Event_Pvp(int id);
	void Cheat_Reveal();
	void Cheat_ShowMinimap();
	void StartPvp(PlayerController* player, Unit* unit);
	void UpdateGameNet(float dt);
	void CheckCredit(bool require_update=false, bool ignore=false);
	void UpdateUnitPhysics(Unit* unit, const VEC3& pos);
	Unit* FindTeamMember(const string& name);
	Unit* FindTeamMember(int netid);
	void WarpNearLocation(LevelContext& ctx, Unit& uint, const VEC3& pos, float extra_radius, bool allow_exact, int tries=20);
	void Train(Unit& unit, bool is_skill, int co, bool add_one=false);
	void ShowStatGain(int co, int ile);
	void BreakPlayerAction(PlayerController* player);
	void ActivateChangeLeaderButton(bool activate);
	void RespawnTraps();
	void WarpToInn(Unit& unit);
	void PayCredit(PlayerController* player, int ile);
	void CreateSaveImage(cstring filename);
	void PlayerUseUseable(Useable* u, bool after_action);
	SOUND GetTalkSound(Unit& u);
	void UnitTalk(Unit& u, cstring text);
	void OnEnterLocation();
	void OnEnterLevel();
	void OnEnterLevelOrLocation();
	Unit* FindTeamMemberById(cstring id);
	inline Unit* FindUnitByIdLocal(UnitData* ud)
	{
		return local_ctx.FindUnitById(ud);
	}
	inline Unit* FindUnitByIdLocal(cstring id)
	{
		return FindUnitByIdLocal(FindUnitData(id));
	}
	inline Object* FindObjectByIdLocal(Obj* obj)
	{
		return local_ctx.FindObjectById(obj);
	}
	inline Object* FindObjectByIdLocal(cstring id)
	{
		return FindObjectByIdLocal(FindObject(id));
	}
	inline Useable* FindUseableByIdLocal(int type)
	{
		return local_ctx.FindUseableById(type);
	}
	Unit* GetRandomArenaHero();
	cstring GetRandomIdleText(Unit& u);
	struct TeamInfo
	{
		int players;
		int npcs;
		int heroes;
		int sane_heroes;
		int insane_heroes;
		int free_members;
	};
	void GetTeamInfo(TeamInfo& info);
	Unit* GetRandomSaneHero();
	UnitData* GetRandomHeroData();
	UnitData* GetUnitDataFromClass(Class clas, bool crazy);

	enum BLOCK_RESULT
	{
		BLOCK_PERFECT,
		BLOCK_GOOD,
		BLOCK_MEDIUM,
		BLOCK_POOR,
		BLOCK_BREAK
	};
	BLOCK_RESULT CheckBlock(Unit& hitted, float angle_dif, float attack_power, float skill, float str);

	void DropGold(int count);

	// dodaje przedmiot do ekwipunku postaci (obs�uguje z�oto, otwarty ekwipunek i multiplayer)
	void AddItem(Unit& unit, const Item* item, uint count, uint team_count, bool send_msg=true);
	inline void AddItem(Unit& unit, const Item* item, uint count=1, bool is_team=true, bool send_msg=true)
	{
		AddItem(unit, item, count, is_team ? count : 0, send_msg);
	}
	inline void AddItem(Unit& unit, const GroundItem& item, bool send_msg=true)
	{
		AddItem(unit, item.item, item.count, item.team_count, send_msg);
	}
	// dodaje przedmiot do skrzyni (obs�uguje otwarty ekwipunek i multiplayer)
	void AddItem(Chest& chest, const Item* item, uint count, uint team_count, bool send_msg=true);
	inline void AddItem(Chest& chest, const Item* item, uint count=1, bool is_team=true, bool send_msg=true)
	{
		AddItem(chest, item, count, is_team ? count : 0, send_msg);
	}
	// dodaje przedmiot do skrzyni, nie sortuje
	void AddItemBare(Chest& chest, const Item* item, uint count, uint team_count);
	inline void AddItemBare(Chest& chest, const Item* item, uint count=1, bool is_team=true)
	{
		AddItemBare(chest, item, count, is_team ? count : 0);
	}
	// usuwa przedmiot z ekwipunku (obs�uguje otwarty ekwipunek, lock i multiplayer), dla 0 usuwa wszystko
	void RemoveItem(Unit& unit, int i_index, uint count);
	bool RemoveItem(Unit& unit, const Item* item, uint count);

	int GetQuestIndex(Quest* quest);
	// szuka gracza kt�ry u�ywa skrzyni, je�li u�ywa nie-gracz to zwraca NULL (aktualnie tylko gracz mo�e ale w przysz�o�ci nie)
	Unit* FindChestUserIfPlayer(Chest* chest);

	Unit* FindPlayerTradingWithUnit(Unit& u);
	bool CheckTeamShareItem(TeamShareItem& tsi);
	INT2 GetSpawnPoint();
	InsideLocationLevel* TryGetLevelData();
	bool ValidateTarget(Unit& u, Unit* target);

	void UpdateLights(vector<Light>& lights);

	bool IsDrunkman(Unit& u);
	void PlayUnitSound(Unit& u, SOUND snd, float range=1.f);
	void UnitFall(Unit& u);
	void UnitDie(Unit& u, LevelContext* ctx, Unit* killer);
	void UnitTryStandup(Unit& u, float dt);
	void UnitStandup(Unit& u);

	void UpdatePostEffects(float dt);
	
	void SpawnDrunkmans();
	void PlayerYell(Unit& u);
	bool CanBuySell(const Item* item);
	void ResetCollisionPointers();
	void SetOutsideParams();

	//-----------------------------------------------------------------
	// GUI
	// panele
	Container* game_gui_container; // kontener na wszystkie elementy gui
	MainMenu* main_menu;
	WorldMapGui* world_map;
	// elementy gui
	GamePanelContainer* gp_trade; // kontener na ekwipunek gracza i handlarza
	Inventory* inventory, *inv_trade_mine, *inv_trade_other;
	StatsPanel* stats;
	TeamPanel* team_panel;
	Journal* journal;
	Minimap* minimap;
	GameGui* game_gui;
	MpBox* mp_box;
	LoadScreen* load_screen;
	GameMessagesContainer* game_messages;
	// dialogi
	Console* console;
	GameMenu* game_menu;
	Options* options;
	SaveLoad* saveload;
	CreateCharacterPanel* create_character;
	MultiplayerPanel* multiplayer_panel;
	CreateServerPanel* create_server_panel;
	PickServerPanel* pick_server_panel;
	ServerPanel* server_panel;
	InfoBox* info_box;
	Controls* controls;
	// inne
	Dialog* dialog_enc;
	Dialog* dialog_pvp;
	bool cursor_allow_move;

	void UpdateGui(float dt);
	bool IsGamePanelOpen();
	void CloseGamePanels();
	void CreateGamePanels();
	void SetGamePanels();
	void NullGui2();
	void InitGui2();
	void RemoveGui2();
	void LoadGui(File& f);
	void GetGamePanels(vector<GamePanel*>& panels);
	void ClearGui();
	void ShowPanel(OpenPanel p, OpenPanel open = OpenPanel::Unknown);
	OpenPanel GetOpenPanel();
	void PositionGui();

	//-----------------------------------------------------------------
	// MENU / MAIN MENU / OPTIONS
	Class quickstart_class, autopick_class; // mo�na po��czy�
	string quickstart_name;
	bool check_updates, skip_tutorial;
	uint skip_version;
	string save_input_text;

	bool CanShowMenu();
	void MainMenuEvent(int index);
	void MenuEvent(int id);
	void OptionsEvent(int id);
	void SaveLoadEvent(int id);
	void SaveEvent(int id);
	void SaveOptions();
	void ShowOptions();
	void ShowMenu();
	void ShowSavePanel();
	void ShowLoadPanel();
	void StartNewGame();
	void StartTutorial();
	void NewGameCommon(Class clas, cstring name, int beard, int mustache, int hair, float height, const VEC4& hair_color);
	void ShowCreateCharacterPanel(bool enter_name, bool redo=false);
	void StartQuickGame();
	void DialogNewVersion(int);
	void MultiplayerPanelEvent(int id);
	void CreateServerEvent(int id);
	void RandomCharacter(Class clas=Class::RANDOM);
	void OnEnterIp(int id);
	void GenericInfoBoxUpdate(float dt);
	void QuickJoinIp();
	void AddMultiMsg(cstring msg);
	void Quit();
	bool ValidateNick(cstring nick);
	void UpdateLobbyNet(float dt);
	void OnCreateCharacter(int id);
	void OnPlayTutorial(int id);
	void OnQuit(int);
	void OnExit(int);
	void ShowQuitDialog();
	void OnPickServer(int id);
	void EndConnecting(cstring msg, bool wait=false);
	void CloseConnection(VoidF f);
	void DoQuit();
	void RestartGame();

	//-----------------------------------------------------------------
	// MULTIPLAYER
	RakNet::RakPeerInterface* peer;
	string server_name, player_name, server_pswd, server_ip, enter_pswd, server_name2;
	int autostart_count;//, kick_timer;
	int players; // aktualna liczba graczy w grze
	int max_players, max_players2; // maksymalna liczba graczy w grze
	int my_id; // moje unikalne id
	int last_id;
	int last_startup_id;
	bool sv_server, sv_online, sv_startup, was_client;
	BitStream server_info;
	vector<byte> packet_data;
	vector<PlayerInfo> game_players, old_players;
	vector<int> players_left;
	SystemAddress server;
	int leader_id, kick_id;
	float startup_timer;
	enum NET_MODE
	{
		NM_CONNECT_IP, // ��czenie serwera z klientem (0 - pingowanie, 1 - podawanie has�a, 2 - ��czenie)
		NM_QUITTING,
		NM_QUITTING_SERVER,
		NM_TRANSFER,
		NM_TRANSFER_SERVER,
		NM_SERVER_SEND
	} net_mode;
	int net_state, net_tries;
	VoidF net_callback;
	string net_adr;
	float net_timer, update_timer, mp_timeout;
	vector<INT2> lobby_updates;
	inline void AddLobbyUpdate(const INT2& u)
	{
		for(vector<INT2>::iterator it = lobby_updates.begin(), end = lobby_updates.end(); it != end; ++it)
		{
			if(*it == u)
				break;
		}
		lobby_updates.push_back(u);
	}
	BitStream net_stream, net_stream2;
	bool change_title_a;
	bool level_generated;
	int netid_counter, item_netid_counter, chest_netid_counter, useable_netid_counter, skip_id_counter, trap_netid_counter, door_netid_counter, electro_netid_counter;
	vector<NetChange> net_changes;
	vector<NetChangePlayer> net_changes_player;
	vector<string*> net_talk;
	union
	{
		//Unit* loot_unit;
		//Chest* loot_chest;
		struct
		{
			GroundItem* picking_item;
			int picking_item_state;
		};
	};
	struct WarpData
	{
		Unit* u;
		int where; // -1-z budynku, >=0-do budynku
		float timer;
	};
	vector<WarpData> mp_warps;
	vector<QuestItemClient*> quest_items;
	float train_move; // u�ywane przez klienta do trenowania przez chodzenie
	bool anyone_talking;
	// u�ywane u klienta kt�ry nie zapami�tuje zmiennej 'pc'
	bool godmode, noclip, invisible;
	vector<INT2> minimap_reveal_mp;
	bool boss_level_mp; // u�ywane u klienta zamiast boss_levels
	bool mp_load;
	float mp_interp;
	bool mp_use_interp;
	ObjectPool<EntityInterpolator> interpolators;
	float interpolate_timer;
	int mp_port;
	bool paused, pick_autojoin;

	void WriteToInterpolator(EntityInterpolator* e, const VEC3& pos, float rot);
	// zwraca czy pozycja si� zmieni�a
	void UpdateInterpolator(EntityInterpolator* e, float dt, VEC3& pos, float& rot);
	void InterpolateUnits(float dt);
	void InterpolatePlayers(float dt);

	// sprawdza czy aktualna gra jest online
	inline bool IsOnline() const { return sv_online; }
	// sprawdza czy ja jestem serwerem
	inline bool IsServer() const { return sv_server; }
	// sprawdza czy ja jestem klientem
	inline bool IsClient() const { return !sv_server; }
	inline bool IsClient2() const { return sv_online && !sv_server; }
	// czy jest serwerem lub pojedy�czy gracz
	inline bool IsLocal() const { return !IsOnline() || IsServer(); }

	void InitServer();
	void InitClient();
	void UpdateServerInfo();
	int FindPlayerIndex(cstring nick, bool not_left=false);
	int FindPlayerIndex(const SystemAddress& adr);
	int GetPlayerIndex(int id);
	void AddServerMsg(cstring msg);
	void KickPlayer(int index);
	void ChangeReady();
	void CheckReady();
	void AddMsg(cstring msg);
	void OnEnterPassword(int id);
	void ForceRedraw();
	void PrepareLevelData(BitStream& stream);
	void WriteUnit(BitStream& stream, Unit& unit);
	void WriteDoor(BitStream& stream, Door& door);
	void WriteObject(BitStream& stream, Object& obj);
	void WriteItem(BitStream& stream, GroundItem& item);
	void WriteUseable(BitStream& stream, Useable& use);
	void WriteBlood(BitStream& stream, Blood& blood);
	void WriteLight(BitStream& stream, Light& light);
	void WriteChest(BitStream& stream, Chest& chest);
	void WriteRoom(BitStream& stream, Pokoj& room);
	void WriteTrap(BitStream& stream, Trap& trap);
	cstring ReadLevelData(BitStream& stream);
	bool ReadUnit(BitStream& stream, Unit& unit);
	bool ReadDoor(BitStream& stream, Door& door);
	bool ReadObject(BitStream& stream, Object& obj);
	bool ReadItem(BitStream& stream, GroundItem& item);
	bool ReadUseable(BitStream& stream, Useable& use);
	bool ReadBlood(BitStream& stream, Blood& blood);
	bool ReadLight(BitStream& stream, Light& light);
	bool ReadChest(BitStream& stream, Chest& chest);
	bool ReadRoom(BitStream& stream, Pokoj& room);
	bool ReadTrap(BitStream& stream, Trap& trap);
	void SendPlayerData(int index);
	cstring ReadPlayerData(BitStream& stream);
	Unit* FindUnit(int netid);
	void UpdateServer(float dt);
	void UpdateClient(float dt);
	void Client_Say(Packet* packet);
	void Client_Whisper(Packet* packet);
	void Client_ServerSay(Packet* packet);
	void Server_Say(Packet* packet, PlayerInfo& info);
	void Server_Whisper(Packet* packet, PlayerInfo& info);
	void ServerProcessUnits(vector<Unit*>& units);
	GroundItem* FindItemNetid(int netid, LevelContext** ctx=NULL);
	inline PlayerInfo& GetPlayerInfo(int id)
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->id == id)
				return *it;
		}
		assert(0);
		return game_players[0];
	}
	inline PlayerInfo* GetPlayerInfoTry(int id)
	{
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
		{
			if(it->id == id)
				return &*it;
		}
		return NULL;
	}
	inline PlayerInfo& GetPlayerInfo(PlayerController* player)
	{
		return GetPlayerInfo(player->id);
	}
	inline PlayerInfo* GetPlayerInfoTry(PlayerController* player)
	{
		return GetPlayerInfoTry(player->id);
	}
	inline void PushNetChange(NetChange::TYPE type)
	{
		NetChange& c = Add1(net_changes);
		c.type = type;
	}
	void UpdateWarpData(float dt);
	inline void Net_AddQuest(int refid)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::ADD_QUEST;
		c.id = refid;
	}
	inline void Net_RegisterItem(const Item* item)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REGISTER_ITEM;
		c.base_item = item;
	}
	inline void Net_AddItem(PlayerController* player, const Item* item, bool is_team)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.pc = player;
		c.item = item;
		c.id = (is_team ? 1 : 0);
		c.ile = 1;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_AddedItemMsg(PlayerController* player)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.pc = player;
		c.type = NetChangePlayer::ADDED_ITEM_MSG;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_AddItems(PlayerController* player, const Item* item, int ile, bool is_team)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::ADD_ITEMS;
		c.pc = player;
		c.item = item;
		c.id = (is_team ? ile : 0);
		c.ile = ile;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_UpdateQuest(int refid)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::UPDATE_QUEST;
		c.id = refid;
	}
	inline void Net_UpdateQuestMulti(int refid, int ile)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::UPDATE_QUEST_MULTI;
		c.id = refid;
		c.ile = ile;
	}
	inline void Net_RenameItem(const Item* item)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::RENAME_ITEM;
		c.base_item = item;
	}
	inline void Net_RemoveQuestItem(PlayerController* player, int refid)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::REMOVE_QUEST_ITEM;
		c.pc = player;
		c.id = refid;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_ChangeLocationState(int id, bool visited)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::CHANGE_LOCATION_STATE;
		c.id = id;
		c.ile = (visited ? 1 : 0);
	}
	inline void Net_RecruitNpc(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::RECRUIT_NPC;
		c.unit = unit;
	}
	inline void Net_RemoveUnit(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::REMOVE_UNIT;
		c.id = unit->netid;
	}
	inline void Net_KickNpc(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::KICK_NPC;
		c.id = unit->netid;
	}
	inline void Net_SpawnUnit(Unit* unit)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::SPAWN_UNIT;
		c.unit = unit;
	}
	inline void Net_PrepareWarp(PlayerController* player)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::PREPARE_WARP;
		c.pc = player;
		GetPlayerInfo(player).NeedUpdate();
	}
	inline void Net_StartDialog(PlayerController* player, Unit* talker)
	{
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = NetChangePlayer::START_DIALOG;
		c.pc = player;
		c.id = talker->netid;
		GetPlayerInfo(player).NeedUpdate();
	}
#define WHERE_LEVEL_UP -1
#define WHERE_LEVEL_DOWN -2
#define WHERE_OUTSIDE -3
#define WHERE_PORTAL 0
	inline void Net_LeaveLocation(int where)
	{
		NetChange& c = Add1(net_changes);
		c.type = NetChange::LEAVE_LOCATION;
		c.id = where;
	}
	void Net_OnNewGameServer();
	void Net_OnNewGameClient();
	// szuka questowych przedmiot�w u klienta
	const Item* FindQuestItemClient(cstring id, int refid) const;
	//void ConvertPlayerToAI(PlayerInfo& info);
	Useable* FindUseable(int netid);
	// odczytuje id przedmiotu (dodatkowo quest_refid je�li jest questowy), szuka go i zwraca wska�nik, obs�uguje "gold", -1-b��d odczytu, 0-pusty przedmiot, 1-ok, 2-brak przedmiotu
	int ReadItemAndFind(BitStream& stream, const Item*& item) const;
	bool ReadItemList(BitStream& s, vector<ItemSlot>& items);
	bool ReadItemListTeam(BitStream& s, vector<ItemSlot>& items);
	Door* FindDoor(int netid);
	Trap* FindTrap(int netid);
	bool RemoveTrap(int netid);
	Chest* FindChest(int netid);
	inline bool SkipBitstream(BitStream& s, uint ile)
	{
		if(s.GetNumberOfUnreadBits()/8 >= ile)
		{
			s.SetReadOffset(s.GetReadOffset()+ile*8);
			return true;
		}
		else
			return false;
	}
	void ReequipItemsMP(Unit& unit); // zak�ada przedmioty kt�re ma w ekipunku, dostaje bro� je�li nie ma, podnosi z�oto
	Electro* FindElectro(int netid);
	void UseDays(PlayerController* player, int ile);
	PlayerInfo* FindOldPlayer(cstring nick);
	void PrepareWorldData(BitStream& s);
	bool ReadWorldData(BitStream& s);
	void WriteNetVars(BitStream& s);
	bool ReadNetVars(BitStream& s);
	void WritePlayerStartData(BitStream& s, PlayerInfo& info);
	bool ReadPlayerStartData(BitStream& s);
	bool CheckMoveNet(Unit* u, const VEC3& pos);
	void Net_PreSave();
	bool FilterOut(NetChange& c);
	bool FilterOut(NetChangePlayer& c);
	void Net_FilterServerChanges();
	void Net_FilterClientChanges();
	void ProcessLeftPlayers();
	void ClosePeer(bool wait=false);
	void DeleteOldPlayers();
	inline NetChangePlayer& AddChange(NetChangePlayer::TYPE type, PlayerController* pc)
	{
		assert(pc);
		NetChangePlayer& c = Add1(net_changes_player);
		c.type = type;
		c.pc = pc;
		pc->player_info->NeedUpdate();
		return c;
	}
	void WriteCharacterData(BitStream& s, Class c, const HumanData& hd, const CreatedCharacter& cc);
	// 0 - ok, 1 - read error, 2 - value error, 3 - validation error
	int ReadCharacterData(BitStream& s, Class& c, HumanData& hd, CreatedCharacter& cc);
	void OnPickCharacter(Class old_class, bool ready);

	//-----------------------------------------------------------------
	// WORLD MAP
	void AddLocations(uint count, LOCATION type, float dist, bool unique_name);
	void AddLocations(uint count, const LOCATION* types, uint type_count, float dist, bool unique_name);
	void AddLocations(const LOCATION* types, uint count, float dist, bool unique_name);
	void EnterLocation(int level=0, int from_portal=-1, bool close_portal=false);
	void GenerateWorld();
	void ApplyTiles(float* h, TerrainTile* tiles);
	void SpawnBuildings(vector<CityBuilding>& buildings);
	void SpawnUnits(City* city);
	void RespawnUnits();
	void RespawnUnits(LevelContext& ctx);
	void LeaveLocation(bool clear=false);
	void GenerateDungeon(Location& loc);
	void SpawnCityPhysics();
	// zwraca Object lub Useable lub Chest!!!, w przypadku budynku rot musi by� r�wne 0, PI/2, PI, 3*2/PI (w przeciwnym wypadku b�dzie 0)
	Object* SpawnObject(LevelContext& ctx, Obj* obj, const VEC3& pos, float rot, float scale=1.f, VEC3* out_point=NULL, int variant=-1);
	void RespawnBuildingPhysics();
	void SpawnCityObjects();
	// roti jest u�ywane tylko do ustalenia czy k�t jest zerowy czy nie, mo�na przerobi� t� funkcj� �eby tego nie u�ywa�a wog�le
	void ProcessBuildingObjects(LevelContext& ctx, City* city, InsideBuilding* inside, Animesh* mesh, Animesh* inside_mesh, float rot, int roti, const VEC3& shift, BUILDING type,
		CityBuilding* building, bool recreate=false, VEC3* out_point=NULL);
	void GenerateForest(Location& loc);
	void SpawnForestObjects(int road_dir=-1); //-1 brak, 0 -, 1 |
	void CreateForestMinimap();
	void SpawnOutsideBariers();
	void GetOutsideSpawnPoint(VEC3& pos, float& dir);
	void SpawnForestUnits(const VEC3& team_pos);
	void RepositionCityUnits();
	void Event_RandomEncounter(int id);
	void GenerateEncounterMap(Location& loc);
	void SpawnEncounterUnits();
	void SpawnEncounterObjects();
	void SpawnEncounterTeam();
	Encounter* AddEncounter(int& id);
	void RemoveEncounter(int id);
	Encounter* GetEncounter(int id);
	Encounter* RecreateEncounter(int id);
	int GetRandomSpawnLocation(const VEC2& pos, SPAWN_GROUP group, float range=160.f);
	void DoWorldProgress(int days);
	Location* CreateLocation(LOCATION type, int levels=-1);
	void UpdateLocation(LevelContext& ctx, int days, int open_chance, bool reset);
	void UpdateLocation(int days, int open_chance, bool reset);
	void GenerateCamp(Location& loc);
	void SpawnCampObjects();
	void SpawnCampUnits();
	Object* SpawnObjectNearLocation(LevelContext& ctx, Obj* obj, const VEC2& pos, float rot, float range=2.f, float margin=0.3f, float scale=1.f);
	Object* SpawnObjectNearLocation(LevelContext& ctx, Obj* obj, const VEC2& pos, const VEC2& rot_target, float range=2.f, float margin=0.3f, float scale=1.f);
	int GetClosestLocation(LOCATION type, const VEC2& pos, int cel=-1);
	int GetClosestLocationNotTarget(LOCATION type, const VEC2& pos, int nie_cel);
	int CreateCamp(const VEC2& pos, SPAWN_GROUP group, float range=64.f, bool allow_exact=true);
	void SpawnTmpUnits(City* city);
	void RemoveTmpUnits(City* city);
	void RemoveTmpUnits(LevelContext& ctx);
	int AddLocation(Location* loc);
	// tworzy lokacj� (je�li range<0 to pozycja jest dowolna a range=-range, level=-1 - losowy poziom, =0 - minimalny, =9 maksymalny, =liczba - okre�lony)
	int CreateLocation(LOCATION type, const VEC2& pos, float range=64.f, int target=-1, SPAWN_GROUP spawn=SG_LOSOWO, bool allow_exact=true, int levels=-1);
	bool FindPlaceForLocation(VEC2& pos, float range=64.f, bool allow_exact=true);
	int FindLocationId(Location* loc);
	void Event_StartEncounter(int id);
	void GenerateMoonwell(Location& loc);
	void SpawnMoonwellObjects();
	void SpawnMoonwellUnits(const VEC3& team_pos);
#define SOE_DONT_SPAWN_PARTICLES (1<<0)
#define SOE_MAGIC_LIGHT (1<<1)
#define SOE_DONT_CREATE_LIGHT (1<<2)
	void SpawnObjectExtras(LevelContext& ctx, Obj* obj, const VEC3& pos, float rot, void* user_ptr, btCollisionObject** phy_result, float scale=1.f, int flags=0);
	void GenerateSecretLocation(Location& loc);
	void SpawnSecretLocationObjects();
	void SpawnSecretLocationUnits();
	void SpawnTeamSecretLocation();
	void GenerateMushrooms(int days_since=10);
	void GenerateCityPickableItems();
	void PickableItemBegin(LevelContext& ctx, Object& o);
	void PickableItemAdd(const Item* item);
	void GenerateDungeonFood();
	void GenerateCityMap(Location& loc);
	void GenerateVillageMap(Location& loc);
	void GetCityEntry(VEC3& pos, float& rot);
	
	vector<Location*> locations; // lokacje w grze, mo�e by� nullptr
	Location* location; // wska�nik na aktualn� lokacj� [odtwarzany]
	int current_location; // aktualna lokacja lub -1
	int picked_location; // zaznaczona lokacja na mapie �wiata, ta do kt�rej si� w�druje lub -1 [tylko je�li world_state==WS_TRAVEL]
	int open_location; // aktualnie otwarta lokacja (w sensie wczytanych zasob�w, utworzonych jednostek itp) lub -1 [odtwarzany]
	int travel_day; // liczba dni w podr�y [tylko je�li world_state==WS_TRAVEL]
	int enc_kierunek; // kierunek z kt�rej strony nadesz�a dru�yna w czasie spotkania [tymczasowe]
	int spotkanie; // rodzaj losowego spotkania [tymczasowe]
	int enc_tryb; // 0 - losowa walka, 1 - specjalne spotkanie, 2 - questowe spotkanie [tymczasowe]
	int empty_locations; // liczba pustych lokacji
	int create_camp; // licznik do stworzenia nowego obozu
	WORLDMAP_STATE world_state; // stan na mapie �wiata (stoi, podr�uje)
	VEC2 world_pos; // pozycja na mapie �wiata
	VEC2 travel_start; // punkt startu podr�y na mapie �wiata [tylko je�li world_state==WS_TRAVEL]
	float travel_time; // czas podr�y na mapie [tylko je�li world_state==WS_TRAVEL]
	float travel_time2; // licznik aktualizacji szansy na spotkanie
	float world_dir; // kierunek podr�y/wej�cia na map�, to jest nowy k�t (0 w prawo), wskazuje od �rodka do kraw�dzi mapy
	float szansa_na_spotkanie; // szansa na spotkanie na mapie �wiata
	bool far_encounter; // czy dru�yna gracza jest daleko w czasie spotkania [tymczasowe]
	bool guards_enc_reward; // czy odebrano nagrod� za uratowanie stra�nik�w w czasie spotkania
	uint cities; // liczba miast i wiosek
	uint encounter_loc; // id lokacji spotkania losowego
	SPAWN_GROUP losowi_wrogowie; // wrogowie w czasie spotkania [tymczasowe]
	vector<Encounter*> encs; // specjalne spotkania na mapie �wiata [odtwarzane przy wczytywaniu quest�w]
	Encounter* game_enc; // spotkanie w czasie podr�y [tymczasowe]
	LocationEventHandler* location_event_handler; // obs�uga wydarze� lokacji
	bool first_city;
	vector<INT2> boss_levels; // dla oznaczenia gdzie gra� muzyk� (x-lokacja, y-poziom)
#define ENTER_FROM_PORTAL 0
#define ENTER_FROM_OUTSIDE -1
#define ENTER_FROM_UP_LEVEL -2
#define ENTER_FROM_DOWN_LEVEL -3
#define ENTER_FROM_UNKNOWN -4
	int enter_from; // sk�d si� przysz�o (u�ywane przy wczytywanie w MP gdy do��cza nowa posta�)
	bool g_have_well;
	INT2 g_well_pt;

	//-----------------------------------------------------------------
	// WORLD STATE
	enum WorldProgressMode
	{
		WPM_NORMAL,
		WPM_TRAVEL,
		WPM_SKIP
	};
	void WorldProgress(int days, WorldProgressMode mode);

	int year; // rok w grze [zaczyna si� od 100, ustawiane w NewGame]
	int month; // miesi�c w grze [od 0 do 11, ustawiane w NewGame]
	int day; // dzie� w grze [od 0 do 29, ustawiane w NewGame]
	int worldtime; // licznik dni [od 0, ustawiane w NewGame]
	int gt_hour; // ilo�� godzin jakie gracz gra
	int gt_minute; // ilo�� minut jakie gracz gra [0-59]
	int gt_second; // ilo�� sekund jakie gracz gra [0-59]
	float gt_tick; // licznik czasu do sekundy grania


	Config cfg;
	void SaveCfg();
};
