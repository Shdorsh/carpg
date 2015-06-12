#pragma once

//-----------------------------------------------------------------------------
enum GameMessages : byte
{
	/* komunikat powitalny do serwera, size:7-21 
	byte - id
	int - wersja gry
	string1 - nick (16)
	*/
	ID_HELLO = ID_USER_PACKET_ENUM+1, 

	/* aktualizacja w lobby, size:4+
	byte - id
	byte - liczba zmian
	[tablica]
	{
		byte - rodzaj (0-aktualizuj gracza,1-dodaj gracza,2-usu� gracza,3-liczba graczy,4-przyw�dca)
		byte - id gracza/liczba graczy
		[je�li 0]
		{
			byte - czy gotowy
			byte - klasa
		}
		[je�li 1]
		{
			string1 - nick
		}
	}
	*/
	ID_LOBBY_UPDATE,

	/* gracz nie mo�e do��czy� do serwera, size:2-6
	byte - id
	byte - pow�d (0-brak miejsca,1-z�a wersja,2-zaj�ty nick,3-z�y nick przy wczytywaniu,4-z�e ID_HELLO,5-nieznany b��d)
	[je�li pow�d=1]
	int - wersja
	*/
	ID_CANT_JOIN,

	/* do��cza gracza do lobby, size:4+
	byte - id
	byte - id gracza
	byte - liczba graczy
	byte - liczba graczy w lobby
	byte - id przyw�dcy
	[dla ka�dego gracza]
	{
		byte - id
		byte - gotowy
		byte - klasa
		string1 - nick
	}
	*/
	ID_JOIN,

	/* client changed character
	byte - id
	[Client]
	CharacterData
	bool - ready
	[Server]
	bool - response if ok
	*/
	ID_PICK_CHARACTER,

	/* client wants info about character / server sends it
	byte - id
	[for client]
	byte - player_id
	[for server]
	bool - (true if ok, false if invalid)
	CharacterData if ok
	*/
	ID_CHARACTER_INFO,
	
	/* player ready changed
	byte - id
	byte - gotowy
	*/
	ID_LOBBY_CHANGE,

	/* wiadomo�� do wszystkich gracz, size:4+
	byte - id
	byte - id gracza
	string1 - tekst
	*/
	ID_SAY,

	/* prywatna wiadomo��, size:4+
	byte - id
	byte - id gracza
	string1-tekst
	*/
	ID_WHISPER,

	/* wiadomo�� od serwera do wszystkich, size:3+
	byte - id
	string1 - tekst
	*/
	ID_SERVER_SAY,

	/* gracz opuszcza serwer, size:1
	byte - id
	*/
	ID_LEAVE,

	/* zamykanie serwera, size:2
	byte - id
	byte - pow�d
	*/
	ID_SERVER_CLOSE,

	/* trwa uruchamianie serwera, size:2
	byte - id
	byte - pozosta�y czas
	*/
	ID_STARTUP,

	/* przerwano odliczanie startu, size:1 */
	ID_END_STARTUP,

	/* info about current server action when loading, size:2
	byte - id
	byte - state (0-generating world, 1-sending world, 2-waiting for players)
	*/
	ID_STATE,

	/* dane �wiata gry
	byte - id
	byte - liczba lokacji
	{
		byte - typ
		[je�li insidelocation]
		byte - poziomy
		[je�li miasto/wioska]
		{
			byte - citzens
			word - citzens_world
		}
		[je�li wioska]
		{
			byte - budynek1
			byte - budynek2
		}
		byte - stan
		VEC2 - pozycja
		string1 - nazwa
	}
	byte - startowa lokacja
	*/
	ID_WORLD_DATA,

	ID_READY,

	/* trwa zmiana poziomu na inny, klient odsy�a ID_
	byte - id
	[SERWER]
	{
		byte - id lokacji
		byte - poziom podziemi
	}
	[KLIENT]
	byte - czy 1 wizyta [0-nie, 1-tak]
	*/
	ID_CHANGE_LEVEL,

	/*
	byte - id
	*/
	ID_LEVEL_DATA,

	ID_PLAYER_DATA2,
	ID_START,
	ID_CONTROL,
	ID_CHANGES,
	ID_PLAYER_UPDATE,

	// dane gracza wysy�ane na pocz�tku gry [WritePlayerStartData/ReadPlayerStartData]
	ID_PLAYER_START_DATA,

	// trwa zapisywanie gry
	//ID_SAVING,

	// przejd� do gry na mapie �wiata
	ID_START_AT_WORLDMAP
};

