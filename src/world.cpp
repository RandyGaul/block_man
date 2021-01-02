/*
	Cute Framework
	Copyright (C) 2020 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#include <world.h>
#include <imgui/imgui.h>

#include <systems/animator_system.h>
#include <systems/board_system.h>
#include <systems/ice_block_system.h>
#include <systems/player_system.h>
#include <systems/reflection_system.h>
#include <systems/transform_system.h>
#include <systems/shadow_system.h>
#include <systems/mochi_system.h>
#include <systems/light_system.h>

#include <components/animator.h>
#include <components/board_piece.h>
#include <components/ice_block.h>
#include <components/player.h>
#include <components/reflection.h>
#include <components/transform.h>
#include <components/shadow.h>
#include <components/mochi.h>
#include <components/fire.h>
#include <components/light.h>
#include <components/oil.h>
#include <components/lamp.h>
#include <components/ladder.h>

#define CUTE_PATH_IMPLEMENTATION
#include <cute/cute_path.h>

World world_instance;
World* world = &world_instance;
aseprite_cache_t* cache;
batch_t* batch;
app_t* app;

const char* schema_ice_block = CUTE_STRINGIZE(
	entity_type = "IceBlock",
	Transform = { },
	Animator = { name = "ice_block.aseprite" },
	BoardPiece = { },
	IceBlock = { },
	Shadow = { },
);

const char* schema_box = CUTE_STRINGIZE(
	entity_type = "Box",
	Transform = { },
	Animator = { name = "box.aseprite" },
	Reflection = { },
	BoardPiece = { },
	Shadow = { },
);

const char* schema_ladder = CUTE_STRINGIZE(
	entity_type = "Ladder",
	Transform = { },
	Animator = { name = "ladder.aseprite" },
	Reflection = { },
	BoardPiece = { },
	Ladder = { },
);

const char* schema_player = CUTE_STRINGIZE(
	entity_type = "Player",
	Transform = { },
	Animator = { name = "girl.aseprite", },
	Reflection = { },
	BoardPiece = { },
	Player = { },
	Shadow = { small = "true" },
);

const char* schema_mochi = CUTE_STRINGIZE(
	entity_type = "Mochi",
	Transform = { },
	Animator = { name = "mochi.aseprite", },
	Reflection = { },
	BoardPiece = { },
	Mochi = { },
	Shadow = { },
);

const char* schema_zzz = CUTE_STRINGIZE(
	entity_type = "zzz",
	Transform = { },
	Animator = { name = "z.aseprite", },
);

const char* schema_fire = CUTE_STRINGIZE(
	entity_type = "Fire",
	Transform = { },
	Animator = { name = "fire.aseprite", floating = "true" },
	Reflection = { },
	BoardPiece = { },
	Shadow = { tiny = "true" },
	Fire = { },
	Light = { },
);

const char* schema_oil = CUTE_STRINGIZE(
	entity_type = "Oil",
	Transform = { },
	Animator = { name = "oil.aseprite", },
	Reflection = { },
	BoardPiece = { },
	Shadow = { small = "true" },
	Oil = { },
);

const char* schema_lamp = CUTE_STRINGIZE(
	entity_type = "Lamp",
	Transform = { },
	Animator = { name = "lamp.aseprite" },
	Reflection = { },
	BoardPiece = { },
	Shadow = { small = "true" },
	Light = { is_lamp = "true", radius = 16 },
	Lamp = { },
);

const char* schema_torch = CUTE_STRINGIZE(
	entity_type = "Torch",
	Transform = { },
	Animator = { name = "torch.aseprite", },
	Reflection = { },
	BoardPiece = { },
	Shadow = { small = "true" },
	Light = { radius = 64 },
);

const char* schema_big_ice_block = CUTE_STRINGIZE(
	entity_type = "BigIceBlock",
	Transform = { },
	Animator = { name = "big_ice_block.aseprite" },
	BoardPiece = { big = "true" },
	IceBlock = { big = "true" },
	Shadow = { big = "true" },
);

array<const char*> schemas = {
	schema_ice_block,
	schema_box,
	schema_ladder,
	schema_player,
	schema_mochi,
	schema_zzz,
	schema_fire,
	schema_oil,
	schema_lamp,
	schema_torch,
	schema_big_ice_block,
};

array<schema_preview_t> schema_previews;

// Previews are shown in the level editor in place of an actual entity.
void add_schema_preview(const char* schema)
{
	// Lookup the "name" key of the Animator component. Use it as the preview image.
	kv_t* kv = kv_make();
	error_t err = kv_parse(kv, schema, CUTE_STRLEN(schema));
	CUTE_ASSERT(!err.is_error());
	kv_object_begin(kv, "Animator");
	kv_key(kv, "name");

	const char* string_raw;
	size_t string_sz;
	err = kv_val_string(kv, &string_raw, &string_sz);
	CUTE_ASSERT(!err.is_error());
	strpool_id id = strpool_inject(app_get_strpool(app), string_raw, (int)string_sz);
	const char* ase_path = strpool_cstr(app_get_strpool(app), id);

	kv_object_end(kv);

	// Lookup the entity type string.
	kv_key(kv, "entity_type");
	err = kv_val_string(kv, &string_raw, &string_sz);
	CUTE_ASSERT(!err.is_error());
	id = strpool_inject(app_get_strpool(app), string_raw, (int)string_sz);
	const char* entity_type = strpool_cstr(app_get_strpool(app), id);

	kv_destroy(kv);

	ase_t* ase = NULL;
	aseprite_cache_load_ase(cache, ase_path, &ase);
	CUTE_ASSERT(ase);

	pixel_t* pixels = (pixel_t*)CUTE_ALLOC(sizeof(pixel_t) * (ase->w + 2) * (ase->h + 2), NULL);
	CUTE_MEMCPY(pixels, (pixel_t*)ase->frames[0].pixels, sizeof(pixel_t) * ase->w * ase->h);

	// Expand image from top-left corner, offset by (1, 1).
	// This expansion is so ImGui renders a "selected" border around ImGui::ImageButton.
	int w = ase->w;
	int h = ase->h;
	int w0 = w;
	int h0 = h;
	w += 2;
	h += 2;
	char* buffer = (char*)pixels;
	int dst_row_stride = w * sizeof(pixel_t);
	int src_row_stride = w0 * sizeof(pixel_t);
	int src_row_offset = sizeof(pixel_t);
	for (int i = 0; i < h - 2; ++i)
	{
		char* src_row = buffer + (h0 - i - 1) * src_row_stride;
		char* dst_row = buffer + (h - i - 2) * dst_row_stride + src_row_offset;
		CUTE_MEMMOVE(dst_row, src_row, src_row_stride);
	}

	// Clear the border pixels.
	int pixel_stride = sizeof(pixel_t);
	CUTE_MEMSET(buffer, 0, dst_row_stride);
	for (int i = 1; i < h - 1; ++i)
	{
		CUTE_MEMSET(buffer + i * dst_row_stride, 0, pixel_stride);
		CUTE_MEMSET(buffer + i * dst_row_stride + src_row_stride + src_row_offset, 0, pixel_stride);
	}
	CUTE_MEMSET(buffer + (h - 1) * dst_row_stride, 0, dst_row_stride);

	texture_t tex = texture_make(pixels, w, h);
	CUTE_FREE(pixels, NULL);

	// Make an actual preview entry. These are dealt with in main.cpp in the ImGui code.
	schema_preview_t preview;
	preview.entity_type = entity_type;
	aseprite_cache_load(cache, ase_path, &preview.sprite);
	preview.tex = tex;
	preview.w = w * 2.0f;
	preview.h = h * 2.0f;
	schema_previews.add(preview);
}

void ecs_registration(app_t* app)
{
	// Order of component registration does not matter.
	ecs_component_begin(app);
	ecs_component_set_name(app, "Animator");
	ecs_component_set_size(app, sizeof(Animator));
	ecs_component_set_optional_serializer(app, Animator_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "BoardPiece");
	ecs_component_set_size(app, sizeof(BoardPiece));
	ecs_component_set_optional_serializer(app, BoardPiece_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "IceBlock");
	ecs_component_set_size(app, sizeof(IceBlock));
	ecs_component_set_optional_serializer(app, IceBlock_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Player");
	ecs_component_set_size(app, sizeof(Player));
	ecs_component_set_optional_serializer(app, Player_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Reflection");
	ecs_component_set_size(app, sizeof(Reflection));
	ecs_component_set_optional_serializer(app, Reflection_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Transform");
	ecs_component_set_size(app, sizeof(Transform));
	ecs_component_set_optional_serializer(app, Transform_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Shadow");
	ecs_component_set_size(app, sizeof(Shadow));
	ecs_component_set_optional_serializer(app, Shadow_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Mochi");
	ecs_component_set_size(app, sizeof(Mochi));
	ecs_component_set_optional_serializer(app, Mochi_serialize);
	ecs_component_set_optional_cleanup(app, Mochi_cleanup);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Fire");
	ecs_component_set_size(app, sizeof(Fire));
	ecs_component_set_optional_serializer(app, Fire_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Light");
	ecs_component_set_size(app, sizeof(Light));
	ecs_component_set_optional_serializer(app, Light_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Oil");
	ecs_component_set_size(app, sizeof(Oil));
	ecs_component_set_optional_serializer(app, Oil_serialize);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Lamp");
	ecs_component_set_size(app, sizeof(Lamp));
	ecs_component_set_optional_serializer(app, Lamp_serialize);
	ecs_component_set_optional_cleanup(app, Lamp_cleanup);
	ecs_component_end(app);

	ecs_component_begin(app);
	ecs_component_set_name(app, "Ladder");
	ecs_component_set_size(app, sizeof(Ladder));
	ecs_component_set_optional_serializer(app, Ladder_serialize);
	ecs_component_end(app);

	// Order of entity registration matters if using `inherits_from`.
	// Any time `inherits_from` is used, that type must have been already registered.
	for (int i = 0; i < schemas.count(); ++i) {
		const char* schema = schemas[i];
		ecs_entity_begin(app);
		ecs_entity_set_optional_schema(app, schema);
		ecs_entity_end(app);
		add_schema_preview(schema);
	}

	/*
	   Order of system registration is the order updates are called.
	   The flow is like so:

	   for each system
	       call system pre update
	       for each entity type with matching component set
	           call system update
	       call system post update

	   Please note that each of these three callbacks are optional and can be NULL.
	*/
	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)transform_system_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)animator_transform_system_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)board_transform_system_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)player_system_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_require_component(app, "Player");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)board_system_update);
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)ice_block_system_update);
	ecs_system_set_optional_pre_update(app, ice_block_system_pre_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_require_component(app, "IceBlock");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)mochi_system_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_require_component(app, "Mochi");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_optional_pre_update(app, draw_background_bricks_system_pre_update);
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)shadow_system_update);
	ecs_system_set_optional_post_update(app, shadow_system_post_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "BoardPiece");
	ecs_system_require_component(app, "Shadow");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)animator_system_update);
	ecs_system_set_optional_post_update(app, animator_system_post_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)reflection_system_update);
	ecs_system_set_optional_pre_update(app, reflection_system_pre_update);
	ecs_system_set_optional_post_update(app, reflection_system_post_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Animator");
	ecs_system_require_component(app, "Reflection");
	ecs_system_end(app);

	ecs_system_begin(app);
	ecs_system_set_update(app, (void*)light_system_update);
	ecs_system_set_optional_post_update(app, reflection_system_post_update);
	ecs_system_require_component(app, "Transform");
	ecs_system_require_component(app, "Light");
	ecs_system_end(app);
}

