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

#include <components/transform.h>
#include <components/light.h>
#include <components/lamp.h>

#include <systems/light_system.h>

#include <shaders/light_shader.h>

static color_t s_gradient = make_color(0.0f, 0.0f, 0.0f, 0.25f);
static color_t s_darkness_color = make_color(0.0f, 0.0f, 0.0f, 0.25f);
static color_t s_crawlies_color = make_color(0.0f, 0.0f, 0.0f, 1.0f);
static color_t s_non_lamp_color = make_color(0.0f, 0.0f, 0.0f, 1.0f);
static array<v2> s_verts0;
static array<v2> s_verts1;
static array<v2> s_lamp_verts;
static array<v2> s_non_lamp_verts;
static sg_shader s_shd;
static sg_pipeline s_just_draw;
static sg_pipeline s_darkness;
static sg_pipeline s_clear;
static sg_pipeline s_cutout;
static triple_buffer_t s_buf;
static sg_buffer s_quad;
static rnd_t s_rnd;

Lamp* LAMP;

namespace Darkness
{
	static array<v2> verts;
	static float t;

	bool is_dark = false;
	float radius = radius_max;
	v2 center = v2(0, 0);

	struct lerp_pair_t
	{
		float a, b;
	};
	array<lerp_pair_t> lerps;
	float lerp_t = 0;
	float radius_t = 1.0f;

	void lerp_to(float t)
	{
		lerp_pair_t pair;
		pair.a = lerps.count() ? lerps.last().b : radius_t;
		pair.b = t;
		lerps.add(pair);
	}

	void do_lerp(float dt)
	{
		if (!lerps.count()) {
			radius = radius_max * Darkness::radius_t;
			return;
		}
		lerp_pair_t pair = lerps[0];
		lerp_t += dt;
		if (lerp_t >= lerp_delay) {
			lerp_t = 0;
			lerps.remove(0);
			Darkness::radius_t = pair.b;
		} else {
			float t = lerp_t / lerp_delay;
			Darkness::radius_t = lerp(pair.a, pair.b, smoothstep(t));
		}

		radius = radius_max * Darkness::radius_t;
	}

	void reset()
	{
		Darkness::radius = Darkness::radius_max;
		lerp_t = 0;
		radius_t = 1.0f;
		lerps.clear();
		is_dark = false;
	}
};

void light_system_init()
{
	s_shd = sg_make_shader(light_shd_shader_desc());

	sg_pipeline_desc params = { 0 };
	params.layout.buffers[0].stride = sizeof(v2);
	params.layout.buffers[0].step_func = SG_VERTEXSTEP_PER_VERTEX;
	params.layout.buffers[0].step_rate = 1;
	params.layout.attrs[0].buffer_index = 0;
	params.layout.attrs[0].offset = 0;
	params.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
	params.primitive_type = SG_PRIMITIVETYPE_TRIANGLES;
	params.shader = s_shd;

	sg_depth_stencil_state stencil;
	CUTE_MEMSET(&stencil, 0, sizeof(stencil));
	stencil.stencil_enabled = true;
	stencil.stencil_front.fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.depth_fail_op = SG_STENCILOP_KEEP;
	stencil.stencil_front.pass_op = SG_STENCILOP_REPLACE;
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_ALWAYS;
	stencil.stencil_back = stencil.stencil_front;
	stencil.stencil_read_mask = 0xFF;
	stencil.stencil_write_mask = 0xFF;
	stencil.stencil_ref = 0x1;
	sg_blend_state blend = { 0 };
	blend.color_write_mask = SG_COLORMASK_NONE;
	params.depth_stencil = stencil;
	params.blend = blend;
	s_clear = sg_make_pipeline(params);

	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_ZERO;
	stencil.stencil_back = stencil.stencil_front;
	params.depth_stencil = stencil;
	s_cutout = sg_make_pipeline(params);

	blend = { 0 };
	blend.enabled = true;
	blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	blend.op_rgb = SG_BLENDOP_ADD;
	blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	blend.op_alpha = SG_BLENDOP_ADD;
	stencil.stencil_front.compare_func = SG_COMPAREFUNC_EQUAL;
	stencil.stencil_front.pass_op = SG_STENCILOP_KEEP;
	stencil.stencil_back = stencil.stencil_front;
	params.depth_stencil = stencil;
	params.blend = blend;
	s_darkness = sg_make_pipeline(params);

	blend = { 0 };
	blend.enabled = true;
	blend.src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA;
	blend.dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	blend.op_rgb = SG_BLENDOP_ADD;
	blend.src_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
	blend.dst_factor_alpha = SG_BLENDFACTOR_ONE;
	blend.op_alpha = SG_BLENDOP_ADD;
	CUTE_MEMSET(&stencil, 0, sizeof(stencil));
	params.blend = blend;
	params.depth_stencil = stencil;
	s_just_draw = sg_make_pipeline(params);

	s_buf = triple_buffer_make(sizeof(v2) * 1024 * 5, sizeof(v2));

	v2 fullscreen_quad[6] = {
		v2(-1, -1),
		v2( 1,  1),
		v2(-1,  1),
		v2( 1,  1),
		v2(-1, -1),
		v2( 1, -1),
	};
	sg_buffer_desc quad = { 0 };
	quad.type = SG_BUFFERTYPE_VERTEXBUFFER;
	quad.usage = SG_USAGE_IMMUTABLE;
	quad.size = sizeof(v2) * 6;
	quad.content = fullscreen_quad;
	s_quad = sg_make_buffer(quad);

	s_rnd = rnd_seed(0);
}

