#include "Pch.h"
#include "Base.h"
#include "Toolset.h"
#include "Engine.h"
#include "Gui2.h"
#include "Overlay.h"
#include "Window.h"
#include "MenuBar.h"
#include "ToolStrip.h"
#include "TabControl.h"
#include "TreeView.h"
#include "GameTypeManager.h"
#include "ListBox.h"
#include "Label.h"
#include "Button.h"
#include "TextBox.h"
#include "GameTypeProxy.h"
#include "Dialog2.h"

using namespace gui;

enum MenuAction
{
	MA_NEW,
	MA_LOAD,
	MA_SAVE,
	MA_SAVE_AS,
	MA_SETTINGS,
	MA_RELOAD,
	MA_EXIT_TO_MENU,
	MA_QUIT,
	MA_BUILDING_GROUPS,
	MA_BUILDINGS,
	MA_BUILDING_SCRIPTS,
	MA_ABOUT
};

enum TreeMenuAction
{
	TMA_ADD,
	TMA_COPY,
	TMA_REMOVE
};

enum ListAction
{
	A_ADD,
	A_DUPLICATE,
	A_REMOVE
};

Toolset::Toolset(GameTypeManager& gt_mgr) : engine(nullptr), gt_mgr(gt_mgr)
{

}

Toolset::~Toolset()
{
	for(auto& toolset_item : toolset_items)
		delete toolset_item.second;
	delete tree_menu;
}

void Toolset::Init(Engine* _engine)
{
	engine = _engine;
	
	Window* window = new Window(true);

	MenuBar* menu = new MenuBar;
	menu->SetHandler(delegate<void(int)>(this, &Toolset::HandleMenuEvent));
	menu->AddMenu("Module", {
		//{"New", MA_NEW},
		//{"Load", MA_LOAD},
		{"Save", MA_SAVE},
		//{"Save As", MA_SAVE_AS},
		//{"Settings", MA_SETTINGS},
		//{"Reload", MA_RELOAD},
		{"Exit to menu", MA_EXIT_TO_MENU},
		{"Quit", MA_QUIT}
	});
	menu->AddMenu("Buildings", {
		{"Building groups", MA_BUILDING_GROUPS},
		//{"Buildings", MA_BUILDINGS},
		//{"Building scripts", MA_BUILDING_SCRIPTS}
	});
	menu->AddMenu("Info", {
		{"About", MA_ABOUT}
	});

	tab_ctrl = new TabControl(false);
	tab_ctrl->SetDocked(true);

	window->SetMenu(menu);
	window->Add(tab_ctrl);

	window->Initialize();

	Add(window);
	GUI.Add(this);

	vector<SimpleMenuCtor> tree_menu_items = {
		{ "Add", (int)TMA_ADD },
		{ "Copy", (int)TMA_COPY },
		{ "Remove", (int)TMA_REMOVE }
	};
	tree_menu = new MenuStrip(tree_menu_items);
	tree_menu->SetHandler(delegate<void(int)>(this, &Toolset::HandleTreeViewMenuEvent));

	vector<SimpleMenuCtor> menu_strip_items = {
		{ "Add", A_ADD },
		{ "Duplicate", A_DUPLICATE },
		{ "Remove", A_REMOVE }
	};
	menu_strip = new MenuStrip(menu_strip_items);
}

void Toolset::Start()
{
	GUI.SetOverlay(this);
	Show();
	closing = Closing::No;
}

void Toolset::OnDraw()
{
	GUI.Draw(engine->wnd_size);
}

bool Toolset::OnUpdate(float dt)
{
	if(closing != Closing::No)
	{
		tab_ctrl->Clear();
		GUI.SetOverlay(nullptr);
		Hide();
		if(closing == Closing::Shutdown)
			engine->EngineShutdown();
		return false;
	}

	return true;
}

void Toolset::Event(GuiEvent event)
{
	switch(event)
	{
	case Event_Save:
		SaveEntity();
		break;
	case Event_Restore:
		RestoreEntity();
		break;
	default:
		Overlay::Event(event);
		break;
	}
}

void Toolset::Update(float dt)
{
	Overlay::Update(dt);

	auto tab = tab_ctrl->GetCurrentTab();
	if(tab)
	{
		bool have_changes = AnyEntityChanges();
		tab->SetHaveChanges(have_changes);
		current_toolset_item->bt_save->SetDisabled(!have_changes);
		current_toolset_item->bt_restore->SetDisabled(!have_changes);
	}

	if(Key.PressedRelease(VK_ESCAPE))
		closing = Closing::Yes;
}

