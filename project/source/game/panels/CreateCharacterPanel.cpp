// character creation screen
#include "Pch.h"
#include "GameCore.h"
#include "CreateCharacterPanel.h"
#include "Game.h"
#include "Language.h"
#include "GetTextDialog.h"
#include "Language.h"
#include "PickItemDialog.h"
#include "ResourceManager.h"
#include "DirectX.h"
#include "Unit.h"

//-----------------------------------------------------------------------------
const int SECTION_H = 40;
const int VALUE_H = 20;

//-----------------------------------------------------------------------------
enum ButtonId
{
	IdCancel = GuiEvent_Custom,
	IdNext,
	IdBack,
	IdCreate,
	IdHardcore,
	IdHair,
	IdMustache,
	IdBeard,
	IdColor,
	IdSize,
	IdRandomSet
};

//=================================================================================================
CreateCharacterPanel::CreateCharacterPanel(DialogInfo& info) : GameDialogBox(info), unit(nullptr)
{
	size = Int2(600, 500);
	unit = new Unit;
	unit->human_data = new Human;
	unit->player = nullptr;
	unit->ai = nullptr;
	unit->hero = nullptr;
	unit->used_item = nullptr;
	unit->weapon_state = WS_HIDDEN;
	unit->pos = unit->visual_pos = Vec3(0, 0, 0);
	unit->rot = 0.f;
	unit->fake_unit = true;
	unit->action = A_NONE;

	btCancel.id = IdCancel;
	btCancel.custom = &custom_x;
	btCancel.size = Int2(32, 32);
	btCancel.parent = this;
	btCancel.pos = Int2(size.x - 32 - 16, 16);

	btBack.id = IdBack;
	btBack.size = Int2(100, 44);
	btBack.parent = this;
	btBack.pos = Int2(16, size.y - 60);

	btNext.id = IdNext;
	btNext.size = Int2(100, 44);
	btNext.parent = this;
	btNext.pos = Int2(size.x - 100 - 16, size.y - 60);

	btCreate.id = IdCreate;
	btCreate.size = Int2(100, 44);
	btCreate.parent = this;
	btCreate.pos = Int2(size.x - 100 - 16, size.y - 60);

	btRandomSet.id = IdRandomSet;
	btRandomSet.size = Int2(100, 44);
	btRandomSet.parent = this;
	btRandomSet.pos = Int2(size.x / 2 - 50, size.y - 60);

	checkbox.bt_size = Int2(32, 32);
	checkbox.checked = false;
	checkbox.id = IdHardcore;
	checkbox.parent = this;
	checkbox.pos = Int2(20, 350);
	checkbox.size = Int2(200, 32);

	{
		Slider& s = slider[0];
		s.id = IdHair;
		s.minv = 0;
		s.maxv = MAX_HAIR - 1;
		s.val = 0;
		s.pos = Int2(20, 100);
		s.parent = this;
	}

	{
		Slider& s = slider[1];
		s.id = IdMustache;
		s.minv = 0;
		s.maxv = MAX_MUSTACHE - 1;
		s.val = 0;
		s.pos = Int2(20, 150);
		s.parent = this;
	}

	{
		Slider& s = slider[2];
		s.id = IdBeard;
		s.minv = 0;
		s.maxv = MAX_BEARD - 1;
		s.val = 0;
		s.pos = Int2(20, 200);
		s.parent = this;
	}

	{
		Slider& s = slider[3];
		s.id = IdColor;
		s.minv = 0;
		s.maxv = n_hair_colors - 1;
		s.val = 0;
		s.pos = Int2(20, 250);
		s.parent = this;
	}

	{
		Slider& s = slider[4];
		s.id = IdSize;
		s.minv = 0;
		s.maxv = 100;
		s.val = 50;
		s.pos = Int2(20, 300);
		s.parent = this;
		s.SetHold(true);
		s.hold_val = 25.f;
	}

	lbClasses.pos = Int2(16, 73 - 18);
	lbClasses.size = Int2(198, 235 + 18);
	lbClasses.SetForceImageSize(Int2(20, 20));
	lbClasses.SetItemHeight(24);
	lbClasses.event_handler = DialogEvent(this, &CreateCharacterPanel::OnChangeClass);
	lbClasses.parent = this;

	tbClassDesc.pos = Int2(130, 335);
	tbClassDesc.size = Int2(341, 93);
	tbClassDesc.SetReadonly(true);
	tbClassDesc.AddScrollbar();

	tbInfo.pos = Int2(130, 335);
	tbInfo.size = Int2(341, 93);
	tbInfo.SetReadonly(true);
	tbInfo.AddScrollbar();

	flow_pos = Int2(368, 73 - 18);
	flow_size = Int2(198, 235 + 18);
	flow_scroll.pos = Int2(flow_pos.x + flow_size.x + 2, flow_pos.y);
	flow_scroll.size = Int2(16, flow_size.y);
	flow_scroll.total = 100;
	flow_scroll.part = 10;

	tooltip.Init(TooltipGetText(this, &CreateCharacterPanel::GetTooltip));

	flowSkills.size = Int2(198, 235 + 18);
	flowSkills.pos = Int2(16, 73 - 18);
	flowSkills.button_size = Int2(16, 16);
	flowSkills.button_tex = custom_bt;
	flowSkills.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickSkill);

	flowPerks.size = Int2(198, 235 + 18);
	flowPerks.pos = Int2(size.x - flowPerks.size.x - 16, 73 - 18);
	flowPerks.button_size = Int2(16, 16);
	flowPerks.button_tex = custom_bt;
	flowPerks.on_button = ButtonEvent(this, &CreateCharacterPanel::OnPickPerk);
}

//=================================================================================================
CreateCharacterPanel::~CreateCharacterPanel()
{
	if(unit->bow_instance)
	{
		game->bow_instances.push_back(unit->bow_instance);
		unit->bow_instance = nullptr;
	}
	delete unit;
}

