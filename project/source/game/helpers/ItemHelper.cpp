#include "Pch.h"
#include "GameCore.h"
#include "ItemHelper.h"
#include "Chest.h"
#include "Stock.h"
#include "Unit.h"

int treasure_value[] = {
	10, //0
	50, //1
	100, //2
	200, //3
	350, //4
	500, //5
	700, //6
	900, //7
	1200, //8
	1500, //9
	1900, //10
	2400, //11
	2900, //12
	3400, //13
	4000, //14
	4600, //15
	5300, //16
	6000, //17
	6800, //18
	7600, //19
	8500 //20
};

const float price_mod_buy[] = { 1.25f, 1.0f, 0.75f };
const float price_mod_sell[] = { 0.25f, 0.5f, 0.75f };
const float price_mod_buy_v[] = { 1.25f, 1.0f, 0.9f };
const float price_mod_sell_v[] = { 0.5f, 0.75f, 0.9f };

template<typename T>
void InsertRandomItem(vector<ItemSlot>& container, vector<T*>& items, int price_limit, int exclude_flags, uint count)
{
	for(int i = 0; i < 100; ++i)
	{
		T* item = items[Rand() % items.size()];
		if(item->value > price_limit || IS_SET(item->flags, exclude_flags))
			continue;
		InsertItemBare(container, item, count);
		return;
	}
}

//=================================================================================================
void ItemHelper::GenerateTreasure(int level, int _count, vector<ItemSlot>& items, int& gold, bool extra)
{
	assert(InRange(level, 1, 20));

	int value = Random(treasure_value[level - 1], treasure_value[level]);

	items.clear();

	const Item* item;
	uint count;

	for(int tries = 0; tries < _count; ++tries)
	{
		switch(Rand() % IT_MAX_GEN)
		{
		case IT_WEAPON:
			item = Weapon::weapons[Rand() % Weapon::weapons.size()];
			count = 1;
			break;
		case IT_ARMOR:
			item = Armor::armors[Rand() % Armor::armors.size()];
			count = 1;
			break;
		case IT_BOW:
			item = Bow::bows[Rand() % Bow::bows.size()];
			count = 1;
			break;
		case IT_SHIELD:
			item = Shield::shields[Rand() % Shield::shields.size()];
			count = 1;
			break;
		case IT_CONSUMABLE:
			item = Consumable::consumables[Rand() % Consumable::consumables.size()];
			count = Random(2, 5);
			break;
		case IT_OTHER:
			item = OtherItem::others[Rand() % OtherItem::others.size()];
			count = Random(1, 4);
			break;
		default:
			assert(0);
			item = nullptr;
			break;
		}

		if(!item->CanBeGenerated())
			continue;

		int cost = item->value * count;
		if(cost > value)
			continue;

		value -= cost;

		InsertItemBare(items, item, count, count);
	}

	if(extra)
		InsertItemBare(items, ItemList::GetItem("treasure"));

	gold = value + level * 5;
}

//=================================================================================================
void ItemHelper::SplitTreasure(vector<ItemSlot>& items, int gold, Chest** chests, int count)
{
	assert(gold >= 0 && count > 0 && chests);

	while(!items.empty())
	{
		for(int i = 0; i < count && !items.empty(); ++i)
		{
			chests[i]->items.push_back(items.back());
			items.pop_back();
		}
	}

	int divided_count = gold / count - 1;
	if(divided_count < 0)
		divided_count = 0;
	gold -= divided_count * count;

	for(int i = 0; i < count; ++i)
	{
		if(i == count - 1)
			divided_count += gold;
		ItemSlot& slot = Add1(chests[i]->items);
		slot.Set(Item::gold, divided_count, divided_count);
		SortItems(chests[i]->items);
	}
}

