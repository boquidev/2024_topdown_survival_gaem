#include "game.h"

void init(Platform_data* memory, App_data* app)
{memory, app;

}

void update(Platform_data* memory, App_data* app, Audio audio, Int2 client_size)
{memory, app, audio, client_size;
   User_input* input = memory->input;

   // PLATFORM INITIALIZATION

   if(!memory->is_initialized)
   {
      memory->is_initialized = 1;
      app->is_initialized = 0;
      f32* temp_texture  = ARENA_PUSH_STRUCTS(memory->temp_arena, f32, WORLD_X_LENGTH*WORLD_Y_LENGTH);
      UNTIL(i, WORLD_X_LENGTH*WORLD_Y_LENGTH)
      {
         temp_texture[i] = memory->rng.next(1.0f);   
      }

      UNTIL(y, WORLD_Y_LENGTH)
      {
         UNTIL(x, WORLD_X_LENGTH)
         {
            // f32 noise_value = sample_2d_perlin_noise(temp_texture, WORLD_X_LENGTH, WORLD_Y_LENGTH, x, y, 4, 5, .5f);
            f32 noise_value = sample_2d_perlin_noise(temp_texture, WORLD_X_LENGTH, WORLD_Y_LENGTH, x, y, 1, WORLD_X_LENGTH/5, .75f);
            {
               u32 center_x = WORLD_X_LENGTH/2;
               u32 center_y = WORLD_Y_LENGTH/2;

               if(noise_value < .5f || (center_x-10 < x && x < center_x+10 && center_y-10 < y && y < center_y+10)){
                  app->world[y][x] = 0;
               }else{
                  app->world[y][x] = 1;
               }
            }
         }
      }

      Asset_request* request;

      #define PUSH_ASSET_REQUEST PUSH_BACK(memory->asset_requests, memory->temp_arena, request)

      asset_request_tex_from_file(memory, &app->tex_player, "data/textures/player.png");
      asset_request_tex_from_file(memory, &app->tex_null, "data/textures/null.png");
      asset_request_tex_from_file(memory, &app->tex_chest, "data/textures/chest_small.png");
      asset_request_tex_from_file(memory, &app->tex_stone, "data/textures/stone.png");
      asset_request_tex_from_file(memory, &app->tex_shroom, "data/textures/shroom.png");
      asset_request_tex_from_file(memory, &app->tex_slime, "data/textures/slime.png");
      asset_request_tex_from_file(memory, &app->tex_spell_null, "data/textures/spells/spell0000.png");
      asset_request_tex_from_file(memory, &app->tex_spell_place_item, "data/textures/spells/spell0001.png");
      asset_request_tex_from_file(memory, &app->tex_spell_destroy, "data/textures/spells/spell0002.png");

      asset_request_tex_from_file(memory, &app->tex_wand, "data/textures/wand.png");
      
      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_FONT_FROM_FILE;
      request->filename = string("data/fonts/Inconsolata-Regular.ttf");
      request->font_uid = ARENA_PUSH_STRUCT(memory->temp_arena, u16);
      request->font_lines_height = 18.0f;
      // request->font_lines_height = 30.0f;

      DEFINE_ARRAY(char*, ied_names, memory->temp_arena, 
         {
            DEFAULT_IED_NAMES
         }
      );
      DEFINE_ARRAY(IE_FORMATS, ied_formats, memory->temp_arena, 
         {
            DEFAULT_IED_FORMATS
         }
      );
      Input_element_desc default_ied = {ied_names, ied_formats, 5};

      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_VS_FROM_FILE;
      request->filename = string("shaders/instancing_vs.cso");
      request->p_uid = &app->vs_instancing;
      request->ied = default_ied;

      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_VS_FROM_FILE;
      request->filename = string("shaders/simple_vs.cso");
      request->p_uid = &app->vs_simple;
      request->ied = default_ied;

      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_CREATE_BLEND_STATE;
      request->p_uid = &app->blend_state_enabled;
      request->enable_alpha_blending = true;

      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_CREATE_RTV;
      request->p_uid = &app->rtv_lowres;
      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_CREATE_DEPTH_STENCIL;
      request->enable_depth = false;
      request->p_uid = &app->depth_stencil_lowres;

      PUSH_ASSET_REQUEST;
      request->type = ASSET_REQUEST_CREATE_DYNAMIC_MESH;
      request->instancing_dynamic_mesh.max_instances = WORLD_X_LENGTH*WORLD_Y_LENGTH;
      request->instancing_dynamic_mesh.instance_size = sizeof(Instance_data);
      request->instancing_dynamic_mesh.topology_uid = TOPOLOGY_LINE_STRIP;
      request->p_uid = &app->dynamic_mesh_tiles;

      {
         PUSH_ASSET_REQUEST
         DEFINE_ARRAY(u16, plane_indices, memory->temp_arena,{0,1,2, 2,1,3});
         DEFINE_ARRAY(Vertex, plane, memory->temp_arena, 
            {
               {{0, 1.0f, 0}, 	{0, 0}, 	{0,0,-1.0f}, {1.0f}},
               {{1.0f, 1.0f, 0}, 	{1, 0},		{0,0,-1.0f}, {1.0f}},
               {{0, 0, 0}, 	{0, 1},		{0,0,-1.0f}, {1.0f}},
               {{1.0f, 0, 0}, 	{1, 1},	{0,0,-1.0f}, {1.0f}}
            }
         );

         request->type = ASSET_REQUEST_MESH_FROM_PRIMITIVES;
         request->p_uid = &app->mesh_plane_y_up;
         request->mesh_primitives.vertices = plane;
         request->mesh_primitives.vertex_size = sizeof(plane[0]);
         request->mesh_primitives.vertex_count = ARRAYLEN(plane);
         request->mesh_primitives.indices = plane_indices;
         request->mesh_primitives.indices_count = ARRAYLEN(plane_indices);
         request->mesh_primitives.topology_uid = TOPOLOGY_TRIANGLE_LIST;
      }
      return;
   }

   // APP INITIALIZATION

   if(!app->is_initialized)
   {
      app->is_initialized = 1;

      app->tiles_px_size = {8, 8};
      app->viewport_size = 2*int2(160, 90);

      app->ui.selection = NULL_UI_SELECTION;
      
      app->item_id_to_tex_uid[ITEM_1] = app->tex_stone;
      app->item_id_to_tex_uid[ITEM_WAND] = app->tex_wand;
      app->item_id_to_tex_uid[ITEM_SPELL_NULL] = app->tex_spell_null;
      app->item_id_to_tex_uid[ITEM_SPELL_PLACE_ITEM] = app->tex_spell_place_item;
      app->item_id_to_tex_uid[ITEM_SPELL_DESTROY] = app->tex_spell_destroy;


      //:INITIALIZE STATIC ITEMS
      UNTIL(id, ITEM_LAST_ID)
      {
         app->items[id].item_id = (Item_id)id;
         app->used_items[id] = true;
         if(id)
            app->items[id].item_count = 1;

         switch(id)
         {
            case ITEM_NULL:
            case ITEM_LAST_ID:
            case ITEM_SPELL_LAST: 
            break;
            
            case ITEM_1: 
            {
               app->items[id].inventory.size = 2;
               app->items[id].inventory.items[0] = ITEM_SPELL_DESTROY;
               app->items[id].inventory.items[1] = ITEM_SPELL_PLACE_ITEM;
            }
            break;
            case ITEM_SPELL_NULL: 
            case ITEM_SPELL_PLACE_ITEM:
            case ITEM_SPELL_DESTROY: 
            break;
            case ITEM_WAND: 
            {
               app->items[id].inventory.is_editable = true;
               app->items[id].inventory.size = 5;
               // TODO: this should be a dynamic inventory
               // app->items[id].inventory.is_editable = true;
            }
            break;
            
            default: ASSERT(false);
            break;
         }
      }

      
      app->used_entities[E_PLAYER_INDEX] = true;
      app->entities[E_PLAYER_INDEX].pos = {0, 0};
      app->entities[E_PLAYER_INDEX].flags = E_RENDER;
      app->entities[E_PLAYER_INDEX].unarmed_item = get_next_available_index(app->used_items, MAX_ITEMS, 0);
      app->items[app->entities[E_PLAYER_INDEX].unarmed_item].inventory.is_editable = true;
      app->items[app->entities[E_PLAYER_INDEX].unarmed_item].inventory.size = 2;
      app->items[app->entities[E_PLAYER_INDEX].unarmed_item].inventory.items[0] = ITEM_SPELL_DESTROY;
      app->items[app->entities[E_PLAYER_INDEX].unarmed_item].inventory.items[1] = 0;
   }

   //TODO: for now doing (memory->is_initialized = 0;) is dangerous
   if(input->keys[INPUT_T] == 1) app->is_initialized = 0;
   
   f32 aspect_ratio = (f32)app->viewport_size.x/app->viewport_size.y;

   if(memory->input->keys[INPUT_R] == 1)
   {
      f32* temp_texture  = ARENA_PUSH_STRUCTS(memory->temp_arena, f32, WORLD_X_LENGTH*WORLD_Y_LENGTH);
      UNTIL(i, WORLD_X_LENGTH*WORLD_Y_LENGTH)
      {
         temp_texture[i] = memory->rng.next(1.0f);   
      }

      UNTIL(y, WORLD_Y_LENGTH)
      {
         UNTIL(x, WORLD_X_LENGTH)
         {
            f32 noise_value = sample_2d_perlin_noise(temp_texture, WORLD_X_LENGTH, WORLD_Y_LENGTH, x, y, 1, WORLD_X_LENGTH/5, .75f);
            // if(noise_value > .5f)
            {
               // u32 palette_count = 2; 
               // app->world[y][x] = (u8)((u8)(noise_value*palette_count)*(256/palette_count));
               u32 center_x = WORLD_X_LENGTH/2;
               u32 center_y = WORLD_Y_LENGTH/2;

               if(noise_value < .5f || (center_x-10 < x && x < center_x+10 && center_y-10 < y && y < center_y+10)){
                  app->world[y][x] = 0;
               }else{
                  app->world[y][x] = 1;
               }
            }
         }
      }
   }

   app->entities[E_PLAYER_INDEX].inventory.selected_slot = (app->entities[E_PLAYER_INDEX].inventory.selected_slot+10-input->delta_wheel)%10;

   if(memory->input->keys[INPUT_TAB] == 1)
   {
      app->is_menu_opened = !app->is_menu_opened;
   }


   V2 input_direction = {
      (f32)((input->keys[INPUT_D] > 0) - (input->keys[INPUT_A] > 0)),
      (f32)((input->keys[INPUT_W] > 0) - (input->keys[INPUT_S] > 0))
   };
   
   V2 normalized_input_direction = v2_normalize(input_direction);
   app->entities[E_PLAYER_INDEX].move_direction = normalized_input_direction;



   // UI PROCESSING

   app->ui.current_widget_uid = 0;
   V2 cursor_screen_pos = {input->cursor_pos.x*aspect_ratio, input->cursor_pos.y};
   V2 cursor_world_pos = app->camera_pos.v2 + cursor_screen_pos;
   app->ui.selection.hot = NULL_INDEX16;
   
   b32 is_mouse_in_ui = 0;

   #define INVENTORY_ROWS_LENGTH 10

   UNTIL(i, MAX_UI)
   {
      if(!(app->ui.widgets[i].flags & UI_SKIP_RENDERING))
      {
         Rect_float widget_rect = {app->ui.global_positions[i], app->ui.widgets[i].style.size};
         if(point_vs_rect_float(cursor_screen_pos, widget_rect))
         {
            if(app->ui.widgets[i].flags & UI_CLICKABLE)
            {
               app->ui.selection.hot = (u16)i;
            }
            is_mouse_in_ui = true;
         }
      }

      app->ui.widgets[i] = {0};
   }

   app->ui.selection.clicked = NULL_INDEX16;
   app->ui.selection.clicked2 = NULL_INDEX16;

   if(input->keys[INPUT_CURSOR_PRIMARY] == 1){
      app->ui.selection.pressed = app->ui.selection.hot;
   }else if(input->keys[INPUT_CURSOR_PRIMARY] == -1){
      if(app->ui.selection.pressed == app->ui.selection.hot){
         app->ui.selection.clicked = app->ui.selection.pressed;
      }
      app->ui.selection.pressed = NULL_INDEX16;
   }
   
   if(input->keys[INPUT_CURSOR_SECONDARY] == 1){
      app->ui.selection.pressed2 = app->ui.selection.hot;
   }else if(input->keys[INPUT_CURSOR_SECONDARY] == -1){
      if(app->ui.selection.pressed2 == app->ui.selection.hot){
         app->ui.selection.clicked2 = app->ui.selection.pressed2;
      }
      app->ui.selection.pressed2 = NULL_INDEX16;
   }

   // AAAAAAAAAAAAAAAAAAAA

   // ROOT WIDGET
   ui_push(&app->ui, do_widget(&app->ui, UI_SKIP_RENDERING, {0}));
   {
      u32 rows = 1;
      u32 visible_slots_count = 10;

      if(app->is_menu_opened)
      {
         rows = (INVENTORY_SIZE+10-1)/10;
         visible_slots_count = INVENTORY_SIZE;
      }

      f32 slots_size = .15f;
      
      Ui_style test_style = {0};
      test_style.layout = UI_LAYOUT_GRID_ROW_MAJOR;
      test_style.line_size = INVENTORY_ROWS_LENGTH;
      test_style.cells_border_size = 0.005f;
      test_style.size = {1.5f, slots_size*rows};
      test_style.pos = {-test_style.size.x/2, .9f - slots_size*rows};
      test_style.color_rect = {.8f,.8f,.8f,1};
      ui_push(&app->ui, do_widget(&app->ui, 0, test_style));

      UNTIL(i, INVENTORY_SIZE)
      {
         test_style = {0};
         if(i >= visible_slots_count)
         {
            ui_pop(&app->ui);
            do_widget(&app->ui, UI_SKIP_RENDERING, test_style);
            do_widget(&app->ui, UI_SKIP_RENDERING, test_style);
         }
         else
         {

            test_style.color_rect = {.5f, .5f, .5f, 1};
            if(app->entities[E_PLAYER_INDEX].inventory.selected_slot == (s32)i){
               test_style.color_rect.rgb = {1, 1, 1};
            }
            
            test_style.tex_uid = app->item_id_to_tex_uid[app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_id];
            u16 slot_uid = ui_push(&app->ui, do_widget(&app->ui, UI_CLICKABLE, test_style));
            {
               if(app->ui.selection.clicked == slot_uid)
               {
                  if(app->is_menu_opened)
                  {
                     //TODO: clicking the slots while the menu is opened
                     u16 temp_item = app->cursor_item;
                     app->cursor_item = app->entities[E_PLAYER_INDEX].inventory.items[i];
                     app->entities[E_PLAYER_INDEX].inventory.items[i] = temp_item;
                  }
                  else
                  {
                     ASSERT(i < 0xff);
                     app->entities[E_PLAYER_INDEX].inventory.selected_slot = (u8)i;
                  }
               }
               
               if(app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_count > 1)
               {
                  test_style.text = u32_to_string(app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_count, memory->temp_arena);
               }
               do_widget(&app->ui, UI_SKIP_RENDERING, test_style);

            }
            ui_pop(&app->ui);
         }
      }
      ui_pop(&app->ui);


      //:UI SPELLS INVENTORY
      u16 equipped_item = app->entities[E_PLAYER_INDEX].inventory.items[app->entities[E_PLAYER_INDEX].inventory.selected_slot];
      if(!equipped_item) {
         equipped_item = app->entities[E_PLAYER_INDEX].unarmed_item;
      }
      
      u32 spells_ui_flags = 0;
      if(!app->items[equipped_item].inventory.size) 
      {
         spells_ui_flags = UI_SKIP_RENDERING;
      }

      test_style = {0};
      test_style.layout = UI_LAYOUT_GRID_ROW_MAJOR;
      test_style.line_size = INVENTORY_ROWS_LENGTH;
      test_style.line_size = app->items[equipped_item].inventory.size;
      test_style.cells_border_size = 0.005f;
      test_style.size = {slots_size*app->items[equipped_item].inventory.size, slots_size};
      test_style.pos = {aspect_ratio-test_style.size.x - .01f, -.99f};
      test_style.color_rect = {.8f,.8f,.8f,1};

      ui_push(&app->ui, do_widget(&app->ui, spells_ui_flags, test_style));
      {
         //TODO: instead of inventory.size use a constant value to keep uid's universal
         UNTIL(spell, app->items[equipped_item].inventory.size)
         {
            Ui_style temp_style = {0};
            u16 spell_item_uid = app->items[equipped_item].inventory.items[spell];
            temp_style.tex_uid = app->item_id_to_tex_uid[app->items[spell_item_uid].item_id];
            temp_style.color_rect = {.3f,.3f,.3f,1};
            // if(app->items[app->items[equipped_item].inventory[spell]].item_count > 1)
            // {
            //    test_style.text = u32_to_string(app->items[equipped_item].inventory[spell].item_count, memory->temp_arena);
            // }
            if(app->items[equipped_item].inventory.selected_slot == spell)
            {
               temp_style.color_rect = {1,1,1,1};
            }
            if(app->items[spell_item_uid].item_count > 1)
            {
               temp_style.text = u32_to_string(app->items[spell_item_uid].item_count, memory->temp_arena);
            }
            if(app->ui.selection.clicked == do_widget(&app->ui, UI_CLICKABLE, temp_style) && app->items[equipped_item].inventory.is_editable)
            {
               if(app->is_menu_opened)
               {
                  //TODO: clicking the slots while the menu is opened
                  u16 temp_item = app->cursor_item;
                  app->cursor_item = spell_item_uid;
                  app->items[equipped_item].inventory.items[spell] = temp_item;
               }
            }
         }
      }ui_pop(&app->ui);


      //:CURSOR ITEM SLOT
      u32 flag = 0;
      if(!(app->is_menu_opened && app->cursor_item))
      {
         flag = UI_SKIP_RENDERING;
      }
      test_style.tex_uid = app->item_id_to_tex_uid[app->items[app->cursor_item].item_id];
      test_style.color_rect = {.99f,.99f,.99f,1};
      test_style.size = {slots_size, slots_size};
      test_style.pos = cursor_screen_pos - v2(.75f*test_style.size.x, .25f*test_style.size.y);
      if(app->items[app->cursor_item].item_count > 1)
      {
         test_style.text = u32_to_string(app->items[app->cursor_item].item_count, memory->temp_arena);
      }
      do_widget(&app->ui, flag, test_style);
   }
   ui_pop(&app->ui);


   
   // BUILDING UI LAYOUTS

   UNTIL(i, MAX_UI)
   {
      u16 parent_uid = app->ui.widgets[i].parent_uid;
      if(app->ui.widgets[parent_uid].style.layout == UI_LAYOUT_DEFAULT)
      {
         app->ui.global_positions[i] = app->ui.widgets[i].style.pos + app->ui.global_positions[parent_uid];
      }
      else if(app->ui.widgets[parent_uid].style.layout == UI_LAYOUT_GRID_ROW_MAJOR)
      {
         f32 border_size = app->ui.widgets[parent_uid].style.cells_border_size;
         int x_index = app->ui.widgets[i].local_uid % app->ui.widgets[parent_uid].style.line_size;
         f32 full_x_size = (app->ui.widgets[parent_uid].style.size.x / app->ui.widgets[parent_uid].style.line_size);
         f32 x_cell_size = full_x_size - (border_size*2);
         f32 x_pos = x_index * full_x_size + (border_size);
         
         int y_index = app->ui.widgets[i].local_uid / app->ui.widgets[parent_uid].style.line_size;
         f32 y_pos = y_index * full_x_size + (border_size);


         V2 parent_upper_left_corner = 
         {
            app->ui.global_positions[parent_uid].x, 
            app->ui.global_positions[parent_uid].y + app->ui.widgets[parent_uid].style.size.y - x_cell_size
         };
         app->ui.global_positions[i] = parent_upper_left_corner + v2(x_pos, -y_pos);
         app->ui.widgets[i].style.size = {x_cell_size, x_cell_size};
      }
   }

   // END OF UI PROCESSING

   V2 tiles_screen_size = size_in_pixels_to_screen(app->tiles_px_size, aspect_ratio, app->viewport_size);

   if(!is_mouse_in_ui){
      app->cursor_tile_pos.x = (int)(((app->entities[E_PLAYER_INDEX].pos.x + (aspect_ratio*input->cursor_pos.x)) / tiles_screen_size.x) + WORLD_X_LENGTH/2);
      app->cursor_tile_pos.y = (int)(((app->entities[E_PLAYER_INDEX].pos.y + input->cursor_pos.y) / tiles_screen_size.y) + WORLD_Y_LENGTH/2);
   }else{
      app->cursor_tile_pos = {NULL_INDEX16, NULL_INDEX16};
   }

   if(0 <= app->cursor_tile_pos.x && app->cursor_tile_pos.x < WORLD_X_LENGTH
   && 0 <= app->cursor_tile_pos.y && app->cursor_tile_pos.y < WORLD_Y_LENGTH 
   )
   {
      if(input->keys[INPUT_CONTROL] > 0 && input->keys[INPUT_CURSOR_PRIMARY] == 1)
      {
         app->entities[0].pos = tile_to_pos(app->cursor_tile_pos, tiles_screen_size);
      }
      
      if(input->keys[INPUT_ALT] > 0 )
      {
         if(input->keys[INPUT_CURSOR_PRIMARY] == 1)
         {
            if(app->world[app->cursor_tile_pos.y][app->cursor_tile_pos.x] != 0)
            {
               destroy_tile(app, app->cursor_tile_pos, tiles_screen_size);
            }
         }
         else if(input->keys[INPUT_CURSOR_SECONDARY] == 1)
         {
            app->world[app->cursor_tile_pos.y][app->cursor_tile_pos.x] = 1;
         }
      }
   }
   
   {
      //TODO: this will be moving across the spells inventory
      u16 equipped_item = get_entity_equipped_item_index(app, E_PLAYER_INDEX);
      app->items[equipped_item].inventory.selected_slot = 0;

      app->entities[E_PLAYER_INDEX].target_direction = v2_normalize(cursor_world_pos - app->entities[E_PLAYER_INDEX].pos);
      if(!is_mouse_in_ui)
      {
         if(input->keys[INPUT_CURSOR_PRIMARY] > 0 || input->keys[INPUT_CURSOR_SECONDARY] > 0)
         {
            app->entities[E_PLAYER_INDEX].is_casting = true;
            if(input->keys[INPUT_CURSOR_SECONDARY] > 0)
            {
               app->items[equipped_item].inventory.selected_slot = app->items[equipped_item].inventory.size - 1;
            }
         }

         if(input->keys[INPUT_F1] == 1)
         {
            u16 debug_wand_pickup_uid = get_next_available_index(app->used_entities, MAX_ENTITIES, 0);
            app->entities[debug_wand_pickup_uid].flags = E_RENDER|E_PICKUP;
            u16 debug_wand_item_uid = get_next_available_index(app->used_items, MAX_ITEMS, 0);
            app->entities[debug_wand_pickup_uid].item = debug_wand_item_uid;
            app->items[debug_wand_item_uid] = app->items[ITEM_WAND];

            app->entities[debug_wand_pickup_uid].pos = cursor_world_pos;
         }
      }
   }


   f32 movement_speed = 0.5f;
   if(input->keys[INPUT_SHIFT] > 0)
   {
      movement_speed = 10.0f;
   }

   //:UPDATE ENTITIES
   #define DEFAULT_RANGE .2f
   
   UNTIL(e, MAX_ENTITIES)
   {
      // f32 x_factor = 160.0f/90;e2_normalize(cursor_world_pos - app->entities[E_PLAYER_INDEX].pos.v2);
      if(app->used_entities[e])
      {
         app->entities[e].casting_cooldown -= memory->fixed_dt;
         if(app->entities[e].is_casting && app->entities[e].casting_cooldown <= 0)
         {
            app->entities[e].casting_cooldown = .5f;
            
            u16 equipped_item = app->entities[e].inventory.items[app->entities[e].inventory.selected_slot];
            if(!equipped_item){
               equipped_item = app->entities[e].unarmed_item;
            }

            Item_id spell = app->items[app->items[equipped_item].inventory.items[app->items[equipped_item].inventory.selected_slot]].item_id;
            switch(spell)
            {
               case ITEM_SPELL_NULL:
               break;
               case ITEM_SPELL_PLACE_ITEM:
               {
                  V2 placing_world_pos = app->entities[e].pos + DEFAULT_RANGE*app->entities[e].target_direction;
                  Int2 placing_tilemap_pos = pos_to_tile(placing_world_pos, tiles_screen_size);

                  if(!app->world[placing_tilemap_pos.y][placing_tilemap_pos.x])
                  {
                     //TODO: placeable objects will have their own tile_uid
                     app->world[placing_tilemap_pos.y][placing_tilemap_pos.x] = 1;
                     //WHAT IN THE ACTUAL FUCK
                     app->items[equipped_item].item_count -= 1;
                     
                     ASSERT(app->items[equipped_item].item_count >= 0);
                     if(app->items[equipped_item].item_count == 0)
                     {
                        app->used_items[equipped_item] = 0;
                        app->items[equipped_item] = {0};
                        app->entities[e].inventory.items[app->entities[e].inventory.selected_slot] = 0;
                     }
                  }
               }
               break;
               case ITEM_SPELL_DESTROY:
               {
                  //TODO: axis aligned 2d ray marching
                  V2 current_pos = app->entities[e].pos;
                  UNTIL(step, 10)
                  {
                     V2 test_world_pos = current_pos + (step*DEFAULT_RANGE/10)*app->entities[e].target_direction;
                     Int2 test_tilemap_pos = pos_to_tile(test_world_pos, tiles_screen_size);
                     if(app->world[test_tilemap_pos.y][test_tilemap_pos.x])
                     {
                        destroy_tile(app, test_tilemap_pos, tiles_screen_size);
                        break;
                     }

                  }
               }
               break;
               default:
               break;
            }
         }
         app->entities[e].is_casting = false;
         
         UNTIL(e2, MAX_ENTITIES)
         {
            if(app->used_entities[e2] && e != e2)
            {
               //:PICKUP
               if(app->entities[e].flags & E_PICKUP && !(app->entities[e2].flags & E_PICKUP))
               {
                  f32 distance = v2_magnitude(app->entities[e2].pos - app->entities[e].pos);
                  if(distance < 0.1f)
                  {
                     u16 first_empty_slot = NULL_INDEX16;
                     u16 total_count = 0;
                     UNTIL(slot, INVENTORY_SIZE)
                     {
                        u16 e2_item_uid = app->entities[e2].inventory.items[slot];

                        if(!app->items[app->entities[e].item].inventory.is_editable // if it's editable, it's not stackable
                        && app->items[app->entities[e].item].item_id == app->items[e2_item_uid].item_id
                        && app->items[app->entities[e2].inventory.items[slot]].item_count < MAX_ITEM_STACK)
                        {
                           total_count = app->items[app->entities[e].item].item_count + app->items[e2_item_uid].item_count;

                           app->items[e2_item_uid].item_count = MIN(total_count, MAX_ITEM_STACK);
                           
                           app->used_items[app->entities[e].item] = 0;
                           app->items[app->entities[e].item] = {0};
                           break;
                        }
                        if(first_empty_slot == NULL_INDEX16 && app->items[app->entities[e2].inventory.items[slot]].item_id == ITEM_NULL)
                        {
                           first_empty_slot = (u16)slot;
                        }
                     }
                     if(!total_count)
                     {
                        total_count = app->items[app->entities[e].item].item_count;
                        app->entities[e2].inventory.items[first_empty_slot] = app->entities[e].item;
                        app->items[app->entities[e].item].item_count = MIN(total_count, MAX_ITEM_STACK);

                        app->entities[e].item = 0;
                        if(total_count > MAX_ITEMS)
                        {
                           u16 new_item_uid = get_next_available_index(app->used_items, MAX_ITEMS, 0);
                           app->entities[e].item = new_item_uid;
                           app->items[new_item_uid] =  app->items[app->entities[e2].inventory.items[first_empty_slot]];
                           app->items[new_item_uid].item_count = total_count - MAX_ITEM_STACK;
                        }
                     }

                     if(total_count < MAX_ITEMS)
                     {
                        // DESTROY ENTITY
                        app->used_entities[e] = 0;
                        app->entities[e] = {0};
                     }
                  }
               }
            }
         }


         //:UPDATE ENTITY POSITIONS

         V2 new_pos = app->entities[e].pos + (movement_speed*memory->fixed_dt*app->entities[e].move_direction);
         Int2 tile_pos = pos_to_tile(new_pos, tiles_screen_size);
         
         if(!app->world[tile_pos.y][tile_pos.x]){
            app->entities[e].pos = new_pos;
         }
      }
   }
   
   app->camera_pos.v2 = app->entities[E_PLAYER_INDEX].pos;
}

