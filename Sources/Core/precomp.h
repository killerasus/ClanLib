/*
**  ClanLib SDK
**  Copyright (c) 1997-2013 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/

#pragma once


#ifdef WIN32
#ifdef _MSC_VER
# pragma warning (disable:4786)
#endif
#define NOMINMAX 
#include <windows.h>
#include <cstdlib>
#else
#include <unistd.h>
#endif

#include "API/Core/System/exception.h"

// Export explicit template instantiations from the Core DLL on Windows.
// Classes like Vec3/Quaternionx carry no CL_API macro (they are visible
// from headers), but several of their members are defined in
// Sources/Core/Math/*.cpp with explicit instantiation (e.g.
// Vec3<float>::distance used by clanSound). Without dllexport on those
// instantiations, other DLLs cannot link them.
#if defined(_WIN32) && defined(CL_API_DLL) && defined(CORE_EXPORT)
#define CL_TEMPLATE_EXPORT __declspec(dllexport)
#else
#define CL_TEMPLATE_EXPORT
#endif

#ifdef __BORLANDC__
#define BAD_MATH
#endif

#ifdef BAD_MATH
#define sqrtf sqrt
#define acosf acos
#define sinf sin
#define cosf cos
#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifdef WIN32
#define BREAKPOINT
#else
#define BREAKPOINT asm("int $03");
#endif


#include <cstring>
#include <algorithm>