//=================================================================================================
int ItemHelper::GetItemPrice(const Item* item, Unit& unit, bool buy)
{
	assert(item);

	int cha = unit.Get(AttributeId::CHA);
	const float* mod_table;

	if(item->type == IT_OTHER && item->ToOther().other_type == Valuable)
	{
		if(buy)
			mod_table = price_mod_buy_v;
		else
			mod_table = price_mod_sell_v;
	}
	else
	{
		if(buy)
			mod_table = price_mod_buy;
		else
			mod_table = price_mod_sell;
	}

	float mod;
	if(cha <= 1)
		mod = mod_table[0];
	else if(cha < 50)
		mod = Lerp(mod_table[0], mod_table[1], float(cha) / 50);
	else if(cha == 50)
		mod = mod_table[1];
	else if(cha < 100)
		mod = Lerp(mod_table[1], mod_table[2], float(cha - 50) / 50);
	else
		mod = mod_table[2];

	int price = int(mod * item->value);
	if(price == 0 && buy)
		price = 1;

	return price;
}

//=================================================================================================
// zwraca losowy przedmiot o maksymalnej cenie, ta funkcja jest powolna!
// mo�e zwr�ci� questowy przedmiot je�li b�dzie wystarczaj�co tani, lub unikat!
const Item* ItemHelper::GetRandomItem(int max_value)
{
	int type = Rand() % 6;

	LocalVector<const Item*> items;

	switch(type)
	{
	case 0:
		for(Weapon* w : Weapon::weapons)
		{
			if(w->value <= max_value && w->CanBeGenerated())
				items->push_back(w);
		}
		break;
	case 1:
		for(Bow* b : Bow::bows)
		{
			if(b->value <= max_value && b->CanBeGenerated())
				items->push_back(b);
		}
		break;
	case 2:
		for(Shield* s : Shield::shields)
		{
			if(s->value <= max_value && s->CanBeGenerated())
				items->push_back(s);
		}
		break;
	case 3:
		for(Armor* a : Armor::armors)
		{
			if(a->value <= max_value && a->CanBeGenerated())
				items->push_back(a);
		}
		break;
	case 4:
		for(Consumable* c : Consumable::consumables)
		{
			if(c->value <= max_value && c->CanBeGenerated())
				items->push_back(c);
		}
		break;
	case 5:
		for(OtherItem* o : OtherItem::others)
		{
			if(o->value <= max_value && o->CanBeGenerated())
				items->push_back(o);
		}
		break;
	}

	return items->at(Rand() % items->size());
}

//=================================================================================================
const Item* ItemHelper::GetBetterItem(const Item* item)
{
	assert(item);

	auto it = better_items.find(item);
	if(it != better_items.end())
		return it->second;

	return nullptr;
}

//=================================================================================================
void ItemHelper::SkipStock(FileReader& f)
{
	uint count;
	f >> count;
	if(count == 0)
		return;

	for(uint i = 0; i < count; ++i)
	{
		const string& item_id = f.ReadString1();
		f.Skip<uint>(); // count
		if(item_id[0] == '$')
			f.Skip<int>(); // quest_refid
	}
}

//=================================================================================================
void ItemHelper::AddRandomItem(vector<ItemSlot>& items, ITEM_TYPE type, int price_limit, int flags, uint count)
{
	switch(type)
	{
	case IT_WEAPON:
		InsertRandomItem(items, Weapon::weapons, price_limit, flags, count);
		break;
	case IT_BOW:
		InsertRandomItem(items, Bow::bows, price_limit, flags, count);
		break;
	case IT_SHIELD:
		InsertRandomItem(items, Shield::shields, price_limit, flags, count);
		break;
	case IT_ARMOR:
		InsertRandomItem(items, Armor::armors, price_limit, flags, count);
		break;
	case IT_OTHER:
		InsertRandomItem(items, OtherItem::others, price_limit, flags, count);
		break;
	case IT_CONSUMABLE:
		InsertRandomItem(items, Consumable::consumables, price_limit, flags, count);
		break;
	case IT_BOOK:
		InsertRandomItem(items, Book::books, price_limit, flags, count);
		break;
	}
}
