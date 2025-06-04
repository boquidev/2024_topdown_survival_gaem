	{
		u32 save_point	= temp_arena->used;
		

		FOREACH(Asset_request, request, memory.asset_requests)
		{
			switch(request->type){
				case ASSET_REQUEST_TEX_FROM_FILE:{
					ASSERT(win_file_exists(request->filename.text));
					ASSERT(request->p_uid);

					int comp;
					Surface tex_surface = {0};
					char temp_buffer [MAX_PATH] = {0}; 
					copy_mem(request->filename.text, temp_buffer, request->filename.length);
					tex_surface.data = stbi_load(temp_buffer, (int*)&tex_surface.width, (int*)&tex_surface.height, &comp, STBI_rgb_alpha);
					ASSERT(tex_surface.data);
					
					*request->p_uid = (u16)LIST_SIZE(memory.tex_infos);
					Tex_info* tex_info; PUSH_BACK(memory.tex_infos, assets_arena, tex_info);
					tex_info->w = tex_surface.width;
					tex_info->h = tex_surface.height;
					tex_info->texrect.x = 0.0f;
					tex_info->texrect.y = 0.0f;
					tex_info->texrect.w = 1.0f;
					tex_info->texrect.h = 1.0f;

					tex_info->texture_uid = LIST_SIZE(textures_list);
					Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
					ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &tex_surface);
					dx11_create_texture_view(dx, texture2d, texture_view);
					texture2d->Release();
				}break;


				case ASSET_REQUEST_VS_FROM_FILE:{
					// COMPILING VS
						// File_data compiled_vs = dx11_get_compiled_shader(request->filename, temp_arena, "vs", VS_PROFILE);
					// CREATING VS
					File_data compiled_vs = win_read_file(request->filename, temp_arena);
					u32 current_index = LIST_SIZE(vertex_shaders_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;
					Vertex_shader* vs; PUSH_BACK(vertex_shaders_list, assets_arena, vs);
					dx11_create_vs(dx, compiled_vs, &vs->shader);

					vs->filename.length = request->filename.length;
					vs->filename.text = (char*)arena_push_data(permanent_arena, request->filename.text, request->filename.length);
					arena_push_size(permanent_arena,1);
					vs->last_write_time = win_get_last_write_time(vs->filename.text);
					

					s32 MAX_IE_SIZE = sizeof(f32)*4;
					
					u32 ie_count = 0;
					#define USE_APPEND_ALIGNED_ELEMENT 1

					#if USE_APPEND_ALIGNED_ELEMENT
					#else
						u32 aligned_byte_offsets [2] = {0};
					#endif
					D3D11_INPUT_ELEMENT_DESC* ied = (D3D11_INPUT_ELEMENT_DESC*)(temp_arena->data+temp_arena->used);
					s32 total_element_size = 0;
					UNTIL(j, ARRAYLEN(request->ied.names))
					{
						s32 current_element_size = ie_formats_sizes[request->ied.formats[j]];
						total_element_size += current_element_size;
						for(s32 semantic_index = 0; current_element_size > 0; semantic_index++)
						{
							ie_count++;
							D3D11_INPUT_ELEMENT_DESC* current_ied = ARENA_PUSH_STRUCT(temp_arena, D3D11_INPUT_ELEMENT_DESC);
							current_ied->SemanticName = request->ied.names[j];
							// this is in case the element is bigger than a float4 (a matrix for example)
							current_ied->SemanticIndex = semantic_index;
							
							current_ied->Format = ie_formats_list[request->ied.formats[j]];

							u32 ie_slot = 0;
							if(request->ied.next_slot_beginning_index && j >= request->ied.next_slot_beginning_index)
							{
								ie_slot = 1;
							}
							
							#if USE_APPEND_ALIGNED_ELEMENT
								current_ied->AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
							#else
								current_ied->AlignedByteOffset = aligned_byte_offsets[ie_slot];
								aligned_byte_offsets[ie_slot] += MIN(MAX_IE_SIZE,current_element_size); 
							#endif
							current_element_size -= MAX_IE_SIZE;
							
							current_ied->InputSlot = ie_slot;// this is for using secondary buffers (like an instance buffer)
							current_ied->InputSlotClass = (D3D11_INPUT_CLASSIFICATION)ie_slot; // 0 PER_VERTEX_DATA vs 1 PER_INSTANCE_DATA
							current_ied->InstanceDataStepRate = ie_slot; // the amount of instances to draw using the PER_INSTANCE data
						}
					}
					UNTIL(e, ie_count)
					{
						ied[e];
						ASSERT(true);
					}
					hr = dx->device->CreateInputLayout(
						ied, ie_count, 
						compiled_vs.data, compiled_vs.size, 
						&vs->input_layout
					); 
					ASSERTHR(hr);
				}break;


				case ASSET_REQUEST_PS_FROM_FILE:{
					u32 current_index = LIST_SIZE(pixel_shaders_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;
					// COMPILING PS
						// File_data compiled_ps = dx11_get_compiled_shader(request->filename, temp_arena, "ps", PS_PROFILE);
					// CREATING PS
					File_data compiled_ps = win_read_file(request->filename, temp_arena);

					Pixel_shader* ps; PUSH_BACK(pixel_shaders_list, assets_arena, ps);
					dx11_create_ps(dx, compiled_ps, &ps->shader);

					ps->filename.length = request->filename.length;
					ps->filename.text = (char*)arena_push_data(permanent_arena, request->filename.text, request->filename.length);
					arena_push_size(permanent_arena,1);
					ps->last_write_time = win_get_last_write_time(ps->filename.text);
				}break;


				case ASSET_REQUEST_MESH_FROM_FILE:
				{
					File_data glb_file = win_read_file(request->filename, temp_arena);
					GLB glb = {0};
					glb_get_chunks(glb_file.data, 
						&glb);
					#if DEBUGMODE
						{ // THIS IS JUST FOR READABILITY OF THE JSON CHUNK
							void* formated_json = arena_push_size(temp_arena,MEGABYTES(4));
							u32 new_size = format_json_more_readable(glb.json_chunk, glb.json_size, formated_json);
							win_write_file(concat_strings(request->filename, string(".json"), temp_arena), formated_json, new_size);
							arena_pop_back_size(temp_arena, MEGABYTES(4));
						}
					#endif
					u32 meshes_count = 0;
					Gltf_mesh* meshes = gltf_get_meshes(&glb, temp_arena, &meshes_count);
					
					Mesh_primitive* primitives = ARENA_PUSH_STRUCTS(assets_arena, Mesh_primitive, meshes_count);
					for(u32 m=0; m<meshes_count; m++)
					{
						Gltf_primitive* mesh_primitive = meshes[m].primitives;
						//TODO: here i am assuming this mesh has only one primitive
						V3 normals [8];
						copy_mem(mesh_primitive->normals, normals, sizeof(V3)*8);
						primitives[m] = gltf_primitives_to_mesh_primitives(assets_arena, &mesh_primitive[0]);
						// u32 primitives_count = meshes[m].primitives_count;
						// for(u32 p=0; p<primitives_count; p++)
						// {	
						// }
					}
					u32 current_index = LIST_SIZE(meshes_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;
					
					Dx_mesh* current_mesh; PUSH_BACK(meshes_list, assets_arena, current_mesh);
					*current_mesh = dx11_init_mesh(dx, 
					&primitives[0],
					D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);	
				}break;


				case ASSET_REQUEST_CREATE_BLEND_STATE:{
					u32 current_index = LIST_SIZE(blend_states_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;

					Dx11_blend_state** blend_state; PUSH_BACK(blend_states_list, assets_arena, blend_state);
					if(!dx11_create_blend_state(dx, blend_state, request->enable_alpha_blending))
					{
						MessageBoxA(window, "CreateBlendState failed", 0, MB_OK|MB_ICONERROR);
						return 10;
					}
				}break;


				case ASSET_REQUEST_CREATE_DEPTH_STENCIL:{
					u32 current_index = LIST_SIZE(depth_stencils_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;

					Depth_stencil* depth_stencil; PUSH_BACK(depth_stencils_list, assets_arena, depth_stencil);
					dx11_create_depth_stencil_state(dx, &depth_stencil->state, request->enable_depth);
				}break;


				case ASSET_REQUEST_FONT_FROM_FILE:
				{
					ASSERT(request->font_lines_height);
					// FONTS 
					File_data font_file = win_read_file(request->filename, temp_arena);
					//currently just supporting ANSI characters
					//TODO: learn how to do UNICODE

					// GETTING BITMAPS AND INFO
					u32 atlas_texview_uid = LIST_SIZE(textures_list);

					stbtt_fontinfo font;
					Color32* charbitmaps [CHARS_COUNT];
					Tex_info temp_charinfos[CHARS_COUNT];
					stbtt_InitFont(&font, (u8*)font_file.data,stbtt_GetFontOffsetForIndex((u8*)font_file.data, 0));
					f32 font_size = stbtt_ScaleForPixelHeight(&font, request->font_lines_height);

					UNTIL(c, CHARS_COUNT)
					{
						u32 codepoint = c+FIRST_CHAR;

						temp_charinfos[c].texture_uid = atlas_texview_uid;

						u8* monobitmap = stbtt_GetCodepointBitmap(
							&font, 0, 
							font_size, 
							// stbtt_ScaleForMappingEmToPixels(&font, request->font_lines_height),
							codepoint,
							&temp_charinfos[c].w, &temp_charinfos[c].h, 
							&temp_charinfos[c].xoffset, &temp_charinfos[c].yoffset
						);
						if(temp_charinfos[c].w && temp_charinfos[c].h)
						{
							u32 bitmap_size = temp_charinfos[c].w * temp_charinfos[c].h;
							charbitmaps[c] = ARENA_PUSH_STRUCTS(temp_arena, Color32, bitmap_size);
							Color32* bitmap =  charbitmaps[c];
							UNTIL(p, bitmap_size){
								bitmap[p].r = 255;
								bitmap[p].g = 255;
								bitmap[p].b = 255;
								bitmap[p].a = monobitmap[p];
							}
						}
						stbtt_FreeBitmap(monobitmap,0);
					}
					// PACKING BITMAP RECTS

					//total_atlas_size = atlas_side*atlas_side

					s32 atlas_side;
					{
						
						s32 min_value = (u32)(request->font_lines_height*(request->font_lines_height/2)*CHARS_COUNT);
						s32 current_side = 2;
						while(min_value>(current_side*current_side)){
							current_side = current_side << 1;
						}
						atlas_side = current_side;

					}


					stbrp_context pack_context = {0};
					stbrp_node* pack_nodes = ARENA_PUSH_STRUCTS(temp_arena, stbrp_node, atlas_side);
					stbrp_init_target(&pack_context, atlas_side, atlas_side, pack_nodes, atlas_side);

					stbrp_rect* rects = ARENA_PUSH_STRUCTS(temp_arena, stbrp_rect, CHARS_COUNT);
					UNTIL(i, CHARS_COUNT){
						rects[i].w = temp_charinfos[i].w;
						rects[i].h = temp_charinfos[i].h;
					}
					stbrp_pack_rects(&pack_context, rects, CHARS_COUNT);

					// CREATE TEXTURE ATLAS AND COPY EACH CHARACTER BITMAP INTO IT USING THE POSITIONS OBTAINED FROM PACK
					Color32* atlas_pixels = ARENA_PUSH_STRUCTS(temp_arena, Color32, atlas_side*atlas_side);

					Int2 atlas_size = {atlas_side, atlas_side};

					*request->font_uid = (u16)LIST_SIZE(memory.fonts_list);
					Font* new_font;
					PUSH_BACK(memory.fonts_list, memory.permanent_arena, new_font);
					new_font->texinfos_count = CHARS_COUNT;
					new_font->texinfo_uids = ARENA_PUSH_STRUCTS(memory.permanent_arena, u32, new_font->texinfos_count);
					new_font->first_char = FIRST_CHAR;
					new_font->lines_height = request->font_lines_height;

					int ascent, descent, line_gap;
					
					stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
					new_font->ascent = font_size * ascent;
					new_font->descent = font_size * descent;
					int x0, y0, x1, y1;
					stbtt_GetFontBoundingBox(&font, &x0, &y0, &x1, &y1);


					UNTIL(i, CHARS_COUNT){
						ASSERT(rects[i].was_packed);
						if(rects[i].was_packed){
							temp_charinfos[i].texrect.x = (f32)rects[i].x / atlas_size.x;
							temp_charinfos[i].texrect.y = (f32)rects[i].y / atlas_size.y;
							temp_charinfos[i].texrect.w = (f32)temp_charinfos[i].w / atlas_size.x;
							temp_charinfos[i].texrect.h = (f32)temp_charinfos[i].h / atlas_size.y;
							
							new_font->texinfo_uids[i] = LIST_SIZE(memory.tex_infos);
							Tex_info* charinfo; PUSH_BACK(memory.tex_infos, assets_arena, charinfo);
							*charinfo = temp_charinfos[i]; 

							// PASTING EACH CHAR INTO THE ATLAS PIXELS
							u32 first_char_pixel = (rects[i].y*atlas_side) + rects[i].x;
							Color32* charpixels = charbitmaps[i];
							UNTIL(y, (u32)rects[i].h){
								UNTIL(x, (u32)rects[i].w){
									u32 current_pixel = first_char_pixel + (y*atlas_side) + x;
									atlas_pixels[current_pixel] = charpixels[(y*rects[i].w) + x];
								}
							}
						}
					}
					new_font->atlas_texinfo_uid = (u16)LIST_SIZE(memory.tex_infos);
					Tex_info* atlas_tex_info; PUSH_BACK(memory.tex_infos, assets_arena, atlas_tex_info);
					atlas_tex_info->texture_uid = atlas_texview_uid;
					atlas_tex_info->w = atlas_size.x;
					atlas_tex_info->h = atlas_size.y;
					atlas_tex_info->texrect.x = 0.0f;
					atlas_tex_info->texrect.y = 0.0f;
					atlas_tex_info->texrect.w = 1.0f;
					atlas_tex_info->texrect.h = 1.0f;

					Dx11_texture_view** atlas_texture; PUSH_BACK(textures_list, assets_arena, atlas_texture);
					Surface atlas_surface = {(u32)atlas_size.x, (u32)atlas_size.y, atlas_pixels};
					ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &atlas_surface);
					dx11_create_texture_view(dx, texture2d, atlas_texture);
					texture2d->Release();
				}break;


				case ASSET_REQUEST_TEX_FROM_SURFACE:{
					u32 current_index = LIST_SIZE(memory.tex_infos);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;

					Tex_info* tex_info; PUSH_BACK(memory.tex_infos, assets_arena, tex_info);
					tex_info->w = request->tex_surface.width;
					tex_info->h = request->tex_surface.height;

					tex_info->texrect.x = 0.0f;
					tex_info->texrect.y = 0.0f;
					tex_info->texrect.w = 1.0f;
					tex_info->texrect.h = 1.0f;
					
					ID3D11Texture2D* texture2d = dx11_create_texture2d(dx, &request->tex_surface);

					tex_info->texture_uid = LIST_SIZE(textures_list);
					Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
					dx11_create_texture_view(dx, texture2d, texture_view);
					texture2d->Release();
				}break;

				case ASSET_REQUEST_CREATE_DYNAMIC_TEXTURE:{
					if(request->dynamic_tex_sizes.x && !request->dynamic_tex_sizes.y && !request->dynamic_tex_sizes.z)
					{
						ASSERT(false);
					}
					else if(request->dynamic_tex_sizes.x && request->dynamic_tex_sizes.y && !request->dynamic_tex_sizes.z)
					{
						D3D11_TEXTURE2D_DESC tex2d_desc = {0};
						tex2d_desc.Width = request->dynamic_tex_sizes.x;
						tex2d_desc.Height = request->dynamic_tex_sizes.y;
						tex2d_desc.ArraySize = 1;
						tex2d_desc.MipLevels = 1;
						// this is for multi-sampling and antialiasing apparently (no idea how it works)
						tex2d_desc.SampleDesc = {1,0}; 
						tex2d_desc.Format = DXGI_FORMAT_R32_UINT;
						tex2d_desc.Usage = D3D11_USAGE_DEFAULT;
						tex2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
						tex2d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

						ID3D11Texture2D* texture2d; 
						hr = dx->device->CreateTexture2D(&tex2d_desc, 0, &texture2d);
						ASSERTHR(hr);

						*request->p_uid = (u16)LIST_SIZE(textures_list);
						Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
						dx11_create_texture_view(dx, texture2d, texture_view);
						texture2d->Release();

					}
					else if(request->dynamic_tex_sizes.x && request->dynamic_tex_sizes.y && request->dynamic_tex_sizes.z)
					{
						
						D3D11_TEXTURE3D_DESC tex3d_desc = {0};
						tex3d_desc.Width = request->dynamic_tex_sizes.x;
						tex3d_desc.Height = request->dynamic_tex_sizes.y;
						tex3d_desc.Depth = request->dynamic_tex_sizes.z;
						tex3d_desc.MipLevels = 1;
						tex3d_desc.Format = DXGI_FORMAT_R32_UINT;
						tex3d_desc.Usage = D3D11_USAGE_DEFAULT;
						tex3d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
						tex3d_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

						ID3D11Texture3D* texture3d; 
						hr = dx->device->CreateTexture3D(&tex3d_desc, 0, &texture3d);
						ASSERTHR(hr);

						*request->p_uid = (u16)LIST_SIZE(textures_list);
						Dx11_texture_view** texture_view; PUSH_BACK(textures_list, assets_arena, texture_view);
						dx11_create_texture_view(dx, texture3d, texture_view);
						texture3d->Release();
					}
					else
					{
						ASSERT(false);
					}
									
				}break;

				case ASSET_REQUEST_CREATE_RTV:
				{
					*request->p_uid = (u16)LIST_SIZE(render_targets_list);
					Render_target* new_render_target;
					PUSH_BACK(render_targets_list, assets_arena, new_render_target);

					PUSH_BACK(textures_list, assets_arena, new_render_target->texture_view);
				}break;


				case ASSET_REQUEST_SOUND_FROM_FILE:{
					Sound_sample* new_audio_samples =  &sounds_list[request->sound_uid];
					ASSERT(!new_audio_samples->samples); // already saved in this index


					File_data audio_file = win_read_file(request->filename, temp_arena);
					u32* scan = (u32*)audio_file.data;
					ASSERT(scan[0] == 'FFIR'); // RIFF
					u32 file_size = scan[1];
					file_size;
					ASSERT(scan[2] == 'EVAW'); // WAVE
					ASSERT(scan[3] == ' tmf'); // fmt\0 format chunk identifier
					u32 format_chunk_size = scan[4];
					format_chunk_size;

					u16* scan16 = (u16*)&scan[5];
					u16 sample_format = scan16[0]; // 1 is PCM
					ASSERT(sample_format == 1);
					u16 channels_count = scan16[1];
					ASSERT(channels_count <= 2);

					u32 sample_hz = scan[6];
					ASSERT(sample_hz == 44100);
					u32 bytes_p_second = scan[7]; // sample_hz * bits_per_sample * channels / 8

					scan16 = (u16*)&scan[8];
					u16 bytes_per_full_sample = scan16[0]; // bits_per_sample*channels / 8
					u16 bits_per_sample = scan16[1]; 
					ASSERT(bits_per_sample == 16);
					ASSERT(bytes_p_second == (sample_hz*bytes_per_full_sample));


					ASSERT(scan[9] == 'atad'); // data
					u32 data_chunk_size = scan[10];

					s16* sample_values = (s16*)&scan[11];
					u16 bytes_per_sample = bits_per_sample / 8;

					new_audio_samples->samples_count = data_chunk_size/bytes_per_sample;
					new_audio_samples->channels = channels_count;
					new_audio_samples->samples = ARENA_PUSH_STRUCTS(assets_arena, s16, new_audio_samples->samples_count);

					UNTIL(i, new_audio_samples->samples_count){
						new_audio_samples->samples[i] = sample_values[i];
					}


				}break;


				case ASSET_REQUEST_MESH_FROM_PRIMITIVES:{
					u32 current_index = LIST_SIZE(meshes_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;

					Dx_mesh* current_mesh; PUSH_BACK(meshes_list, assets_arena, current_mesh);
					*current_mesh = dx11_init_mesh(dx, 
					&request->mesh_primitives, 
					topologies_list[request->mesh_primitives.topology_uid]);
				}break;

				case ASSET_REQUEST_CREATE_CONSTANT_BUFFER:{
					D3D_constant_buffer* new_constant_buffer = &renderer_variables[request->constant_buffer.register_index];
					Shader_constant_buffer_register_index register_index = (Shader_constant_buffer_register_index)(request->constant_buffer.register_index%14);

					u16 buffer_size = 16*((request->constant_buffer.size+15)/16);

					
					// ASSUMING I NEVER ASSIGN INITIAL DATA TO THE CONSTANT BUFFER
					dx11_create_constant_buffer(dx, new_constant_buffer, buffer_size, register_index, 0);
					if(request->constant_buffer.register_index < 14)
					{
						dx->context->VSSetConstantBuffers(register_index, 1, &new_constant_buffer->buffer);
					}
					else
					{
						dx->context->PSSetConstantBuffers(register_index, 1, &new_constant_buffer->buffer);
					}
				}break;

				case ASSET_REQUEST_CREATE_DYNAMIC_MESH:{
					u32 current_index = LIST_SIZE(meshes_list);
					ASSERT(current_index < 0xffff);
					*request->p_uid = (u16)current_index;

					Dx_mesh* current_mesh; 
					PUSH_BACK(meshes_list, assets_arena, current_mesh);
					current_mesh->topology = topologies_list[request->mesh_primitives.topology_uid];
					current_mesh->vertex_size = request->mesh_primitives.vertex_size;
					current_mesh->vertices_count = request->mesh_primitives.vertex_count;
					current_mesh->indices_count = request->mesh_primitives.indices_count;

					// VERTEX BUFFER

					D3D11_BUFFER_DESC bd = {0};
					bd.ByteWidth        = current_mesh->vertices_count * current_mesh->vertex_size;
					bd.Usage            = D3D11_USAGE_DYNAMIC;
					bd.BindFlags        = D3D11_BIND_VERTEX_BUFFER;
					bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
					
					D3D11_SUBRESOURCE_DATA buffer_init_data = {0};
					buffer_init_data.pSysMem  = request->mesh_primitives.vertices;
					if(request->mesh_primitives.vertices)
					{
						ASSERTHR(dx->device->CreateBuffer( &bd, &buffer_init_data, &current_mesh->vertex_buffer));
					}
					else
					{
						ASSERTHR(dx->device->CreateBuffer( &bd, 0, &current_mesh->vertex_buffer ));
					}

					// INDEX BUFFER
					if(request->mesh_primitives.indices_count)
					{
						bd = {0};
						bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
						bd.Usage = D3D11_USAGE_DYNAMIC;
						bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
						bd.MiscFlags = 0;
						bd.ByteWidth = current_mesh->indices_count*sizeof(u16);
						bd.StructureByteStride = sizeof(u16);

						buffer_init_data = {0};
						buffer_init_data.pSysMem = request->mesh_primitives.indices;
						
						if(request->mesh_primitives.indices)
						{
							ASSERTHR(dx->device->CreateBuffer(&bd, &buffer_init_data, &current_mesh->index_buffer));
						}
						else
						{
							ASSERTHR(dx->device->CreateBuffer(&bd, 0, &current_mesh->index_buffer));
						}
					}
				}break;

				case FORGOR_TO_SET_ASSET_TYPE:
				default:
					ASSERT(false);
				break;
			}
			assets_count++;
		}

		
		if(temp_arena->used != save_point)
		{
			s32 difference = temp_arena->used - save_point;
			temp_arena->used =  save_point;
			set_mem(temp_arena->data + save_point, difference, 0);
		}
	}