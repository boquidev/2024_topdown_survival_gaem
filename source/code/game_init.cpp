#include "game.h"

void init(Platform_data* memory, App_data* app, Init_data* init_data)
{memory, app, init_data;

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

#define PUSH_ASSET_REQUEST PUSH_BACK(init_data->asset_requests, memory->temp_arena, request)

   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/player.png");
   request->p_uid = &app->tex_player;

   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/null.png");
   request->p_uid = &app->tex_null;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/chest_small.png");
   request->p_uid = &app->tex_chest;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/stone.png");
   request->p_uid = &app->tex_stone;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/shroom.png");
   request->p_uid = &app->tex_shroom;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/slime.png");
   request->p_uid = &app->tex_slime;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/spells/spell0000.png");
   request->p_uid = &app->tex_spell_null;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/spells/spell0001.png");
   request->p_uid = &app->tex_spell_place_item;
   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/spells/spell0002.png");
   request->p_uid = &app->tex_spell_destroy;

   PUSH_ASSET_REQUEST;
   request->type = ASSET_REQUEST_TEX_FROM_FILE;
   request->filename = string("data/textures/wand.png");
   request->p_uid = &app->tex_wand;
   
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
   request->type = ASSET_REQUEST_VS_FROM_FILE;
   request->filename = string("shaders/vs_instancing_static.cso");
   request->p_uid = &app->vs_instancing_static;
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
}