void Toolset::HandleMenuEvent(int id)
{
	switch(id)
	{
	case MA_SAVE:
		Save();
		break;
	case MA_RELOAD:
		Reload();
		break;
	case MA_EXIT_TO_MENU:
		ExitToMenu();
		break;
	case MA_QUIT:
		Quit();
		break;
	case MA_BUILDING_GROUPS:
		ShowGameType(GT_BuildingGroup);
		break;
	case MA_BUILDINGS:
		ShowGameType(GT_Building);
		break;
	case MA_BUILDING_SCRIPTS:
		ShowGameType(GT_BuildingScript);
		break;
	case MA_ABOUT:
		SimpleDialog("TOOLSET\n=========\nNothing interesting here yet...");
		break;
	}
}

void Toolset::Save()
{
	if(!AnyUnsavedChanges())
		return;

	if(!SaveEntity())
		return;

	gt_mgr.SaveText(current_toolset_item->game_type.GetGameTypeId());
}

void Toolset::Reload()
{

}

void Toolset::ExitToMenu()
{
	if(AnyUnsavedChanges())
	{
		DialogInfo dialog;
		dialog.type = DIALOG_YESNO;
		dialog.text = "Discard unsaved changes and exit to menu?";
		dialog.event = [this](int id) { if(id == BUTTON_YES) closing = Closing::Yes; };
		GUI.ShowDialog(dialog);
		return;
	}

	closing = Closing::Yes;
}

void Toolset::Quit()
{
	if(AnyUnsavedChanges())
	{
		DialogInfo dialog;
		dialog.type = DIALOG_YESNO;
		dialog.text = "Discard unsaved changes and quit?";
		dialog.event = [this](int id) { if(id == BUTTON_YES) closing = Closing::Shutdown; };
		GUI.ShowDialog(dialog);
		return;
	}

	closing = Closing::Shutdown;
}

void Toolset::ShowGameType(GameTypeId id)
{
	GameType& type = gt_mgr.GetGameType(id);
	TabControl::Tab* tab = tab_ctrl->Find(type.GetId().c_str());
	if(tab)
	{
		tab->Select();
		return;
	}

	tab_ctrl->AddTab(type.GetId().c_str(), type.GetFriendlyName().c_str(), GetToolsetItem(id)->window);
}

ToolsetItem* Toolset::GetToolsetItem(GameTypeId id)
{
	auto it = toolset_items.find(id);
	if(it != toolset_items.end())
		return it->second;
	ToolsetItem* toolset_item = CreateToolsetItem(id);
	toolset_items[id] = toolset_item;
	return toolset_item;
}

class TreeItem : public TreeNode
{
public:
	TreeItem(GameType& type, GameTypeItem item) : type(type), item(item)
	{
		SetText(type.GetItemId(item));
	}

	GameType& type;
	GameTypeItem item;
};

struct GameItemElement : public GuiElement
{
	ToolsetItem::Entity* entity;
	GameType* type;

	GameItemElement(ToolsetItem::Entity* entity, GameType* type) : entity(entity), type(type) {}

	cstring ToString() override
	{
		GameTypeItem item = entity->GetItem();
		return type->GetItemId(item).c_str();
	}
};

