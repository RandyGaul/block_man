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

#include <cute.h>
using namespace cute;

#include <components/transform.h>

void transform_system_update(app_t* app, float dt, void* udata, Transform* transforms, int entity_count)
{
	for (int i = 0; i < entity_count; ++i) {
		Transform* transform = transforms + i;

		// Clear the transforms for each new game tick.
		// The other components build these up additively each game tick.
		transform->world = make_transform();
		transform->local = make_transform();
	}
}
