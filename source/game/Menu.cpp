#include "Pch.h"
#include "Base.h"
#include "Game.h"
#include "Language.h"
#include "Terrain.h"
#include "Wersja.h"

extern cstring g_ctime;
extern const uint g_build;

// STA�E
const float T_WAIT_FOR_HELLO = 5.f;
const float T_TRY_CONNECT = 5.f;
const float T_LOBBY_UPDATE = 0.2f;
const int T_SHUTDOWN = 1000;
const float T_WAIT_FOR_DATA = 5.f;

//=================================================================================================
// Czy mo�na otworzy� menu?
//=================================================================================================
bool Game::CanShowMenu()
{
	return !GUI.HaveDialog() && !IsGamePanelOpen() && game_state != GS_MAIN_MENU && death_screen != 3 && !koniec_gry && !dialog_context.dialog_mode;
}

//=================================================================================================
void Game::ShowMenu()
{
	GUI.ShowDialog(game_menu);
}

//=================================================================================================
// Obs�uga akcji w g��wnym menu
//=================================================================================================
void Game::MainMenuEvent(int id)
{
	switch(id)
	{
	case MainMenu::IdNewGame:
		sv_online = false;
		sv_server = false;
		ShowCreateCharacterPanel(true);
		break;
	case MainMenu::IdLoadGame:
		sv_online = false;
		sv_server = false;
		ShowLoadPanel();
		break;
	case MainMenu::IdMultiplayer:
		multiplayer_panel->Show();
		break;
	case MainMenu::IdOptions:
		ShowOptions();
		break;
	case MainMenu::IdInfo:
		GUI.SimpleDialog(Format(main_menu->txInfoText, VERSION_STR_FULL, g_build, g_ctime), NULL);
		break;
	case MainMenu::IdWebsite:
		ShellExecute(NULL, "open", main_menu->txUrl, NULL, NULL, SW_SHOWNORMAL);
		break;
	case MainMenu::IdQuit:
		Quit();
		break;
	}
}

//=================================================================================================
// Obs�uga akcji w menu
//=================================================================================================
void Game::MenuEvent(int index)
{
	switch(index)
	{
	case GameMenu::IdReturnToGame: // wr��
		GUI.CloseDialog(game_menu);
		break;
	case GameMenu::IdSaveGame: // zapisz
		ShowSavePanel();
		break;
	case GameMenu::IdLoadGame: // wczytaj
		ShowLoadPanel();
		break;
	case GameMenu::IdOptions: // opcje
		ShowOptions();
		break;
	case GameMenu::IdExit: // wr�� do menu
		{
			DialogInfo info;
			info.event = fastdelegate::FastDelegate1<int>(this, &Game::OnExit);
			info.name = "exit_to_menu";
			info.parent = NULL;
			info.pause = true;
			info.text = game_menu->txExitToMenuDialog;
			info.order = ORDER_TOP;
			info.type = DIALOG_YESNO;

			GUI.ShowDialog(info);
		}
		break;
	case GameMenu::IdQuit: // wyjd�
		ShowQuitDialog();
		break;
	}
}

void Game::OptionsEvent(int index)
{
	if(index == Options::IdOk)
	{
		GUI.CloseDialog(options);
		SaveOptions();
		return;
	}

	switch(index)
	{
	case Options::IdFullscreen:
		ChangeMode(options->check[0].checked);
		break;
	case Options::IdChangeRes:
		break;
	case Options::IdSoundVolume:
		sound_volume = options->sound_volume;
		group_default->setVolume(float(sound_volume)/100);
		break;
	case Options::IdMusicVolume:
		music_volume = options->music_volume;
		group_music->setVolume(float(music_volume)/100);
		break;
	case Options::IdMouseSensitivity:
		mouse_sensitivity = options->mouse_sensitivity;
		break;
	case Options::IdGrassRange:
		grass_range = (float)options->grass_range;
		break;
	case Options::IdControls:
		GUI.ShowDialog(controls);
		break;
	case Options::IdGlow:
		cl_glow = options->check[1].checked;
		break;
	case Options::IdNormal:
		cl_normalmap = options->check[2].checked;
		break;
	case Options::IdSpecular:
		cl_specularmap = options->check[3].checked;
		break;
	}
}

void Game::SaveLoadEvent(int id)
{
	if(id == SaveLoad::IdCancel)
	{
		mp_load = false;
		GUI.CloseDialog(saveload);
	}
	else
	{
		// zapisz/wczytaj
		if(saveload->save_mode)
		{
			// zapisywanie
			SaveSlot& slot = saveload->slots[saveload->choice];
			if(saveload->choice == MAX_SAVE_SLOTS-1)
			{
				// szybki zapis
				GUI.CloseDialog(saveload);
				SaveGameSlot(saveload->choice+1, saveload->txQuickSave);
			}
			else
			{
				// podaj tytu� zapisu
				cstring names[] = {NULL, saveload->txSave};
				if(slot.valid)
					save_input_text = slot.text;
				else if(hardcore_mode)
					save_input_text = hardcore_savename;
				else
					save_input_text.clear();
				GetTextDialogParams params(saveload->txSaveName, save_input_text);
				params.custom_names = names;
				params.event = fastdelegate::FastDelegate1<int>(this, &Game::SaveEvent);
				params.parent = saveload;
				GetTextDialog::Show(params);
			}
		}
		else
		{
			// wczytywanie
			try
			{
				GUI.CloseDialog(saveload);
				if(game_menu->visible)
					GUI.CloseDialog(game_menu);

				LoadGameSlot(saveload->choice+1);

				if(mp_load)
					create_server_panel->Show();
			}
			catch(cstring err)
			{
				err = Format("%s%s", txLoadError, err);
				ERROR(err);
				GUI.SimpleDialog(err, saveload);
				mp_load = false;
			}
		}
	}
}

void Game::SaveEvent(int id)
{
	if(id == BUTTON_OK)
	{
		if(SaveGameSlot(saveload->choice+1, save_input_text.c_str()))
		{
			// zapisywanie si� uda�o, zamknij okno zapisu
			GUI.CloseDialog(saveload);
		}
	}
}

//=================================================================================================
// Zapisz opcje
//=================================================================================================
void Game::SaveOptions()
{
	cfg.Add("fullscreen", fullscreen ? "1" : "0");
	cfg.Add("cl_glow", cl_glow ? "1" : "0");
	cfg.Add("cl_normalmap", cl_normalmap ? "1" : "0");
	cfg.Add("cl_specularmap", cl_specularmap ? "1" : "0");
	cfg.Add("sound_volume", Format("%d", sound_volume));
	cfg.Add("music_volume", Format("%d", music_volume));
	cfg.Add("mouse_sensitivity", Format("%d", mouse_sensitivity));
	cfg.Add("grass_range", Format("%g", grass_range));
	cfg.Add("resolution", Format("%dx%d", wnd_size.x, wnd_size.y));
	cfg.Add("refresh", Format("%d", wnd_hz));
	cfg.Add("skip_tutorial", skip_tutorial ? "1" : "0");
	cfg.Add("language", g_lang_prefix.c_str());
	int ms, msq;
	GetMultisampling(ms, msq);
	cfg.Add("multisampling", Format("%d", ms));
	cfg.Add("multisampling_quality", Format("%d", msq));
	SaveCfg();
}

//=================================================================================================
// Poka� opcje
//=================================================================================================
void Game::ShowOptions()
{
	GUI.ShowDialog(options);
}

void Game::ShowSavePanel()
{
	saveload->SetSaveMode(true, IsOnline(), IsOnline() ? multi_saves : single_saves);
	GUI.ShowDialog(saveload);
}

void Game::ShowLoadPanel()
{
	saveload->SetSaveMode(false, mp_load, mp_load ? multi_saves : single_saves);
	GUI.ShowDialog(saveload);
}

void Game::StartNewGame()
{
	Human& h = *create_character->unit->human_data;
	NewGameCommon(create_character->clas, create_character->name.c_str(), h.beard, h.mustache, h.hair, h.height, h.hair_color);
	in_tutorial = false;

	GenerateWorld();
	InitQuests();
	EnterLocation();
}

void Game::OnQuit(int id)
{
	if(id == BUTTON_YES)
	{
		GUI.GetDialog("dialog_alt_f4")->visible = false;
		Quit();
	}
}

void Game::OnExit(int id)
{
	if(id == BUTTON_YES)
		ExitToMenu();
}

void Game::ShowCreateCharacterPanel(bool require_name, bool redo)
{
	create_character->enter_name = require_name;
	create_character->event = DialogEvent(this, &Game::OnCreateCharacter);

	if(redo)
	{
		PlayerInfo& info = game_players[0];
		create_character->Redo(info.clas, info.hd, info.cc);
	}
	else
		create_character->Random();

	GUI.ShowDialog(create_character);
}

void Game::StartQuickGame()
{
	sv_online = false;
	sv_server = false;

	Class clas;
	if(quickstart_class == Class::RANDOM)
		clas = ClassInfo::GetRandomPlayer();
	else
		clas = quickstart_class;

	NewGameCommon(clas, quickstart_name.c_str(), rand2()%MAX_BEARD-1, rand2()%MAX_MUSTACHE-1, rand2()%MAX_HAIR-1, random(0.95f,1.05f), g_hair_colors[rand2()%n_hair_colors]);
	in_tutorial = false;

	GenerateWorld();
	InitQuests();
	EnterLocation();
}

void Game::NewGameCommon(Class clas, cstring name, int beard, int mustache, int hair, float height, const VEC4& hair_color)
{
	main_menu->visible = false;
	sv_online = false;
	game_state = GS_LEVEL;
	hardcore_mode = hardcore_option;

	UnitData& ud = *g_classes[(int)clas].unit_data;

	Unit* u = CreateUnit(ud, -1, NULL, false);
	u->MakeItemsTeam(false);
	u->human_data->beard = beard;
	u->human_data->mustache = mustache;
	u->human_data->hair = hair;
	u->human_data->height = height;
	u->human_data->hair_color = hair_color;
	u->human_data->ApplyScale(aHumanBase);
	team.clear();
	active_team.clear();
	team.push_back(u);
	active_team.push_back(u);
	leader = u;

	u->player = new PlayerController;
	pc = u->player;
	pc->Init(*u);
	pc->clas = clas;
	pc->name = name;
	pc->unit->RecalculateWeight();
	pc->dialog_ctx = &dialog_context;
	pc->dialog_ctx->dialog_mode = false;
	pc->dialog_ctx->next_talker = NULL;
	pc->dialog_ctx->pc = pc;
	pc->dialog_ctx->is_local = true;
	dialog_context.pc = pc;

	u->level = u->CalculateLevel();

	ClearGameVarsOnNewGame();
	SetGamePanels();

	fallback_co = FALLBACK_NONE;
	fallback_t = 0.f;
	if(change_title_a)
		ChangeTitle();
}