ToolsetItem* Toolset::CreateToolsetItem(GameTypeId id)
{
	ToolsetItem* toolset_item = new ToolsetItem(gt_mgr.GetGameType(id));
	toolset_item->window = new Window;


	GameType& type = toolset_item->game_type;
	GameTypeHandler* handler = type.GetHandler();

	toolset_item->items.reserve(handler->Count());
	for(auto e : handler->ForEach())
	{
		auto new_e = new ToolsetItem::Entity(e);
		new_e->id = &type.GetItemId(e);
		toolset_item->items.push_back(new_e);
	}

	Window* w = toolset_item->window;

	/*TreeView* tree = new TreeView;
	tree->SetMultiselect(true);
	tree->SetKeyEvent(KeyEvent(this, &Toolset::HandleTreeViewKeyEvent));
	tree->SetMouseEvent(MouseEvent(this, &Toolset::HandleTreeViewMouseEvent));
	for(GameTypeItem item : handler->ForEach())
		tree->AddChild(new TreeItem(type, item));
	w->Add(tree);*/

	Label* label = new Label(Format("%s (%u):", type.GetFriendlyName().c_str(), handler->Count()));
	label->SetPosition(INT2(5, 5));
	w->Add(label);

	ListBox* list_box = new ListBox(true);
	list_box->menu_strip = menu_strip;
	list_box->event_handler2 = DialogEvent2(this, &Toolset::HandleListBoxEvent);
	list_box->SetPosition(INT2(5, 30));
	list_box->SetSize(INT2(200, 500));
	list_box->Init();
	for(auto e : toolset_item->items)
		list_box->Add(new GameItemElement(e, &type));
	w->Add(list_box);

	Button* bt = new Button;
	bt->text = "Save";
	bt->id = Event_Save;
	bt->SetPosition(INT2(5, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);
	toolset_item->bt_save = bt;

	bt = new Button;
	bt->text = "Restore";
	bt->id = Event_Restore;
	bt->SetPosition(INT2(110, 535));
	bt->SetSize(INT2(100, 30));
	w->Add(bt);
	toolset_item->bt_restore = bt;

	Panel* panel = new Panel;
	panel->custom_color = 0;
	panel->use_custom_color = true;
	panel->SetPosition(INT2(210, 5));
	panel->SetSize(tab_ctrl->GetAreaSize() - list_box->GetSize());

	label = new Label("Name");
	label->SetPosition(INT2(0, 0));
	panel->Add(label);

	TextBox* text_box = new TextBox(false, true);
	text_box->SetPosition(INT2(0, 30));
	text_box->SetSize(INT2(300, 30));
	panel->Add(text_box);

	w->Add(panel);
	w->SetDocked(true);
	w->SetEventProxy(this);
	w->Initialize();

	toolset_item->list_box = list_box;
	toolset_item->box = text_box;
	current_toolset_item = toolset_item;


	if(list_box->IsEmpty())
		panel->Disable();
	else
		list_box->Select(0, true);

	return toolset_item;
}

void Toolset::HandleTreeViewKeyEvent(gui::KeyEventData& e)
{
	/*
	lewo - zwi�, id� do parenta (z shift dzia�a)
	prawo - rozwi� - id� do 1 childa (shift)
	g�ra - z shift
	d�
	del - usu�
	spacja - zaznacz
	litera - przejd� do nast�pnego o tej literze

	klik - zaznacza, z ctrl dodaje/usuwa zaznaczenie
		z shift - zaznacza wszystkie od poprzedniego focusa do teraz
		z shift zaznacza obszar od 1 klika do X, 1 miejsce sie nie zmienia
		z shift i ctrl nie usuwa nigdy zaznaczenia (mo�na doda� obszary)
	*/
}

void Toolset::HandleTreeViewMouseEvent(gui::MouseEventData& e)
{
	if(e.button == VK_RBUTTON && !e.pressed)
	{
		TreeView* tree = (TreeView*)e.control;
		bool clicked_node = (tree->GetClickedNode() != nullptr);
		tree_menu->FindItem(TMA_COPY)->SetEnabled(clicked_node);
		tree_menu->FindItem(TMA_REMOVE)->SetEnabled(clicked_node);
	}
}

void Toolset::HandleTreeViewMenuEvent(int id)
{
	switch(id)
	{
	case TMA_ADD:
	case TMA_COPY:
	case TMA_REMOVE:
		break;
	}
}

bool Toolset::HandleListBoxEvent(int action, int id)
{
	switch(action)
	{
	case ListBox::A_BEFORE_CHANGE_INDEX:
		if(!SaveEntity())
			return false;
		break;
	case ListBox::A_INDEX_CHANGED:
		{
			GameItemElement* e = current_toolset_item->list_box->GetItemCast<GameItemElement>();
			current_toolset_item->box->SetText(e->type->GetItemId(e->entity->GetItem()).c_str());
			current_entity = e->entity;
		}
		break;
	case ListBox::A_BEFORE_MENU_SHOW:
		{
			if(!SaveEntity())
				return false;

			bool enabled = (id != -1);
			menu_strip->FindItem(A_DUPLICATE)->SetEnabled(enabled);
			menu_strip->FindItem(A_REMOVE)->SetEnabled(enabled);
			context_index = id;
		}
		break;
	case ListBox::A_MENU:
		switch(id)
		{
		case A_ADD:
			{
				auto new_e = new ToolsetItem::Entity(nullptr, current_toolset_item->game_type.GetHandler()->Create());
				new_e->id = &current_toolset_item->game_type.GetItemId(new_e->current);
				*new_e->id = GenerateEntityName(Format("new %s", current_toolset_item->game_type.GetId().c_str()), false);
				auto item = new GameItemElement(new_e, &current_toolset_item->game_type);
				if(context_index == -1)
				{
					current_toolset_item->list_box->Add(item);
					current_toolset_item->list_box->Select(current_toolset_item->list_box->GetItems().size() - 1, true);
					current_toolset_item->items.push_back(new_e);
				}
				else
				{
					current_toolset_item->list_box->Insert(item, context_index);
					current_toolset_item->list_box->Select(context_index, true);
					current_toolset_item->items.insert(current_toolset_item->items.begin() + context_index, new_e);
				}
			}
			break;
		case A_DUPLICATE:
			{
				auto new_e = new ToolsetItem::Entity(nullptr, current_toolset_item->game_type.Duplicate(current_entity->GetItem()));
				new_e->id = &current_toolset_item->game_type.GetItemId(new_e->current);
				*new_e->id = GenerateEntityName(current_entity->id->c_str(), true);
				auto item = new GameItemElement(new_e, &current_toolset_item->game_type);
				current_toolset_item->list_box->Insert(item, context_index + 1);
				current_toolset_item->list_box->Select(context_index + 1, true);
				current_toolset_item->items.insert(current_toolset_item->items.begin() + context_index + 1, new_e);
			}
			break;
		case A_REMOVE:
			{
				delete current_entity;
				current_toolset_item->items.erase(current_toolset_item->items.begin() + context_index);
				current_toolset_item->list_box->Remove(context_index);
			}
			break;
		}
		break;
	}

	return true;
}

bool Toolset::AnyEntityChanges()
{
	if(!current_entity)
		return false;
	return current_entity->deattached
		|| current_toolset_item->game_type.GetItemId(current_entity->GetItem()) != current_toolset_item->box->GetText();
}

bool Toolset::SaveEntity()
{
	if(!AnyEntityChanges())
		return true;

	if(!ValidateEntity())
		return false;

	GameTypeItem item = current_entity->current;
	if(!item)
	{
		current_entity->current = current_toolset_item->game_type.GetHandler()->Create();
		current_entity->id = &current_toolset_item->game_type.GetItemId(current_entity->current);
		item = current_entity->current;
	}

	GameTypeProxy proxy(current_toolset_item->game_type, item);
	proxy.GetId() = current_toolset_item->box->GetText();

	current_entity->deattached = false;

	return true;
}

void Toolset::RestoreEntity()
{
	if(!AnyEntityChanges())
		return;

	if(current_entity->deattached)
	{
		ForgetEntity();
		return;
	}

	GameTypeItem item = current_entity->GetItem();

	current_toolset_item->box->SetText(current_toolset_item->game_type.GetItemId(item).c_str());
}

bool Toolset::ValidateEntity()
{
	cstring err_msg = nullptr;
	const string& new_id = current_toolset_item->box->GetText();
	if(new_id.length() < 1)
		err_msg = "Id must not be empty.";
	else if(new_id.length() > 100)
		err_msg = "Id max length is 100.";
	else
	{
		for(auto e : current_toolset_item->items)
		{
			if(*e->id == new_id && e != current_entity)
			{
				err_msg = "Id must be unique.";
				break;
			}
		}
	}

	if(err_msg)
	{
		GUI.SimpleDialog(Format("Validation failed\n----------------------\n%s", err_msg), this);
		return false;
	}
	else
		return true;
}

void Toolset::ForgetEntity()
{
	current_toolset_item->items.erase(current_toolset_item->items.begin() + current_toolset_item->list_box->GetIndex());
	current_toolset_item->game_type.GetHandler()->Destroy(current_entity->current);
	current_entity = nullptr;
	current_toolset_item->list_box->Remove(current_toolset_item->list_box->GetIndex());
}

cstring Toolset::GenerateEntityName(cstring name, bool dup)
{
	string old_name = name;
	int index = 1;
	bool ok = true;

	if(!dup)
	{
		for(auto e : current_toolset_item->items)
		{
			if(*e->id == name)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return name;
	}
	else
	{
		cstring pos = strrchr(name, '(');
		if(pos)
		{
			int new_index;
			if(sscanf(pos, "(%d)", &new_index) == 1)
			{
				index = new_index + 1;
				if(name != pos)
					old_name = string(name, pos - name - 1);
				else
					old_name = string(name, pos - name);
			}
		}
	}

	string new_name;
	while(true)
	{
		new_name = Format("%s (%d)", old_name.c_str(), index);
		ok = true;
		for(auto e : current_toolset_item->items)
		{
			if(*e->id == new_name)
			{
				ok = false;
				break;
			}
		}
		if(ok)
			return Format("%s", new_name.c_str());
		++index;
	}
}

bool Toolset::AnyUnsavedChanges()
{
	return AnyEntityChanges();
}