//-----------------------------------------------------------------------------
struct NetChange
{
	enum TYPE
	{
		UNIT_POS,
		CHANGE_EQUIPMENT, // zmiana ekwipunku jednostki SERWER[int(netid)-jednostka, byte(id)-item slot, Item-przedmiot(automatycznie ustawiany)] / KLIENT[int(id)-i_index, dla slot�w zdejmuje]
		TAKE_WEAPON, //id 0-wyjmuj,1-chowaj
		ATTACK, // atak postaci SERWER[int(netid), byte(id)-co/flagi, float(f[1])-szybko��] / KLIENT[byte(id)-co/flagi, float(f[1])-szybko��, float(f[2])-yspeed dla �uku]
		CHANGE_FLAGS, //zmiana flag gry (0x01-bandyta,0x02-atak szale�c�w)
		FALL, // jednostka upada na ziemie
		DIE, // jednostka umiera
		SPAWN_BLOOD, // tworzy krew (id-type,pos)
		UPDATE_HP, // aktualizacja hp jednostki [int(Unit::netid), float(Unit::hp), float(Unit::hpmax)]
		HURT_SOUND, // okrzyk po trafieniu
		DROP_ITEM, // wyrzucanie przedmiotu SERWER[int(Unit::netid)-jednostka dla kt�rej odtworzy� animacje] / KLIENT[int(id)-i_index przedmiotu, int(ile)]
		SPAWN_ITEM,
		PICKUP_ITEM, // podnoszenie przedmiotu przez posta� SERWER[int(Unit::netid), byte(ile)-0 na dole 1 u g�ry] / KLIENT[int(id)-item netid]
		REMOVE_ITEM, // usuwa przedmiot z ziemi [int(netid)]
		CONSUME_ITEM, // posta� konsumuje przedmiot SERWER[int(Unit::netid)-posta�, string1((const Item*)id)-u�ywany przedmiot, byte(ile)-force] / KLIENT[int(id)-index przedmiotu]
		USE_POTION2,
		HIT_SOUND, // VEC3 pos, id-mat1, ile-mat2
		STUNNED,
		SHOT_ARROW, // strza� z �uku [int(Unit::netid), VEC3(pos), float(f[0],rotY), float(f[1],speedY), float(f[2],rotX)
		LOOT_UNIT, // gracz chce ograbia� posta� [int(Unit::netid)]
		GET_ITEM, // zabiera sprzedmiot z kontenera lub kupuje [int(id)-i_index przedmiotu, int(ile)-ile przedmiot�w]
		GET_ALL_ITEMS, // gracz podni�s� wszystkie przedmioty []
		PUT_ITEM, // chowanie do zw�ok/skrzyni, sprzedawanie, dawanie sojusznikowi (I_SHARE), sprzedawanie sojusznikowi (I_GIVE) [int(id)-i_index przedmiotu, int(ile)-ile przedmiot�w]
		STOP_TRADE,
		UPDATE_CREDIT,
		TAKE_ITEM_CREDIT,
		IDLE,
		HELLO,
		ALL_QUESTS_COMPLETED, // wszystkie unikalne questy uko�czone
		TALK,
		LOOT_CHEST, // gracz chce ograbia� skrzyni� [int(Chest::netid)]
		CHEST_OPEN,
		CHEST_CLOSE,
		CHOICE,
		SKIP_DIALOG,
		CHANGE_LOCATION_STATE,
		ADD_RUMOR,
		TELL_NAME,
		HAIR_COLOR,
		ENTER_BUILDING,
		EXIT_BUILDING,
		WARP,
		ADD_NOTE, // informacja o dodaniu notatki przez gracza, wysy�a ostatni� dodan� notatk� [String1]
		REGISTER_ITEM,
		ADD_QUEST,
		UPDATE_QUEST,
		RENAME_ITEM,
		UPDATE_QUEST_MULTI,
		REMOVE_PLAYER,
		CHANGE_LEADER,
		RANDOM_NUMBER,
		CHEAT_WARP_TO_BUILDING,
		CHEAT_SKIP_DAYS,
		CHEAT_KILL_ALL,
		CHEAT_NOCLIP,
		CHEAT_GODMODE,
		CHEAT_INVISIBLE,
		CHEAT_SCARE,
		CHEAT_SUICIDE,
		CHEAT_HEAL_UNIT,
		CHEAT_KILL,
		CHEAT_HEAL,
		CHEAT_SPAWN_UNIT,
		CHEAT_ADD_ITEM,
		CHEAT_ADD_GOLD,
		CHEAT_ADD_GOLD_TEAM,
		CHEAT_MOD_STAT,
		CHEAT_REVEAL,
		CHEAT_GOTO_MAP,
		USE_USEABLE,
		STAND_UP,
		GAME_OVER, // koniec gry - przegrana
		RECRUIT_NPC,
		KICK_NPC,
		REMOVE_UNIT,
		SPAWN_UNIT,
		IS_BETTER_ITEM, // klient sprawdza czy przedmiot jest lepszy dla npca [int(id)-i_index]
		PVP,
		CHEAT_CITZEN,
		CHEAT_SHOW_MINIMAP,
		CHANGE_ARENA_STATE,
		ARENA_SOUND,
		SHOUT,
		LEAVE_LOCATION,
		EXIT_TO_MAP,
		ENTER_LOCATION,
		TRAVEL,
		WORLD_TIME,
		USE_DOOR,
		CREATE_EXPLOSION, // tworzy eksplozj� [byte(id)-type, VEC3(pos)]
		REMOVE_TRAP,
		TRIGGER_TRAP,
		TRAIN_MOVE,
		EVIL_SOUND,
		ENCOUNTER,
		CLOSE_ENCOUNTER,
		CLOSE_PORTAL,
		CLEAN_ALTAR,
		ADD_LOCATION,
		REMOVE_CAMP,
		CHANGE_AI_MODE,
		CHANGE_UNIT_BASE, // zmienia bazowy typ postaci [int(Unit::netid), string1(Unit::data->id)]
		CHEAT_CHANGE_LEVEL, // gracz chce zmieni� poziom [byte(id) - 0 w g�re, 1 w d�]
		CHEAT_WARP_TO_STAIRS, // gracz chce przenie�� si� na schody [byte(id) - 0 w g�r�, 1 w d�]
		CAST_SPELL, // jednostka rzuca czar [int(Unit::netid)]
		CREATE_SPELL_BALL, // tworzy efekt czaru - pocisk [int(Unit::netid), VEC3(pos), float(f[0],rotY), float(f[1],speedY), int(i,Spell::id)
		SPELL_SOUND, // d�wi�k rzucania czaru [byte(id,Spell::id), VEC3(pos)]
		CREATE_DRAIN, // efekt wyssania krwi [int(Unit::netid-do kogo idzie krew)]
		CREATE_ELECTRO, // efekt czaru piorun [int(e_id,netid), VEC3(pos), VEC3(f,pos2)]
		UPDATE_ELECTRO, // dodaje kolejny kawa�ek elektro [int(e_id,netid), VEC3(pos)]
		ELECTRO_HIT, // efek trafienia przez elektro [VEC3(pos)]
		RAISE_EFFECT, // efekt o�ywiania [VEC3(pos)]
		REVEAL_MINIMAP, // odkrywa minimap� [word-ile {byte,byte-pozycja}]
		CHEAT_NOAI, // zmiana zmiennej noai [byte(id)]
		END_OF_GAME, // czas gry min��, pora przej�� na emerytur�
		REST,
		TRAIN,
		UPDATE_FREE_DAYS,
		CHANGE_MP_VARS,
		GAME_SAVED, // gra zosta�a zapisana, wy�wietl komunikat [-]
		PAY_CREDIT, // gracz sp�aca kredyt [int(id-ile)]
		GIVE_GOLD, // gracz daje z�oto innej postaci [int(id)-netid, int(ile)-ile]
		DROP_GOLD, // gracz upuszcza z�oto na ziemi� [int(id)-ile]
		HERO_LEAVE,
		PAUSED, // gra zatrzymana/wznowiona [byte(id)-1 zatrzymana/0 wznowiona]
		HEAL_EFFECT,
		SECRET_TEXT, // aktualizuje tekst listu [string1-text()]
		PUT_GOLD, // wk�adanie z�ota do kontenera [int(ile)]
		UPDATE_MAP_POS, // aktualizacja pozycji na mapie [VEC2-pos]
		CHEAT_TRAVEL, // przenosi na mapie �wiata [int(id)-id lokacji]
		CHEAT_HURT, // zadaje postaci 100 obra�e� [int(Unit::netid)]
		CHEAT_BREAK_ACTION, // przerywa akcj� postaci [int(Unit::netid)]
		CHEAT_FALL, // posta� upada na ziemi� na kilka sekund [int(Unit::netid)]
		REMOVE_USED_ITEM, // usuwa u�ywany przedmiot z r�ki [int(Unit::netid)]
		GAME_STATS, // statystki wy�wiatlane na koniec gry [int-kills]
		USEABLE_SOUND, // odtwarza d�wi�k u�ywania obiektu [int(Unit::netid)]
		YELL, // okrzyk gracza �eby ai si� odsun�� []
	} type;
	union
	{
		Unit* unit;
		GroundItem* item;
		const Item* base_item;
		UnitData* base_unit;
		int e_id;
	};
	union
	{
		struct
		{
			int id, ile;
			union
			{
				int i;
				string* str;
			};
		};
		float f[3];
	};
	VEC3 pos;
};

