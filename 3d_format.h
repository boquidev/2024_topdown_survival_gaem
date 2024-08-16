

enum BLOCK_TYPES_3D_ASSET_FORMAT
{
   BT3D_NULL,
   BT3D_MESH,

   BT3D_JOINT_POSITIONS,
   BT3D_BONE_JOINTS_INDICES,
   BT3D_ANIMATIONS,

   BT3D_MATERIALS, //TODO:

   BT3D_LAST_ENTRY,
};

enum MESH_BLOCK_ENTRY_TYPE
{
   MESH_ET_NULL,

   MESH_ET_HEADER,
   MESH_ET_NAME,
   MESH_ET_VPOSITIONS,
   MESH_ET_VTEXCOORDS,
   MESH_ET_VNORMALS,
   MESH_ET_VCOLORS,
   MESH_ET_VWEIGHT_INDICES,
   MESH_ET_VWEIGHT_VALUES,
   MESH_ET_INDICES,

   MESH_ET_LAST_ENTRY,
};

enum ANIM_BLOCK_ENTRY_TYPE
{
   ANIM_ET_NULL,
   ANIM_ET_KEYFRAME_TIMES,
   ANIM_ET_KEYFRAME_BONE_POSES,
   ANIM_ET_BONES_KEYFRAMES_COUNT,

   ANIM_ET_LAST_ENTRY,
};

struct Block_entry
{
   union
   {
      BLOCK_TYPES_3D_ASSET_FORMAT block_type;
      MESH_BLOCK_ENTRY_TYPE mesh_block_type;
      ANIM_BLOCK_ENTRY_TYPE anim_block_type;
   };
   u32 offset;
   u32 count;
   u32 size;
};


struct Imported_mesh
{
	u32 vertex_count;
	
   struct {
      V3* positions;
      V2* texcoords;
      V3* normals;
      Color* colors;
      Indices_triad16* weight_indices;
      V3* weight_values;
   }vertices;

	u32 indices_count;
	u32 topology_uid;
   union
   {
      Triangle* triangles;
      u16* indices;
   };
};
struct Imported_animation
{
   f32 length;
   u16 keyframes_count;
   u16 bones_count;

   f32* keyframe_times;
   Bone* keyframe_bone_poses;

   u16* bones_keyframes_count;
   u16* bones_root_keyframe_index;
};

struct Imported_3d_asset
{
   LIST(Imported_mesh, meshes_list);
   LIST(Imported_animation, animations_list);

   u16 joints_count;
   u16 bones_count;

   V3* joint_positions;
   Bone_joint_indices* bone_joints;
};
typedef Imported_3d_asset Exporting_3d_asset;


