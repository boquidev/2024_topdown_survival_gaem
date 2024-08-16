#include "game.h"
#include "game_init.cpp"
#include "game_render.cpp"

void update(Platform_data* memory, App_data* app, Audio audio, Int2 client_size)
{memory, app, audio, client_size;
   User_input* input = memory->input;

   if(!memory->is_initialized)
   {
      app->tiles_px_size = {8, 8};
      memory->is_initialized = 1;
      app->viewport_size = 2*int2(160, 90);

      app->ui.selection = NULL_UI_SELECTION;
      
      app->item_id_to_tex_uid[ITEM_1] = app->tex_stone;
      app->item_id_to_tex_uid[ITEM_WAND] = app->tex_wand;
      app->item_id_to_tex_uid[ITEM_SPELL_NULL] = app->tex_spell_null;
      app->item_id_to_tex_uid[ITEM_SPELL_PLACE_ITEM] = app->tex_spell_place_item;
      app->item_id_to_tex_uid[ITEM_SPELL_DESTROY] = app->tex_spell_destroy;


      //:INITIALIZE STATIC ITEMS
      //TODO: this will be the unarmed index of the entity instead of ITEM_NULL
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
   if(input->keys[INPUT_T] == 1) memory->is_initialized = 0;
   
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
            app->items[app->items[equipped_item].inventory.items[app->items[equipped_item].inventory.selected_slot]].item_id;

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

void close_app(Platform_data* memory, App_data* app)
{memory, app;
   
}