void Game::MultiplayerPanelEvent(int id)
{
	player_name = multiplayer_panel->textbox.text;

	if(id == MultiplayerPanel::IdCancel)
	{
		multiplayer_panel->CloseDialog();
		return;
	}

	// sprawd� czy podano nick
	if(player_name.empty())
	{
		GUI.SimpleDialog(multiplayer_panel->txNeedEnterNick, multiplayer_panel);
		return;
	}

	// sprawd� czy nick jest poprawny
	if(!ValidateNick(player_name.c_str()))
	{
		GUI.SimpleDialog(multiplayer_panel->txEnterValidNick, multiplayer_panel);
		return;
	}

	// zapisz nick
	cfg.Add("nick", player_name.c_str());
	SaveCfg();

	switch(id)
	{
	case MultiplayerPanel::IdJoinLan:
		// do��cz lan
		pick_server_panel->Show();
		break;
	case MultiplayerPanel::IdJoinIp:
		{
			// wy�wietl okno na podawanie adresu ip
			GetTextDialogParams params(txEnterIp, server_ip);
			params.event = DialogEvent(this, &Game::OnEnterIp);
			params.limit = 100;
			params.parent = multiplayer_panel;
			GetTextDialog::Show(params);
		}
		break;
	case MultiplayerPanel::IdCreate:
		// za�� serwer
		create_server_panel->Show();
		break;
	case MultiplayerPanel::IdLoad:
		// wczytaj gr�
		mp_load = true;
		net_changes.clear();
		if(!net_talk.empty())
			StringPool.Free(net_talk);
		ShowLoadPanel();
		break;
	}
}

void Game::CreateServerEvent(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(mp_load)
		{
			ClearGame();
			mp_load = false;
		}
		create_server_panel->CloseDialog();
	}
	else
	{
		// kopiuj
		server_name = create_server_panel->textbox[0].text;
		max_players = atoi(create_server_panel->textbox[1].text.c_str());
		server_pswd = create_server_panel->textbox[2].text;

		// sprawd� dane
		cstring error_text = NULL;
		Control* give_focus = NULL;
		if(server_name.empty())
		{
			error_text = create_server_panel->txEnterServerName;
			give_focus = &create_server_panel->textbox[0];
		}
		else if(!in_range(max_players, 1, MAX_PLAYERS))
		{
			error_text = Format(create_server_panel->txInvalidPlayersCount, MAX_PLAYERS);
			give_focus = &create_server_panel->textbox[1];
		}
		if(error_text)
		{
			GUI.SimpleDialog(error_text, create_server_panel);
			// daj focus textboxowi w kt�rym by� b��d
			create_server_panel->cont.give_focus = give_focus;
			return;
		}

		// zapisz
		cfg.Add("server_name", server_name.c_str());
		cfg.Add("server_pswd", server_pswd.c_str());
		cfg.Add("server_players", Format("%d", max_players));
		SaveCfg();

		// zamknij okna
		create_server_panel->CloseDialog();
		multiplayer_panel->CloseDialog();

		// je�li ok to uruchom serwer
		try
		{
			InitServer();
		}
		catch(cstring err)
		{
			GUI.SimpleDialog(err, main_menu);
			return;
		}

		// przejd� do okna serwera
		server_panel->Show();
		Net_OnNewGameServer();
		UpdateServerInfo();

		if(change_title_a)
			ChangeTitle();
	}
}

void Game::CheckReady()
{
	bool all_ready = true;

	for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
	{
		if(!it->ready)
		{
			all_ready = false;
			break;
		}
	}

	if(all_ready)
		server_panel->bts[4].state = Button::NONE;
	else
		server_panel->bts[4].state = Button::DISABLED;

	if(sv_startup)
		server_panel->StopStartup();
}

void Game::ChangeReady()
{
	if(sv_server)
	{
		// zmie� info
		if(players > 1)
			AddLobbyUpdate(INT2(Lobby_UpdatePlayer,0));
		CheckReady();
	}
	else
	{
		PlayerInfo& info = game_players[0];
		byte b[] = {ID_LOBBY_CHANGE, (info.ready ? 1 : 0), (byte)info.clas};
		peer->Send((cstring)b, 3, HIGH_PRIORITY, RELIABLE_ORDERED, 1, server, false);
	}

	server_panel->bts[1].text = (game_players[0].ready ? server_panel->txNotReady : server_panel->txReady);
}

void Game::AddMsg(cstring msg)
{
	if(server_panel->visible)
		server_panel->AddMsg(msg);
	else
		AddMultiMsg(msg);
}

void Game::RandomCharacter(Class clas)
{
	create_character->Random(clas);
	create_character->event(BUTTON_OK);
}

