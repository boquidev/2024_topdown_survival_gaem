#include "game.h"

void render(Platform_data* memory, App_data* app, LIST(Renderer_request, render_list), Int2 client_size)
{memory, app, render_list, client_size;
   Font* default_font = memory->fonts_list[0];
   Renderer_request* request = 0;

   memory->bg_color = {.5f, .0f, .5f, 1};


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