static void s_add_level(const char* name)
{
	void* data;
	size_t sz;
	file_system_read_entire_file_to_memory_and_nul_terminate(name, &data, &sz);
	world->levels.add((const char*)data);
	world->level_names.add(name);
}

void load_all_levels_from_disk_into_ram()
{
	s_add_level("level0.txt");
	s_add_level("level1.txt");
	s_add_level("level2.txt");
	s_add_level("level3.txt");
	s_add_level("level4.txt");
	s_add_level("level5.txt");
	s_add_level("level6.txt");
	s_add_level("level7.txt");
	s_add_level("level8.txt");
	s_add_level("level9.txt");
	s_add_level("level10.txt");
	s_add_level("level11.txt");
	s_add_level("level12.txt");
	s_add_level("level13.txt");
	s_add_level("level14.txt");
	s_add_level("level15.txt");
	s_add_level("level16.txt");
	s_add_level("level17.txt");
	s_add_level("level18.txt");
	s_add_level("level19.txt");
}

void setup_write_directory()
{
	// Setup the write directory for development. This will be changed to something else
	// when the game is finally being released. For now it just finds the "data" folder.
	char buf[1024];
	const char* base = file_system_get_base_dir();

	path_pop(base, NULL, buf);
	sprintf(buf, "%s%s", file_system_get_base_dir(), "data");
	file_system_set_write_dir(buf);
	file_system_mount(buf, "");
}