void Game::OnEnterIp(int id)
{
	if(id == BUTTON_OK)
	{
		SystemAddress adr = UNASSIGNED_SYSTEM_ADDRESS;
		if(adr.FromString(server_ip.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
		{
			// zapisz ip
			cfg.Add("server_ip", server_ip.c_str());
			SaveCfg();

			try
			{
				InitClient();
			}
			catch(cstring error)
			{
				GUI.SimpleDialog(error, multiplayer_panel);
				return;
			}

			LOG(Format("Pinging %s...", server_ip.c_str()));
			info_box->Show(txConnecting);
			net_mode = NM_CONNECT_IP;
			net_state = 0;
			net_timer = T_CONNECT_PING;
			net_tries = I_CONNECT_TRIES;
			net_adr = adr.ToString(false);
			peer->Ping(net_adr.c_str(), (word)mp_port, false);
		}
		else
			GUI.SimpleDialog(txInvalidIp, multiplayer_panel);
	}
}

cstring PacketToString(Packet* packet)
{
	static string s;
	s = Format("%d (size:%d", packet->data[0], packet->length);
	if(packet->length == 1)
		s += ")";
	else if(packet->length == 2)
		s += Format(";%d)", packet->data[1]);
	else
	{
		s += ";";
		for(uint i=0; i<min(50u,packet->length-2); ++i)
		{
			s += Format("%d,", packet->data[i+1]);
		}
		if(packet->length-2 > 50)
			s += "...)";
		else
			s += Format("%d)", packet->data[packet->length-1]);
	}
	return s.c_str();
}

void Game::EndConnecting(cstring msg, bool wait)
{	
	info_box->CloseDialog();
	if(msg)
		GUI.SimpleDialog(msg, pick_server_panel->visible ? (Dialog*)pick_server_panel : (Dialog*)multiplayer_panel);
	if(wait)
		ForceRedraw();
	ClosePeer(wait);
}

void Game::GenericInfoBoxUpdate(float dt)
{
	switch(net_mode)
	{
	case NM_CONNECT_IP: // ��czanie klienta do serwera
		{
			if(net_state == 0) // pingowanie
			{
				net_timer -= dt;
				if(net_timer <= 0.f)
				{
					if(net_tries == 0)
					{
						// czas na po��czenie min��
						WARN("NM_CONNECT_IP(0): Connection timeout.");
						EndConnecting(txConnectTimeout);
						return;
					}
					else
					{
						// pinguj kolejny raz
						LOG(Format("Pinging %s...", net_adr.c_str()));
						peer->Ping(net_adr.c_str(), (word)mp_port, false);
						--net_tries;
						net_timer = T_CONNECT_PING;
					}
				}

				// oczekiwanie na po��czenie
				Packet* packet;
				for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
				{
					if(packet->data[0] != ID_UNCONNECTED_PONG)
					{
						// nieoczekiwany pakiet od serwera
						WARN(Format("NM_CONNECT_IP(0): Unknown server response: %s.", PacketToString(packet)));
						continue;
					}

					if(packet->length == sizeof(RakNet::TimeMS)+1)
					{
						WARN("NM_CONNECT_IP(0): Server not set SetOfflinePingResponse yet.");
						continue;
					}
					else if(packet->length < sizeof(RakNet::TimeMS)+11)
					{
						// zbyt kr�tka odpowied�, to nie jest serwer carpg
						peer->DeallocatePacket(packet);
						ERROR(Format("NM_CONNECT_IP(0): Too short packet to be valid: %s.", PacketToString(packet)));
						EndConnecting(txConnectInvalid);
						return;
					}

					// struktura pakietu
					// 0 char C
					// 1 char A
					// 2-5 int - wersja
					// 6 byte - gracze
					// 7 byte - max graczy
					// 8 byte - flagi (0x01 - has�o, 0x02 - wczytana gra)
					// 9+ byte - nazwa
					// ? - build
					BitStream s(packet->data+1+sizeof(RakNet::TimeMS), packet->length-1-sizeof(RakNet::TimeMS), false);
					char sign_ca[2];
					s.Read(sign_ca[0]);
					s.Read(sign_ca[1]);
					if(sign_ca[0] != 'C' || sign_ca[1] != 'A')
					{
						// z�a sygnatura pakietu, to nie serwer carpg
						peer->DeallocatePacket(packet);
						ERROR(Format("NM_CONNECT_IP(0): Wrong packet signature 0x%x%x: %s.", byte(sign_ca[0]), byte(sign_ca[1]), PacketToString(packet)));
						EndConnecting(txConnectInvalid);
						return;
					}
					
					// odczytaj
					uint wersja, build;
					byte ile_graczy, max_graczy, flagi;

					s.Read(wersja);
					s.Read(ile_graczy);
					s.Read(max_graczy);
					s.Read(flagi);

					if(!ReadString1(s))
					{
						peer->DeallocatePacket(packet);
						ERROR(Format("NM_CONNECT_IP(0): Can't read server name: %s.", PacketToString(packet)));
						EndConnecting(txConnectInvalid);
					}

					s.Read(build);

					// sprawd� wersj�
					if(wersja != WERSJA || g_build != build)
					{
						// brak zgodno�ci wersji
						peer->DeallocatePacket(packet);
						cstring s = VersionToString(wersja);
						ERROR(Format("NM_CONNECT_IP(0): Invalid client version (%s;%u) vs server (%s;%u).", WERSJA_STR, g_build, s, build));
						EndConnecting(Format(txConnectVersion, WERSJA_STR, g_build, s, build));
						return;
					}

					peer->DeallocatePacket(packet);

					// ustaw serwer
					max_players2 = max_graczy;
					server_name2 = BUF;
					LOG(Format("NM_CONNECT_IP(0): Server information. Name:%s; players:%d/%d; flags:%d.", server_name2.c_str(), ile_graczy, max_graczy, flagi));
					if(IS_SET(flagi, 0xFC))
						WARN("NM_CONNECT_IP(0): Unknown server flags.");
					server = packet->systemAddress;
					enter_pswd.clear();

					if(IS_SET(flagi, 0x01))
					{
						// jest has�o
						net_state = 1;
						info_box->Show(txWaitingForPswd);
						GetTextDialogParams params(Format(txEnterPswd, server_name2.c_str()), enter_pswd);
						params.event = DialogEvent(this, &Game::OnEnterPassword);
						params.limit = 16;
						params.parent = info_box;
						GetTextDialog::Show(params);
						LOG("NM_CONNECT_IP(0): Waiting for password...");
					}
					else
					{
						// po��cz
						ConnectionAttemptResult result = peer->Connect(server.ToString(false), (word)mp_port, NULL, 0);
						if(result == CONNECTION_ATTEMPT_STARTED)
						{
							// trwa ��czenie z serwerem
							net_timer = T_CONNECT;
							net_state = 2;
							info_box->Show(Format(txConnectingTo, server_name2.c_str()));
							LOG(Format("NM_CONNECT_IP(0): Connecting to server %s...", server_name2.c_str()));
						}
						else
						{
							// b��d ��czenie z serwerem
							ERROR(Format("NM_CONNECT_IP(0): Can't connect to server: raknet error %d.", result));
							EndConnecting(txConnectRaknet);
						}
					}

					break;
				}
			}
			else if(net_state == 2) // ��czenie
			{
				Packet* packet;
				for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
				{
					switch(packet->data[0])
					{
					case ID_CONNECTION_REQUEST_ACCEPTED:
						{
							// zaakceptowano nasz po��czenie, wy�lij komunikat powitalny
							// byte - ID_HELLO
							// int - wersja
							// int - build
							// string1 - nick
							LOG("NM_CONNECT_IP(2): Connected with server.");
							net_stream.Reset();
							net_stream.Write(ID_HELLO);
							net_stream.Write(WERSJA);
							net_stream.Write(g_build);
							WriteString1(net_stream, player_name);
							peer->Send(&net_stream, IMMEDIATE_PRIORITY, RELIABLE, 0, server, false);
						}
						break;
					case ID_JOIN:
						// serwer nas zaakceptowa�, przys�a� informacje o graczach i mojej postaci
						{
							BitStream s(packet->data+1, packet->length-1, false);
							int ile, load_char;

							// odczytaj
							if( !s.ReadCasted<byte>(my_id) ||
								!s.ReadCasted<byte>(players) ||
								!s.ReadCasted<byte>(leader_id) ||
								!s.ReadCasted<byte>(ile))
							{
								peer->DeallocatePacket(packet);
								ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN: %s.", PacketToString(packet)));
								EndConnecting(txCantJoin, true);
								return;
							}

							// dodaj posta�
							game_players.clear();
							PlayerInfo& info = Add1(game_players);
							info.clas = Class::INVALID;
							info.ready = false;
							info.name = player_name;
							info.id = my_id;
							info.state = PlayerInfo::IN_LOBBY;
							info.update_flags = 0;
							info.left = false;
							info.loaded = false;
							info.buffs = 0;

							// odczytaj pozosta�e postacie
							for(int i=0; i<ile; ++i)
							{
								PlayerInfo& info2 = Add1(game_players);
								info2.state = PlayerInfo::IN_LOBBY;
								info2.left = false;
								info2.loaded = false;

								if( !s.ReadCasted<byte>(info2.id) ||
									!ReadBool(s, info2.ready) ||
									!s.ReadCasted<byte>(info2.clas) ||
									!ReadString1(s, info2.name))
								{
									peer->DeallocatePacket(packet);
									ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN(2): %s.", PacketToString(packet)));
									EndConnecting(txCantJoin, true);
									return;
								}

								// sprawd� klas�
								if(!ClassInfo::IsPickable(info2.clas))
								{
									peer->DeallocatePacket(packet);
									ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN, player %s has class %d: %s.", info2.name.c_str(), info2.clas, PacketToString(packet)));
									EndConnecting(txCantJoin, true);
									return;
								}
							}

							// informacja o zapisie
							if(!s.ReadCasted<byte>(load_char))
							{
								peer->DeallocatePacket(packet);
								ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN(4): %s.", PacketToString(packet)));
								EndConnecting(txCantJoin, true);
								return;
							}
							if(load_char == 2 && !s.ReadCasted<byte>(game_players[0].clas))
							{
								peer->DeallocatePacket(packet);
								ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN(3): %s.", PacketToString(packet)));
								EndConnecting(txCantJoin, true);
								return;
							}

							peer->DeallocatePacket(packet);

							// sprawd� przyw�dc�
							int index = GetPlayerIndex(leader_id);
							if(index == -1)
							{
								ERROR(Format("NM_CONNECT_IP(2): Broken packet ID_JOIN, no player with leader id %d: %s.", leader_id, PacketToString(packet)));
								EndConnecting(txCantJoin, true);
								return;
							}

							// przejd� do lobby
							if(multiplayer_panel->visible)
								multiplayer_panel->CloseDialog();
							server_panel->Show();
							server_panel->grid.AddItems(ile+1);
							if(load_char != 0)
								server_panel->UseLoadedCharacter(load_char == 2);
							if(load_char != 2)
								server_panel->CheckAutopick();
							info_box->CloseDialog();
							if(change_title_a)
								ChangeTitle();
							return;
						}
						break;
					case ID_CANT_JOIN:
						{
							cstring reason, reason_eng;
							// serwer odrzuci� nasze po��czenie
							// (0-brak miejsca,1-z�a wersja,2-zaj�ty nick,3-(nieu�ywane)u,4-z�e ID_HELLO,5-nieznany b��d,6-b��dny nick)

							const int type = packet->length >= 2 ? packet->data[1] : 5;
							switch(type)
							{
							case 0:
								reason = txServerFull;
								reason_eng = "server is full";
								break;
							case 1:
								if(packet->length == 6)
								{
									int w;
									memcpy(&w, packet->data+2, 4);
									cstring s = VersionToString(w);
									reason = Format(txInvalidVersion2, WERSJA, s);
									reason_eng = Format("invalid version (%s) vs server (%s)", WERSJA, s);
								}
								else
								{
									reason = txInvalidVersion;
									reason_eng = "invalid version";
								}
								break;
							case 2:
								reason = txNickUsed;
								reason_eng = "nick used";
								break;
							//case 3: unused
							case 4:
								reason = txInvalidData;
								reason_eng = "invalid data";
								break;
							case 5:
							default:
								reason = NULL;
								reason_eng = NULL;
								break;
							case 6:
								reason = txInvalidNick;
								reason_eng = "invalid nick";
								break;
							}

							peer->DeallocatePacket(packet);
							if(reason)
							{
								LOG(Format("NM_CONNECT_IP(2):%s: %s.", txCantJoin2, reason_eng));
								EndConnecting(Format("%s:\n%s", txCantJoin2, reason), true);
							}
							else
							{
								LOG(Format("NM_CONNECT_IP(2):%s(%d).", txCantJoin2, type));
								EndConnecting(txCantJoin2, true);
							}
							return;
						}
					case ID_CONNECTION_LOST:
					case ID_DISCONNECTION_NOTIFICATION:
						// utracono po��czenie z serwerem albo nas wykopano
						peer->DeallocatePacket(packet);
						WARN(packet->data[0] == ID_CONNECTION_LOST ? "NM_CONNECT_IP(2): Lost connection with server." : "NM_CONNECT_IP(2): Disconnected from server.");
						EndConnecting(txLostConnection);
						return;
					case ID_INVALID_PASSWORD:
						// podano b��dne has�o
						peer->DeallocatePacket(packet);
						ERROR("NM_CONNECT_IP(2): Invalid password.");
						EndConnecting(txInvalidPswd);
						return;
					default:
						WARN(Format("NM_CONNECT_IP(2): Unknown packet from server: %s.", PacketToString(packet)));
						break;
					}
				}

				net_timer -= dt;
				if(net_timer <= 0.f)
				{
					WARN("NM_CONNECT_IP(2): Connection timeout.");
					EndConnecting(txConnectTimeout);
					return;
				}
			}
		}
		break;

	case NM_QUITTING: // od��czenie si� klienta od serwera
		{
			Packet* packet;

			for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
			{
				if(packet->data[0] == ID_DISCONNECTION_NOTIFICATION || packet->data[0] == ID_CONNECTION_LOST)
				{
					LOG("NM_QUITTING: Server accepted disconnection.");
					peer->DeallocatePacket(packet);
					ClosePeer();
					if(server_panel->visible)
						server_panel->CloseDialog();
					if(net_callback)
						net_callback();
					info_box->CloseDialog();
					return;
				}
				else
					LOG(Format("NM_QUITTING: Ignored packet: %s.", PacketToString(packet)));
			}

			net_timer -= dt;
			if(net_timer <= 0.f)
			{
				WARN("NM_QUITTING: Disconnected without accepting.");
				peer->DeallocatePacket(packet);
				ClosePeer();
				if(server_panel->visible)
					server_panel->CloseDialog();
				if(net_callback)
					net_callback();
				info_box->CloseDialog();
				return;
			}
		}
		break;

	case NM_QUITTING_SERVER: // zamykanie serwera, wyrzucanie graczy
		{
			Packet* packet;
			for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
			{
				int index = FindPlayerIndex(packet->systemAddress);
				if(index == -1)
					LOG(Format("NM_QUITTING_SERVER: Ignoring packet from unconnected player %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
				else
				{
					PlayerInfo& info = game_players[index];
					if(packet->data[0] == ID_DISCONNECTION_NOTIFICATION || packet->data[0] == ID_CONNECTION_LOST)
					{
						if(info.state == PlayerInfo::IN_LOBBY)
							LOG(Format("NM_QUITTING_SERVER: Player %s left lobby.", info.name.c_str()));
						else
							LOG(Format("NM_QUITTING_SERVER: %s left lobby.", packet->systemAddress.ToString()));
						--players;
						if(players == 0)
						{
							peer->DeallocatePacket(packet);
							LOG("NM_QUITTING_SERVER: All players disconnected from server. Closing...");
							ClosePeer();
							info_box->CloseDialog();
							if(server_panel->visible)
								server_panel->CloseDialog();
							if(net_callback)
								net_callback();
							return;
						}
					}
					else
					{
						LOG(Format("NM_QUITTING_SERVER: Ignoring packet from %s: %s.", info.state == PlayerInfo::IN_LOBBY ? info.name.c_str() : packet->systemAddress.ToString(),
							PacketToString(packet)));
					}
				}
			}

			net_timer -= dt;
			if(net_timer <= 0.f)
			{
				WARN("NM_QUITTING_SERVER: Not all players disconnected on time. Closing server...");
				ClosePeer();
				info_box->CloseDialog();
				if(server_panel->visible)
					server_panel->CloseDialog();
				if(net_callback)
					net_callback();
				return;
			}
		}
		break;

	case NM_TRANSFER:
		{
			Packet* packet;
			for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
			{
				switch(packet->data[0])
				{
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
					peer->DeallocatePacket(packet);
					ClosePeer();
					info_box->CloseDialog();
					GUI.SimpleDialog(txLostConnection, NULL);
					LOG("NM_TRANSFER: Lost connection with server.");
					ExitToMenu();
					return;
				case ID_STATE:
					if(packet->length != 2)
						WARN(Format("NM_TRANSFER: Broken packet ID_STATE: %s.", PacketToString(packet)));
					else
					{
						if(packet->data[1] == 0)
							info_box->Show(txGeneratingWorld);
					}
					break;
				case ID_WORLD_DATA:
					{
						LOG("NM_TRANSFER: Received world data.");

						LoadingStep("");
						ClearGameVarsOnNewGame();
						Net_OnNewGameClient();

						fallback_co = FALLBACK_NONE;
						fallback_t = 0.f;
						net_state = 0;

						BitStream s(packet->data+1, packet->length-1, false);
						if(ReadWorldData(s))
						{
							// ode�lij informacje o gotowo�ci
							byte b[] = {ID_READY, 2};
							peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
							info_box->Show(txLoadedWorld);
							LoadingStep("");
							LOG("NM_TRANSFER: Loaded world data.");
						}
						else
						{
							ERROR("NM_TRANSFER: Failed to read world data.");
							peer->DeallocatePacket(packet);
							EndConnecting(txWorldDataError, true);
							return;
						}
					}
					break;
				case ID_PLAYER_START_DATA:
					if(net_state == 0)
					{
						++net_state;
						LoadingStep("");
						BitStream s(packet->data+1, packet->length-1, false);
						if(ReadPlayerStartData(s))
						{
							// ode�lij informacje o gotowo�ci
							byte b[] = {ID_READY, 3};
							peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
							info_box->Show(txLoadedPlayer);
							LoadingStep("");
							LOG("NM_TRANSFER: Loaded player data.");
						}
						else
						{
							ERROR("NM_TRANSFER: Failed to read player data.");
							peer->DeallocatePacket(packet);
							EndConnecting(txPlayerDataError);
							return;
						}
					}
					else
					{
						assert(0);
					}
					break;
				case ID_CHANGE_LEVEL:
					if(packet->length == 3)
					{
						if(net_state == 1)
						{
							++net_state;
							byte loc = packet->data[1];
							byte level = packet->data[2];
							if(loc < locations.size())
							{
								if(game_state == GS_LOAD)
									LoadingStep("");
								else
									LoadingStart(4);
								current_location = loc;
								location = locations[loc];
								dungeon_level = level;
								LOG(Format("NM_TRANSFER: Level change to %s (id:%d, level:%d).", location->name.c_str(), current_location, dungeon_level));
								info_box->Show(txGeneratingLocation);
							}
							else
								WARN(Format("NM_TRANSFER: Broken packet ID_CHANGE_LEVEL, invalid location %d: %s.", (int)loc, PacketToString(packet)));
						}
						else
						{
							assert(0);
						}
					}
					else
						WARN(Format("NM_TRANSFER: Broken packat ID_CHANGE_LEVEL: %s.", PacketToString(packet)));
					break;
				case ID_LEVEL_DATA:
					if(net_state == 2)
					{
						++net_state;
						info_box->Show(txLoadingLocation);
						LoadingStep("");
						BitStream s(packet->data+1, packet->length-1, false);
						cstring err = ReadLevelData(s);
						if(err)
						{
							string s = err; // format nadpisze ten tekst przez PacketToString ...
							ERROR(Format("NM_TRANSFER: Failed to read location data, %s: %s.", s.c_str(), PacketToString(packet)));
							peer->DeallocatePacket(packet);
							ClearGame();
							EndConnecting(txLoadingLocationError);
							return;
						}
						else
						{
							LOG("NM_TRANSFER: Loaded level data.");
							byte b[] = {ID_READY, 3};
							peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
							LoadingStep("");
						}
					}
					else
					{
						assert(0);
					}
					break;
				case ID_PLAYER_DATA2:
					if(net_state == 3)
					{
						++net_state;
						info_box->Show(txLoadingChars);
						LoadingStep("");
						BitStream s(packet->data+1, packet->length-1, false);
						cstring err = ReadPlayerData(s);
						if(err)
						{
							string s = err; // PacketToString nadpisuje format
							ERROR(Format("NM_TRANSFER: Failed to read player data, %s: %s.", s.c_str(), PacketToString(packet)));
							peer->DeallocatePacket(packet);
							ClearGame();
							EndConnecting(txLoadingCharsError);
							return;
						}
						else
						{
							LOG("NM_TRANSFER: Loaded player data.");
							byte b[] = {ID_READY, 4};
							peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, server, false);
							LoadingStep("");
						}
					}
					else
					{
						assert(0);
					}
					break;
				case ID_START:
					if(net_state == 4)
					{
						++net_state;
						LOG("NM_TRANSFER: Level started.");
						clear_color = clear_color2;
						game_state = GS_LEVEL;
						load_screen->visible = false;
						main_menu->visible = false;
						game_gui_container->visible = true;
						game_messages->visible = true;
						world_map->visible = false;
						info_box->CloseDialog();
						update_timer = 0.f;
						fallback_co = FALLBACK_NONE;
						fallback_t = 0.f;
						first_frame = true;
						rot_buf = 0.f;
						if(change_title_a)
							ChangeTitle();
						peer->DeallocatePacket(packet);
						SetGamePanels();
						OnEnterLevelOrLocation();
					}
					else
					{
						assert(0);
					}
					return;
				case ID_START_AT_WORLDMAP:
					if(net_state == 1)
					{
						++net_state;
						LOG("NM_TRANSFER: Starting at world map.");
						clear_color = WHITE;
						game_state = GS_WORLDMAP;
						load_screen->visible = false;
						main_menu->visible = false;
						game_gui_container->visible = false;
						game_messages->visible = true;
						world_map->visible = true;
						world_state = WS_MAIN;
						info_box->CloseDialog();
						update_timer = 0.f;
						leader_id = 0;
						leader = NULL;
						pc = NULL;
						SetMusic(MUSIC_TRAVEL);
						if(change_title_a)
							ChangeTitle();
						peer->DeallocatePacket(packet);
					}
					else
					{
						assert(0);
					}
					return;
				default:
					WARN(Format("NM_TRANSFER: Unknown packet: %s.", PacketToString(packet)));
					break;
				}
			}
		}
		break;

	case NM_TRANSFER_SERVER:
		{
			Packet* packet;
			for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
			{
				int index = FindPlayerIndex(packet->systemAddress);
				if(index == -1)
				{
					LOG(Format("NM_TRANSFER_SERVER: Ignoring packet from %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
					continue;
				}

				PlayerInfo& info = game_players[index];

				switch(packet->data[0])
				{
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
					LOG(Format("NM_TRANSFER_SERVER: Player %s left game.", info.name.c_str()));
					--players;
					game_players.erase(game_players.begin()+index);
					return;
				case ID_READY:
					if(net_state == 3 || packet->length != 2)
					{
						if(packet->data[1] == 2)
						{
							LOG(Format("NM_TRANSFER_SERVER: %s read world data.", info.name.c_str()));
							net_stream2.Reset();
							net_stream2.WriteCasted<byte>(ID_PLAYER_START_DATA);
							WritePlayerStartData(net_stream2, info);
							peer->Send(&net_stream2, MEDIUM_PRIORITY, RELIABLE, 0, info.adr, false);
						}
						else
						{
							LOG(Format("NM_TRANSFER_SERVER: %s is ready.", info.name.c_str()));
							info.ready = true;
						}
					}
					else
						WARN(Format("NM_TRANSFER_SERVER: Unexpected packet ID_READY from %s: %s.", info.name.c_str(), PacketToString(packet)));
					break;
				default:
					WARN(Format("NM_TRANSFER_SERVER: Unknown packet from %s: %s.", info.name.c_str(), PacketToString(packet)));
					break;
				}
			}

			if(net_state == 0)
			{
				if(!mp_load)
				{
					info_box->Show(txGeneratingWorld);

					// send info about generating world
					if(players > 1)
					{
						byte b[] = { ID_STATE, 0 };
						peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
					}

					// do it
					ClearGameVarsOnNewGame();
					free_recruit = false;
					fallback_co = FALLBACK_NONE;
					fallback_t = 0.f;
					main_menu->visible = false;
					load_screen->visible = true;
					clear_color = BLACK;
					GenerateWorld();
					InitQuests();
				}

				net_state = 1;
			}
			else if(net_state == 1)
			{
				// prepare world data if there is any players
				if(players > 1)
				{
					info_box->Show(txSendingWorld);

					// send info about preparing world data
					byte b[] = { ID_STATE, 1 };
					peer->Send((cstring)b, 2, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

					// prepare & send world data
					PrepareWorldData(net_stream);
					peer->Send(&net_stream, MEDIUM_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
					LOG(Format("NM_TRANSFER_SERVER: Send world data, size %d.", net_stream.GetNumberOfBytesUsed()));
					net_state = 2;
					net_timer = mp_timeout;
					for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
					{
						if(it->id != my_id)
							it->ready = false;
						else
							it->ready = true;
					}
					info_box->Show(txWaitingForPlayers);
				}
				else
					net_state = 3;

				vector<Unit*> prev_team;

				// create team
				if(mp_load)
					prev_team = team;
				team.clear();
				active_team.clear();
				const bool in_level = (open_location != -1);
				for(PlayerInfo& info : game_players)
				{
					Unit* u;

					if(!info.loaded)
					{
						UnitData& ud = *g_classes[(int)info.clas].unit_data;

						u = CreateUnit(ud, -1, NULL, in_level);
						info.u = u;
						u->MakeItemsTeam(false);
						info.hd.Set(*u->human_data);
						u->human_data->ApplyScale(aHumanBase);
						u->ani->need_update = true;

						u->player = new PlayerController;
						u->player->id = info.id;
						u->player->clas = info.clas;
						u->player->name = info.name;
						u->player->Init(*u);
						u->RecalculateWeight();
					}
					else
					{
						PlayerInfo* old = FindOldPlayer(info.name.c_str());
						old->loaded = true;
						u = old->u;
						info.u = u;
						u->player->id = info.id;
					}

					team.push_back(u);
					active_team.push_back(u);

					info.pc = u->player;
					u->player->player_info = &info;
					if(info.id == my_id)
					{
						pc = u->player;
						u->player->dialog_ctx = &dialog_context;
						u->player->dialog_ctx->is_local = true;
						u->player->is_local = true;
					}
					else
					{
						u->player->dialog_ctx = new DialogContext;
						u->player->dialog_ctx->is_local = false;
					}
					u->player->dialog_ctx->dialog_mode = false;
					u->player->dialog_ctx->next_talker = NULL;
					u->player->dialog_ctx->pc = u->player;
					u->interp = interpolators.Get();
					u->interp->Reset(u->pos, u->rot);
				}

				// add ai
				bool anyone_left = false;
				for(vector<Unit*>::iterator it = prev_team.begin(), end = prev_team.end(); it != end; ++it)
				{
					if(!(*it)->IsPlayer())
					{
						if((*it)->hero->free)
							team.push_back(*it);
						else
						{
							if(team.size() < MAX_TEAM_SIZE)
							{
								team.push_back(*it);
								active_team.push_back(*it);
							}
							else
							{
								// za du�o postaci w dru�ynie, wywal ai
								NetChange& c = Add1(net_changes);
								c.type = NetChange::HERO_LEAVE;
								c.unit = *it;

								AddMultiMsg(Format(txMpNPCLeft, (*it)->hero->name.c_str()));
								if(city_ctx)
									(*it)->hero->mode = HeroData::Wander;
								else
									(*it)->hero->mode = HeroData::Leave;
								(*it)->hero->team_member = false;
								(*it)->hero->kredyt = 0;
								(*it)->ai->city_wander = false;
								(*it)->ai->loc_timer = random(5.f, 10.f);
								(*it)->MakeItemsTeam(true);
								(*it)->temporary = true;

								anyone_left = true;
							}
						}
					}
				}

				// recalculate credit if someone left
				if(anyone_left)
					CheckCredit(false, true);

				// set leader
				int index = GetPlayerIndex(leader_id);
				if(index == -1)
				{
					leader_id = 0;
					leader = game_players[0].u;
				}
				leader = game_players[GetPlayerIndex(leader_id)].u;
			}
			else if(net_state == 2)
			{
				// wait for all players
				bool ok = true;
				for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
				{
					if(!it->ready)
					{
						ok = false;
						break;
					}
				}
				net_timer -= dt;
				if(!ok && net_timer <= 0.f)
				{
					for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end;)
					{
						if(!it->ready)
						{
							LOG(Format("Disconnecting player %s due no response.", it->name.c_str()));
							--players;
							peer->CloseConnection(it->adr, true, 0, IMMEDIATE_PRIORITY);
							it = game_players.erase(it);
							end = game_players.end();
						}
						else
							++it;
					}
					ok = true;
				}
				if(ok)
				{
					LOG("All players ready.");
					net_state = 3;
				}
			}
			else if(net_state == 3)
			{
				// wejd� do lokacji
				info_box->Show(txLoadingLevel);
				if(!mp_load)
					EnterLocation();
				else
				{
					if(open_location == -1)
					{
						// zapisano na mapie �wiata
						byte b = ID_START_AT_WORLDMAP;
						peer->Send((cstring)&b, 1, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);

						DeleteOldPlayers();
						
						clear_color = clear_color2;
						game_state = GS_WORLDMAP;
						load_screen->visible = false;
						world_map->visible = true;
						game_messages->visible = true;
						game_gui_container->visible = false;
						mp_load = false;
						clear_color = WHITE;
						world_state = WS_MAIN;
						update_timer = 0.f;
						SetMusic(MUSIC_TRAVEL);
						info_box->CloseDialog();
					}
					else
					{
						packet_data.resize(3);
						packet_data[0] = ID_CHANGE_LEVEL;
						packet_data[1] = (byte)current_location;
						packet_data[2] = 0;
						int ack = peer->Send((cstring)&packet_data[0], 3, HIGH_PRIORITY, RELIABLE_WITH_ACK_RECEIPT, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
						for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
						{
							if(it->id == my_id)
								it->state = PlayerInfo::IN_GAME;
							else
							{
								it->state = PlayerInfo::WAITING_FOR_RESPONSE;
								it->ack = ack;
								it->timer = mp_timeout;
							}
						}

						// znajd� jakiego� gracza kt�ry jest w zapisie i teraz
						Unit* center_unit = NULL;
						// domy�lnie to lider
						if(GetPlayerInfo(leader->player).loaded)
							center_unit = leader;
						else
						{
							// ktokolwiek
							for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
							{
								if(it->loaded)
								{
									center_unit = it->u;
									break;
								}
							}
						}

						// stw�rz postacie kt�rych nie by�o (przenie� do powy�szej postaci lub na pocz�tek poziomu)
						if(center_unit)
						{
							LevelContext& ctx = GetContext(*center_unit);
							for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
							{
								if(!it->loaded)
								{
									ctx.units->push_back(it->u);
									WarpNearLocation(ctx, *it->u, center_unit->pos, location->outside ? 4.f : 2.f, false, 20);
									it->u->rot = lookat_angle(it->u->pos, center_unit->pos);
									it->u->interp->Reset(it->u->pos, it->u->rot);
								}
							}
						}
						else
						{
							VEC3 pos;
							float rot;
							Portal* portal;

							if(city_ctx)
								GetCityEntry(pos, rot);
							else if(enter_from >= ENTER_FROM_PORTAL && (portal = location->GetPortal(enter_from)))
							{
								pos = portal->pos + VEC3(sin(portal->rot)*2, 0, cos(portal->rot)*2);
								rot = clip(portal->rot+PI);
							}
							else if(location->type == L_DUNGEON || location->type == L_CRYPT)
							{
								InsideLocation* inside = (InsideLocation*)location;
								InsideLocationLevel& lvl = inside->GetLevelData();
								if(enter_from == ENTER_FROM_DOWN_LEVEL)
								{
									pos = pt_to_pos(lvl.schody_dol+dir_to_pos(lvl.schody_dol_dir));
									rot = dir_to_rot(lvl.schody_dol_dir);
								}
								else
								{
									pos = pt_to_pos(lvl.schody_gora+dir_to_pos(lvl.schody_gora_dir));
									rot = dir_to_rot(lvl.schody_gora_dir);
								}
							}
							else
								GetOutsideSpawnPoint(pos, rot);

							for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
							{
								if(!it->loaded)
								{
									local_ctx.units->push_back(it->u);
									WarpNearLocation(local_ctx, *it->u, pos, location->outside ? 4.f : 2.f, false, 20);
									it->u->rot = rot;
									it->u->interp->Reset(it->u->pos, it->u->rot);
								}
							}
						}

						DeleteOldPlayers();

						net_mode = NM_SERVER_SEND;
						net_state = 0;
						if(players > 1)
						{
							PrepareLevelData(net_stream);
							LOG(Format("Generated level packet: %d.", net_stream.GetNumberOfBytesUsed()));
							info_box->Show(txWaitingForPlayers);
						}
					}
				}
			}
		}
		break;

	case NM_SERVER_SEND:
		{
			Packet* packet;

			for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
			{
				int index = FindPlayerIndex(packet->systemAddress);
				if(index == -1)
				{
					LOG(Format("Ignoring packet from %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
					continue;
				}

				PlayerInfo& info = game_players[index];
				if(info.left)
				{
					LOG(Format("Packet from %s who left game.", info.name.c_str(), PacketToString(packet)));
					continue;
				}

				switch(packet->data[0])
				{
				case ID_DISCONNECTION_NOTIFICATION:
				case ID_CONNECTION_LOST:
					players_left.push_back(info.id);
					info.left = true;
					info.left_reason = 2;
					return;
				case ID_SND_RECEIPT_ACKED:
					if(info.state != PlayerInfo::WAITING_FOR_RESPONSE)
						WARN(Format("Unexpected packet ID_SND_RECEIPT_ACKED from %s: %s.", info.name.c_str(), PacketToString(packet)));
					if(packet->length != 5)
						WARN(Format("Broken packet ID_SND_RECEIPT_ACKED from %s: %s.", info.name.c_str(), PacketToString(packet)));
					else
					{
						int ack;
						memcpy(&ack, packet->data+1, 4);
						if(ack == info.ack)
						{
							// wy�lij dane poziomu
							peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
							info.timer = mp_timeout;
							info.state = PlayerInfo::WAITING_FOR_DATA;
							LOG(Format("Send level data to %s.", info.name.c_str()));
						}
						else
							WARN(Format("Unexpected packet ID_SND_RECEIPT_ACKED from %s (2): %s.", info.name.c_str(), PacketToString(packet)));
					}
					break;
				case ID_READY:
					if(info.state == PlayerInfo::IN_GAME || info.state == PlayerInfo::WAITING_FOR_RESPONSE)
						WARN(Format("Unexpected packet ID_READY from %s: %s.", info.name.c_str(), PacketToString(packet)));
					else if(info.state == PlayerInfo::WAITING_FOR_DATA)
					{
						LOG(Format("Send player data to %s.", info.name.c_str()));
						info.state = PlayerInfo::WAITING_FOR_DATA2;
						SendPlayerData(index);
					}
					else
					{
						LOG(Format("Player %s is ready.", info.name.c_str()));
						info.state = PlayerInfo::IN_GAME;
					}
					break;
				case ID_CONTROL:
					LOG(Format("Ignoring packet ID_CONTROL from %s.", info.name.c_str()));
					break;
				default:
					WARN(Format("MN_SERVER_SEND: Invalid packet %d from %s: %s.", packet->data[0], info.name.c_str(), PacketToString(packet)));
					break;
				}
			}

			bool ok = true;
			for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end;)
			{
				if(it->state != PlayerInfo::IN_GAME && !it->left)
				{
					it->timer -= dt;
					if(it->timer <= 0.f)
					{
						// czas min��, usu�
						LOG(Format("Disconnecting player %s due to no response.", it->name.c_str()));
						peer->CloseConnection(it->adr, true, 0, IMMEDIATE_PRIORITY);
						players_left.push_back(it->id);
						it->left = true;
						it->left_reason = 2;
					}
					else
					{
						ok = false;
						++it;
					}
				}
				else
					++it;
			}
			if(ok)
			{
				if(players > 1) // tu nie uwzgl�dnia usuni�tych :S
				{
					byte b = ID_START;
					peer->Send((cstring)&b, 1, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				}
				for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
					it->update_timer = 0.f;
				for(vector<Unit*>::iterator it = local_ctx.units->begin(), end = local_ctx.units->end(); it != end; ++it)
					(*it)->changed = false;
				if(city_ctx)
				{
					for(vector<InsideBuilding*>::iterator it = city_ctx->inside_buildings.begin(), end = city_ctx->inside_buildings.end(); it != end; ++it)
					{
						for(vector<Unit*>::iterator it2 = (*it)->units.begin(), end2 = (*it)->units.end(); it2 != end2; ++it2)
							(*it2)->changed = false;
					}
				}
				LOG("All players ready. Starting game.");
				clear_color = clear_color2;
				game_state = GS_LEVEL;
				info_box->CloseDialog();
				load_screen->visible = false;
				main_menu->visible = false;
				game_gui_container->visible = true;
				game_messages->visible = true;
				world_map->visible = false;
				mp_load = false;
				SetMusic();
				SetGamePanels();
			}
		}
		break;
	}
}

void Game::OnEnterPassword(int id)
{
	if(id == BUTTON_CANCEL)
	{
		// nie podano has�a
		EndConnecting(NULL);
	}
	else
	{
		// podano has�o do serwera
		LOG("Password entered.");
		// po��cz
		ConnectionAttemptResult result = peer->Connect(server.ToString(false), (word)mp_port, enter_pswd.c_str(), enter_pswd.length());
		if(result == CONNECTION_ATTEMPT_STARTED)
		{
			net_state = 2;
			net_timer = T_CONNECT;
			info_box->Show(Format(txConnectingTo, server_name2.c_str()));
			LOG(Format("NM_CONNECT_IP(1): Connecting to server %s...", server_name2.c_str()));
		}
		else
		{
			LOG(Format("NM_CONNECT_IP(1): Can't connect to server: raknet error %d.", result));
			EndConnecting(txConnectRaknet);
		}
	}
}

void Game::ForceRedraw()
{
	DoPseudotick();
}

void Game::QuickJoinIp()
{
	SystemAddress adr = UNASSIGNED_SYSTEM_ADDRESS;
	if(adr.FromString(server_ip.c_str()) && adr != UNASSIGNED_SYSTEM_ADDRESS)
	{
		try
		{
			InitClient();
		}
		catch(cstring error)
		{
			GUI.SimpleDialog(error, NULL);
			return;
		}

		LOG(Format("Pinging %s...", server_ip.c_str()));
		LOG("sv_online = true");
		sv_online = true;
		sv_server = false;
		info_box->Show(txConnecting);
		net_mode = NM_CONNECT_IP;
		net_state = 0;
		net_timer = T_CONNECT_PING;
		net_tries = I_CONNECT_TRIES;
		net_adr = adr.ToString(false);
		peer->Ping(net_adr.c_str(), (word)mp_port, false);
	}
	else
		WARN("Can't quick connect to server, invalid ip.");
}

void Game::AddMultiMsg(cstring _msg)
{
	assert(_msg);

	mp_box->itb.Add(_msg);
}

void Game::Quit()
{
	LOG("Game: Quit.");

	if(main_menu->check_version_thread)
	{
		TerminateThread(main_menu->check_version_thread, 0);
		CloseHandle(main_menu->check_version_thread);
		main_menu->check_version_thread = NULL;
	}

	if(sv_online)
		CloseConnection(VoidF(this, &Game::DoQuit));
	else
		DoQuit();
}

void Game::DoQuit()
{
	exit_mode = true;
	clearup_shutdown = true;
	ClearGame();
	EngineShutdown();
}

void Game::CloseConnection(VoidF f)
{
	if(info_box->visible)
	{
		switch(net_mode)
		{
		case NM_QUITTING:
		case NM_QUITTING_SERVER:
			if(!net_callback)
				net_callback = f;
			break;
		case NM_CONNECT_IP:
		case NM_TRANSFER:
			info_box->Show(txDisconnecting);
			ForceRedraw();
			ClosePeer(true);
			f();
			break;
		case NM_TRANSFER_SERVER:
		case NM_SERVER_SEND:
			LOG("ServerPanel: Closing server.");

			// zablokuj do��czanie
			peer->SetMaximumIncomingConnections(0);
			// wy��cz info o serwerze
			peer->SetOfflinePingResponse(NULL, 0);

			if(players > 1)
			{
				// roz��cz graczy
				LOG("ServerPanel: Disconnecting clients.");
				const byte b[] = {ID_SERVER_CLOSE, 0};
				peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				net_mode = Game::NM_QUITTING_SERVER;
				--players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				info_box->Show(server_panel->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// nie ma graczy, mo�na zamkn��
				info_box->CloseDialog();
				ClosePeer();
				f();
			}
			break;
		}
	}
	else if(server_panel->visible)
		server_panel->ExitLobby(f);
	else
	{
		if(sv_server)
		{
			LOG("ServerPanel: Closing server.");

			// zablokuj do��czanie
			peer->SetMaximumIncomingConnections(0);
			// wy��cz info o serwerze
			peer->SetOfflinePingResponse(NULL, 0);

			if(players > 1)
			{
				// roz��cz graczy
				LOG("ServerPanel: Disconnecting clients.");
				const byte b[] = {ID_SERVER_CLOSE, 0};
				peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				net_mode = Game::NM_QUITTING_SERVER;
				--players;
				net_timer = T_WAIT_FOR_DISCONNECT;
				info_box->Show(server_panel->txDisconnecting);
				net_callback = f;
			}
			else
			{
				// nie ma graczy, mo�na zamkn��
				ClosePeer();
				if(f)
					f();
			}
		}
		else
		{
			info_box->Show(txDisconnecting);
			ForceRedraw();
			ClosePeer(true);
			f();
		}
	}
}

bool Game::ValidateNick(cstring nick)
{
	assert(nick);

	int len = strlen(nick);
	if(len == 0)
		return false;

	for(int i=0; i<len; ++i)
	{
		if(!(isalnum(nick[i]) || nick[i] == '_'))
			return false;
	}

	if(strcmp(nick, "all") == 0)
		return false;

	return true;
}

void Game::UpdateLobbyNet(float dt)
{
	Packet* packet;

	if(sv_server)
	{
		if(!sv_startup && autostart_count != -1 && autostart_count <= players)
		{
			bool ok = true;
			for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
			{
				if(!it->ready)
				{
					ok = false;
					break;
				}
			}

			if(ok)
			{
				// automatyczne uruchamianie gry, rozpocznij odliczanie
				LOG("UpdateLobbyNet: Automatic server startup.");
				autostart_count = -1;
				sv_startup = true;
				startup_timer = float(STARTUP_TIMER);
				last_startup_id = STARTUP_TIMER;
				byte b[] = {ID_STARTUP, STARTUP_TIMER};
				peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
				server_panel->bts[5].text = server_panel->txStop;
				server_panel->AddMsg(Format(server_panel->txStarting, STARTUP_TIMER));
				LOG(Format("UpdateLobbyNet: Starting in %d...", STARTUP_TIMER));
			}
		}

		for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			// pobierz informacje o graczu
			int index = FindPlayerIndex(packet->systemAddress);
			PlayerInfo* info = (index != -1 ? &game_players[index] : NULL);

			if(info && info->state == PlayerInfo::REMOVING)
			{
				if(packet->data[0] == ID_DISCONNECTION_NOTIFICATION || packet->data[0] == ID_CONNECTION_LOST)
				{
					// nie odes�a� informacji tylko si� roz��czy�
					LOG(!info->name.empty() ? Format("UpdateLobbyNet: Player %s has disconnected.", info->name.c_str()) :
						Format("UpdateLobbyNet: %s has disconnected.", packet->systemAddress.ToString()));
					--players;
					if(players > 1)
						AddLobbyUpdate(INT2(Lobby_ChangeCount,0));
					UpdateServerInfo();
					server_panel->grid.RemoveItem(index);
					game_players.erase(game_players.begin()+index);
				}
				else
				{
					// nieznany komunikat od roz��czanego gracz, ignorujemy go
					LOG(Format("UpdateLobbyNet: Ignoring packet from %s while disconnecting: %s.", !info->name.empty() ? info->name.c_str() : packet->systemAddress.ToString(),
						PacketToString(packet)));
				}

				continue;
			}

			switch(packet->data[0])
			{
			case ID_UNCONNECTED_PING:
			case ID_UNCONNECTED_PING_OPEN_CONNECTIONS:
				assert(!info);
				LOG(Format("UpdateLobbyNet: Ping from %s.", packet->systemAddress.ToString()));
				break;
			case ID_NEW_INCOMING_CONNECTION:
				if(info)
					WARN(Format("UpdateLobbyNet: Packet about connecting from already connected player %s.", info->name.c_str()));
				else
				{
					LOG(Format("UpdateLobbyNet: New connection from %s.", packet->systemAddress.ToString()));

					// ustal id dla nowego gracza
					do
					{
						++last_id;
						if(last_id == 256)
							last_id = 0;
						bool ok = true;
						for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
						{
							if(it->id == last_id)
							{
								ok = false;
								break;
							}
						}
						if(ok)
							break;
					}
					while(1);

					// dodaj
					PlayerInfo& info = Add1(game_players);
					info.adr = packet->systemAddress;
					info.id = last_id;
					info.loaded = false;
					server_panel->grid.AddItem();

					if(players == max_players)
					{
						// brak miejsca na serwerze, wy�lij komunikat i czekaj a� si� roz��czy
						byte b[] = {ID_CANT_JOIN, 0};
						peer->Send((cstring)b, 2, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
						info.state = PlayerInfo::REMOVING;
						info.timer = T_WAIT_FOR_DISCONNECT;
					}
					else
					{
						// czekaj a� wy�le komunikat o wersji, nick
						info.clas = Class::INVALID;
						info.ready = false;
						info.state = PlayerInfo::WAITING_FOR_HELLO;
						info.timer = T_WAIT_FOR_HELLO;
						info.update_flags = 0;
						info.cheats = CHEATS_START_MODE;
						info.left_reason = 0;
						info.left = false;
						info.warping = false;
						info.buffs = 0;
						if(players > 1)
							AddLobbyUpdate(INT2(Lobby_ChangeCount,0));
						++players;
						UpdateServerInfo();
					}
				}
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
				// klient si� roz��czy�
				{
					bool dis = (packet->data[0] == ID_CONNECTION_LOST);
					if(!info)
					{
						WARN(Format(dis ? "UpdateLobbyNet: Disconnect notification from unconnected address %s." :
							"UpdateLobbyNet: Lost connection with unconnected address %s.", packet->systemAddress.ToString()));
					}
					else
					{
						server_panel->grid.RemoveItem(index);
						if(info->state == PlayerInfo::WAITING_FOR_HELLO)
						{
							// roz��czy� si� przed przyj�ciem do lobby, mo�na go usun��
							server_panel->AddMsg(Format(dis ? txDisconnected : txLost, packet->systemAddress.ToString()));
							LOG(Format("UpdateLobbyNet: %s %s.", packet->systemAddress.ToString(), dis ? "disconnected" : "lost connection"));
							game_players.erase(game_players.begin()+index);
							--players;
							if(players > 1)
								AddLobbyUpdate(INT2(Lobby_ChangeCount,0));
							UpdateServerInfo();
						}
						else
						{
							// roz��czy� si� b�d�c w lobby
							--players;
							server_panel->AddMsg(Format(dis ? txLeft : txLost2, info->name.c_str()));
							LOG(Format("UpdateLobbyNet: Player %s %s.", info->name.c_str(), dis ? "disconnected" : "lost connection"));
							if(players > 1)
							{
								AddLobbyUpdate(INT2(Lobby_RemovePlayer,info->id));
								AddLobbyUpdate(INT2(Lobby_ChangeCount,0));
							}
							if(leader_id == info->id)
							{
								// serwer zostaje przyw�dc�
								LOG("UpdateLobbyNet: You are leader now.");
								leader_id = my_id;
								if(players > 1)
									AddLobbyUpdate(INT2(Lobby_ChangeLeader,0));
								server_panel->AddMsg(server_panel->txYouAreLeader);
							}
							game_players.erase(game_players.begin()+index);
							UpdateServerInfo();
							CheckReady();
						}
					}
				}
				break;
			case ID_HELLO:
				// informacje od gracza
				if(!info)
					WARN(Format("UpdateLobbyNet: Packet ID_HELLO from unconnected client %s.", packet->systemAddress.ToString()));
				else if(info->state != PlayerInfo::WAITING_FOR_HELLO)
					WARN(Format("UpdateLobbyNet: Packet ID_HELLO from player %s who already joined.", info->name.c_str()));
				else
				{
					BitStream s(packet->data+1, packet->length-1, false);
					int version, reason = -1;
					cstring reason_text;
					uint build;

					if(!s.Read(version) || !s.Read(build) || !ReadString1(s, info->name))
					{
						// b��d odczytu pakietu
						reason = 4;
						reason_text = Format("UpdateLobbyNet: Broken packet ID_HELLO from %s: %s.", packet->systemAddress.ToString(), PacketToString(packet));
					}
					else if(version != WERSJA || build != g_build)
					{
						// z�a wersja
						reason = 1;
						reason_text = Format("UpdateLobbbyNet: Invalid version from %s. Our (%s;%u) vs him (%s;%u).", packet->systemAddress.ToString(), VersionToString(version), build,
							WERSJA_STR, g_build);
					}
					else if(!ValidateNick(info->name.c_str()))
					{
						// z�y nick
						reason = 6;
						reason_text = Format("UpdateLobbyNet: Invalid nick (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
					}
					else
					{
						// sprawd� czy nick jest unikalny
						bool ok = true;
						for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
						{
							if(it->id != info->id && it->state == PlayerInfo::IN_LOBBY && it->name == info->name)
							{
								ok = false;
								break;
							}
						}
						if(!ok)
						{
							// nick jest zaj�ty
							reason = 2;
							reason_text = Format("UpdateLobbyNet: Nick already in use (%s) from %s.", info->name.c_str(), packet->systemAddress.ToString());
						}
					}

					if(reason != -1)
					{
						WARN(reason_text);
						byte b[] = {ID_CANT_JOIN, (byte)reason};
						peer->Send((cstring)b, 2, MEDIUM_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
						info->state = PlayerInfo::REMOVING;
						info->timer = T_WAIT_FOR_DISCONNECT;
					}
					else
					{
						// wszystko jest ok, niech gracz do��cza
						net_stream.Reset();
						net_stream.Write(ID_JOIN);
						net_stream.WriteCasted<byte>(info->id);
						net_stream.WriteCasted<byte>(players);
						net_stream.WriteCasted<byte>(leader_id);
						net_stream.WriteCasted<byte>(0);
						int ile = 0;
						for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end; ++it)
						{
							if(it->id == info->id || it->state != PlayerInfo::IN_LOBBY)
								continue;
							++ile;
							net_stream.WriteCasted<byte>(it->id);
							net_stream.WriteCasted<byte>(it->ready ? 1 : 0);
							net_stream.WriteCasted<byte>(it->clas);
							WriteString1(net_stream, it->name);
						}
						int off = net_stream.GetWriteOffset();
						net_stream.SetWriteOffset(4*8);
						net_stream.WriteCasted<byte>(ile);
						net_stream.SetWriteOffset(off);
						if(mp_load)
						{
							// informacja o postaci w zapisie
							PlayerInfo* old = FindOldPlayer(info->name.c_str());
							if(old)
							{
								net_stream.WriteCasted<byte>(2);
								net_stream.WriteCasted<byte>(old->clas);

								info->clas = old->clas;
								info->loaded = true;
								info->cheats = old->cheats;
								info->hd.CopyFrom(old->hd);
								info->notes = old->notes;

								if(players > 2)
									AddLobbyUpdate(INT2(Lobby_UpdatePlayer, info->id));

								LOG(Format("UpdateLobbyNet: Player %s is using loaded character.", info->name.c_str()));
							}
							else
								net_stream.WriteCasted<byte>(1);
						}
						else
							net_stream.WriteCasted<byte>(0);
						peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE, 0, packet->systemAddress, false);
						info->state = PlayerInfo::IN_LOBBY;

						server_panel->AddMsg(Format(server_panel->txJoined, info->name.c_str()));
						LOG(Format("UpdateLobbyNet: Player %s (%s) joined, id: %d.", info->name.c_str(), packet->systemAddress.ToString(), info->id));

						// informacja dla pozosta�ych
						if(players > 2)
							AddLobbyUpdate(INT2(Lobby_AddPlayer, info->id));

						CheckReady();
					}
				}
				break;
			case ID_LOBBY_CHANGE:
				if(!info)
					WARN(Format("UpdateLobbyNet: Packet ID_LOBBY_CHANGE from unconnected client %s.", packet->systemAddress.ToString()));
				else if(packet->length != 3)
					WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_CHANGE from client %s: %s.", info->name.c_str(), PacketToString(packet)));
				else
				{
					bool ready = (packet->data[1] == 1);
					Class clas = (Class)packet->data[2];

					if(ClassInfo::IsPickable(clas))
					{
						// nie ma innych mo�liwo�ci bo nie mo�e by� gotowy bez wybrania postaci
						info->clas = clas;
						info->ready = ready;
						if(players > 2)
							AddLobbyUpdate(INT2(Lobby_UpdatePlayer, info->id));
						CheckReady();
					}
					else
						WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_CHANGE, class %d from client %s: %s.", clas, info->name.c_str(), PacketToString(packet)));
				}
				break;
			case ID_SAY:
				if(!info)
					WARN(Format("UpdateLobbyNet: Packet ID_SAY from unconnected client %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
				else
					Server_Say(packet, *info);
				break;
			case ID_WHISPER:
				if(!info)
					WARN(Format("UpdateLobbyNet: Packet ID_WHISPER from unconnected client %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
				else
					Server_Whisper(packet, *info);
				break;
			case ID_LEAVE:
				if(!info)
					WARN(Format("UpdateLobbyNet: Packet ID_LEAVE from unconnected client %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
				else
				{
					// czekano na dane a on zquitowa� mimo braku takiej mo�liwo�ci :S
					cstring name = info->state == PlayerInfo::WAITING_FOR_HELLO ? info->adr.ToString() : info->name.c_str();
					server_panel->AddMsg(Format(server_panel->txPlayerLeft, name));
					LOG(Format("UpdateLobbyNet: Player %s left lobby.", name));
					--players;
					if(players > 1)
					{
						AddLobbyUpdate(INT2(Lobby_ChangeCount,0));
						if(info->state == PlayerInfo::IN_LOBBY)
							AddLobbyUpdate(INT2(Lobby_RemovePlayer,info->id));
					}
					server_panel->grid.RemoveItem(index);
					UpdateServerInfo();
					peer->CloseConnection(packet->systemAddress, true);
					game_players.erase(game_players.begin()+index);
					CheckReady();
				}
				break;
			case ID_PICK_CHARACTER:
				if(!info || info->state != PlayerInfo::IN_LOBBY)
					WARN(Format("UpdateLobbyNet: Packet ID_PICK_CHARACTER from unconnected client %s: %s.", packet->systemAddress.ToString(), PacketToString(packet)));
				else
				{
					BitStream stream(packet->data, packet->length, false);
					Class prev_clas = Class::INVALID;
					int result = ReadCharacterData(stream, info->clas, info->hd, info->cc);
					if(result != 0)
					{
						cstring msgs[3] = {
							"Broken packet.",
							"Invalid data.",
							"Validation failed."
						};
						ERROR(Format("UpdateLobbyNet: Packet ID_PICK_CHARACTER from %s: %s.", info->name.c_str(), msgs[result - 1]));
						info->clas = Class::INVALID;
					}
					
					// inform other players about class change
					if(prev_clas != info->clas && players > 1)
					{

					}
				}
				break;
			default:
				WARN(Format("UpdateLobbyNet: Unknown packet from %s: %s.", info ? info->name.c_str() : packet->systemAddress.ToString(), PacketToString(packet)));
				break;
			}
		}
	}
	else
	{
		// obs�uga komunikat�w otrzymywanych przez klienta
		for(packet=peer->Receive(); packet; peer->DeallocatePacket(packet), packet=peer->Receive())
		{
			switch(packet->data[0])
			{
			case ID_SAY:
				Client_Say(packet);
				break;
			case ID_WHISPER:
				Client_Whisper(packet);
				break;
			case ID_SERVER_SAY:
				Client_ServerSay(packet);
				break;
			case ID_LOBBY_UPDATE:
				{
					BitStream s(packet->data+1, packet->length-1, false);
					byte ile;

					if(!s.Read(ile))
					{
						WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE: %s.", PacketToString(packet)));
						break;
					}

					for(int i=0; i<ile; ++i)
					{
						byte co, id;
						if(!s.Read(co) || !s.Read(id))
						{
							WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE(2): %s.", PacketToString(packet)));
							break;
						}

						switch(co)
						{
						case Lobby_UpdatePlayer: // aktualizuj gracza
							{
								int index = GetPlayerIndex(id);
								if(index == -1)
								{
									WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE, invalid player id %d: %s.", id, PacketToString(packet)));
									goto blad;
								}
								PlayerInfo& info = game_players[index];
								if(!ReadBool(s, info.ready) || !s.ReadCasted<byte>(info.clas))
								{
									WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE(3): %s.", PacketToString(packet)));
									goto blad;
								}
								if(!ClassInfo::IsPickable(info.clas))
								{
									WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE(3), player %d have class %d: %s.", id, info.clas));
									goto blad;
								}
							}
							break;
						case Lobby_AddPlayer: // dodaj gracza
							if(!ReadString1(s))
							{
								WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE(4): %s.", PacketToString(packet)));
								goto blad;
							}
							else if(id != my_id)
							{
								PlayerInfo& info = Add1(game_players);
								info.ready = false;
								info.state = PlayerInfo::IN_LOBBY;
								info.clas = Class::INVALID;
								info.id = id;
								info.loaded = true;
								info.name = BUF;
								info.left = false;
								server_panel->grid.AddItem();

								server_panel->AddMsg(Format(server_panel->txJoined, BUF));
								LOG(Format("UpdateLobbyNet: Player %s joined lobby.", BUF));
							}
							break;
						case Lobby_RemovePlayer: // usu� gracza
						case Lobby_KickPlayer:
							{
								bool is_kick = (co == Lobby_KickPlayer);
								int index = GetPlayerIndex(id);
								if(index == -1)
								{
									WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE, invalid player id %d: %s.", id, PacketToString(packet)));
									goto blad;
								}
								PlayerInfo& info = game_players[index];
								LOG(Format("UpdateLobbyNet: Player %s %s.", info.name.c_str(), is_kick ? "was kicked" : "left lobby"));
								server_panel->AddMsg(Format(is_kick ? txPlayerKicked : txPlayerLeft, info.name.c_str()));
								server_panel->grid.RemoveItem(index);
								game_players.erase(game_players.begin()+index);
							}
							break;
						case Lobby_ChangeCount: // zmie� liczb� graczy
							players = id;
							break;
						case Lobby_ChangeLeader: // zmiana przyw�dcy
							{
								int index = GetPlayerIndex(id);
								if(index == -1)
								{
									WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE, invalid player id %d: %s", id, PacketToString(packet)));
									goto blad;
								}
								PlayerInfo& info = game_players[index];
								LOG(Format("%s is now leader.", info.name.c_str()));
								leader_id = id;
								if(my_id == id)
									server_panel->AddMsg(server_panel->txYouAreLeader);
								else
									server_panel->AddMsg(Format(server_panel->txLeaderChanged, info.name.c_str()));
							}
							break;
						default:
							WARN(Format("UpdateLobbyNet: Broken packet ID_LOBBY_UPDATE, unknown change type %d: %s.", co, PacketToString(packet)));
							goto blad;
						}
					}
blad:
					;
				}
				break;
			case ID_DISCONNECTION_NOTIFICATION:
			case ID_CONNECTION_LOST:
			case ID_SERVER_CLOSE:
				{
					cstring reason, reason_eng;
					if(packet->data[0] == ID_DISCONNECTION_NOTIFICATION)
					{
						reason = txDisconnected;
						reason_eng = "disconnected";
					}
					else if(packet->data[0] == ID_CONNECTION_LOST)
					{
						reason = txLostConnection;
						reason_eng = "lost connection";
					}
					else if(packet->length == 2)
					{
						switch(packet->data[1])
						{
						case 0:
							reason = txClosing;
							reason_eng = "closing";
							break;
						case 1:
							reason = txKicked;
							reason_eng = "kicked";
							break;
						default:
							reason = Format(txUnknown, packet->data[1]);
							reason_eng = Format("unknown (%d)", packet->data[1]);
							break;
						}
					}
					else
					{
						reason = txUnknown2;
						reason_eng = "unknown";
					}

					LOG(Format("UpdateLobbyNet: Disconnected from server: %s.", reason_eng));
					GUI.SimpleDialog(Format(txUnconnected, reason), NULL);

					server_panel->CloseDialog();
					peer->DeallocatePacket(packet);
					ClosePeer(true);
					return;
				}
			case ID_STARTUP:
				if(packet->length != 2)
					WARN(Format("UpdateLobbyNet: Broken packet ID_STARTUP: %s.", PacketToString(packet)));
				else
				{
					LOG(Format("UpdateLobbyNet: Starting in %d...", packet->data[1]));
					server_panel->AddMsg(Format(server_panel->txStartingIn, packet->data[1]));

					if(packet->data[1] == 0)
					{
						// close lobby and wait for server
						LOG("UpdateLobbyNet: Send character data.");
						LoadingStart(9);
						main_menu->visible = false;
						server_panel->CloseDialog();
						info_box->Show(txWaitingForServer);
						net_mode = NM_TRANSFER;
						net_state = 0;						
					}
				}
				break;
			case ID_END_STARTUP:
				LOG("Startup canceled.");
				AddMsg(server_panel->txStartingStop);
				break;
			default:
				WARN(Format("UpdateLobbyNet: Unknown packet: %s.", PacketToString(packet)));
				break;
			}
		}
	}

	if(sv_server)
	{
		int index = 0;
		for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end;)
		{
			if(it->state != PlayerInfo::IN_LOBBY)
			{
				it->timer -= dt;
				if(it->timer <= 0.f)
				{
					// czas oczekiwania min��, zkickuj
					LOG(Format("UpdateLobbyNet: Removed %s due to inactivity.", it->adr.ToString()));
					peer->CloseConnection(it->adr, false);
					--players;
					if(players > 1)
						AddLobbyUpdate(INT2(Lobby_RemovePlayer,0));
					it = game_players.erase(it);
					end = game_players.end();
					server_panel->grid.RemoveItem(index);
				}
				else
				{
					++index;
					++it;
				}
			}
			else
			{
				++it;
				++index;
			}
		}

		// wysy�anie aktualizacji lobby
		update_timer += dt;
		if(update_timer >= T_LOBBY_UPDATE)
		{
			update_timer = 0.f;
			if(!lobby_updates.empty())
			{
				assert(lobby_updates.size() < 255);
				if(players > 1)
				{
					// aktualizacje w lobby
					net_stream.Reset();
					net_stream.Write(ID_LOBBY_UPDATE);
					net_stream.WriteCasted<byte>(0);
					int ile = 0;
					for(uint i=0; i<min(lobby_updates.size(),255u); ++i)
					{
						INT2& u = lobby_updates[i];
						switch(u.x)
						{
						case Lobby_UpdatePlayer:
							{
								int index = GetPlayerIndex(u.y);
								if(index != -1)
								{
									PlayerInfo& info = game_players[index];
									++ile;
									net_stream.WriteCasted<byte>(u.x);
									net_stream.WriteCasted<byte>(info.id);
									net_stream.WriteCasted<byte>(info.ready ? 1 : 0);
									net_stream.WriteCasted<byte>(info.clas);
								}
							}
							break;
						case Lobby_AddPlayer:
							{
								int index = GetPlayerIndex(u.y);
								if(index != -1)
								{
									PlayerInfo& info = game_players[index];
									++ile;
									net_stream.WriteCasted<byte>(u.x);
									net_stream.WriteCasted<byte>(info.id);
									WriteString1(net_stream, info.name);
								}
							}
							break;
						case Lobby_RemovePlayer:
						case Lobby_KickPlayer:
							++ile;
							net_stream.WriteCasted<byte>(u.x);
							net_stream.WriteCasted<byte>(u.y);
							break;
						case Lobby_ChangeCount:
							++ile;
							net_stream.WriteCasted<byte>(u.x);
							net_stream.WriteCasted<byte>(players);
							break;
						case Lobby_ChangeLeader:
							++ile;
							net_stream.WriteCasted<byte>(u.x);
							net_stream.WriteCasted<byte>(leader_id);
							break;
						default:
							assert(0);
							break;
						}
					}
					int off = net_stream.GetWriteOffset();
					net_stream.SetWriteOffset(8);
					net_stream.WriteCasted<byte>(ile);
					net_stream.SetWriteOffset(off);
					peer->Send(&net_stream, HIGH_PRIORITY, RELIABLE_ORDERED, 2, UNASSIGNED_SYSTEM_ADDRESS, true);
				}
				lobby_updates.clear();
			}
		}

		// uruchamianie gry
		if(sv_startup)
		{
			startup_timer -= dt;
			int co = int(startup_timer)+1;
			int d = -1;
			if(startup_timer <= 0.f)
			{
				// change server status
				LOG("UpdateLobbyNet: Starting game.");
				sv_startup = false;
				d = 0;
				peer->SetMaximumIncomingConnections(0);
				net_mode = NM_TRANSFER_SERVER;
				net_timer = mp_timeout;
				net_state = 0;
				// kick players that connected but didn't join
				for(vector<PlayerInfo>::iterator it = game_players.begin(), end = game_players.end(); it != end;)
				{
					if(it->state != PlayerInfo::IN_LOBBY)
					{
						peer->CloseConnection(it->adr, true);
						WARN(Format("UpdateLobbyNet: Disconnecting %s.", it->adr.ToString()));
						it = game_players.erase(it);
						end = game_players.end();
						--players;
					}
				}
				server_panel->CloseDialog();
				info_box->Show(txStartingGame);
			}
			else if(co != last_startup_id)
			{
				last_startup_id = co;
				d = co;
			}
			if(d != -1)
			{
				LOG(Format("UpdateLobbyNet: Starting in %d...", d));
				server_panel->AddMsg(Format(server_panel->txStartingIn, d));
				byte b[] = {ID_STARTUP, (byte)d};
				peer->Send((cstring)b, 2, IMMEDIATE_PRIORITY, RELIABLE, 0, UNASSIGNED_SYSTEM_ADDRESS, true);
			}
		}
	}
}

void Game::ShowQuitDialog()
{
	DialogInfo di;
	di.text = txRealyQuit;
	di.event.bind(this, &Game::OnQuit);
	di.type = DIALOG_YESNO;
	di.name = "dialog_alt_f4";
	di.parent = NULL;
	di.order = ORDER_TOPMOST;
	di.pause = true;

	GUI.ShowDialog(di);
}

void Game::OnCreateCharacter(int id)
{
	if(id != BUTTON_OK)
	{
		if(IsOnline())
		{
			if(server_panel->had_char)
			{
				server_panel->had_char = false;
				server_panel->have_char = true;
				create_character->mode = CreateCharacterPanel::Mode::PickAppearance;
			}
		}
		return;
	}

	if(IsOnline())
	{
		PlayerInfo& info = game_players[0];
		server_panel->have_char = true;
		server_panel->bts[1].state = Button::NONE;
		server_panel->bts[0].text = server_panel->txChangeChar;
		// copy stats
		Class old_class = info.clas;
		info.clas = create_character->clas;
		info.hd.Get(*create_character->unit->human_data);
		info.cc = create_character->c;
		// send changes
		OnPickCharacter(old_class, info.ready);
	}
	else
	{
		if(!skip_tutorial)
		{
			DialogInfo info;
			info.event = DialogEvent(this, &Game::OnPlayTutorial);
			info.have_tick = true;
			info.name = "tutorial_dialog";
			info.order = ORDER_TOP;
			info.parent = NULL;
			info.pause = false;
			info.text = txTutPlay;
			info.tick_text = txTutTick;
			info.type = DIALOG_YESNO;

			GUI.ShowDialog(info);
		}
		else
			StartNewGame();
	}
}

void Game::OnPlayTutorial(int id)
{
	if(id == BUTTON_CHECKED)
		skip_tutorial = true;
	else if(id == BUTTON_UNCHECKED)
		skip_tutorial = false;
	else
	{
		GUI.GetDialog("tutorial_dialog")->visible = false;
		SaveOptions();
		if(id == BUTTON_YES)
			StartTutorial();
		else
			StartNewGame();
	}
}

void Game::OnPickServer(int id)
{
	if(id == BUTTON_CANCEL)
	{
		ClosePeer();
		peer->Shutdown(0);
		pick_server_panel->CloseDialog();
	}
	else
	{
		PickServerPanel::ServerData& info = pick_server_panel->servers[pick_server_panel->grid.selected];
		server = info.adr;
		server_name2 = info.name;
		max_players2 = info.max_players;
		players = info.players;
		if(IS_SET(info.flags, SERVER_PASSWORD))
		{
			// podaj has�o
			net_mode = NM_CONNECT_IP;
			net_state = 1;
			net_tries = 1;
			info_box->Show(txWaitingForPswd);
			GetTextDialogParams params(Format(txEnterPswd, server_name2.c_str()), enter_pswd);
			params.event = DialogEvent(this, &Game::OnEnterPassword);
			params.limit = 16;
			params.parent = info_box;
			GetTextDialog::Show(params);
			LOG("OnPickServer: Waiting for password...");
			pick_server_panel->CloseDialog();
		}
		else
		{
			// po��cz
			ConnectionAttemptResult result = peer->Connect(info.adr.ToString(false), (word)mp_port, NULL, 0);
			if(result == CONNECTION_ATTEMPT_STARTED)
			{
				// trwa ��czenie z serwerem
				net_mode = NM_CONNECT_IP;
				net_timer = T_CONNECT;
				net_tries = 1;
				net_state = 2;
				info_box->Show(Format(txConnectingTo, info.name.c_str()));
				LOG(Format("OnPickServer: Connecting to server %s...", info.name.c_str()));
				pick_server_panel->CloseDialog();
			}
			else
			{
				// b��d ��czenie z serwerem
				ERROR(Format("NM_CONNECT_IP(0): Can't connect to server: raknet error %d.", result));
				EndConnecting(txConnectRaknet);
			}
		}
	}
}

void Game::DeleteOldPlayers()
{
	const bool in_level = (open_location != -1);
	for(vector<PlayerInfo>::iterator it = old_players.begin(), end = old_players.end(); it != end; ++it)
	{
		if(!it->loaded && it->u)
		{
			if(in_level)
				RemoveElement(GetContext(*it->u).units, it->u);
			if(it->u->cobj)
			{
				delete it->u->cobj->getCollisionShape();
				phy_world->removeCollisionObject(it->u->cobj);
				delete it->u->cobj;
			}
			delete it->u;
		}
	}
	old_players.clear();
}