internal Imported_3d_asset
import_3d_asset(Memory_arena* arena, File_data file)
{arena;
   Imported_3d_asset result = {0};
   u8* scan = file.bytes;

   scan += 8; // 8 bytes of padding for now in case i want to put something at the beginning in the future

   result.joints_count = SCAN(scan, u16);
   result.bones_count = SCAN(scan, u16);
   u16 submeshes_count = SCAN(scan, u16);
   u16 animations_count = SCAN(scan, u16);

   Block_entry current_block_entry = SCAN(scan, Block_entry);
   while(current_block_entry.block_type != BT3D_LAST_ENTRY)
   {
      if(current_block_entry.block_type == BT3D_JOINT_POSITIONS)
      {
         result.joint_positions = ARENA_PUSH_STRUCTS(arena, V3, result.joints_count);
         copy_mem(scan, result.joint_positions, result.joints_count*sizeof(V3));
         scan += result.joints_count*sizeof(V3);
      }
      else if(current_block_entry.block_type == BT3D_BONE_JOINTS_INDICES)
      {
         result.bone_joints = ARENA_PUSH_STRUCTS(arena, Bone_joint_indices, result.bones_count);
         copy_mem(scan, result.bone_joints, result.bones_count*sizeof(Bone_joint_indices));
         scan += result.bones_count*sizeof(Bone_joint_indices);
      }
      else if(current_block_entry.block_type == BT3D_MESH)
      {
         Imported_mesh* current_mesh;
         PUSH_BACK(result.meshes_list, arena, current_mesh);

         current_mesh->vertex_count = SCAN(scan, u32);
         current_mesh->indices_count = SCAN(scan, u32);
         current_mesh->topology_uid = SCAN(scan, u32);

         Block_entry current_mesh_entry = SCAN(scan, Block_entry);

         while(current_mesh_entry.block_type != MESH_ET_LAST_ENTRY)
         {
            if(current_mesh_entry.mesh_block_type == MESH_ET_NAME)
            {
               // TODO: name
               ASSERT(false);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VPOSITIONS)
            {
               current_mesh->vertices.positions = ARENA_PUSH_STRUCTS(arena, V3, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.positions, current_mesh->vertex_count*sizeof(V3));
               scan += current_mesh->vertex_count*sizeof(V3);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VTEXCOORDS)
            {
               current_mesh->vertices.texcoords = ARENA_PUSH_STRUCTS(arena, V2, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.texcoords, current_mesh->vertex_count*sizeof(V2));
               scan += current_mesh->vertex_count*sizeof(V2);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VNORMALS)
            {
               current_mesh->vertices.normals = ARENA_PUSH_STRUCTS(arena, V3, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.normals, current_mesh->vertex_count*sizeof(V3));
               scan += current_mesh->vertex_count*sizeof(V3);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VCOLORS)
            {
               current_mesh->vertices.colors = ARENA_PUSH_STRUCTS(arena, Color, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.colors, current_mesh->vertex_count*sizeof(Color));
               scan += current_mesh->vertex_count*sizeof(Color);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VWEIGHT_INDICES)
            {
               current_mesh->vertices.weight_indices = ARENA_PUSH_STRUCTS(arena, Indices_triad16, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.weight_indices, current_mesh->vertex_count*sizeof(Indices_triad16));
               scan += current_mesh->vertex_count*sizeof(Indices_triad16);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_VWEIGHT_VALUES)
            {
               current_mesh->vertices.weight_values = ARENA_PUSH_STRUCTS(arena, V3, current_mesh->vertex_count);
               copy_mem(scan, current_mesh->vertices.weight_values, current_mesh->vertex_count*sizeof(V3));
               scan += current_mesh->vertex_count*sizeof(V3);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_INDICES)
            {
               current_mesh->indices = ARENA_PUSH_STRUCTS(arena, u16, current_mesh->indices_count);
               copy_mem(scan, current_mesh->indices, current_mesh->indices_count*sizeof(u16));
               scan += current_mesh->indices_count*sizeof(u16);
            }
            else if(current_mesh_entry.mesh_block_type == MESH_ET_NULL)
            {
               ASSERT(false);
            }
            else
            {
               ASSERT(false);
            }
            current_mesh_entry = SCAN(scan, Block_entry);
         }
      }
      else if(current_block_entry.block_type == BT3D_ANIMATIONS)
      {
         Imported_animation* current_animation;
         PUSH_BACK(result.animations_list, arena, current_animation);

         current_animation->bones_count = result.bones_count;
         ASSERT(result.bones_count);
         
         u32 keyframes_count = SCAN(scan, u32);
         ASSERT(keyframes_count < 0xffff);
         current_animation->keyframes_count = (u16)keyframes_count;

         current_animation->length = SCAN(scan, f32);
         
         Block_entry current_anim_entry = SCAN(scan, Block_entry);

         while(current_anim_entry.block_type != ANIM_ET_LAST_ENTRY)
         {
            if(current_anim_entry.anim_block_type == ANIM_ET_KEYFRAME_TIMES)
            {
               current_animation->keyframe_times = ARENA_PUSH_STRUCTS(arena, f32, current_animation->keyframes_count);
               copy_mem(scan, current_animation->keyframe_times, current_animation->keyframes_count*sizeof(f32));
               scan += current_animation->keyframes_count*sizeof(f32);
            }
            else if(current_anim_entry.anim_block_type == ANIM_ET_KEYFRAME_BONE_POSES)
            {
               current_animation->keyframe_bone_poses = ARENA_PUSH_STRUCTS(arena, Bone, current_animation->keyframes_count);
               copy_mem(scan, current_animation->keyframe_bone_poses, current_animation->keyframes_count*sizeof(Bone));
               scan += current_animation->keyframes_count*sizeof(Bone);
            }
            else if(current_anim_entry.anim_block_type == ANIM_ET_BONES_KEYFRAMES_COUNT)
            {
               current_animation->bones_keyframes_count = ARENA_PUSH_STRUCTS(arena, u16, current_animation->bones_count);
               copy_mem(scan, current_animation->bones_keyframes_count, current_animation->bones_count*sizeof(u16));
               scan += current_animation->bones_count*sizeof(u16);

               current_animation->bones_root_keyframe_index = ARENA_PUSH_STRUCTS(arena, u16, current_animation->bones_count);
               u16 current_index = 0;
               UNTIL(b, current_animation->bones_count)
               {
                  current_animation->bones_root_keyframe_index[b] = current_index;
                  u16 current_bone_keyframes_count = current_animation->bones_keyframes_count[b];
                  ASSERT(current_bone_keyframes_count);
                  current_index += current_bone_keyframes_count;
               }
               ASSERT(current_index == current_animation->keyframes_count);
            }
            else if(current_anim_entry.anim_block_type == ANIM_ET_NULL)
            {
               ASSERT(false);
            }
            else
            {
               ASSERT(false);
            }
            current_anim_entry = SCAN(scan, Block_entry);
         }

      }
      else if(current_block_entry.block_type == BT3D_MATERIALS)
      {
         ASSERT(false); //TODO:
      }
      else if(current_block_entry.block_type == BT3D_LAST_ENTRY)
      {
         ASSERT(false);
      }
      else if(current_block_entry.block_type == BT3D_NULL)
      {
         ASSERT(false);  
      }
      else
      {
         ASSERT(false);
      }
      current_block_entry = SCAN(scan, Block_entry);
   }

   ASSERT(LIST_SIZE(result.meshes_list) == submeshes_count);
   ASSERT(LIST_SIZE(result.animations_list) == animations_count);

   return result;
}

