#ifndef GAME_H
#define GAME_H

#include "../template/app.h"

#define WORLD_X_LENGTH 1000
#define WORLD_Y_LENGTH 1000

#define MAX_ENTITIES 1000
#define MAX_UI 1000

enum Item_id : u16
{
	ITEM_NULL,

	ITEM_1,

	ITEM_SPELL_NULL,
	ITEM_SPELL_PLACE_ITEM,
	ITEM_SPELL_DESTROY,
	ITEM_SPELL_LAST,

	ITEM_WAND,
	ITEM_LAST_ID,
};


// #define MAX_SPELL_SLOTS 20
// struct Spell_inventory
// {
// 	u16 size;
// 	Spell spells [MAX_SPELL_SLOTS];
// };
#define INVENTORY_SIZE 20

struct Inventory{
	b8 is_editable;
	u8 size;
	u8 selected_slot;

	u16 items[INVENTORY_SIZE];
	
};

struct Item
{
	u16 item_count;
	Item_id item_id;

	Inventory inventory;
};

internal u16 
get_new_item(Item* items)
{
	for(u16 i=ITEM_LAST_ID; i<0xffff; i++)
	{
		if(!items[i].item_id)
		{
			return i;
		}
	}
	ASSERT(false);
	return NULL_INDEX16;
}

//TODO: test this as a small number
//TODO: is this still needed
#define MAX_SPELL_INVENTORIES 500


//@@bitwise_enum@@
enum Entity_flag : u64
{
   E_RENDER = 0b1,
	E_PICKUP = 0b10,
   E_LAST_FLAG = 0b100,
};

#define E_PLAYER_INDEX 0

#define MAX_ITEM_STACK 255
struct Entity
{
   u64 flags;
	struct {
		u16 item;
	};// pickup
   V2 pos;
   V2 move_direction;
	V2 target_direction;

	f32 casting_cooldown;
	b8 is_casting;
	
	Inventory inventory;
	u16 unarmed_item;
};

enum Ui_layout
{
	UI_LAYOUT_DEFAULT,
	UI_LAYOUT_GRID_ROW_MAJOR,


	UI_LAYOUT_LAST,
};

struct Ui_style
{
	Ui_layout layout;
	u32 line_size;
	f32 cells_border_size;

	String text;
	
	// union {
	// 	struct{
	// 		Int2 pos;
	// 		Int2 size;
	// 	};
	// 	Rect_int rect;
	// };
	union {
		struct{
			V2 pos;
			V2 size;
		};
		Rect_float rect;
	};
	s32 zpos;

	Color color_rect;
	Color color_text;	

	u16 tex_uid;
};

//@@bitwise_enum@@
enum UI_FLAGS : u64
{
	UI_SKIP_RENDERING = 0b1,
	UI_CLICKABLE = 0b10,
	UI_TEXTBOX = 0b100,
	UI_SKIP_RENDERING_CHILDREN = 0b1000,
	UI_SELECTABLE = 0b10000,
	UI_LAST_FLAG = 0b100000
};


struct Ui_widget
{
	u64 flags;

	union{
		struct{
			u16 parent_uid;
			u16 local_uid;
		};
		u32 full_uid;
	};
	// u32 last_frame_uid;
	u16 child_count;

   Ui_style style;
};

// struct Ui_uid
// {
// 	union 
// 	{
// 		struct{
// 			u16 parent_uid;
// 			u16 local_uid;
// 		};
// 		u32 full_uid;
// 	};
// 	u16 widget_uid;	
// };

// internal b32
// operator ==(Ui_uid id1, Ui_uid id2)
// {
// 	return id1.full_uid == id2.full_uid;
// }

// struct Ui_selection
// {
// 	Ui_uid hot;
	
// 	Ui_uid pressed; // cursor primary button
// 	Ui_uid clicked;

// 	Ui_uid pressed2; // cursor secondary button
// 	Ui_uid clicked2;

// 	void init_indices()
// 	{
// 		hot = {0};
// 		clicked = {0};
// 		clicked2 = {0};
// 	}
// };
struct Ui_selection
{
	u16 hot

	,pressed
	,clicked

	,pressed2
	,clicked2
	;

};
Ui_selection NULL_UI_SELECTION = {NULL_INDEX16,NULL_INDEX16, NULL_INDEX16, NULL_INDEX16, NULL_INDEX16};

struct UI
{
	Ui_widget widgets[MAX_UI];
	V2 global_positions[MAX_UI];
	int global_zpositions[MAX_UI];

	u16 current_parent_uid;

	u16 current_widget_uid;
	
	u16 focus;
	Int2 focused_ui_position;

	Ui_selection selection;
	// Ui_uid selected_button; // TODO: maybe add this to the Ui_selection struct
};