static void s_add_light(Light* light, v2 p, float r_offset, array<v2>* verts)
{
	float r = light->radius + light->radius_delta + r_offset;
	v2 prev = v2(r, 0);
	int iters = 40;

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * 3.14159265f);
		v2 next = from_angle(a) * r;
		verts->add(p + prev);
		verts->add(p + next);
		verts->add(p);
		prev = next;
	}
}

static float s_pulse(coroutine_t* co, float dt)
{
	float* pulse_ptr = (float*)coroutine_local_var(co, sizeof(float));
	float pulse = *pulse_ptr;

	const float t0 = 0.05f;
	const float t1 = 0.25f;
	const float r0 = 0.5f;
	COROUTINE_START(co);
	pulse = 0.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_PAUSE(co, rnd_next_range(&s_rnd, t0, t1), dt);
	pulse = 1.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_PAUSE(co, rnd_next_range(&s_rnd, t0, t1), dt);
	pulse = 2.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_PAUSE(co, rnd_next_range(&s_rnd, t0, t1), dt);
	pulse = 3.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_PAUSE(co, rnd_next_range(&s_rnd, t0, t1), dt);
	pulse = 2.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_PAUSE(co, rnd_next_range(&s_rnd, t0, t1), dt);
	pulse = 1.0f + rnd_next_range(&s_rnd, -r0, r0);
	COROUTINE_END(co);

	*pulse_ptr = pulse;
	return pulse;
}

void light_system_update(app_t* app, float dt, void* udata, Transform* transforms, Light* lights, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Light* light = lights + i;

		if (light->is_lamp) {
			light->radius = Darkness::radius;

			// Center all of the darkness onto the lamp.
			Darkness::center = transform->get().p;
		}
		light->radius_delta = s_pulse(&light->co, dt);
		v2 p = transform->get().p;

		// This if-else-if-statement prevents a weird artifact where non-lamp lights cutout
		// the gradient section of the lamp lights.
		if (!light->is_lamp && s_darkness_color.a != 1) {
			s_add_light(light, p, 0, &s_verts0);
		} else if (light->is_lamp) {
			s_add_light(light, p, 0, &s_verts0);
		}

		s_add_light(light, p, 3, &s_verts1);
		if (!light->is_lamp) {
			s_add_light(light, p, 3, &s_non_lamp_verts);
		} else {
			s_add_light(light, p, 3, &s_lamp_verts);
		}
	}
}