internal File_data
export_3d_asset(Memory_arena* temp_arena, Exporting_3d_asset* asset)
{
   File_data result = {0};
   result.data = temp_arena->data+temp_arena->used;
   u32 initial_used = temp_arena->used;
   Memory_arena* export_arena = temp_arena;

   arena_push_size(export_arena, 8); // 8 bytes of padding

   *ARENA_PUSH_STRUCT(export_arena, u16) = asset->joints_count;
   *ARENA_PUSH_STRUCT(export_arena, u16) = asset->bones_count;
   *ARENA_PUSH_STRUCT(export_arena, u16) = (u16)LIST_SIZE(asset->meshes_list);
   *ARENA_PUSH_STRUCT(export_arena, u16) = (u16)LIST_SIZE(asset->animations_list);

   Block_entry* current_entry;
   
   if(asset->bones_count)
   {
      current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
      current_entry->block_type = BT3D_JOINT_POSITIONS;

      V3* joint_positions = ARENA_PUSH_STRUCTS(export_arena, V3, asset->joints_count);
      
      UNTIL(j, asset->joints_count)
      {
         joint_positions[j] = asset->joint_positions[j];
      }
      
      
      current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
      current_entry->block_type = BT3D_BONE_JOINTS_INDICES;

      Bone_joint_indices* bone_joints = ARENA_PUSH_STRUCTS(export_arena, Bone_joint_indices, asset->bones_count);
      
      UNTIL(b, asset->bones_count)
      {
         bone_joints[b] = asset->bone_joints[b];
      }


      FOREACH(Imported_animation, current_anim, asset->animations_list)
      {
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->block_type = BT3D_ANIMATIONS;

         *ARENA_PUSH_STRUCT(export_arena, u32) = current_anim->keyframes_count;
         *ARENA_PUSH_STRUCT(export_arena, f32) = current_anim->length;


         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->anim_block_type = ANIM_ET_KEYFRAME_TIMES;

         f32* keyframe_times = ARENA_PUSH_STRUCTS(export_arena, f32, current_anim->keyframes_count);

         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->anim_block_type = ANIM_ET_KEYFRAME_BONE_POSES;
         
         Bone* keyframe_bone_poses = ARENA_PUSH_STRUCTS(export_arena, Bone, current_anim->keyframes_count);

         UNTIL(k, current_anim->keyframes_count)
         {
            keyframe_times[k] = current_anim->keyframe_times[k];
            keyframe_bone_poses[k] = current_anim->keyframe_bone_poses[k];
         }

         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->anim_block_type = ANIM_ET_BONES_KEYFRAMES_COUNT;

         u16* bones_keyframes_count = ARENA_PUSH_STRUCTS(export_arena, u16, current_anim->bones_count);
         UNTIL(b, current_anim->bones_count)
         {
            bones_keyframes_count[b] = current_anim->bones_keyframes_count[b];
         }
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->anim_block_type = ANIM_ET_LAST_ENTRY;
      }
   }

   FOREACH(Imported_mesh, current_mesh, asset->meshes_list)
   {
      current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
      current_entry->block_type = BT3D_MESH;

      *ARENA_PUSH_STRUCT(export_arena, u32) = current_mesh->vertex_count;
      *ARENA_PUSH_STRUCT(export_arena, u32) = current_mesh->indices_count;
      *ARENA_PUSH_STRUCT(export_arena, u32) = current_mesh->topology_uid;
      
      if(current_mesh->vertex_count)
      {
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VPOSITIONS;

         V3* vpositions = ARENA_PUSH_STRUCTS(export_arena, V3, current_mesh->vertex_count);
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VTEXCOORDS;

         V2* vtexcoords = ARENA_PUSH_STRUCTS(export_arena, V2, current_mesh->vertex_count);
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VNORMALS;

         V3* vnormals = ARENA_PUSH_STRUCTS(export_arena, V3, current_mesh->vertex_count);
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VCOLORS;

         Color* vcolors = ARENA_PUSH_STRUCTS(export_arena, Color, current_mesh->vertex_count);
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VWEIGHT_INDICES;

         Indices_triad16* vweight_indices = ARENA_PUSH_STRUCTS(export_arena, Indices_triad16, current_mesh->vertex_count);
         
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_VWEIGHT_VALUES;

         V3* vweight_values = ARENA_PUSH_STRUCTS(export_arena, V3, current_mesh->vertex_count);

         copy_mem(current_mesh->vertices.positions, vpositions, current_mesh->vertex_count*sizeof(vpositions[0]));
         copy_mem(current_mesh->vertices.texcoords, vtexcoords, current_mesh->vertex_count*sizeof(vtexcoords[0]));
         copy_mem(current_mesh->vertices.normals, vnormals, current_mesh->vertex_count*sizeof(vnormals[0]));
         copy_mem(current_mesh->vertices.colors, vcolors, current_mesh->vertex_count*sizeof(vcolors[0]));
         copy_mem(current_mesh->vertices.weight_indices, vweight_indices, current_mesh->vertex_count*sizeof(vweight_indices[0]));
         copy_mem(current_mesh->vertices.weight_values, vweight_values, current_mesh->vertex_count*sizeof(vweight_values[0]));

      }

      if(current_mesh->indices_count)
      {
         current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
         current_entry->mesh_block_type = MESH_ET_INDICES;

         u16* indices = ARENA_PUSH_STRUCTS(export_arena, u16, current_mesh->indices_count);

         copy_mem(current_mesh->indices, indices, current_mesh->indices_count*sizeof(indices[0]));
      }
      
      current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
      current_entry->mesh_block_type = MESH_ET_LAST_ENTRY;
   }

   
   current_entry = ARENA_PUSH_STRUCT(export_arena, Block_entry);
   current_entry->block_type = BT3D_LAST_ENTRY;

   result.size = export_arena->used - initial_used;
   return result;
}
