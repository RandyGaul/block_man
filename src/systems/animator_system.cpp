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

#include <systems/animator_system.h>
#include <components/transform.h>
#include <components/animator.h>

static float s_float_offset(Animator* animator, float dt)
{
	static float floating_offset = 0;
	coroutine_t* co = &animator->float_co;
	COROUTINE_START(co);
	floating_offset = 1.0f;
	COROUTINE_PAUSE(co, Animator::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(co, Animator::float_delay, dt);
	floating_offset = 3.0f;
	COROUTINE_PAUSE(co, Animator::float_delay, dt);
	floating_offset = 2.0f;
	COROUTINE_PAUSE(co, Animator::float_delay, dt);
	COROUTINE_END(co);
	return floating_offset;
}

void animator_transform_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;

		// Look this is already done inside the sprite itself... So no need!
		//transform->local.p -= animator->sprite.local_offset;

		if (animator->floating) {
			transform->local.p.y += s_float_offset(animator, dt);
		}
	}
}

void animator_system_update(app_t* app, float dt, void* udata, Transform* transforms, Animator* animators, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;
		Animator* animator = animators + i;

		transform_t tx = transform->get();
		animator->update(dt);
		animator->draw(batch, tx);
		//batch_quad(batch, make_aabb(tx.p, 2.0f, 2.0f), color_red());
		//batch_quad(batch, make_aabb(transform->world.p, 2.0f, 2.0f), color_blue());
	}
}

void animator_system_post_update(app_t* app, float dt, void* udata)
{
	batch_flush(batch);
}