//=================================================================================================
void CreateCharacterPanel::LoadLanguage()
{
	txHardcoreMode = Str("hardcoreMode");
	txHair = Str("hair");
	txMustache = Str("mustache");
	txBeard = Str("beard");
	txHairColor = Str("hairColor");
	txSize = Str("size");
	txCharacterCreation = Str("characterCreation");
	txName = Str("name");
	txAttributes = Str("attributes");
	txRelatedAttributes = Str("relatedAttributes");
	txCreateCharWarn = Str("createCharWarn");
	txSkillPoints = Str("skillPoints");
	txPerkPoints = Str("perkPoints");
	txPickAttribIncrease = Str("pickAttribIncrease");
	txPickAttribDecrease = Str("pickAttribDecrease");
	txPickTwoSkillsDecrease = Str("pickTwoSkillsDecrease");
	txPickSkillIncrease = Str("pickSkillIncrease");
	txAvailablePerks = Str("availablePerks");
	txUnavailablePerks = Str("unavailablePerks");
	txTakenPerks = Str("takenPerks");
	txCreateCharTooMany = Str("createCharTooMany");
	txFlawExtraPerk = Str("flawExtraPerk");

	btBack.text = Str("goBack");
	btNext.text = Str("next");
	btCreate.text = Str("create");
	btRandomSet.text = Str("randomSet");

	checkbox.text = txHardcoreMode;

	slider[0].text = txHair;
	slider[1].text = txMustache;
	slider[2].text = txBeard;
	slider[3].text = txHairColor;
	slider[4].text = txSize;

	tbInfo.SetText(Str("createCharText"));
}

//=================================================================================================
void CreateCharacterPanel::LoadData()
{
	auto& tex_mgr = ResourceManager::Get<Texture>();
	tex_mgr.AddLoadTask("klasa_cecha.png", tKlasaCecha);
	tex_mgr.AddLoadTask("close.png", custom_x.tex[Button::NONE]);
	tex_mgr.AddLoadTask("close_hover.png", custom_x.tex[Button::HOVER]);
	tex_mgr.AddLoadTask("close_down.png", custom_x.tex[Button::DOWN]);
	tex_mgr.AddLoadTask("close_disabled.png", custom_x.tex[Button::DISABLED]);
	tex_mgr.AddLoadTask("plus.png", custom_bt[0].tex[Button::NONE]);
	tex_mgr.AddLoadTask("plus_hover.png", custom_bt[0].tex[Button::HOVER]);
	tex_mgr.AddLoadTask("plus_down.png", custom_bt[0].tex[Button::DOWN]);
	tex_mgr.AddLoadTask("plus_disabled.png", custom_bt[0].tex[Button::DISABLED]);
	tex_mgr.AddLoadTask("minus.png", custom_bt[1].tex[Button::NONE]);
	tex_mgr.AddLoadTask("minus_hover.png", custom_bt[1].tex[Button::HOVER]);
	tex_mgr.AddLoadTask("minus_down.png", custom_bt[1].tex[Button::DOWN]);
	tex_mgr.AddLoadTask("minus_disabled.png", custom_bt[1].tex[Button::DISABLED]);
}