void s_uniforms(matrix_t mvp, color_t color)
{
	light_vs_params_t vs_params;
	light_fs_params_t fs_params;
	vs_params.u_mvp = mvp;
	sg_apply_uniforms(SG_SHADERSTAGE_VS, 0, &vs_params, sizeof(vs_params));
	fs_params.u_color = color;
	sg_apply_uniforms(SG_SHADERSTAGE_FS, 0, &fs_params, sizeof(fs_params));
}

static void s_do_lights(array<v2>* verts, color_t color)
{
	sg_bindings bind = { 0 };
	bind.vertex_buffers[0] = s_quad;

	// Set stencil buffer to all 1's.
	sg_apply_pipeline(s_clear);
	sg_apply_bindings(bind);
	s_uniforms(matrix_identity(), color);
	sg_draw(0, 6, 1);

	// Cutout light radii.
	if (verts->count()) {
		sg_apply_pipeline(s_cutout);
		error_t err = triple_buffer_append(&s_buf, verts->count(), verts->data());
		CUTE_ASSERT(!err.is_error());
		sg_apply_bindings(s_buf.bind());
		s_uniforms(matrix_ortho_2d(320, 240, 0, 0), color);
		sg_draw(0, verts->count(), 1);
		s_buf.advance();
	}

	// Render the darkness.
	sg_apply_pipeline(s_darkness);
	sg_apply_bindings(bind);
	s_uniforms(matrix_identity(), color);
	sg_draw(0, 6, 1);
}

static void s_non_lamp_lights(color_t color)
{
	if (s_non_lamp_verts.count()) {
		// Set stencil buffer to all 1's on the non-lamp lights.
		sg_apply_pipeline(s_clear);
		error_t err = triple_buffer_append(&s_buf, s_non_lamp_verts.count(), s_non_lamp_verts.data());
		CUTE_ASSERT(!err.is_error());
		sg_apply_bindings(s_buf.bind());
		s_uniforms(matrix_ortho_2d(320, 240, 0, 0), color);
		sg_draw(0, s_non_lamp_verts.count(), 1);
		s_buf.advance();

		// Cutout lamp radii.
		if (s_lamp_verts.count()) {
			sg_apply_pipeline(s_cutout);
			error_t err = triple_buffer_append(&s_buf, s_lamp_verts.count(), s_lamp_verts.data());
			CUTE_ASSERT(!err.is_error());
			sg_apply_bindings(s_buf.bind());
			s_uniforms(matrix_ortho_2d(320, 240, 0, 0), color);
			sg_draw(0, s_lamp_verts.count(), 1);
			s_buf.advance();
		}

		// Render the darkness.
		sg_apply_pipeline(s_darkness);
		sg_bindings bind = { 0 };
		bind.vertex_buffers[0] = s_quad;
		sg_apply_bindings(bind);
		s_uniforms(matrix_identity(), color);
		sg_draw(0, 6, 1);
	}
}

static void s_spinner(float r, float thickness, float a0, float a1, v2 offset, float timescale)
{
	transform_t tx = make_transform(Darkness::center + offset, Darkness::t * timescale);
	float delta = angle_diff(a0, a1);
	v2 prev0 = from_angle(a0) * r;
	v2 prev1 = from_angle(a0) * r;

	for (int i = 1; i <= 20; ++i) {
		float a = delta * (i / 20.0f);
		float t = (i / 20.0f);
		if (t < 0.5f) {
			t = remap(t, 0.0f, 0.5f);
		} else {
			t = 1.0f - remap(t, 0.5f, 1.0f);
		}
		v2 v = from_angle(a);
		v2 next0 = v * (r);
		v2 next1 = v * (r - ease_out_sin(t) * thickness);

		Darkness::verts.add(mul(tx, prev0));
		Darkness::verts.add(mul(tx, prev1));
		Darkness::verts.add(mul(tx, next0));
		Darkness::verts.add(mul(tx, next0));
		Darkness::verts.add(mul(tx, prev1));
		Darkness::verts.add(mul(tx, next1));

		prev0 = next0;
		prev1 = next1;
	}
}