void init_world(int argc, const char** argv)
{
	int options = CUTE_APP_OPTIONS_WINDOW_POS_CENTERED;
#ifdef CUTE_WINDOWS
	options |= CUTE_APP_OPTIONS_D3D11_CONTEXT;
#elif defined(CUTE_EMSCRIPTEN)
	options |= CUTE_APP_OPTIONS_OPENGLES_CONTEXT;
#else
	options |= CUTE_APP_OPTIONS_OPENGL_CONTEXT;
#endif
	app = app_make("Block Man", 0, 0, 960, 720, options, argv[0]);
	app_init_upscaling(app, UPSCALE_PIXEL_PERFECT_AT_LEAST_2X, 320, 240);
	ImGui::SetCurrentContext(app_init_imgui(app));
	setup_write_directory();

	cache = aseprite_cache_make(app);
	batch = batch_make(aseprite_cache_get_pixels_fn(cache), cache);
	batch_set_projection(batch, matrix_ortho_2d(320, 240, 0, 0));

	ecs_registration(app);
	ice_block_system_init();
	shadow_system_init();
	light_system_init();
	load_all_levels_from_disk_into_ram();
}

sprite_t load_sprite(string_t path)
{
	sprite_t s;
	cute::error_t err = aseprite_cache_load(cache, path.c_str(), &s);
	char buf[1024];
	if (err.is_error()) {
		sprintf(buf, "Unable to load sprite at path \"%s\".\n", path.c_str());
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	if (is_odd(s.w)) {
		sprintf(buf, "Sprite \"%s\" has an odd width of %d (should be even).\n", path.c_str(), s.w);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	if (is_odd(s.h)) {
		sprintf(buf, "Sprite \"%s\" has an odd height of %d (should be even).\n", path.c_str(), s.h);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "ERROR", buf);
	}
	return s;
}

v2 tile2world(int x, int y)
{
	int bw = World::LEVEL_W / 2;
	int bh = World::LEVEL_H / 2;
	float x_offset = World::TILE_H / 2.0f;
	float y_offset = (float)(world->board.data.count() * World::TILE_H);
	return v2((float)(x-bw) * World::TILE_W + x_offset, -(float)(y+bh+1) * World::TILE_H + y_offset);
}

void world2tile(v2 p, int* x_out, int* y_out)
{
	int bw = World::LEVEL_W / 2;
	int bh = World::LEVEL_H / 2;
	float x_offset = World::TILE_H / 2.0f;
	float y_offset = (float)(world->board.data.count() * World::TILE_H);
	float x = (p.x - x_offset) / World::TILE_W + bw;
	float y = -((p.y - y_offset) / World::TILE_H) - bh;
	*x_out = (int)round(x);
	*y_out = (int)round(y) - 1;
}

array<array<string_t>> background_maps = {
	{
		"XXXX0000XXXX00000XXX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"00000000000XXX0000XX",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXXX0X0XX00XXXXX00X0",
		"XXX0000XX000XXX0000X",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
	{
		"00000000000XXX0000XX",
		"XXXX00000XXXXXX000XX",
		"00XX0000000XXX000000",
		"XXXX0X0XX00XXXXX00X0",
		"00XX000000000X00XXXX",
		"00XXX000XX00XXX00XX0",
		"XXXX0000XXXX00000XXX",
		"00XXXX00XX0000000000",
		"0000XX0000XX00XXX000",
		"XXX0000XX000XXX0000X",
		"XXX0000XX0XX00XXX000",
		"XXX0XX0XX000XXXX0000",
		"XXXXXX0XX00XXXXX00XX",
		"XXX00XXXX000XXX00XXX",
		"XXX00XXXX000XXX00XX0",
	},
};

int sort_bits(v2 p)
{
	int x, y;
	world2tile(p, &x, &y);
	return World::LEVEL_W * y + x;
}

int sort_bits(int x, int y)
{
	return World::LEVEL_W * y + x;
}

bool in_grid(int x, int y, int w, int h)
{
	return x >= 0 && y >= 0 && x < w && y < h;
}

bool in_board(int x, int y)
{
	return in_grid(x, y, World::LEVEL_W, World::LEVEL_H);
}

void init_bg_bricks(int seed)
{
	rnd_t rnd = rnd_seed(seed);
	int background_index = rnd_next_range(rnd, 0, background_maps.count() - 1);

	world->board.background_bricks.clear();
	transform_t transform = make_transform();
	for (int i = 0; i < 15; ++i) {
		for (int j = 0; j < 20; ++j) {
			sprite_t sprite;
			if (background_maps[background_index][i][j] == 'X') {
				if ((i & 1) ^ (j & 1)) {
					sprite = load_sprite("bricks_even.aseprite");
				} else {
					sprite = load_sprite("bricks_odd.aseprite");
				}
			} else {
				sprite = load_sprite("bricks_empty.aseprite");
			}
			sprite.frame_index = rnd_next_range(rnd, 0, sprite.frame_count() - 1);
			transform.p = v2((float)(j * 16 + 8 - 320/2), (float)((15 - 1 - i) * 16 + 8 - 240/2));
			world->board.background_bricks.add(sprite.batch_sprite(transform));
		}
	}
}

void draw_background_bricks_system_pre_update(app_t* app, float dt, void* udata)
{
	for (int i = 0; i < world->board.background_bricks.count(); ++i) {
		batch_push(batch, world->board.background_bricks[i]);
	}
	batch_flush(batch);
}

array<char> entity_codes = {
	'x', // IceBlock
	'1', // Box
	'e', // Ladder
	'p', // Player
	'c', // Mochi
	'0', // zzz
	'f', // Fire
	'o', // Oil
	'L', // Lamp
	't', // Torch
	'X', // BigIceBlock
};

void make_entity_at(const char* entity_type, int x, int y)
{
	for (int i = 0; i < schema_previews.count(); ++i) {
		if (!CUTE_STRCMP(entity_type, schema_previews[i].entity_type)) {
			make_entity_at(i, x, y);
			break;
		}
	}
}

void make_entity_at(int selection, int x, int y)
{
	const char* entity_type = schema_previews[selection].entity_type;
	cute::error_t err;
	entity_t e = entity_make(app, entity_type, &err);
	CUTE_ASSERT(!err.is_error());
	BoardSpace space;
	space.entity = e;
	space.code = entity_codes[selection];
	space.is_empty = false;
	BoardPiece* board_piece = (BoardPiece*)entity_get_component(app, e, "BoardPiece");
	board_piece->x = board_piece->x0 = x;
	board_piece->y = board_piece->y0 = y;
	if (board_piece->has_replicas) {
		board_piece->x_replicas[0] = board_piece->x + 1;
		board_piece->x_replicas[1] = board_piece->x + 1;
		board_piece->x_replicas[2] = board_piece->x;
		board_piece->y_replicas[0] = board_piece->y;
		board_piece->y_replicas[1] = board_piece->y - 1;
		board_piece->y_replicas[2] = board_piece->y - 1;
	}
	BoardSpace old_space = world->board.data[y][x];
	if (!old_space.is_empty) {
		destroy_entity_at(x, y);
	}
	if (board_piece->has_replicas) {
		for (int i = 0; i < 3; ++i) {
			int rx = board_piece->x_replicas[i];
			int ry = board_piece->y_replicas[i];
			destroy_entity_at(rx, ry);
		}
	}
	CUTE_ASSERT(!err.is_error());
	world->board.data[y][x] = space;
}

void destroy_entity_at(int x, int y)
{
	BoardSpace empty;
	empty.entity = INVALID_ENTITY;
	empty.code = '0';
	empty.is_empty = true;

	BoardSpace old_space = world->board.data[y][x];
	if (!old_space.is_empty) {
		BoardPiece* board_piece = (BoardPiece*)entity_get_component(app, old_space.entity, "BoardPiece");
		if (board_piece->has_replicas) {
			for (int i = 0; i < 3; ++i) {
				int rx = board_piece->x_replicas[i];
				int ry = board_piece->y_replicas[i];
				world->board.data[ry][rx] = empty;
			}
			world->board.data[board_piece->y][board_piece->x] = empty;
		}
		entity_destroy(app, old_space.entity);
	}
	world->board.data[y][x] = empty;
}

void delayed_destroy_entity_at(int x, int y)
{
	BoardSpace old_space = world->board.data[y][x];
	if (!old_space.is_empty) {
		entity_delayed_destroy(app, old_space.entity);
	}
	BoardSpace space;
	space.entity = INVALID_ENTITY;
	space.code = '0';
	space.is_empty = true;
	world->board.data[y][x] = space;
}

void debug_draw_non_empty_board_spaces()
{
	for (int i = 0; i < world->board.data.count(); ++i) {
		int len = world->board.data[i].count();
		for (int j = 0; j < len; ++j) {
			BoardSpace space = world->board.data[i][j];
			if (!space.is_empty) {
				v2 p = tile2world(j, i);
				batch_quad(batch, expand(make_aabb(p, p), v2(2, 2)), color_white());
			}
		}
	}
	batch_flush(batch);
}

void destroy_all_entities()
{
	// Delete old entities.
	for (int i = 0; i < world->board.data.count(); ++i) {
		int len = world->board.data[i].count();

		for (int j = 0; j < len; ++j) {
			BoardSpace space = world->board.data[i][j];
			if (!space.is_empty) {
				entity_destroy(app, space.entity);
			}
		}
	}

	// Reset board data.
	world->board.data.clear();
	world->board.data.ensure_count(World::LEVEL_H);
	for (int i = 0; i < World::LEVEL_H; ++i) {
		world->board.data[i].ensure_count(World::LEVEL_W);
	}
}

void select_level(int index)
{
	world->load_level_dirty_flag = false;

	if (world->level_index >= world->levels.size()) {
		char buf[1024];
		sprintf(buf, "Tried to load level %d, but the highest is %d. Loading level 0 instead.", world->level_index, world->levels.size() - 1);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "BAD LEVEL INDEX", buf);
		world->level_index = index = 0;
	}

	destroy_all_entities();

	// Load up new entities.
	const char* level_string = world->levels[index];
	int i = 0, j = 0;
	char c;
	while ((c = *level_string++)) {
		if (c == '\n') {
			++i;
			j = 0;
			continue;
		} else if (c == '\r') {
			continue;
		}

		entity_t e = INVALID_ENTITY;
		cute::error_t err;
		switch (c) {
		case '0': break;
		case '1': e = entity_make(app, "Box", &err); break;
		case 'x': e = entity_make(app, "IceBlock", &err); break;
		case 'p': e = entity_make(app, "Player", &err); break;
		case 'e': e = entity_make(app, "Ladder", &err); break;
		case 'c': e = entity_make(app, "Mochi", &err); break;
		case 'f': e = entity_make(app, "Fire", &err); break;
		case 'o': e = entity_make(app, "Oil", &err); break;
		case 'L': e = entity_make(app, "Lamp", &err); break;
		case 't': e = entity_make(app, "Torch", &err); break;
		case 'X': e = entity_make(app, "BigIceBlock", &err); break;
		}
		CUTE_ASSERT(!err.is_error());
		BoardSpace space;
		space.code = c;
		space.entity = e;
		space.is_empty = c == '0' ? true : false;
		if (!space.is_empty) {
			BoardPiece* board_piece = (BoardPiece*)entity_get_component(app, e, "BoardPiece");
			board_piece->x = board_piece->x0 = j;
			board_piece->y = board_piece->y0 = i;
			if (c == 'X') {
				board_piece->has_replicas = true;
				board_piece->x_replicas[0] = board_piece->x + 1;
				board_piece->x_replicas[1] = board_piece->x + 1;
				board_piece->x_replicas[2] = board_piece->x;
				board_piece->y_replicas[0] = board_piece->y;
				board_piece->y_replicas[1] = board_piece->y - 1;
				board_piece->y_replicas[2] = board_piece->y - 1;
			}
		}
		world->board.data[i][j] = space;

		++j;
	}

	world->level_name = world->level_names[world->level_index];
	init_bg_bricks(world->level_index);
	board_system_spread_out_replicas();
}

int select_level(const char* name)
{
	int index = 0;
	bool found = false;
	for (int i = 0; i < world->level_names.count(); ++i) {
		if (!CUTE_STRCMP(name, world->level_names[i])) {
			world->next_level(i);
			found = true;
			index = i;
			break;
		}
	}

	if (!found) {
		char buf[1024];
		sprintf(buf, "Unable to select level file %s.", name);
		window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "LEVEL FILE NOT FOUND", buf);
	}

	return index;
}

void reload_level(const char* name)
{
	int index = select_level(name);
	void* data;
	size_t sz;
	file_system_read_entire_file_to_memory_and_nul_terminate(name, &data, &sz);
	CUTE_FREE((void*)world->levels[index], NULL);
	world->levels[index] = (const char*)data;
	world->level_names[index] = name;
	world->level_name = name;
}

void play_sound(const char* path, float volume)
{
	static dictionary<const char*, audio_t*> sounds;
	audio_t* audio;
	if (sounds.find(path, &audio).is_error()) {
		audio = audio_load_wav(path);
		if (!audio) {
			char buf[1024];
			snprintf(buf, 1024, "Unable to find audio file %s.\n", path);
			window_message_box(app, WINDOW_MESSAGE_BOX_TYPE_ERROR, "File Not Found", buf);
			return;
		}
		sounds.insert(path, audio);
	}
	sound_params_t params;
	params.volume = volume;
	sound_play(app, audio, params);
}