void render(Platform_data* memory, App_data* app, LIST(Renderer_request, render_list), Int2 client_size)
{memory, app, render_list, client_size;
   Font* default_font = memory->fonts_list[0];
   Renderer_request* request = 0;

   memory->bg_color = {.5f, .0f, .5f, 1};

   // if(!app->is_initialized)
   // {
   //    memory->bg_color = {.1f,.1f,.1f, 1};
   //    return;
   // }

   // Int2 virtual_client_size = 2*int2(160, 90);
   Int2 virtual_client_size = client_size;
   app->viewport_size = 2*int2(160, 90);
   f32 aspect_ratio = (f32)app->viewport_size.x/app->viewport_size.y;
   
   V2 tiles_screen_size = size_in_pixels_to_screen(app->tiles_px_size, aspect_ratio, app->viewport_size);

   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_RESIZE_TARGET_VIEW
      |REQUEST_FLAG_RESIZE_DEPTH_STENCIL_VIEW
      |REQUEST_FLAG_CHANGE_VIEWPORT_SIZE
      |REQUEST_FLAG_SET_RENDER_TARGET_AND_DEPTH_STENCIL
      |REQUEST_FLAG_SET_PS
      |REQUEST_FLAG_SET_VS
      |REQUEST_FLAG_CLEAR_RTV
      |REQUEST_FLAG_SET_BLEND_STATE
   ;
   request->resize_depth_stencil_view_uid = app->depth_stencil_lowres;
   request->resize_rtv_uid = app->rtv_lowres;
   request->new_viewport_size = virtual_client_size;
   request->set_rtv_count = 1;
   request->clear_rtv.uid = app->rtv_lowres;
   request->clear_rtv.color = {.5f,.5f,.5f,1};
   request->set_rtv_uids = &app->rtv_lowres;
   request->set_depth_stencil_uid = app->depth_stencil_lowres;
   {
      PUSH_BACK_RENDER_REQUEST(render_list);
      request->type_flags = REQUEST_FLAG_MODIFY_RENDERER_VARIABLE;
      request->renderer_variable.register_index = REGISTER_INDEX_VS_WORLD_VIEW_MATRIX;

      request->renderer_variable.new_data = ARENA_PUSH_STRUCT(memory->temp_arena, Matrix);
      *(Matrix*)request->renderer_variable.new_data= 
         matrix_translation( v3_invert(app->camera_pos) ) *
         matrix_from_quaternion(quaternion_invert(UNIT_QUATERNION))
      ;
      
      PUSH_BACK_RENDER_REQUEST(render_list);
      request->type_flags = REQUEST_FLAG_MODIFY_RENDERER_VARIABLE;
      request->renderer_variable.register_index = REGISTER_INDEX_VS_PROJECTION_MATRIX;

      request->renderer_variable.new_data = ARENA_PUSH_STRUCT(memory->temp_arena, Matrix);
      *(Matrix*)request->renderer_variable.new_data = build_orthographic_matrix(aspect_ratio, 2.0f, 0.001f, 100.0f);
   }
   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_RENDER_INSTANCES
   |REQUEST_FLAG_SET_VS
   ;
   request->vshader_uid = app->vs_instancing;
   request->instancing_data.dynamic_instances_mesh = app->dynamic_mesh_tiles;
   request->instancing_data.mesh_uid = app->mesh_plane_y_up;
   request->instancing_data.texinfo_uid = app->tex_stone;

   Int2 camera_tile_pos;
   // camera_tile_pos.x = (int)((app->player_pos.x)*8.0f) + WORLD_X_LENGTH/2;
   // camera_tile_pos.y = (int)((app->player_pos.y)*8.0f) + WORLD_Y_LENGTH/2;

   camera_tile_pos.x = (s32)((app->camera_pos.x/tiles_screen_size.x)+ WORLD_X_LENGTH/2);
   camera_tile_pos.y = (s32)((app->camera_pos.y/tiles_screen_size.y)+ WORLD_Y_LENGTH/2);

   Int2 render_rect_size = {42,26};

   Int2 render_rect_min, render_rect_max;
   render_rect_min.x = CLAMP(0, camera_tile_pos.x - render_rect_size.x/2, WORLD_X_LENGTH-render_rect_size.x);
   render_rect_min.y = CLAMP(0, camera_tile_pos.y - render_rect_size.y/2, WORLD_Y_LENGTH-render_rect_size.x);

   render_rect_max.x = CLAMP(render_rect_size.x, camera_tile_pos.x + render_rect_size.x/2, WORLD_X_LENGTH);
   render_rect_max.y = CLAMP(render_rect_size.y, camera_tile_pos.y + render_rect_size.y/2, WORLD_Y_LENGTH);

   request->instancing_data.instances_count = render_rect_size.x*render_rect_size.y;
   request->instancing_data.instances = ARENA_PUSH_STRUCTS(memory->temp_arena, Instance_data, request->instancing_data.instances_count);
   
   for(int y=render_rect_min.y; y < render_rect_max.y; y++)
   {
      f32 y_final_pos = ((f32)(y - WORLD_Y_LENGTH/2)*tiles_screen_size.y);
      for(int x = render_rect_min.x; x < render_rect_max.x; x++)
      {
         f32 x_final_pos = ((f32)(x - WORLD_X_LENGTH/2)*tiles_screen_size.x);
         {
            if(app->world[y][x])
            {
               Instance_data* instance = &request->instancing_data.instances[(y-render_rect_min.y)*render_rect_size.x + (x-render_rect_min.x)];
               // instance->object_transform =  
               //    matrix_scale(v3(tiles_screen_size.x, tiles_screen_size.y, 1))*
               //    // matrix_scale(v3(0.01f, 0.01f, 1))*
               //    matrix_translation(v3(x_final_pos, y_final_pos,0))
               // ;
               instance->object_transform = pos_scale_rot_to_transform_matrix(
                  v3(x_final_pos, y_final_pos, 0),
                  v3(tiles_screen_size.x, tiles_screen_size.y, 1),
                  UNIT_QUATERNION
               );
               if(x==camera_tile_pos.x && y==camera_tile_pos.y)
               {
                  instance->color = {1,1,1,1};
               }
               else if(x==app->cursor_tile_pos.x && y==app->cursor_tile_pos.y)
               {
                  instance->color = {1,1,0,1};
               }
               else{
                  f32 color_value = .99f;
                  instance->color = {color_value,color_value,color_value,1};
                  // instance->color = {((f32)(x%8)/8), ((f32)(y%8)/8), 0, 1};
               }
               instance->texrect = {0,0,1,1};
            }
         }
      }
   }
   
   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_SET_VS;
   request->vshader_uid = 0;
   
   
   UNTIL(e, MAX_ENTITIES)
   {
      if(app->entities[e].flags & E_RENDER)
      {
         PUSH_BACK_RENDER_REQUEST(render_list);
         request->type_flags = REQUEST_FLAG_RENDER_OBJECT
         ;
         request->object3d.default();
         if(e == E_PLAYER_INDEX){
            request->object3d.texinfo_uid = app->tex_player;
         }else{
            request->object3d.texinfo_uid = app->item_id_to_tex_uid[app->items[app->entities[e].item].item_id];
         }

         Tex_info* texinfo; LIST_GET(memory->tex_infos, request->object3d.texinfo_uid, texinfo);
         request->scale = {
            (2*aspect_ratio*(f32)texinfo->w)/app->viewport_size.x,
            (2*(f32)texinfo->h)/app->viewport_size.y,
            1
         };
         if(app->entities[e].flags & E_PICKUP)
         {
            request->scale = .5f*request->scale;
         }
         request->pos.x = app->entities[e].pos.x - request->scale.x/2;
         request->pos.y = app->entities[e].pos.y + request->scale.y/2;
      }
   }

   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_MODIFY_RENDERER_VARIABLE
   |REQUEST_FLAG_SET_BLEND_STATE
   ;
   request->blend_state_uid = app->blend_state_enabled;
   request->renderer_variable.register_index = REGISTER_INDEX_VS_WORLD_VIEW_MATRIX;
   {

      request->renderer_variable.new_data = ARENA_PUSH_STRUCT(memory->temp_arena, Matrix);
      *(Matrix*)request->renderer_variable.new_data = matrix_translation(v3(0,0,0));
   }

   #if 1
      UNTIL(i, app->ui.current_widget_uid)
      {
         if(app->ui.widgets[i].flags != UI_SKIP_RENDERING)
         {
            PUSH_BACK_RENDER_REQUEST(render_list);
            request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
            request->object3d.default();
            request->object3d.mesh_uid = app->mesh_plane_y_up;
            request->object3d.texinfo_uid = app->ui.widgets[i].style.tex_uid;
            if(request->object3d.texinfo_uid != 0)
            {
               ASSERT(true);
            }
            
            V3 widget_pos = {app->ui.global_positions[i].x, app->ui.global_positions[i].y, 0};
            V3 widget_scale = {app->ui.widgets[i].style.size.x, app->ui.widgets[i].style.size.y, 1};
            request->object3d.pos = widget_pos;
            request->object3d.scale = widget_scale;
            
            request->object3d.color = app->ui.widgets[i].style.color_rect;
            if(app->ui.selection.pressed == i){
               request->object3d.color.rgb = {.9f, .5f, 0};
            }else if(app->ui.selection.hot == i){
               request->object3d.color.rgb = {1,0,1};
            }
         }
         //TODO: extract this to instancing once i solve the zpos thing
         if(app->ui.widgets[i].style.text.length && app->ui.widgets[i].style.text.text)
         {
            V3 widget_pos = {app->ui.global_positions[i].x, app->ui.global_positions[i].y, 0};
            V3 widget_scale = {app->ui.widgets[i].style.size.x, app->ui.widgets[i].style.size.y, 1};
            f32 current_x_pos = 0;
            UNTIL(c, app->ui.widgets[i].style.text.length)
            {
               PUSH_BACK_RENDER_REQUEST(render_list);
               request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
               request->object3d.default();
               request->object3d.mesh_uid = app->mesh_plane_y_up;
               
               char character = app->ui.widgets[i].style.text.text[c];
               request->object3d.texinfo_uid = default_font->texinfo_uids[character-default_font->first_char];
               
               Tex_info* texinfo;
               LIST_GET(memory->tex_infos, request->object3d.texinfo_uid, texinfo);
               
               request->object3d.pos.v2 = v2(widget_pos.x + current_x_pos, widget_pos.y+0.005f);
               request->object3d.scale.v2 = v2(aspect_ratio*texinfo->w/app->viewport_size.x, 1.0f*texinfo->h/app->viewport_size.y);
               current_x_pos += request->object3d.scale.x;
            }
         }
      }
   #else
      PUSH_BACK_RENDER_REQUEST(render_list);
      request->type_flags = REQUEST_FLAG_RENDER_INSTANCES
      |REQUEST_FLAG_SET_VS
      |REQUEST_FLAG_SET_BLEND_STATE
      ;
      request->blend_state_uid = app->blend_state_enabled;
      request->vshader_uid = app->vs_instancing_static;
      request->instancing_data.dynamic_instances_mesh = app->dynamic_mesh_tiles;
      request->instancing_data.mesh_uid = app->mesh_plane_y_up;
      request->instancing_data.texinfo_uid = 0;
      request->instancing_data.instances = ARENA_PUSH_STRUCTS(memory->temp_arena, Instance_data, 0);

      UNTIL(i, MAX_UI)
      {
         if(app->ui.widgets[i].flags != UI_SKIP_RENDERING)
         {
            Instance_data* current_instance = ARENA_PUSH_STRUCT(memory->temp_arena, Instance_data);
            current_instance->color = app->ui.widgets[i].style.color_rect;
            if(app->ui.selection.pressed == i){
               current_instance->color.rgb = {.9f, .5f, 0};
            }else if(app->ui.selection.hot == i){
               current_instance->color.rgb = {1,0,1};
            }

            // V3 widget_pos = {app->ui.widgets[i].style.pos.x, app->ui.widgets[i].style.pos.y, 0};
            V3 widget_pos = {app->ui.global_positions[i].x, app->ui.global_positions[i].y, 0};
            V3 widget_scale = {app->ui.widgets[i].style.size.x, app->ui.widgets[i].style.size.y, 1};
            current_instance->object_transform = pos_scale_rot_to_transform_matrix(
               widget_pos,
               widget_scale,
               UNIT_QUATERNION
            );
            request->instancing_data.instances_count++;
         }
      }
   #endif
   
   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_SET_VS;
   request->vshader_uid = app->vs_simple;

   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_SET_SHADER_RESOURCE_FROM_RENDER_TARGET
   |REQUEST_FLAG_SET_RENDER_TARGET_AND_DEPTH_STENCIL
   |REQUEST_FLAG_CHANGE_VIEWPORT_SIZE
   ;
   request->set_shader_resource_from_rtv.rtv_uid = app->rtv_lowres;
   request->set_shader_resource_from_rtv.target_index = 0;
   request->set_rtv_count = 1;
   request->set_rtv_uids = ARENA_PUSH_STRUCT(memory->temp_arena, u16);
   request->set_depth_stencil_uid = 0;
   request->new_viewport_size = client_size;

   
   
   PUSH_BACK_RENDER_REQUEST(render_list);
   request->type_flags = REQUEST_FLAG_RENDER_OBJECT;
   request->object3d.default();
   request->pos = {-1,1,0};
   request->scale = {2, 2.0f, 1};
   request->object3d.texinfo_uid = NULL_INDEX16;
}

void close_app(Platform_data* memory, App_data* app)
{memory, app;
   
}