static void s_pulser(float r, v2 p)
{
	v2 prev = v2(r, 0);
	int iters = 20;

	for (int i = 1; i <= iters; ++i) {
		float a = (i / (float)iters) * (2.0f * CUTE_PI);
		v2 next = from_angle(a) * r;
		Darkness::verts.add(p + prev);
		Darkness::verts.add(p + next);
		Darkness::verts.add(p);
		prev = next;
	}
}

static void s_draw_crawlies()
{
	sg_apply_pipeline(s_just_draw);
	error_t err = triple_buffer_append(&s_buf, Darkness::verts.count(), Darkness::verts.data());
	CUTE_ASSERT(!err.is_error());
	sg_apply_bindings(s_buf.bind());
	s_uniforms(matrix_ortho_2d(320, 240, 0, 0), s_crawlies_color);
	sg_draw(0, Darkness::verts.count(), 1);
	s_buf.advance();
	Darkness::verts.clear();
}

static void s_pulsers(float dt)
{
	static coroutine_t pulse_co;
	rnd_t rnd = rnd_seed(2);
	float pulse = s_pulse(&pulse_co, dt);
	float r = 7.0f + Darkness::radius;
	float t = remap(Darkness::radius / Darkness::radius_max, 0.0f, 0.5f);
	int iters = 15;

	for (int i = 0; i < iters; ++i) {
		v2 d = from_angle(rnd_next_range(&rnd, 0.0f, 2.0f * CUTE_PI));
		v2 p = Darkness::center + d * r + v2(rnd_next_range(&rnd, -1.0f, 1.0f), rnd_next_range(&rnd, -1.0f, 1.0f)) * 2;
		float pulse_r = rnd_next_range(&rnd, 1.0f, 5.0f) + pulse + t * 10.0f;
		v2 p_offset = d * t * pulse_r * 0.5f;
		s_pulser(pulse_r, p + p_offset);
	}
} 

static void s_spinners(float dt)
{
	rnd_t rnd = rnd_seed(0);
	float t = remap(Darkness::radius / Darkness::radius_max, 0.0f, 0.5f);
	int iters = 3;

	for (int i = 0; i < iters; ++i) {
		float r = rnd_next_range(&rnd, 16.0f, 32.0f);
		float thickness = rnd_next_range(&rnd, 3.0f, 6.0f);
		float timescale = rnd_next_range(&rnd, 0.5f, 1.0f);
		float a_max = rnd_next_range(&rnd, CUTE_PI * 0.5f, CUTE_PI * 1.5f);
		v2 d = from_angle(rnd_next_range(&rnd, 0.0f, rnd_next_range(&rnd, CUTE_PI*(3.0f/2.0f), CUTE_PI*(3.0f/2.0f) + CUTE_PI*0.5f)));
		v2 p = d * (Darkness::radius + r);
		v2 offset = d * 3.5f;
		s_spinner(r, thickness, 0, a_max, p - offset, timescale);
	}
}

static void s_do_crawlies(float dt)
{
	s_spinners(dt);
	s_pulsers(dt);
	s_draw_crawlies();
}

void light_system_post_update(app_t* app, float dt, void* udata)
{
	Darkness::do_lerp(dt);
	Darkness::t = Darkness::t + CUTE_PI * dt * 0.5f;
	float t = 1.0f - Darkness::radius_t;
	s_darkness_color.a = clamp01(remap(t, 0, 0.6f));
	s_crawlies_color.a = clamp01(remap(t, 0.6f, 0.7f));
	s_non_lamp_color.a = clamp01(remap(t, 0.5f, 0.6f));

	s_do_lights(&s_verts0, s_gradient);
	s_do_lights(&s_verts1, s_darkness_color);
	s_non_lamp_lights(s_non_lamp_color);

	if (s_darkness_color.a == 1) {
		Darkness::is_dark = true;
		s_do_crawlies(dt);
	} else {
		Darkness::is_dark = false;
	}

	s_verts0.clear();
	s_verts1.clear();
	s_lamp_verts.clear();
	s_non_lamp_verts.clear();
}