internal u16
do_widget_old(UI* ui, u32 widget_flags, Ui_style style)
{
	// {
		// u32 parent_uid = ui->current_parent_uid;
		// int current_y_pos = style.pos.y;
		// int cumulative_y_pos = style.pos.y;
		// while(parent_uid)
		// {
		// 	if(current_y_pos > 0 || cumulative_y_pos > 0)
		// 	{
		// 		widget_flags = UI_SKIP_RENDERING;
		// 	}
		// }
	// }
	if(style.pos.y > 0 || ui->widgets[ui->current_parent_uid].flags & UI_SKIP_RENDERING_CHILDREN)
	{
		widget_flags = UI_SKIP_RENDERING|UI_SKIP_RENDERING_CHILDREN;
	}
	u16 result = ui->current_widget_uid;
	ui->current_widget_uid++;

	Ui_widget* new_widget = &ui->widgets[result];
	new_widget->style = style;
	new_widget->flags = widget_flags;
	new_widget->parent_uid = ui->current_parent_uid;
	new_widget->local_uid = ui->widgets[ui->current_parent_uid].child_count++;
	
	// if(ui_push_inside) ui->current_parent_uid = new_widget_index;
	
   new_widget->child_count = 0;
   ui->widgets[new_widget->parent_uid].child_count++;

	return result;
}

internal u16
do_widget(UI* ui, u32 widget_flags, Ui_style style)
{
	u16 result = ui->current_widget_uid++;
	Ui_widget* new_widget;
	new_widget = &ui->widgets[result];
	new_widget->style = style;
	new_widget->flags = widget_flags;
	new_widget->parent_uid = ui->current_parent_uid;
	new_widget->local_uid = ui->widgets[ui->current_parent_uid].child_count++;

	return result;
}

internal u16
ui_push(UI* ui, u16 uid)
{
	ASSERT(uid < MAX_UI);
	ui->current_parent_uid = uid;
	return uid;
}

internal void
ui_pop(UI* ui)
{
	ASSERT(ui->current_parent_uid < MAX_UI);
	ui->current_parent_uid = ui->widgets[ui->current_parent_uid].parent_uid;
}

internal b32
point_vs_ui_rect(Int2 p, Rect_int rect)
{
	b32 x_inside = rect.pos.x <=  p.x && p.x  < rect.pos.x + rect.size.x;
	b32 y_inside = rect.pos.y - rect.size.y <=  p.y && p.y  < rect.pos.y;
	return x_inside && y_inside;
}

#define MAX_ITEMS 0xffff

struct App_data
{
   V3 camera_pos;
   Int2 viewport_size;

   Entity entities[MAX_ENTITIES];
	u8 used_entities[MAX_ENTITIES];
   UI ui;

   Int2 cursor_tile_pos;

   u8 world [WORLD_Y_LENGTH][WORLD_X_LENGTH];
   Int2 tiles_px_size;

   b32 is_menu_opened;

	//indices from 0 to ITEM_LAST_ID are reserved for static inventories
	u8 used_items[MAX_ITEMS];
	Item items[MAX_ITEMS];
	u16 cursor_item;

	u16 item_id_to_tex_uid [ITEM_LAST_ID];

   u16 tex_null
	,tex_player
	,tex_stone
	,tex_chest
	,tex_shroom
	,tex_slime
	,tex_wand
	,tex_spell_null
	,tex_spell_place_item
	,tex_spell_destroy

	,mesh_plane_y_up
   ,blend_state_enabled
   ,rtv_lowres
   ,depth_stencil_lowres
   ,dynamic_mesh_tiles
   ,vs_instancing
   ,vs_simple
   ;
};

internal Int2
pos_to_tile(V2 v, V2 tiles_screen_size)
{
   Int2 result;
   result.x = (s32)((v.x/tiles_screen_size.x)+ WORLD_X_LENGTH/2);
   result.y = (s32)((v.y/tiles_screen_size.y)+ WORLD_Y_LENGTH/2);
   
   return result;
}

internal V2
tile_to_pos(Int2 tile, V2 tiles_screen_size)
{
	V2 result;
	result.x = (tile.x-(WORLD_X_LENGTH/2)+.5f)*tiles_screen_size.x;
	result.y = (tile.y-(WORLD_Y_LENGTH/2)+.5f)*tiles_screen_size.y;
	return result;
}

internal void
destroy_tile(App_data* app, Int2 tile_pos, V2 tiles_screen_size)
{
	app->world[tile_pos.y][tile_pos.x] = 0;
	u16 e_id = get_next_available_index(app->used_entities, MAX_ENTITIES, 0);
	app->entities[e_id].flags = E_PICKUP|E_RENDER;
	u16 new_item_id = get_next_available_index(app->used_items, MAX_ITEMS, 0);
	app->entities[e_id].item = new_item_id;
	//TODO: tiles will have their own item_id instead of ITEM_1
	app->items[new_item_id] = app->items[ITEM_1];

	app->entities[e_id].pos = tile_to_pos(tile_pos, tiles_screen_size);
}

internal u16
get_entity_equipped_item_index(App_data* app, u16 entity_index)
{
	u16 result = app->entities[entity_index].inventory.items[app->entities[entity_index].inventory.selected_slot];
	if(!result)
		result = app->entities[entity_index].unarmed_item;
	return result;
}


#endif