//=================================================================================================
void CreateCharacterPanel::Draw(ControlDrawData*)
{
	// background
	GUI.DrawSpriteFull(tBackground, Color::Alpha(128));

	// panel
	GUI.DrawItem(tDialog, global_pos, size, Color::Alpha(222), 16);

	// top text
	Rect rect0 = { 12 + pos.x, 12 + pos.y, pos.x + size.x - 12, 12 + pos.y + 72 };
	GUI.DrawText(GUI.fBig, txCharacterCreation, DTF_CENTER, Color::Black, rect0);

	// character
	GUI.DrawSprite(game->tChar, Int2(pos.x + 228, pos.y + 64));

	// close button
	btCancel.Draw();

	Rect rect;
	Matrix mat;

	switch(mode)
	{
	case Mode::PickClass:
		{
			btNext.Draw();

			// class list
			lbClasses.Draw();

			// class desc
			tbClassDesc.Draw();

			// attribute/skill flow panel
			Int2 fpos = flow_pos + global_pos;
			GUI.DrawItem(GUI.tBox, fpos, flow_size, Color::White, 8, 32);
			flow_scroll.Draw();

			rect = Rect::Create(fpos + Int2(2, 2), flow_size - Int2(4, 4));
			Rect r = rect, part = { 0, 0, 256, 32 };
			r.Top() += 20;

			for(OldFlowItem& fi : flow_items)
			{
				r.Top() = fpos.y + fi.y - (int)flow_scroll.offset;
				cstring item_text = GetText(fi.group, fi.id);
				if(fi.section)
				{
					r.Bottom() = r.Top() + SECTION_H;
					if(!GUI.DrawText(GUI.fBig, item_text, DTF_SINGLELINE, Color::Black, r, &rect))
						break;
				}
				else
				{
					if(fi.part > 0)
					{
						mat = Matrix::Transform2D(nullptr, 0.f, &Vec2(float(flow_size.x - 4) / 256, 17.f / 32), nullptr, 0.f, &Vec2(r.LeftTop()));
						part.Right() = int(fi.part * 256);
						GUI.DrawSprite2(tKlasaCecha, mat, &part, &rect, Color::White);
					}
					r.Bottom() = r.Top() + VALUE_H;
					if(!GUI.DrawText(GUI.default_font, item_text, DTF_SINGLELINE, Color::Black, r, &rect))
						break;
				}
			}

			tooltip.Draw();
		}
		break;
	case Mode::PickSkillPerk:
		{
			btNext.Draw();
			btBack.Draw();
			btRandomSet.Draw();

			flowSkills.Draw();
			flowPerks.Draw();

			tbInfo.Draw();

			// left text "SkillId points: X/Y"
			Rect r = { global_pos.x + 16, global_pos.y + 310, global_pos.x + 216, global_pos.y + 360 };
			GUI.DrawText(GUI.default_font, Format(txSkillPoints, cc.sp, cc.sp_max), 0, Color::Black, r);

			// right text "Perks: X/Y"
			Rect r2 = { global_pos.x + size.x - 216, global_pos.y + 310, global_pos.x + size.x - 16, global_pos.y + 360 };
			GUI.DrawText(GUI.default_font, Format(txPerkPoints, cc.perks, cc.perks_max), DTF_RIGHT, Color::Black, r2);

			tooltip.Draw();
		}
		break;
	case Mode::PickAppearance:
		{
			btCreate.Draw();
			btRandomSet.Draw();
			btBack.Draw();

			if(!Net::IsOnline())
				checkbox.Draw();

			for(int i = 0; i < 5; ++i)
				slider[i].Draw();
		}
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::Update(float dt)
{
	RenderUnit();
	UpdateUnit(dt);

	// rotating unit
	if(PointInRect(GUI.cursor_pos, Int2(pos.x + 228, pos.y + 94), Int2(128, 256)) && Key.Focus() && focus)
	{
		bool rotate = false;
		if(rotating)
		{
			if(Key.Down(VK_LBUTTON))
				rotate = true;
			else
				rotating = false;
		}
		else if(Key.Pressed(VK_LBUTTON))
		{
			rotate = true;
			rotating = true;
		}

		if(rotate)
			unit->rot = Clip(unit->rot - float(GUI.cursor_pos.x - pos.x - 228 - 64) / 16 * dt);
	}
	else
		rotating = false;

	// x button
	btCancel.mouse_focus = focus;
	btCancel.Update(dt);

	switch(mode)
	{
	case Mode::PickClass:
		{
			lbClasses.mouse_focus = focus;
			lbClasses.Update(dt);

			btNext.mouse_focus = focus;
			btNext.Update(dt);

			tbClassDesc.mouse_focus = focus;
			tbClassDesc.Update(dt);

			flow_scroll.mouse_focus = focus;
			flow_scroll.Update(dt);

			int group = -1, id = -1;

			if(!flow_scroll.clicked && PointInRect(GUI.cursor_pos, flow_pos + global_pos, flow_size))
			{
				if(focus && Key.Focus())
					flow_scroll.ApplyMouseWheel();

				int y = GUI.cursor_pos.y - global_pos.y - flow_pos.y + (int)flow_scroll.offset;
				for(OldFlowItem& fi : flow_items)
				{
					if(y >= fi.y && y <= fi.y + 20)
					{
						group = fi.group;
						id = fi.id;
						break;
					}
					else if(y < fi.y)
						break;
				}
			}
			else if(!flow_scroll.clicked && PointInRect(GUI.cursor_pos, flow_scroll.pos + global_pos, flow_scroll.size) && focus && Key.Focus())
				flow_scroll.ApplyMouseWheel();

			tooltip.UpdateTooltip(dt, group, id);
		}
		break;
	case Mode::PickSkillPerk:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btNext.mouse_focus = focus;
			btNext.Update(dt);

			btRandomSet.mouse_focus = focus;
			btRandomSet.Update(dt);

			tbInfo.mouse_focus = focus;
			tbInfo.Update(dt);

			int group = -1, id = -1;

			flowSkills.mouse_focus = focus;
			flowSkills.Update(dt);
			flowSkills.GetSelected(group, id);

			flowPerks.mouse_focus = focus;
			flowPerks.Update(dt);
			flowPerks.GetSelected(group, id);

			tooltip.UpdateTooltip(dt, group, id);
		}
		break;
	case Mode::PickAppearance:
		{
			btBack.mouse_focus = focus;
			btBack.Update(dt);

			btCreate.mouse_focus = focus;
			btCreate.Update(dt);

			btRandomSet.mouse_focus = focus;
			btRandomSet.Update(dt);

			if(!Net::IsOnline())
			{
				checkbox.mouse_focus = focus;
				checkbox.Update(dt);
			}

			for(int i = 0; i < 5; ++i)
			{
				slider[i].mouse_focus = focus;
				slider[i].Update(dt);
			}
		}
		break;
	}

	if(focus && Key.Focus() && Key.PressedRelease(VK_ESCAPE))
		Event((GuiEvent)IdCancel);
}

//=================================================================================================
void CreateCharacterPanel::Event(GuiEvent e)
{
	if(e == GuiEvent_Show || e == GuiEvent_WindowResize)
	{
		if(e == GuiEvent_Show)
		{
			visible = true;
			unit->rot = 0;
			dist = -2.5f;
		}
		pos = global_pos = (GUI.wnd_size - size) / 2;
		btCancel.global_pos = global_pos + btCancel.pos;
		btBack.global_pos = global_pos + btBack.pos;
		btNext.global_pos = global_pos + btNext.pos;
		btCreate.global_pos = global_pos + btCreate.pos;
		btRandomSet.global_pos = global_pos + btRandomSet.pos;
		for(int i = 0; i < 5; ++i)
			slider[i].global_pos = global_pos + slider[i].pos;
		checkbox.global_pos = global_pos + checkbox.pos;
		lbClasses.Event(GuiEvent_Moved);
		tbClassDesc.Move(pos);
		tbInfo.Move(pos);
		flow_scroll.global_pos = global_pos + flow_scroll.pos;
		flowSkills.UpdatePos(global_pos);
		flowPerks.UpdatePos(global_pos);
		tooltip.Clear();
	}
	else if(e == GuiEvent_Close)
	{
		visible = false;
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e == GuiEvent_LostFocus)
	{
		lbClasses.Event(GuiEvent_LostFocus);
		tbClassDesc.LostFocus();
		flow_scroll.LostFocus();
	}
	else if(e >= GuiEvent_Custom)
	{
		switch(e)
		{
		case IdCancel:
			CloseDialog();
			event(BUTTON_CANCEL);
			break;
		case IdNext:
			if(mode == Mode::PickClass)
			{
				mode = Mode::PickSkillPerk;
				if(reset_skills_perks)
					ResetSkillsPerks();
			}
			else
			{
				if(cc.sp < 0 || cc.perks < 0)
					GUI.SimpleDialog(txCreateCharTooMany, this);
				else if(cc.sp != 0 || cc.perks != 0)
				{
					DialogInfo di;
					di.event = [this](int id)
					{
						if(id == BUTTON_YES)
							mode = Mode::PickAppearance;
					};
					di.name = "create_char_warn";
					di.order = ORDER_TOP;
					di.parent = this;
					di.pause = false;
					di.text = txCreateCharWarn;
					di.type = DIALOG_YESNO;
					GUI.ShowDialog(di);
				}
				else
					mode = Mode::PickAppearance;
			}
			break;
		case IdBack:
			if(mode == Mode::PickSkillPerk)
				mode = Mode::PickClass;
			else
				mode = Mode::PickSkillPerk;
			break;
		case IdCreate:
			if(enter_name)
			{
				GetTextDialogParams params(Format("%s:", txName), player_name);
				params.event = [this](int id)
				{
					if(id == BUTTON_OK)
					{
						last_hair_color_index = hair_color_index;
						CloseDialog();
						event(BUTTON_OK);
					}
				};
				params.limit = 16;
				params.parent = this;
				GetTextDialog::Show(params);
			}
			else
			{
				last_hair_color_index = hair_color_index;
				CloseDialog();
				event(BUTTON_OK);
			}
			break;
		case IdHardcore:
			game->hardcore_option = checkbox.checked;
			game->cfg.Add("hardcore", game->hardcore_option);
			game->SaveCfg();
			break;
		case IdHair:
			unit->human_data->hair = slider[0].val - 1;
			slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
			break;
		case IdMustache:
			unit->human_data->mustache = slider[1].val - 1;
			slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
			break;
		case IdBeard:
			unit->human_data->beard = slider[2].val - 1;
			slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
			break;
		case IdColor:
			unit->human_data->hair_color = g_hair_colors[slider[3].val];
			slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
			break;
		case IdSize:
			unit->human_data->height = Lerp(0.9f, 1.1f, float(slider[4].val) / 100);
			slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
			unit->human_data->ApplyScale(unit->mesh_inst->mesh);
			unit->mesh_inst->need_update = true;
			break;
		case IdRandomSet:
			if(mode == Mode::PickAppearance)
				RandomAppearance();
			else
			{
				cc.Random(clas);
				RebuildSkillsFlow();
				RebuildPerksFlow();
				UpdateInventory();
			}
			break;
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::RenderUnit()
{
	// rysuj obrazek
	HRESULT hr = game->device->TestCooperativeLevel();
	if(hr != D3D_OK)
		return;

	game->SetAlphaBlend(false);
	game->SetAlphaTest(false);
	game->SetNoCulling(false);
	game->SetNoZWrite(false);

	// ustaw render target
	SURFACE surf = nullptr;
	if(game->sChar)
		V(game->device->SetRenderTarget(0, game->sChar));
	else
	{
		V(game->tChar->GetSurfaceLevel(0, &surf));
		V(game->device->SetRenderTarget(0, surf));
	}

	// pocz�tek renderowania
	V(game->device->Clear(0, nullptr, D3DCLEAR_ZBUFFER | D3DCLEAR_TARGET, 0, 1.f, 0));
	V(game->device->BeginScene());

	static vector<Lights> lights;

	game->SetOutsideParams();

	Matrix matView, matProj;
	Vec3 from = Vec3(0.f, 2.f, dist);
	matView = Matrix::CreateLookAt(from, Vec3(0.f, 1.f, 0.f), Vec3(0, 1, 0));
	matProj = Matrix::CreatePerspectiveFieldOfView(PI / 4, 0.5f, 1.f, 5.f);
	game->cam.matViewProj = matView * matProj;
	game->cam.center = from;
	game->cam.matViewInv = matView.Inverse();

	game->cam.frustum.Set(game->cam.matViewProj);
	game->ListDrawObjectsUnit(nullptr, game->cam.frustum, true, *unit);
	game->DrawSceneNodes(game->draw_batch.nodes, lights, true);
	game->draw_batch.Clear();

	// koniec renderowania
	V(game->device->EndScene());

	// kopiuj je�li jest mipmaping
	if(game->sChar)
	{
		V(game->tChar->GetSurfaceLevel(0, &surf));
		V(game->device->StretchRect(game->sChar, nullptr, surf, nullptr, D3DTEXF_NONE));
	}
	surf->Release();

	// przywr�c poprzedni render target
	V(game->device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surf));
	V(game->device->SetRenderTarget(0, surf));
	surf->Release();
}

//=================================================================================================
void CreateCharacterPanel::UpdateUnit(float dt)
{
	// aktualizacja postaci
	t -= dt;
	if(t <= 0.f)
	{
		switch(anim)
		{
		case DA_ATAK:
		case DA_SCHOWAJ_BRON:
		case DA_SCHOWAJ_LUK:
		case DA_STRZAL:
		case DA_WYJMIJ_BRON:
		case DA_WYJMIJ_LUK:
		case DA_ROZGLADA:
			assert(0);
			break;
		case DA_BLOK:
			if(Rand() % 2 == 0)
				anim = DA_ATAK;
			else
				anim = DA_BOJOWY;
			break;
		case DA_BOJOWY:
			if(unit->weapon_taken == W_ONE_HANDED)
			{
				int co = Rand() % (unit->HaveShield() ? 3 : 2);
				if(co == 0)
					anim = DA_ATAK;
				else if(co == 1)
					anim = DA_SCHOWAJ_BRON;
				else
					anim = DA_BLOK;
			}
			else
			{
				if(Rand() % 2 == 0)
					anim = DA_STRZAL;
				else
					anim = DA_SCHOWAJ_LUK;
			}
			break;
		case DA_STOI:
		case DA_IDZIE:
			{
				int co = Rand() % (unit->HaveBow() ? 5 : 4);
				if(co == 0)
					anim = DA_ROZGLADA;
				else if(co == 1)
					anim = DA_STOI;
				else if(co == 2)
					anim = DA_IDZIE;
				else if(co == 3)
					anim = DA_WYJMIJ_BRON;
				else
					anim = DA_WYJMIJ_LUK;
			}
			break;
		default:
			assert(0);
			break;
		}
	}

	if(anim != anim2)
	{
		anim2 = anim;
		switch(anim)
		{
		case DA_ATAK:
			unit->attack_id = unit->GetRandomAttack();
			unit->mesh_inst->Play(NAMES::ani_attacks[unit->attack_id], PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[0].speed = unit->GetAttackSpeed();
			unit->animation_state = 0;
			t = 100.f;
			unit->mesh_inst->frame_end_info = false;
			break;
		case DA_BLOK:
			unit->mesh_inst->Play(NAMES::ani_block, PLAY_PRIO2, 0);
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 1.f;
			break;
		case DA_BOJOWY:
			unit->mesh_inst->Play(NAMES::ani_battle, PLAY_PRIO2, 0);
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 1.f;
			break;
		case DA_IDZIE:
			unit->mesh_inst->Play(NAMES::ani_move, PLAY_PRIO2, 0);
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 2.f;
			break;
		case DA_ROZGLADA:
			unit->mesh_inst->Play("rozglada", PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 100.f;
			unit->mesh_inst->frame_end_info = false;
			break;
		case DA_SCHOWAJ_BRON:
			unit->mesh_inst->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2 | PLAY_ONCE | PLAY_BACK, 0);
			unit->mesh_inst->groups[1].speed = 1.f;
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 100.f;
			unit->animation_state = 0;
			unit->weapon_state = WS_HIDING;
			unit->weapon_taken = W_NONE;
			unit->weapon_hiding = W_ONE_HANDED;
			unit->mesh_inst->frame_end_info = false;
			break;
		case DA_SCHOWAJ_LUK:
			unit->mesh_inst->Play(NAMES::ani_take_bow, PLAY_PRIO2 | PLAY_ONCE | PLAY_BACK, 0);
			unit->mesh_inst->groups[1].speed = 1.f;
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 100.f;
			unit->animation_state = 0;
			unit->weapon_state = WS_HIDING;
			unit->weapon_taken = W_NONE;
			unit->weapon_hiding = W_BOW;
			unit->mesh_inst->frame_end_info = false;
			break;
		case DA_STOI:
			unit->mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO2, 0);
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 2.f;
			break;
		case DA_STRZAL:
			unit->mesh_inst->Play(NAMES::ani_shoot, PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[0].speed = unit->GetBowAttackSpeed();
			unit->animation_state = 0;
			t = 100.f;
			unit->bow_instance = game->GetBowInstance(unit->GetBow().mesh);
			unit->bow_instance->Play(&unit->bow_instance->mesh->anims[0], PLAY_ONCE | PLAY_PRIO1 | PLAY_NO_BLEND, 0);
			unit->bow_instance->groups[0].speed = unit->mesh_inst->groups[0].speed;
			unit->mesh_inst->frame_end_info = false;
			unit->action = A_SHOOT;
			break;
		case DA_WYJMIJ_BRON:
			unit->mesh_inst->Play(unit->GetTakeWeaponAnimation(true), PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[1].speed = 1.f;
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 100.f;
			unit->animation_state = 0;
			unit->weapon_state = WS_TAKING;
			unit->weapon_taken = W_ONE_HANDED;
			unit->weapon_hiding = W_NONE;
			unit->mesh_inst->frame_end_info = false;
			break;
		case DA_WYJMIJ_LUK:
			unit->mesh_inst->Play(NAMES::ani_take_bow, PLAY_PRIO2 | PLAY_ONCE, 0);
			unit->mesh_inst->groups[1].speed = 1.f;
			unit->mesh_inst->groups[0].speed = 1.f;
			t = 100.f;
			unit->animation_state = 0;
			unit->weapon_state = WS_TAKING;
			unit->weapon_taken = W_BOW;
			unit->weapon_hiding = W_NONE;
			unit->mesh_inst->frame_end_info = false;
			break;
		default:
			assert(0);
			break;
		}
	}

	switch(anim)
	{
	case DA_ATAK:
		if(unit->mesh_inst->frame_end_info)
		{
			unit->mesh_inst->groups[0].speed = 1.f;
			if(Rand() % 2 == 0)
			{
				anim = DA_ATAK;
				anim2 = DA_STOI;
			}
			else
				anim = DA_BOJOWY;
		}
		break;
	case DA_SCHOWAJ_BRON:
	case DA_SCHOWAJ_LUK:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() <= unit->data->frames->t[F_TAKE_WEAPON] || unit->mesh_inst->frame_end_info))
			unit->animation_state = 1;
		if(unit->mesh_inst->frame_end_info)
		{
			unit->weapon_state = WS_HIDDEN;
			unit->weapon_hiding = W_NONE;
			anim = DA_STOI;
			t = 1.f;
		}
		break;
	case DA_STRZAL:
		if(unit->mesh_inst->GetProgress() > 20.f / 40 && unit->animation_state < 2)
			unit->animation_state = 2;
		else if(unit->mesh_inst->GetProgress() > 35.f / 40)
		{
			unit->animation_state = 3;
			if(unit->mesh_inst->frame_end_info)
			{
				assert(unit->bow_instance);
				game->bow_instances.push_back(unit->bow_instance);
				unit->bow_instance = nullptr;
				unit->mesh_inst->groups[0].speed = 1.f;
				unit->action = A_NONE;
				if(Rand() % 2 == 0)
				{
					anim = DA_STRZAL;
					anim2 = DA_STOI;
				}
				else
					anim = DA_BOJOWY;
				break;
			}
		}
		unit->bow_instance->groups[0].time = min(unit->mesh_inst->groups[0].time, unit->bow_instance->groups[0].anim->length);
		unit->bow_instance->need_update = true;
		break;
	case DA_WYJMIJ_BRON:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->mesh_inst->frame_end_info))
			unit->animation_state = 1;
		if(unit->mesh_inst->frame_end_info)
		{
			unit->weapon_state = WS_TAKEN;
			anim = DA_ATAK;
		}
		break;
	case DA_WYJMIJ_LUK:
		if(unit->animation_state == 0 && (unit->mesh_inst->GetProgress() >= unit->data->frames->t[F_TAKE_WEAPON] || unit->mesh_inst->frame_end_info))
			unit->animation_state = 1;
		if(unit->mesh_inst->frame_end_info)
		{
			unit->weapon_state = WS_TAKEN;
			anim = DA_STRZAL;
		}
		break;
	case DA_ROZGLADA:
		if(unit->mesh_inst->frame_end_info)
			anim = DA_STOI;
		break;
	case DA_BLOK:
	case DA_BOJOWY:
	case DA_STOI:
	case DA_IDZIE:
		break;
	default:
		assert(0);
		break;
	}

	unit->mesh_inst->Update(dt);
}

//=================================================================================================
void CreateCharacterPanel::Init()
{
	unit->mesh_inst = new MeshInstance(game->aHumanBase);

	for(ClassInfo& ci : ClassInfo::classes)
	{
		if(ci.IsPickable())
			lbClasses.Add(new DefaultGuiElement(ci.name.c_str(), (int)ci.class_id, ci.icon));
	}
	lbClasses.Sort();
	lbClasses.Initialize();
}

//=================================================================================================
void CreateCharacterPanel::RandomAppearance()
{
	Unit& u = *unit;
	u.human_data->beard = Rand() % MAX_BEARD - 1;
	u.human_data->hair = Rand() % MAX_HAIR - 1;
	u.human_data->mustache = Rand() % MAX_MUSTACHE - 1;
	hair_color_index = Rand() % n_hair_colors;
	u.human_data->hair_color = g_hair_colors[hair_color_index];
	u.human_data->height = Random(0.95f, 1.05f);
	u.human_data->ApplyScale(game->aHumanBase);
	SetControls();
}

//=================================================================================================
void CreateCharacterPanel::Show(bool enter_name)
{
	clas = ClassInfo::GetRandomPlayer();
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	RandomAppearance();

	reset_skills_perks = true;
	this->enter_name = enter_name;
	mode = Mode::PickClass;

	SetControls();
	SetCharacter();
	GUI.ShowDialog(this);
	ResetDoll(true);
	RenderUnit();
}

//=================================================================================================
void CreateCharacterPanel::ShowRedo(Class clas, HumanData& hd, CreatedCharacter& cc)
{
	this->clas = clas;
	lbClasses.Select(lbClasses.FindIndex((int)clas));
	ClassChanged();
	hair_color_index = last_hair_color_index;
	unit->ApplyHumanData(hd);
	this->cc = cc;
	RebuildSkillsFlow();
	RebuildPerksFlow();

	reset_skills_perks = false;
	enter_name = false;
	mode = Mode::PickAppearance;

	SetControls();
	SetCharacter();
	GUI.ShowDialog(this);
	ResetDoll(true);
}

//=================================================================================================
void CreateCharacterPanel::SetControls()
{
	slider[0].val = unit->human_data->hair + 1;
	slider[0].text = Format("%s %d/%d", txHair, slider[0].val, slider[0].maxv);
	slider[1].val = unit->human_data->mustache + 1;
	slider[1].text = Format("%s %d/%d", txMustache, slider[1].val, slider[1].maxv);
	slider[2].val = unit->human_data->beard + 1;
	slider[2].text = Format("%s %d/%d", txBeard, slider[2].val, slider[2].maxv);
	slider[3].val = hair_color_index;
	slider[3].text = Format("%s %d/%d", txHairColor, slider[3].val, slider[3].maxv);
	slider[4].val = int((unit->human_data->height - 0.9f) * 500);
	slider[4].text = Format("%s %d/%d", txSize, slider[4].val, slider[4].maxv);
}

//=================================================================================================
void CreateCharacterPanel::SetCharacter()
{
	anim = anim2 = DA_STOI;
	unit->mesh_inst->Play(NAMES::ani_stand, PLAY_PRIO2 | PLAY_NO_BLEND, 0);
	unit->mesh_inst->groups[0].speed = 1.f;
}

//=================================================================================================
void CreateCharacterPanel::OnChangeClass(int index)
{
	clas = (Class)lbClasses.GetItem()->value;
	ClassChanged();
	reset_skills_perks = true;
	ResetDoll(false);
}

//=================================================================================================
cstring CreateCharacterPanel::GetText(int group, int id)
{
	switch((Group)group)
	{
	case Group::Section:
		if(id == -1)
			return txAttributes;
		else
			return SkillGroup::groups[id].name.c_str();
	case Group::Attribute:
		{
			StatProfile& profile = unit->data->GetStatProfile();
			return Format("%s: %d", Attribute::attributes[id].name.c_str(), profile.attrib[id]);
		}
	case Group::Skill:
		{
			StatProfile& profile = unit->data->GetStatProfile();
			return Format("%s: %d", Skill::skills[id].name.c_str(), profile.skill[id]);
		}
	default:
		return "MISSING";
	}
}

//=================================================================================================
void CreateCharacterPanel::GetTooltip(TooltipController* ptr_tool, int group, int id)
{
	TooltipController& tool = *ptr_tool;

	switch((Group)group)
	{
	case Group::Section:
	default:
		tool.anything = false;
		break;
	case Group::Attribute:
		{
			Attribute& ai = Attribute::attributes[id];
			tool.big_text = ai.name;
			tool.text = ai.desc;
			tool.small_text.clear();
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Skill:
		{
			Skill& si = Skill::skills[id];
			tool.big_text = si.name;
			tool.text = si.desc;
			if(si.attrib2 != AttributeId::NONE)
			{
				tool.small_text = Format("%s: %s, %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str(),
					Attribute::attributes[(int)si.attrib2].name.c_str());
			}
			else
				tool.small_text = Format("%s: %s", txRelatedAttributes, Attribute::attributes[(int)si.attrib].name.c_str());
			tool.anything = true;
			tool.img = nullptr;
		}
		break;
	case Group::Perk:
		{
			tool.anything = true;
			tool.img = nullptr;
			tool.small_text.clear();
			PerkInfo& pi = PerkInfo::perks[id];
			tool.big_text = pi.name;
			tool.text = pi.desc;
			if(IS_SET(pi.flags, PerkInfo::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	case Group::TakenPerk:
		{
			TakenPerk& taken = cc.taken_perks[id];
			tool.anything = true;
			tool.img = nullptr;
			PerkInfo& pi = PerkInfo::perks[(int)taken.perk];
			tool.big_text = pi.name;
			tool.text = pi.desc;
			taken.GetDesc(tool.small_text);
			if(IS_SET(pi.flags, PerkInfo::Flaw))
			{
				tool.text += "\n\n";
				tool.text += txFlawExtraPerk;
			}
		}
		break;
	}
}

//=================================================================================================
void CreateCharacterPanel::ClassChanged()
{
	ClassInfo& ci = ClassInfo::classes[(int)clas];
	unit->data = ci.unit_data;
	anim = DA_STOI;
	t = 1.f;
	tbClassDesc.Reset();
	tbClassDesc.SetText(ci.desc.c_str());
	tbClassDesc.UpdateScrollbar();

	flow_items.clear();

	int y = 0;

	StatProfile& profile = ci.unit_data->GetStatProfile();
	profile.Set(0, unit->stats);
	profile.Set(0, unit->unmod_stats);
	unit->CalculateStats();

	// attributes
	flow_items.push_back(OldFlowItem((int)Group::Section, -1, y));
	y += SECTION_H;
	for(int i = 0; i < (int)AttributeId::MAX; ++i)
	{
		flow_items.push_back(OldFlowItem((int)Group::Attribute, i, 35, 70, profile.attrib[i], y));
		y += VALUE_H;
	}

	// skills
	SkillGroupId group = SkillGroupId::NONE;
	for(int i = 0; i < (int)SkillId::MAX; ++i)
	{
		if(profile.skill[i] >= 0)
		{
			Skill& si = Skill::skills[i];
			if(si.group != group)
			{
				// start new section
				flow_items.push_back(OldFlowItem((int)Group::Section, (int)si.group, y));
				y += SECTION_H;
				group = si.group;
			}
			flow_items.push_back(OldFlowItem((int)Group::Skill, i, 0, 20, profile.skill[i], y));
			y += VALUE_H;
		}
	}
	flow_scroll.total = y;
	flow_scroll.part = flow_scroll.size.y;
	flow_scroll.offset = 0.f;

	cc.Clear(clas);
	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkill(int group, int id)
{
	assert(group == (int)Group::PickSkill_Button);

	if(!cc.s[id].add)
	{
		// add
		--cc.sp;
		cc.s[id].Add(5, true);
	}
	else
	{
		// remove
		++cc.sp;
		cc.s[id].Add(-5, false);
	}

	// update buttons image / text
	FlowItem* find_item = nullptr;
	for(FlowItem* item : flowSkills.items)
	{
		if(item->type == FlowItem::Button)
		{
			if(!cc.s[item->id].add)
			{
				item->state = (cc.sp > 0 ? Button::NONE : Button::DISABLED);
				item->tex_id = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->tex_id = 1;
			}
		}
		else if(item->type == FlowItem::Item && item->id == id)
			find_item = item;
	}

	flowSkills.UpdateText(find_item, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::OnPickPerk(int group, int id)
{
	assert(group == (int)Group::PickPerk_AddButton || group == (int)Group::PickPerk_RemoveButton);

	if(group == (int)Group::PickPerk_AddButton)
	{
		// add perk
		switch((Perk)id)
		{
		case Perk::Strength:
			PickAttribute(txPickAttribIncrease, Perk::Strength);
			break;
		case Perk::Weakness:
			PickAttribute(txPickAttribDecrease, Perk::Weakness);
			break;
		case Perk::Skilled:
		case Perk::Wealthy:
		case Perk::AlchemistApprentice:
		case Perk::FamilyHeirloom:
		case Perk::Leader:
		case Perk::VeryWealthy:
			AddPerk((Perk)id);
			break;
		case Perk::SkillFocus:
			step = 0;
			PickSkill(txPickTwoSkillsDecrease, Perk::SkillFocus, true, 2);
			break;
		case Perk::Talent:
			PickSkill(txPickSkillIncrease, Perk::Talent, false);
			break;
		default:
			assert(0);
			break;
		}
	}
	else
	{
		// remove perk
		TakenPerk& taken = cc.taken_perks[id];
		taken.Remove(cc, id);
		CheckSkillsUpdate();
		RebuildPerksFlow();
	}
}

//=================================================================================================
void CreateCharacterPanel::RebuildSkillsFlow()
{
	flowSkills.Clear();
	SkillGroupId last_group = SkillGroupId::NONE;

	for(Skill& si : Skill::skills)
	{
		int i = (int)si.skill_id;
		if(cc.s[i].value >= 0)
		{
			if(si.group != last_group)
			{
				flowSkills.Add()->Set(SkillGroup::groups[(int)si.group].name.c_str());
				last_group = si.group;
			}
			bool plus = !cc.s[i].add;
			flowSkills.Add()->Set((int)Group::PickSkill_Button, i, (plus ? 0 : 1), plus && cc.sp <= 0);
			flowSkills.Add()->Set(Format("%s: %d", si.name.c_str(), cc.s[i].value), (int)Group::Skill, i);
		}
	}

	flowSkills.Reposition();
}

//=================================================================================================
void CreateCharacterPanel::RebuildPerksFlow()
{
	// group perks by availability
	available_perks.clear();
	unavailable_perks.clear();
	for(PerkInfo& perk : PerkInfo::perks)
	{
		bool taken = false;
		if(!IS_SET(perk.flags, PerkInfo::Multiple))
		{
			for(TakenPerk& tp : cc.taken_perks)
			{
				if(tp.perk == perk.perk_id)
				{
					taken = true;
					break;
				}
			}
		}
		if(!taken)
		{
			if(!IS_SET(perk.flags, PerkInfo::Check) || ValidatePerk(perk.perk_id))
				available_perks.push_back(perk.perk_id);
			else
				unavailable_perks.push_back(perk.perk_id);
		}
	}
	taken_perks.clear();
	LocalVector2<string*> strs;
	for(int i = 0; i < (int)cc.taken_perks.size(); ++i)
	{
		PerkInfo& perk = PerkInfo::perks[(int)cc.taken_perks[i].perk];
		if(IS_SET(perk.flags, PerkInfo::RequireFormat))
		{
			string* s = StringPool.Get();
			*s = cc.taken_perks[i].FormatName();
			strs.push_back(s);
			taken_perks.push_back(std::pair<cstring, int>(s->c_str(), i));
		}
		else
			taken_perks.push_back(std::pair<cstring, int>(perk.name.c_str(), i));
	}

	// sort perks
	std::sort(available_perks.begin(), available_perks.end(), SortPerks);
	std::sort(unavailable_perks.begin(), unavailable_perks.end(), SortPerks);
	std::sort(taken_perks.begin(), taken_perks.end(), SortTakenPerks);

	// fill flow
	flowPerks.Clear();
	if(!available_perks.empty())
	{
		flowPerks.Add()->Set(txAvailablePerks);
		for(Perk perk : available_perks)
		{
			PerkInfo& info = PerkInfo::perks[(int)perk];
			bool can_pick = (cc.perks == 0 && !IS_SET(info.flags, PerkInfo::Free));
			flowPerks.Add()->Set((int)Group::PickPerk_AddButton, (int)perk, 0, can_pick);
			flowPerks.Add()->Set(info.name.c_str(), (int)Group::Perk, (int)perk);
		}
	}
	if(!unavailable_perks.empty())
	{
		flowPerks.Add()->Set(txUnavailablePerks);
		for(Perk p : unavailable_perks)
		{
			flowPerks.Add()->Set((int)Group::PickPerk_DisabledButton, (int)p, 0, true);
			flowPerks.Add()->Set(PerkInfo::perks[(int)p].name.c_str(), (int)Group::Perk, (int)p);
		}
	}
	if(!cc.taken_perks.empty())
	{
		flowPerks.Add()->Set(txTakenPerks);
		for(auto& tp : taken_perks)
		{
			flowPerks.Add()->Set((int)Group::PickPerk_RemoveButton, tp.second, 1, false);
			flowPerks.Add()->Set(tp.first, (int)Group::TakenPerk, tp.second);
		}
	}
	flowPerks.Reposition();
	StringPool.Free(strs.Get());
}

//=================================================================================================
void CreateCharacterPanel::ResetSkillsPerks()
{
	reset_skills_perks = false;
	cc.Clear(clas);
	RebuildSkillsFlow();
	RebuildPerksFlow();
	flowSkills.ResetScrollbar();
	flowPerks.ResetScrollbar();
}

//=================================================================================================
void CreateCharacterPanel::PickAttribute(cstring text, Perk perk)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickAttributeForPerk);
	params.get_tooltip = TooltipGetText(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;

	for(int i = 0; i < (int)AttributeId::MAX; ++i)
		params.AddItem(Format("%s: %d", Attribute::attributes[i].name.c_str(), cc.a[i].value), (int)Group::Attribute, i, cc.a[i].mod);

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::PickSkill(cstring text, Perk perk, bool positive, int multiple)
{
	picked_perk = perk;

	PickItemDialogParams params;
	params.event = DialogEvent(this, &CreateCharacterPanel::OnPickSkillForPerk);
	params.get_tooltip = TooltipGetText(this, &CreateCharacterPanel::GetTooltip);
	params.parent = this;
	params.text = text;
	params.multiple = multiple;

	SkillGroupId last_group = SkillGroupId::NONE;
	for(Skill& info : Skill::skills)
	{
		int i = (int)info.skill_id;
		if(cc.s[i].value > 0 || (!positive && cc.s[i].value == 0))
		{
			if(info.group != last_group)
			{
				params.AddSeparator(SkillGroup::groups[(int)info.group].name.c_str());
				last_group = info.group;
			}
			params.AddItem(Format("%s: %d", Skill::skills[i].name.c_str(), cc.s[i].value), (int)Group::Skill, i, cc.s[i].mod);
		}
	}

	pickItemDialog = PickItemDialog::Show(params);
}

//=================================================================================================
void CreateCharacterPanel::OnPickAttributeForPerk(int id)
{
	if(id == BUTTON_CANCEL)
		return;

	int group, selected;
	pickItemDialog->GetSelected(group, selected);

	AddPerk(picked_perk, selected);
}

//=================================================================================================
void CreateCharacterPanel::OnPickSkillForPerk(int id)
{
	if(id == BUTTON_CANCEL)
	{
		if(picked_perk == Perk::SkillFocus && step == 1)
		{
			cc.s[step_var].Mod(5, false);
			cc.s[step_var2].Mod(5, false);
		}
		return;
	}

	if(picked_perk == Perk::SkillFocus)
	{
		if(step == 0)
		{
			auto& items = pickItemDialog->GetSelected();
			step_var = items[0]->id;
			step_var2 = items[1]->id;
			step = 1;
			cc.s[step_var].Mod(-5, true);
			cc.s[step_var2].Mod(-5, true);
			PickSkill(txPickSkillIncrease, Perk::SkillFocus, false);
		}
		else
		{
			int group, selected;
			pickItemDialog->GetSelected(group, selected);
			cc.s[selected].Mod(10, true);
			flowSkills.UpdateText((int)Group::Skill, step_var, Format("%s: %d", Skill::skills[step_var].name.c_str(), cc.s[step_var].value), true);
			flowSkills.UpdateText((int)Group::Skill, step_var2, Format("%s: %d", Skill::skills[step_var2].name.c_str(), cc.s[step_var2].value), true);
			flowSkills.UpdateText((int)Group::Skill, selected, Format("%s: %d", Skill::skills[selected].name.c_str(), cc.s[selected].value), true);
			flowSkills.UpdateText();
			AddPerk(Perk::SkillFocus, Join3(selected, step_var, step_var2), false);
			UpdateInventory();
		}
	}
	else
	{
		int group, selected;
		pickItemDialog->GetSelected(group, selected);
		AddPerk(Perk::Talent, selected);
		UpdateInventory();
	}
}

//=================================================================================================
void CreateCharacterPanel::UpdateSkill(SkillId s, int value, bool mod)
{
	int id = (int)s;
	cc.s[id].value += value;
	cc.s[id].mod = mod;
	flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));
}

//=================================================================================================
void CreateCharacterPanel::UpdateSkillButtons()
{
	for(FlowItem* item : flowSkills.items)
	{
		if(item->type == FlowItem::Button)
		{
			if(!cc.s[item->id].add)
			{
				item->state = (cc.sp > 0 ? Button::NONE : Button::DISABLED);
				item->tex_id = 0;
			}
			else
			{
				item->state = Button::NONE;
				item->tex_id = 1;
			}
		}
	}
}

//=================================================================================================
void CreateCharacterPanel::AddPerk(Perk perk, int value, bool apply)
{
	TakenPerk taken(perk, value);
	cc.taken_perks.push_back(taken);
	if(apply)
	{
		taken.Apply(cc);
		CheckSkillsUpdate();
	}
	else
	{
		PerkInfo& info = PerkInfo::perks[(int)perk];
		if(!IS_SET(info.flags, PerkInfo::Free))
			--cc.perks;
		if(IS_SET(info.flags, PerkInfo::Flaw))
		{
			++cc.perks;
			++cc.perks_max;
		}
	}
	RebuildPerksFlow();
}

//=================================================================================================
bool CreateCharacterPanel::ValidatePerk(Perk perk)
{
	switch(perk)
	{
	case Perk::VeryWealthy:
		return cc.HavePerk(Perk::Wealthy);
	default:
		assert(0);
		return false;
	}
}

//=================================================================================================
void CreateCharacterPanel::CheckSkillsUpdate()
{
	if(cc.update_skills)
	{
		UpdateSkillButtons();
		cc.update_skills = false;
	}
	if(!cc.to_update.empty())
	{
		if(cc.to_update.size() == 1)
		{
			int id = (int)cc.to_update[0];
			flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value));
		}
		else
		{
			for(SkillId s : cc.to_update)
			{
				int id = (int)s;
				flowSkills.UpdateText((int)Group::Skill, id, Format("%s: %d", Skill::skills[id].name.c_str(), cc.s[id].value), true);
			}
			flowSkills.UpdateText();
		}
		cc.to_update.clear();
	}

	UpdateInventory();
}

//=================================================================================================
void CreateCharacterPanel::UpdateInventory()
{
	const Item* old_items[SLOT_MAX];
	for(int i = 0; i < SLOT_MAX; ++i)
		old_items[i] = items[i];

	cc.GetStartingItems(items);

	bool same = true;
	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(items[i] != old_items[i])
		{
			same = false;
			break;
		}
	}
	if(same)
		return;

	for(int i = 0; i < SLOT_MAX; ++i)
	{
		if(items[i])
			game->PreloadItem(items[i]);
		unit->slots[i] = items[i];
	}

	bool reset = false;

	if((anim == DA_WYJMIJ_LUK || anim == DA_SCHOWAJ_LUK || anim == DA_STRZAL) && !items[SLOT_BOW])
		reset = true;
	if(anim == DA_BLOK && !items[SLOT_SHIELD])
		reset = true;

	if(reset)
	{
		anim = DA_STOI;
		unit->weapon_state = WS_HIDDEN;
		unit->weapon_taken = W_NONE;
		unit->weapon_hiding = W_NONE;
		t = 0.25f;
	}
}

//=================================================================================================
void CreateCharacterPanel::ResetDoll(bool instant)
{
	anim = DA_STOI;
	unit->weapon_state = WS_HIDDEN;
	unit->weapon_taken = W_NONE;
	unit->weapon_hiding = W_NONE;
	if(instant)
	{
		UpdateUnit(0.f);
		unit->SetAnimationAtEnd();
	}
	if(unit->bow_instance)
	{
		game->bow_instances.push_back(unit->bow_instance);
		unit->bow_instance = nullptr;
	}
	unit->action = A_NONE;
}