//-----------------------------------------------------------------------------
struct NetChangePlayer
{
	enum TYPE
	{
		PICKUP, // odpowied� serwera na podnoszenie przedmiotu z ziemi [int(id)-ile, int(ile)-team count]
		LOOT, // zezwolenie na pl�drowanie [byte(id)-wynik {0-anuluj, 1-ok}, je�li ok to: uint-ile przedmiot�w, (Item,uint-count,uint-team_count)]
		GOLD_MSG,
		START_DIALOG,
		END_DIALOG,
		SHOW_CHOICES,
		START_TRADE, // rozpoczyna handel [int(id)-netid handlarza/uint-ile przedmiot�w, {Item-przedmiot,uint-ile}[ile]]
		START_SHARE, // rozpoczyna wymian� przedmiot�w [-/int-waga, int-max waga, int-z�oto, byte-atrbuty i skille, uint-ile przedmiot�w, {Item-przedmiot, uint-ile, uint-ile dru�ynowo}]
		START_GIVE, // rozpoczyna dawanie przedmiot�w [*** jak wy�ej ***]
		SET_FROZEN,
		REMOVE_QUEST_ITEM,
		CHEATS,
		USE_USEABLE,
		IS_BETTER_ITEM, // odpowied� serwera na IS_BETTER_ITEM (byte(id)-1 lepszy, 0 gorszy)
		PVP,
		ADD_ITEMS, // dodaje kilka przedmiot�w [int(id)-ile dru�ynowych, int(ile)-ile, Item-przedmiot]
		ADD_ITEMS_TRADER, // dodano przedmioty npc kt�ry handluje z graczem [int(id)-netid postaci, int(ile)-ile, int(a)-ile dru�ynowo, Item-przedmiot]
		ADD_ITEMS_CHEST, // dodano przedmiot do otwartej skrzyni [int(id)-netid skrzyni, int(ile)-ile, int(a)-ile dru�ynowo, Item-przedmiot]
		REMOVE_ITEMS, // usuwa kilka przedmiot�w [int(id)-index, int(ile)-ile]
		REMOVE_ITEMS_TRADER, // usuwa kilka przedmiot�w npc kt�ry handluje z graczem [int(id)-netid postaci, int(ile)-ile, int(a)-index]
		PREPARE_WARP,
		ENTER_ARENA,
		START_ARENA_COMBAT,
		EXIT_ARENA,
		NO_PVP,
		CANT_LEAVE_LOCATION,
		LOOK_AT,
		END_FALLBACK,
		REST, // odpowied� serwera na odpoczynek w karczmie [int(id)- wi�ksze od zera to dni odpoczynku, mniejsze od zera - brak�o z�ota]
		TRAIN,
		UNSTUCK,
		GOLD_RECEIVED, // otrzymano z�oto od innego gracza [int(id)-id gracza, int(ile)-ile z�ota]
		GAIN_STAT, // gracz zdoby� atrybuty/skille [byte(id)-co; byte(ile)]
		BREAK_ACTION, // wywo�uje BreakPlayerAction []
		UPDATE_TRADER_GOLD, // zmiana liczby z�ota u handlarza [int(id)-netid handlarza, int(ile)-ile z�ota]
		UPDATE_TRADER_INVENTORY, // zmiana w ekwipunku handlarza, aktualnie wywo�ywane gdy damy herosowi przedmiot [int(Unit::netid)-netid postaci, ItemList]
		PLAYER_STATS, // zmiana statystyk gracza [int(id)-co, warto�ci]
		ADDED_ITEM_MSG, // wiadomo�� o otrzymanym przedmiocie []
		ADDED_ITEMS_MSG, // wiadomo�� o otrzymaniu kilku przedmiot�w [byte(ile)-ile]
		STAT_CHANGED, // stat changed [byte(id)-ChangedStatType, byte(a)-stat id, int(ile)-value]
	} type;
	PlayerController* pc;
	int id, ile;
	union
	{
		int a;
		Unit* unit;
	};
	const Item* item;
	VEC3 pos;
};

//-----------------------------------------------------------------------------
enum AttackId
{
	AID_Attack,
	AID_PowerAttack,
	AID_Shoot,
	AID_StartShoot,
	AID_Block,
	AID_Bash,
	AID_RunningAttack,
	AID_StopBlock
};

//-----------------------------------------------------------------------------
enum class ChangedStatType
{
	ATTRIBUTE,
	SKILL,
	BASE_ATTRIBUTE,
	BASE_SKILL
};
