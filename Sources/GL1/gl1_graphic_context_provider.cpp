/*
**  ClanLib SDK
**  Copyright (c) 1997-2012 The ClanLib Team
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
**    Harry Storbacka
**    Ken Hirsch
**    Claudio Zanella
*/

#include "GL1/precomp.h"
#include "gl1_graphic_context_provider.h"
#include "gl1_texture_provider.h"
#include "gl1_frame_buffer_provider.h"
#include "gl1_program_object_provider.h"
#include "gl1_render_buffer_provider.h"
#include "gl1_primitives_array_provider.h"
#include "gl1_vertex_array_buffer_provider.h"
#include "gl1_uniform_buffer_provider.h"
#include "gl1_transfer_buffer_provider.h"
#include "API/Core/IOData/cl_endian.h"
#include "API/Core/System/databuffer.h"
#include "API/Core/Math/vec3.h"
#include "API/Core/Text/string_help.h"
#include "API/Core/Text/string_format.h"
#include "API/Display/Render/texture.h"
#include "API/Display/Render/primitives_array.h"
#include "API/Display/Image/pixel_buffer.h"
#include "API/Display/Render/frame_buffer.h"
#include "API/Display/Render/primitives_array.h"
#include "API/Display/Render/program_object.h"
#include "API/Display/TargetProviders/display_window_provider.h"
#include "API/Display/Render/shared_gc_data.h"
#include "API/GL1/light_model.h"
#include "API/GL1/light_source.h"
#include "API/GL1/material.h"
#include "API/GL1/texture_unit.h"
#include "opengl1.h"
#include "opengl1_wrap.h"
#include "API/Display/2D/image.h"

#include "../Display/2D/render_batch_triangle.h"

#ifdef WIN32
#include "WGL/gl1_window_provider_wgl.h"
#else
#include "GLX/gl1_window_provider_glx.h"
#include "GLX/pbuffer_impl.h"
#endif
#include "pbuffer.h"

namespace clan
{

class GL1SelectedTexture
{
public:
	GL1SelectedTexture() : texture(NULL) {};

	GL1TextureProvider *texture;
};

/////////////////////////////////////////////////////////////////////////////
// GL1GraphicContextProvider Construction:

GL1GraphicContextProvider::GL1GraphicContextProvider(const DisplayWindowProvider * const render_window, bool render_window_is_a_pbuffer)
: render_window(render_window),
  framebuffer_bound(false), prim_arrays_set(false), num_set_tex_arrays(0),
  primitives_array_texture_set(false), primitives_array_texindex_set(false), scissor_enabled(false)
{
	check_opengl_version();
	max_texture_coords = get_max_texture_coords();
	// Hack, so the sprite render batcher does not exceed the allowed number of textures
	if (max_texture_coords < RenderBatchTriangle::max_textures)
	{
		if (max_texture_coords > 0)
		{
			RenderBatchTriangle::max_textures = max_texture_coords;
		}
		else
		{
			RenderBatchTriangle::max_textures = 1;
		}
	}
	// Limit the internal texture coords, to avoid situations where the opengl driver says there are unlimited texture coords
	if (max_texture_coords > 255)
		max_texture_coords = 255;

	selected_textures.resize(max_texture_coords);

	internal_program_provider = new GL1ProgramObjectProvider(this);	// <-- To be owned by "standard_program"
	internal_program = ProgramObject(internal_program_provider);

	// Enable point sprites for legacy opengl
	cl1Enable(GL_POINT_SPRITE);

	if (!render_window_is_a_pbuffer)		// It is not clear if sharing GC from a pbuffer is valid from the specification. Also this is problematic in obtaining the shared gc context in the main provider. For now, we simply do not allow it.
		SharedGCData::add_provider(this);
}

GL1GraphicContextProvider::~GL1GraphicContextProvider()
{
	dispose();
}

void GL1GraphicContextProvider::on_dispose()
{
	while (!disposable_objects.empty())
		disposable_objects.front()->dispose();

	SharedGCData::remove_provider(this);
	GL1::remove_active(this);
}

void GL1GraphicContextProvider::add_disposable(DisposableObject *disposable)
{
	disposable_objects.push_back(disposable);
}

void GL1GraphicContextProvider::remove_disposable(DisposableObject *disposable)
{
	for (size_t i = 0; i < disposable_objects.size(); i++)
	{
		if (disposable_objects[i] == disposable)
		{
			disposable_objects.erase(disposable_objects.begin() + i);
			return;
		}
	}
}

void GL1GraphicContextProvider::check_opengl_version()
{
	int version_major = 0;
	int version_minor = 0;
	int version_release = 0;

	get_opengl_version(version_major, version_minor, version_release);
	bool valid_version = false;
	if (version_major > 1)
		valid_version = true;
	if ( (version_major == 1) && (version_minor >= 3) )
		valid_version = true;

	if(!valid_version)
		throw Exception(string_format("This application requires OpenGL 1.3. Your hardware only supports OpenGL %1.%2.%3. Try updating your drivers, or upgrade to a newer graphics card.", version_major, version_minor, version_release));
}

void GL1GraphicContextProvider::get_opengl_version(int &version_major, int &version_minor, int &version_release)
{
/*	The GL_VERSION string begins with a version number. The version number uses one of these forms: 
	major_number.minor_number 
	major_number.minor_number.release_number 
	Vendor-specific information may follow the version number. Its format depends on the implementation, but a space always separates the version number and the vendor-specific information. 
	All strings are null-terminated. 
	If an error is generated, glGetString returns zero.
*/
	GL1::set_active(this);
	std::string version = (char*)cl1GetString(GL_VERSION);
	version_major = 0;
	version_minor = 0;
	version_release = 0;

	std::vector<std::string> split_version = StringHelp::split_text(version, ".");
	if(split_version.size() > 0)
		version_major = StringHelp::text_to_int(split_version[0]);
	if(split_version.size() > 1)
		version_minor = StringHelp::text_to_int(split_version[1]);
	if(split_version.size() > 2)
		version_release = StringHelp::text_to_int(split_version[2]);
}

/////////////////////////////////////////////////////////////////////////////
// GL1GraphicContextProvider Attributes:

int GL1GraphicContextProvider::get_max_texture_coords()
{
	set_active();
	GLint max_textures = 0;
	cl1GetIntegerv(GL_MAX_TEXTURE_COORDS, &max_textures);

	if(cl1GetError() == GL_INVALID_ENUM)
		cl1GetIntegerv(GL_MAX_TEXTURE_UNITS, &max_textures);

	return max_textures;
}

int GL1GraphicContextProvider::get_max_attributes()
{
	set_active();
	GLint max_attributes = 0;
	cl1GetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attributes);
    if(max_attributes < 16)
        max_attributes = 16;
	return max_attributes;
}

int GL1GraphicContextProvider::get_light_count()
{
	set_active();
	GLint max_lights = 0;
	cl1GetIntegerv(GL_MAX_LIGHTS, &max_lights);
	return max_lights;
}

Size GL1GraphicContextProvider::get_max_texture_size() const
{
	set_active();
	GLint max_size = 0;
	cl1GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	return Size(max_size, max_size);
}

Size GL1GraphicContextProvider::get_display_window_size() const
{
	return render_window->get_viewport().get_size();
}

PBuffer_GL1 GL1GraphicContextProvider::create_pbuffer(Size size)
{
#ifdef WIN32
	const GL1WindowProvider_WGL *wptr = dynamic_cast<const GL1WindowProvider_WGL *> (render_window);
	if (wptr == NULL)
		throw Exception("Render window type is not known");

	return ((GL1WindowProvider_WGL *) wptr)->create_pbuffer(this, size);
#else
	const GL1WindowProvider_GLX *wptr = dynamic_cast<const GL1WindowProvider_GLX *> (render_window);
	if (wptr == NULL)
		throw Exception("Render window type is not known");

	return ((GL1WindowProvider_GLX *) wptr)->create_pbuffer(this, size);
#endif

}

#ifdef __APPLE__
static CFBundleRef gl1_gBundleRefOpenGL = 0;
#endif

GL1ProcAddress *GL1GraphicContextProvider::get_proc_address(const std::string& function_name) const
{

#ifdef WIN32
	return (void (*)())wglGetProcAddress(function_name.c_str());
#else
#ifdef __APPLE__
	// Mac OS X doesn't have an OpenGL extension fetch function. Isn't that silly?
	if (gl1_gBundleRefOpenGL == 0)
	{
		gl1_gBundleRefOpenGL = CFBundleGetBundleWithIdentifier(CFSTR("com.apple.opengl"));
		if (gl1_gBundleRefOpenGL == 0)
			throw Exception("Unable to find com.apple.opengl bundle");
	}

	return (GL1ProcAddress *) CFBundleGetFunctionPointerForName(
		gl1_gBundleRefOpenGL,
		CFStringCreateWithCStringNoCopy(
			0,
			function_name.c_str(),
			CFStringGetSystemEncoding(),
			0));
#else
	// FIXME: This is very messy
	const GL1WindowProvider_GLX *wptr = dynamic_cast<const GL1WindowProvider_GLX *> (render_window);
	if (wptr)
		return wptr->get_proc_address(function_name);

	const PBuffer_GL1_Impl *pptr = dynamic_cast<const PBuffer_GL1_Impl *> (render_window);
	if (pptr)
		return pptr->get_proc_address(function_name);

	return NULL;

#endif
#endif

}

/////////////////////////////////////////////////////////////////////////////
// GL1GraphicContextProvider Operations:

void GL1GraphicContextProvider::set_active() const
{
	if (framebuffer_bound)
	{
		framebuffer_provider->set_active();
	}
	else
	{
		GL1::set_active(this);
	}
}

OcclusionQueryProvider *GL1GraphicContextProvider::alloc_occlusion_query()
{
	throw Exception("Occlusion Queries are not supported for OpenGL 1.3");
}

ProgramObjectProvider *GL1GraphicContextProvider::alloc_program_object()
{
	throw Exception("Program Objects are not supported for OpenGL 1.3");
}

ShaderObjectProvider *GL1GraphicContextProvider::alloc_shader_object()
{
	throw Exception("Shader Objects are not supported for OpenGL 1.3");
}

TextureProvider *GL1GraphicContextProvider::alloc_texture(TextureDimensions texture_dimensions)
{
	return new GL1TextureProvider(texture_dimensions);
}

FrameBufferProvider *GL1GraphicContextProvider::alloc_frame_buffer()
{
	return new GL1FrameBufferProvider(this);
}

RenderBufferProvider *GL1GraphicContextProvider::alloc_render_buffer()
{
	return new GL1RenderBufferProvider(this);
}

VertexArrayBufferProvider *GL1GraphicContextProvider::alloc_vertex_array_buffer()
{
	return new GL1VertexArrayBufferProvider;
}

UniformBufferProvider *GL1GraphicContextProvider::alloc_uniform_buffer()
{
	return new GL1UniformBufferProvider;
}

StorageBufferProvider *GL1GraphicContextProvider::alloc_storage_buffer()
{
	throw Exception("Storage Buffers are not supported for OpenGL 1.3");
}

ElementArrayBufferProvider *GL1GraphicContextProvider::alloc_element_array_buffer()
{
	throw Exception("Element Array Buffers are not supported for OpenGL 1.3");
}

TransferBufferProvider *GL1GraphicContextProvider::alloc_transfer_buffer()
{
	return new GL1TransferBufferProvider;
}

PixelBufferProvider *GL1GraphicContextProvider::alloc_pixel_buffer()
{
	throw Exception("Pixel Buffers Objects are not supported for OpenGL 1.3");
}

PrimitivesArrayProvider *GL1GraphicContextProvider::alloc_primitives_array()
{
	return new GL1PrimitivesArrayProvider();
}

std::shared_ptr<RasterizerStateProvider> GL1GraphicContextProvider::create_rasterizer_state(const RasterizerStateDescription &desc)
{
	std::map<RasterizerStateDescription, std::shared_ptr<RasterizerStateProvider> >::iterator it = rasterizer_states.find(desc);
	if (it != rasterizer_states.end())
	{
		return it->second;
	}
	else
	{
		std::shared_ptr<RasterizerStateProvider> state(new GL1RasterizerStateProvider(desc));
		rasterizer_states[desc.clone()] = state;
		return state;
	}
}

std::shared_ptr<BlendStateProvider> GL1GraphicContextProvider::create_blend_state(const BlendStateDescription &desc)
{
	std::map<BlendStateDescription, std::shared_ptr<BlendStateProvider> >::iterator it = blend_states.find(desc);
	if (it != blend_states.end())
	{
		return it->second;
	}
	else
	{
		std::shared_ptr<BlendStateProvider> state(new GL1BlendStateProvider(desc));
		blend_states[desc.clone()] = state;
		return state;
	}
}

std::shared_ptr<DepthStencilStateProvider> GL1GraphicContextProvider::create_depth_stencil_state(const DepthStencilStateDescription &desc)
{
	std::map<DepthStencilStateDescription, std::shared_ptr<DepthStencilStateProvider> >::iterator it = depth_stencil_states.find(desc);
	if (it != depth_stencil_states.end())
	{
		return it->second;
	}
	else
	{
		std::shared_ptr<DepthStencilStateProvider> state(new GL1DepthStencilStateProvider(desc));
		depth_stencil_states[desc.clone()] = state;
		return state;
	}
}

void GL1GraphicContextProvider::set_rasterizer_state(RasterizerStateProvider *state)
{
	if (state)
	{
		GL1RasterizerStateProvider *gl1_state = static_cast<GL1RasterizerStateProvider*>(state);

		set_culled(gl1_state->desc.get_culled());
		enable_line_antialiasing(gl1_state->desc.get_enable_line_antialiasing());
		set_face_cull_mode(gl1_state->desc.get_face_cull_mode());
		set_face_fill_mode(gl1_state->desc.get_face_fill_mode());
		set_front_face(gl1_state->desc.get_front_face());
		scissor_enabled = gl1_state->desc.get_enable_scissor();

	}
}

void GL1GraphicContextProvider::set_blend_state(BlendStateProvider *state, const Vec4f &blend_color, unsigned int sample_mask)
{
	if (state)
	{
		GL1BlendStateProvider *gl1_state = static_cast<GL1BlendStateProvider*>(state);

		bool red, green, blue, alpha;
		BlendEquation equation_color, equation_alpha;
		BlendFunc src, dest, src_alpha, dest_alpha;
		gl1_state->desc.get_color_write(red, green, blue, alpha);
		gl1_state->desc.get_blend_equation(equation_color, equation_alpha);
		gl1_state->desc.get_blend_function(src, dest, src_alpha, dest_alpha);

		enable_color_write(red, green, blue, alpha);
		enable_blending(gl1_state->desc.is_blending_enabled());
		set_blend_color(Colorf(blend_color));
		set_blend_equation(equation_color, equation_alpha);
		set_blend_function(src, dest, src_alpha, dest_alpha);
	}
}

void GL1GraphicContextProvider::set_depth_stencil_state(DepthStencilStateProvider *state, int stencil_ref)
{
	if (state)
	{
		GL1DepthStencilStateProvider *gl1_state = static_cast<GL1DepthStencilStateProvider*>(state);

		CompareFunction front; int front_ref; int front_mask;
		CompareFunction back; int back_ref; int back_mask;
		unsigned char front_facing_mask, back_facing_mask;
		StencilOp fail_front, pass_depth_fail_front, pass_depth_pass_front;
		StencilOp fail_back, pass_depth_fail_back, pass_depth_pass_back;
		gl1_state->desc.get_stencil_compare_front(front, front_ref, front_mask);
		gl1_state->desc.get_stencil_compare_back(back, back_ref, back_mask);
		gl1_state->desc.get_stencil_write_mask(front_facing_mask, back_facing_mask);
		gl1_state->desc.get_stencil_op_front(fail_front, pass_depth_fail_front, pass_depth_pass_front);
		gl1_state->desc.get_stencil_op_back(fail_back, pass_depth_fail_back, pass_depth_pass_back);

		enable_stencil_test(gl1_state->desc.is_stencil_test_enabled());
		set_stencil_compare_front(front, front_ref, front_mask);
		set_stencil_compare_back(back, back_ref, back_mask);
		set_stencil_write_mask(front_facing_mask, back_facing_mask);
		set_stencil_op_front(fail_front, pass_depth_fail_front, pass_depth_pass_front);
		set_stencil_op_back(fail_back, pass_depth_fail_back, pass_depth_pass_back);
		enable_depth_test(gl1_state->desc.is_depth_test_enabled());
		enable_depth_write(gl1_state->desc.is_depth_write_enabled());
		set_depth_compare_function(gl1_state->desc.get_depth_compare_function());
	}
}

PixelBuffer GL1GraphicContextProvider::get_pixeldata(const Rect& rect, TextureFormat texture_format, bool clamp) const 
{
	GLenum format;
	GLenum type;
	bool found = GL1::to_opengl_pixelformat(texture_format, format, type);
	if (!found)
		throw Exception("Unsupported pixel format passed to GraphicContext::get_pixeldata");

	PixelBuffer pbuf(rect.get_width(), rect.get_height(), texture_format);
	set_active();
	if (!framebuffer_bound)
		cl1ReadBuffer(GL_BACK);

	Size display_size = get_display_window_size();

	cl1PixelStorei(GL_PACK_ALIGNMENT, 1);
	cl1PixelStorei(GL_PACK_ROW_LENGTH, pbuf.get_pitch() / pbuf.get_bytes_per_pixel());
	cl1PixelStorei(GL_PACK_SKIP_PIXELS, 0);
	cl1PixelStorei(GL_PACK_SKIP_ROWS, 0);
	cl1ReadPixels(rect.left, display_size.height - rect.bottom, rect.get_width(), rect.get_height(), format, type, pbuf.get_data());
	pbuf.flip_vertical();
	return pbuf;
}

void GL1GraphicContextProvider::set_texture_unit(int unit_index, const TextureUnit_GL1 &unit)
{
	set_active();

	cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	
	cl1TexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, (float)unit.get_lod_bias() );

	float color_array[4];
	Colorf color = unit.get_color();
	color_array[0] = color.r;
	color_array[1] = color.g;
	color_array[2] = color.b;
	color_array[3] = color.a;

	cl1TexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color_array);
	cl1TexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, to_enum(unit.get_combine_rgb_mode()) );
	cl1TexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, to_enum(unit.get_combine_alpha_mode()) );
	cl1TexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, to_enum(unit.get_mode()));

	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC0_RGB, to_enum(unit.get_combine_arg_src_rgb(0)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC1_RGB, to_enum(unit.get_combine_arg_src_rgb(1)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC2_RGB, to_enum(unit.get_combine_arg_src_rgb(2)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC0_ALPHA, to_enum(unit.get_combine_arg_src_alpha(0)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC1_ALPHA, to_enum(unit.get_combine_arg_src_alpha(1)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_SRC2_ALPHA, to_enum(unit.get_combine_arg_src_alpha(2)));

	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_RGB, to_enum(unit.get_combine_arg_operand_rgb(0)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_RGB, to_enum(unit.get_combine_arg_operand_rgb(1)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_RGB, to_enum(unit.get_combine_arg_operand_rgb(2)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, to_enum(unit.get_combine_arg_operand_alpha(0)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, to_enum(unit.get_combine_arg_operand_alpha(1)));
	cl1TexEnvi( GL_TEXTURE_ENV, GL_OPERAND2_ALPHA, to_enum(unit.get_combine_arg_operand_alpha(2)));

	cl1TexEnvi( GL_POINT_SPRITE, GL_COORD_REPLACE, unit.get_point_coord_replace() ? GL_TRUE : GL_FALSE);
}

void GL1GraphicContextProvider::set_uniform_buffer(int index, const UniformBuffer &buffer)
{
	GL1UniformBufferProvider *provider = static_cast<GL1UniformBufferProvider*>(buffer.get_provider());

	const char *data = (const char *) provider->get_data();

	///   mat4 modelviewMatrix;
	///   mat4 projectionMatrix;
	///   mat4 modelviewProjectionMatrix;
	///   mat3 normalMatrix;
	Mat4f modelview( (float * ) (data) );
	Mat4f projection( (float * ) (data + (sizeof(float) * 16 )));

	if (last_projection != projection)
	{
		last_projection = projection;
		set_active();
		cl1MatrixMode(GL_PROJECTION);
		cl1LoadMatrixf(last_projection.matrix);
	}

	if (last_modelview != modelview)
	{
		last_modelview = modelview;
		set_active();
		cl1MatrixMode(GL_MODELVIEW);
		cl1LoadMatrixf(last_modelview);
	}
}

void GL1GraphicContextProvider::reset_uniform_buffer(int index)
{
}

void GL1GraphicContextProvider::set_storage_buffer(int index, const StorageBuffer &buffer)
{
}

void GL1GraphicContextProvider::reset_storage_buffer(int index)
{
}

void GL1GraphicContextProvider::set_texture(int unit_index, const Texture &texture)
{
	set_active();

	if ( (unit_index <0) || (unit_index >= max_texture_coords) )
	{
		throw Exception("Invalid texture unit index in GL1 target");
	}
	selected_textures[unit_index].texture = NULL;

	if (cl1ActiveTexture != 0)
	{
		cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	}
	else if (unit_index > 0)
	{
		return;
	}

	if (texture.is_null())
	{
		cl1Disable(GL_TEXTURE_1D);
		cl1Disable(GL_TEXTURE_2D);
		cl1Disable(GL_TEXTURE_3D);
		cl1Disable(GL_TEXTURE_CUBE_MAP);
	}
	else
	{
		GL1TextureProvider *provider = static_cast<GL1TextureProvider *>(texture.get_provider());
		selected_textures[unit_index].texture = provider;

		cl1Enable(provider->get_texture_type());
		cl1BindTexture(provider->get_texture_type(), provider->get_handle());
	}
}

void GL1GraphicContextProvider::reset_texture(int unit_index)
{
	set_active();

	if ( (unit_index >=0) && (unit_index < max_texture_coords) )
	{
		selected_textures[unit_index].texture = NULL;
	}

	if (cl1ActiveTexture != 0)
	{
		cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	}
	else if (unit_index > 0)
	{
		return;
	}

	cl1Disable(GL_TEXTURE_1D);
	cl1Disable(GL_TEXTURE_2D);
	cl1Disable(GL_TEXTURE_3D);
	cl1Disable(GL_TEXTURE_CUBE_MAP);
	cl1BindTexture(GL_TEXTURE_2D, 0);

}

void GL1GraphicContextProvider::set_image_texture(int unit_index, const Texture &texture)
{
}

void GL1GraphicContextProvider::reset_image_texture(int unit_index)
{
}

void GL1GraphicContextProvider::push_texture_matrix(int unit_index)
{
	set_active();
	cl1MatrixMode(GL_TEXTURE);
	cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	cl1PushMatrix();
	cl1MatrixMode(GL_MODELVIEW);
}

void GL1GraphicContextProvider::set_texture_matrix(int unit_index, const Mat4f &matrix)
{
	set_active();
	cl1MatrixMode(GL_TEXTURE);
	cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	cl1LoadMatrixf(matrix);
	cl1MatrixMode(GL_MODELVIEW);
}

void GL1GraphicContextProvider::pop_texture_matrix(int unit_index)
{
	set_active();
	cl1MatrixMode(GL_TEXTURE);
	cl1ActiveTexture( GL_TEXTURE0 + unit_index );
	cl1PopMatrix();
	cl1MatrixMode(GL_MODELVIEW);
}

bool GL1GraphicContextProvider::is_frame_buffer_owner(const FrameBuffer &fb)
{
	return true;
	/*GL1FrameBufferProvider *fb_provider = dynamic_cast<GL1FrameBufferProvider *>(fb.get_provider());
	if (fb_provider)
		return fb_provider->get_gc_provider() == this;
	else
		return false;*/
}

void GL1GraphicContextProvider::set_frame_buffer(const FrameBuffer &w_buffer, const FrameBuffer &r_buffer)
{
	framebuffer_provider = dynamic_cast<GL1FrameBufferProvider *>(w_buffer.get_provider());
	framebuffer_provider->set_active();
	framebuffer_provider->start();

	framebuffer_bound = true;
}

void GL1GraphicContextProvider::reset_frame_buffer()
{
	if (framebuffer_bound)
	{
		framebuffer_bound = false;

		framebuffer_provider->set_active();
		framebuffer_provider->stop();
		GL1::set_active(this);
	}
}

void GL1GraphicContextProvider::set_program_object(StandardProgram standard_program)
{
}

ProgramObject GL1GraphicContextProvider::get_program_object(StandardProgram standard_program) const
{
	return internal_program;
}

void GL1GraphicContextProvider::set_program_object(const ProgramObject &program)
{
	throw Exception("GLSL program objects are not supported on OpenGL 1.3");
}

void GL1GraphicContextProvider::reset_program_object()
{
}

void GL1GraphicContextProvider::set_light_model(const LightModel_GL1 &light_model)
{
	set_active();

	if (light_model.is_lighting_enabled())
		cl1Enable(GL_LIGHTING);
	else
		cl1Disable(GL_LIGHTING);

	cl1LightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, light_model.is_local_viewer() ? GL_TRUE : GL_FALSE);
	cl1LightModeli(GL_LIGHT_MODEL_TWO_SIDE, light_model.get_two_sided_materials() ? GL_TRUE : GL_FALSE);

	switch (light_model.get_color_control())
	{
	case cl_color_control_single_color:
		cl1LightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
		break;
	case cl_color_control_separate_specular_color:
		cl1LightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
		break;
	}

	if (light_model.get_flat_shading())
		cl1Disable(GL_SMOOTH);
	else
		cl1Enable(GL_SMOOTH);

	Colorf color = light_model.get_scene_ambient_light();
	GLfloat color_array[4] = { color.r, color.g, color.b, color.a };
	cl1LightModelfv(GL_LIGHT_MODEL_AMBIENT, color_array);
}

void GL1GraphicContextProvider::reset_light_model()
{
	set_active();
	cl1Disable(GL_LIGHTING);
}

void GL1GraphicContextProvider::set_material(const Material_GL1 &material)
{
	set_material(GL_FRONT_AND_BACK, material);
}

void GL1GraphicContextProvider::set_front_material(const Material_GL1 &material)
{
	set_material(GL_FRONT, material);
}

void GL1GraphicContextProvider::set_back_material(const Material_GL1 &material)
{
	set_material(GL_BACK, material);
}

void GL1GraphicContextProvider::reset_front_material()
{
	Material_GL1 material;
	set_front_material(material);
}

void GL1GraphicContextProvider::reset_back_material()
{
	Material_GL1 material;
	set_back_material(material);
}

void GL1GraphicContextProvider::set_material(GLenum face, const Material_GL1 &material)
{
	set_active();

	Colorf color;
	GLfloat color_array[4];

	color = material.get_ambient();
	color_array[0] = (GLfloat) color.r;
	color_array[1] = (GLfloat) color.g;
	color_array[2] = (GLfloat) color.b;
	color_array[3] = (GLfloat) color.a;
	cl1Materialfv(face, GL_AMBIENT, color_array);

	color = material.get_diffuse();
	color_array[0] = (GLfloat) color.r;
	color_array[1] = (GLfloat) color.g;
	color_array[2] = (GLfloat) color.b;
	color_array[3] = (GLfloat) color.a;
	cl1Materialfv(face, GL_DIFFUSE, color_array);

	color = material.get_specular();
	color_array[0] = (GLfloat) color.r;
	color_array[1] = (GLfloat) color.g;
	color_array[2] = (GLfloat) color.b;
	color_array[3] = (GLfloat) color.a;
	cl1Materialfv(face, GL_SPECULAR, color_array);

	color = material.get_emission();
	color_array[0] = (GLfloat) color.r;
	color_array[1] = (GLfloat) color.g;
	color_array[2] = (GLfloat) color.b;
	color_array[3] = (GLfloat) color.a;
	cl1Materialfv(face, GL_EMISSION, color_array);

	cl1Materialf(face, GL_SHININESS, (GLfloat) material.get_shininess());
}

void GL1GraphicContextProvider::reset_material()
{
	Material_GL1 material;
	set_material(material);
}

void GL1GraphicContextProvider::set_color_material(ColorMaterial_GL1 color)
{
	set_color_material(GL_FRONT_AND_BACK, color);
}

void GL1GraphicContextProvider::set_color_material_front(ColorMaterial_GL1 color)
{
	set_color_material(GL_FRONT_AND_BACK, color);
}

void GL1GraphicContextProvider::set_color_material_back(ColorMaterial_GL1 color)
{
	set_color_material(GL_FRONT_AND_BACK, color);
}

void GL1GraphicContextProvider::reset_color_material()
{
	set_active();
	cl1Disable(GL_COLOR_MATERIAL);
}

void GL1GraphicContextProvider::set_color_material(GLenum face, ColorMaterial_GL1 color)
{
	set_active();
	cl1Enable(GL_COLOR_MATERIAL);
	switch (color)
	{
	case cl_color_material_emission:
		cl1ColorMaterial(face, GL_EMISSION);
		break;
	case cl_color_material_ambient:
		cl1ColorMaterial(face, GL_AMBIENT);
		break;
	case cl_color_material_diffuse:
		cl1ColorMaterial(face, GL_DIFFUSE);
		break;
	case cl_color_material_specular:
		cl1ColorMaterial(face, GL_SPECULAR);
		break;
	case cl_color_material_ambient_and_diffuse:
		cl1ColorMaterial(face, GL_AMBIENT_AND_DIFFUSE);
		break;
	}
}

void GL1GraphicContextProvider::set_light(int light_index, const LightSource_GL1 &light)
{
	set_active();
	cl1Enable(GL_LIGHT0 + light_index);

	cl1Lightf(GL_LIGHT0 + light_index, GL_SPOT_EXPONENT, (GLfloat) light.get_spot_exponent());
	cl1Lightf(GL_LIGHT0 + light_index, GL_SPOT_CUTOFF, (GLfloat) light.get_spot_cutoff());
	cl1Lightf(GL_LIGHT0 + light_index, GL_CONSTANT_ATTENUATION, (GLfloat) light.get_constant_attenuation());
	cl1Lightf(GL_LIGHT0 + light_index, GL_LINEAR_ATTENUATION, (GLfloat) light.get_linear_attenuation());
	cl1Lightf(GL_LIGHT0 + light_index, GL_QUADRATIC_ATTENUATION, (GLfloat) light.get_quadratic_attenuation());

	Colorf c;
	GLfloat cf[4];

	c = light.get_ambient_intensity();
	cf[0] = (GLfloat) c.r; cf[1] = (GLfloat) c.g; cf[2] = (GLfloat) c.b; cf[3] = (GLfloat) c.a;
	cl1Lightfv(GL_LIGHT0 + light_index, GL_AMBIENT, cf);

	c = light.get_diffuse_intensity();
	cf[0] = (GLfloat) c.r; cf[1] = (GLfloat) c.g; cf[2] = (GLfloat) c.b; cf[3] = (GLfloat) c.a;
	cl1Lightfv(GL_LIGHT0 + light_index, GL_DIFFUSE, cf);

	c = light.get_specular_intensity();
	cf[0] = (GLfloat) c.r; cf[1] = (GLfloat) c.g; cf[2] = (GLfloat) c.b; cf[3] = (GLfloat) c.a;
	cl1Lightfv(GL_LIGHT0 + light_index, GL_SPECULAR, cf);

	Vec4f v = light.get_position();
	cf[0] = (GLfloat) v.x;
	cf[1] = (GLfloat) v.y;
	cf[2] = (GLfloat) v.z;
	cf[3] = (GLfloat) v.w;
	cl1Lightfv(GL_LIGHT0 + light_index, GL_POSITION, cf);

	Vec3f dir = light.get_spot_direction();
	cf[0] = (GLfloat) dir.x;
	cf[1] = (GLfloat) dir.y;
	cf[2] = (GLfloat) dir.z;
	cl1Lightfv(GL_LIGHT0 + light_index, GL_SPOT_DIRECTION, cf);
}

void GL1GraphicContextProvider::reset_light(int light_index)
{
	set_active();
	cl1Disable(GL_LIGHT0 + light_index);
}

bool GL1GraphicContextProvider::is_primitives_array_owner(const PrimitivesArray &primitives_array)
{
	return true;
}

void GL1GraphicContextProvider::draw_primitives(PrimitivesType type, int num_vertices, const PrimitivesArray &primitives_array)
{
	set_primitives_array(primitives_array);
	draw_primitives_array(type, 0, num_vertices);
	reset_primitives_array();
}

void GL1GraphicContextProvider::set_primitives_array(const PrimitivesArray &primitives_array)
{
	GL1PrimitivesArrayProvider * prim_array = static_cast<GL1PrimitivesArrayProvider *>(primitives_array.get_provider());
	if (prim_arrays_set)
		reset_primitives_array();
	set_active();
	prim_arrays_set = true;

	num_set_tex_arrays = 0;

	for(size_t attribute_index = 0; attribute_index < prim_array->attributes.size(); attribute_index++)
	{
		if (!prim_array->attribute_set[attribute_index])
			continue;

		const PrimitivesArrayProvider::VertexData &attribute = prim_array->attributes[attribute_index];

		GL1VertexArrayBufferProvider *vertex_array_ptr = static_cast<GL1VertexArrayBufferProvider *>(attribute.array_provider);
		if (!vertex_array_ptr)
			throw Exception("Invalid BindBuffer Provider");

		const char *data_ptr = ((const char *) vertex_array_ptr->get_data()) + attribute.offset;

		switch(attribute_index)
		{
			case 0: // POSITION
				cl1EnableClientState(GL_VERTEX_ARRAY);
				cl1VertexPointer(attribute.size, to_enum(attribute.type),  attribute.stride,  data_ptr);
				break;
			case 1: // COLOR
				cl1EnableClientState(GL_COLOR_ARRAY);
				cl1ColorPointer(attribute.size, to_enum(attribute.type),  attribute.stride,  data_ptr);

				break;
			case 2: // TEXTURE
				primitives_array_texture = attribute;
				primitives_array_texture_set = true;
				break;
			case 3: // TEXINDEX
				primitives_array_texindex = attribute;
				primitives_array_texindex_set = true;
				break;
			case 4: // NORMAL
				cl1EnableClientState(GL_NORMAL_ARRAY);
				cl1NormalPointer(to_enum(attribute.type),  attribute.stride,  data_ptr);
				break;
		}
	}
}

void GL1GraphicContextProvider::draw_primitives_array(PrimitivesType type, int offset, int num_vertices)
{
	set_active();

	// Simple condition - No textures set
	if (!primitives_array_texture_set)
	{
		cl1DrawArrays(to_enum(type), offset, num_vertices);
		return;
	}

	GLenum primitive_type = to_enum(type);

	int total_vertices = offset + num_vertices;

	// Normal condition - No texture index set
	if (!primitives_array_texindex_set)
	{
		set_primitive_texture( 0, primitives_array_texture, offset, num_vertices, total_vertices );
		cl1DrawArrays(primitive_type, offset, num_vertices);
		reset_primitive_texture( 0 );
		return;
	}

	// For code simplicity, disable all textures
	reset_primitive_texture_all();

	GL1VertexArrayBufferProvider *vertex_array_ptr = static_cast<GL1VertexArrayBufferProvider *>(primitives_array_texindex.array_provider);
	if (!vertex_array_ptr)
		throw Exception("Invalid BindBuffer Provider");

	const char *data_ptr = ((const char *) vertex_array_ptr->get_data()) + primitives_array_texindex.offset;

	// Difficult condition - Draw all texture indexes
	while(num_vertices > 0)
	{
		// First, find these values...
		int texture_index;
		int num_vertices_in_group;

		if (primitives_array_texindex.size <= 0)
		{
			// Invalid texindex array (ignore)
			texture_index = 0;
			num_vertices_in_group = num_vertices;
		}
		// Multiple textures possible
		switch(primitives_array_texindex.type)
		{
			case type_float:
			{
				int stride_float;
				if (primitives_array_texindex.stride)
				{
					stride_float = (primitives_array_texindex.stride / sizeof(float));
				}
				else
				{
					stride_float = primitives_array_texindex.size;
				}

				float *dptr = (float *) data_ptr;
				dptr += offset * stride_float;
				num_vertices_in_group = 1;
				texture_index = *dptr;
				dptr += stride_float;
				for (int texcnt = 1; texcnt < num_vertices; ++texcnt)
				{
					// Note, we assume all textures in "primitives_array_texindex.size" are identical
					if ( (*dptr) != texture_index )
						break;
					dptr += stride_float;
					num_vertices_in_group++;
				}

				break;
			}
			default:
				throw Exception("Implement me!");
		}

		set_primitive_texture( texture_index, primitives_array_texture, offset, num_vertices_in_group, total_vertices );
		cl1DrawArrays(primitive_type, offset, num_vertices_in_group);
		reset_primitive_texture( texture_index );

		offset += num_vertices_in_group;
		num_vertices -=	num_vertices_in_group;
	}
}

void GL1GraphicContextProvider::draw_primitives_array_instanced(PrimitivesType type, int offset, int num_vertices, int instance_count)
{
	throw Exception("Cannot draw instanced for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::set_primitives_elements(ElementArrayBufferProvider *array_provider)
{
	throw Exception("Cannot draw Element Array Buffers for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::draw_primitives_elements(PrimitivesType type, int count, VertexAttributeDataType indices_type, size_t offset)
{
	throw Exception("Cannot draw Element Array Buffers for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::draw_primitives_elements_instanced(PrimitivesType type, int count, VertexAttributeDataType indices_type, size_t offset, int instance_count)
{
	throw Exception("Cannot draw instanced for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::reset_primitives_elements()
{
	throw Exception("Cannot draw Element Array Buffers for the OpenGL 1.3 target");
}

/*
void GL1GraphicContextProvider::draw_primitives_elements(PrimitivesType type, int count, unsigned int *indices)
{
	set_active();
	cl1DrawElements(to_enum(type), count, GL_UNSIGNED_INT, indices);
}

void GL1GraphicContextProvider::draw_primitives_elements(PrimitivesType type, int count, unsigned short *indices)
{
	set_active();
	cl1DrawElements(to_enum(type), count, GL_UNSIGNED_SHORT, indices);
}

void GL1GraphicContextProvider::draw_primitives_elements(PrimitivesType type, int count, unsigned char *indices)
{
	set_active();
	cl1DrawElements(to_enum(type), count, GL_UNSIGNED_BYTE, indices);
}
*/

void GL1GraphicContextProvider::draw_primitives_elements(
	PrimitivesType type,
	int count,
	ElementArrayBufferProvider *array_provider,
	VertexAttributeDataType indices_type,
	void *offset)
{
	throw Exception("Cannot draw Element Array Buffers for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::draw_primitives_elements_instanced(
	PrimitivesType type,
	int count,
	ElementArrayBufferProvider *array_provider,
	VertexAttributeDataType indices_type,
	void *offset,
	int instance_count)
{
	throw Exception("Cannot draw Element Array Buffers for the OpenGL 1.3 target");
}

void GL1GraphicContextProvider::reset_primitives_array()
{
	set_active();

	primitives_array_texture_set = false;
	primitives_array_texindex_set = false;

	cl1DisableClientState(GL_VERTEX_ARRAY);
	cl1DisableClientState(GL_COLOR_ARRAY);
	cl1DisableClientState(GL_NORMAL_ARRAY);
	cl1DisableClientState(GL_EDGE_FLAG_ARRAY);
	cl1DisableClientState(GL_TEXTURE_COORD_ARRAY);

	if (cl1ClientActiveTexture)
	{
		for (int i = 0; i < num_set_tex_arrays; ++i)
		{
			cl1ClientActiveTexture(GL_TEXTURE0+i);
			cl1DisableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		num_set_tex_arrays = 0;
	}
	else
	{
		cl1DisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	prim_arrays_set = false;
}

void GL1GraphicContextProvider::set_scissor(const Rect &rect)
{
	set_active();

	if (!scissor_enabled)
		throw Exception("RasterizerState must be set with enable_scissor() for clipping to work");

	cl1Enable(GL_SCISSOR_TEST);
	cl1Scissor(
		rect.left,
		rect.top,
		rect.get_width(),
		rect.get_height());
}

void GL1GraphicContextProvider::reset_scissor()
{
	set_active();
	cl1Disable(GL_SCISSOR_TEST);
}

void GL1GraphicContextProvider::dispatch(int x, int y, int z)
{
	throw Exception("Dispatch not supported by OpenGL 1!");
}

void GL1GraphicContextProvider::clear(const Colorf &color)
{
	set_active();
	cl1ClearColor(
		(GLclampf) color.r,
		(GLclampf) color.g,
		(GLclampf) color.b,
		(GLclampf) color.a);
	cl1Clear(GL_COLOR_BUFFER_BIT);
}

void GL1GraphicContextProvider::clear_stencil(int value)
{
	set_active();
	cl1ClearStencil(value);
	cl1Clear(GL_STENCIL_BUFFER_BIT);
}

void GL1GraphicContextProvider::clear_depth(float value)
{
	set_active();
	cl1ClearDepth(value);
	cl1Clear(GL_DEPTH_BUFFER_BIT);
}

void GL1GraphicContextProvider::on_window_resized()
{
	window_resized_signal.invoke(render_window->get_viewport().get_size());
}

void GL1GraphicContextProvider::set_viewport(const Rectf &viewport)
{
	set_active();
	cl1Viewport(
		GLsizei(viewport.left),
		GLsizei(viewport.top),
		GLsizei(viewport.right - viewport.left),
		GLsizei(viewport.bottom - viewport.top));
}

void GL1GraphicContextProvider::set_viewport(int index, const Rectf &viewport)
{
	if (index==0)
		set_viewport(viewport);
}

void GL1GraphicContextProvider::set_depth_range(float n, float f)
{
	set_active();
	cl1DepthRange((float)n, (float)f);
}

void GL1GraphicContextProvider::set_depth_range(int viewport, float n, float f)
{
	if (viewport == 0)
	{
		set_active();
		cl1DepthRange((float)n, (float)f);
	}
}

void GL1GraphicContextProvider::enable_blending(bool value)
{
	set_active();
	if( value )
		cl1Enable(GL_BLEND);
	else
		cl1Disable(GL_BLEND);

}

void GL1GraphicContextProvider::set_blend_color(const Colorf &color)
{
	set_active();
	if (cl1BlendColor)
	{
		cl1BlendColor(
			GLclampf(color.get_red()),
			GLclampf(color.get_green()),
			GLclampf(color.get_blue()),
			GLclampf(color.get_alpha()));
	}
}

void GL1GraphicContextProvider::set_blend_equation(BlendEquation equation_color, BlendEquation equation_alpha)
{
	set_active();
	if (cl1BlendEquation)
		cl1BlendEquation(to_enum(equation_color));
}

void GL1GraphicContextProvider::set_blend_function(BlendFunc src, BlendFunc dest, BlendFunc src_alpha, BlendFunc dest_alpha)
{
	set_active();
	if( src == src_alpha && dest == dest_alpha )
	{
		if (cl1BlendFunc)
			cl1BlendFunc(to_enum(src), to_enum(dest));
	}
	else
	{
		if (cl1BlendFuncSeparate)
		{
			cl1BlendFuncSeparate( 
				to_enum(src),
				to_enum(dest),
				to_enum(src_alpha),
				to_enum(dest_alpha) );
		}
		else
		{
			if (cl1BlendFunc)
				cl1BlendFunc(to_enum(src), to_enum(dest));
		}
	}
}


void GL1GraphicContextProvider::set_point_size(float value)
{
	set_active();
	cl1PointSize((GLfloat)value);
}

void GL1GraphicContextProvider::set_point_fade_treshold_size(float value)
{
// (OpenGL 1.4)
//	set_active();
//	if (cl1PointParameterf)
//	{
//		cl1PointParameterf(GL_POINT_FADE_THRESHOLD_SIZE, (GLfloat)pen.get_point_fade_treshold_size());
//	}
}

void GL1GraphicContextProvider::set_line_width(float value)
{
	set_active();
	cl1LineWidth(value);

}

void GL1GraphicContextProvider::enable_line_antialiasing(bool enabled)
{
	set_active();
	if (enabled)
		cl1Enable(GL_LINE_SMOOTH);
	else
		cl1Disable(GL_LINE_SMOOTH);
}

void GL1GraphicContextProvider::enable_vertex_program_point_size(bool enabled)
{
	set_active();

	if (enabled)
		cl1Enable(GL_VERTEX_PROGRAM_POINT_SIZE);
	else
		cl1Disable(GL_VERTEX_PROGRAM_POINT_SIZE);
}

void GL1GraphicContextProvider::set_point_sprite_origin(PointSpriteOrigin origin)
{
//	set_active();
// (OpenGL 1.4)
//	if(cl1PointParameterf)
//	{
//		switch (origin)
//		{
//		case origin_upper_left:
//			cl1PointParameterf(GL_POINT_SPRITE_COORD_ORIGIN, GL_UPPER_LEFT);
//			break;
//		case origin_lower_left:
//			cl1PointParameterf(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT);
//			break;
//		}
//	}
}

void GL1GraphicContextProvider::set_antialiased(bool value)
{
	set_active();

	if (value)
		cl1Enable(GL_POLYGON_SMOOTH);
	else
		cl1Disable(GL_POLYGON_SMOOTH);
}

void GL1GraphicContextProvider::set_culled(bool value)
{
	set_active();
	if (value)
		cl1Enable(GL_CULL_FACE);
	else
		cl1Disable(GL_CULL_FACE);

}

void GL1GraphicContextProvider::set_point_offset(bool value)
{
	set_active();

	if (value)
		cl1Enable(GL_POLYGON_OFFSET_POINT);
	else
		cl1Disable(GL_POLYGON_OFFSET_POINT);

}

void GL1GraphicContextProvider::set_line_offset(bool value)
{
	set_active();
	if (value)
		cl1Enable(GL_POLYGON_OFFSET_LINE);
	else
		cl1Disable(GL_POLYGON_OFFSET_LINE);


}

void GL1GraphicContextProvider::set_polygon_offset(bool value)
{
	set_active();
	if (value)
		cl1Enable(GL_POLYGON_OFFSET_FILL);
	else
		cl1Disable(GL_POLYGON_OFFSET_FILL);
}

void GL1GraphicContextProvider::set_face_cull_mode(CullMode value)
{
	set_active();
	switch (value)
	{
	case cull_front:
		cl1CullFace(GL_FRONT);
		break;
	case cull_back:
		cl1CullFace(GL_BACK);
		break;
	case cull_front_and_back:
		cl1CullFace(GL_FRONT_AND_BACK);
		break;
	}
}

void GL1GraphicContextProvider::set_face_fill_mode(FillMode value)
{
	set_active();
	cl1PolygonMode(GL_FRONT, to_enum(value));
	cl1PolygonMode(GL_BACK, to_enum(value));
}

void GL1GraphicContextProvider::set_front_face(FaceSide value)
{
	set_active();
	switch (value)
	{
	case face_counter_clockwise:
		cl1FrontFace(GL_CCW);
		break;
	case face_clockwise:
		cl1FrontFace(GL_CW);
		break;
	}
}

void GL1GraphicContextProvider::set_offset_factor(float value)
{
	set_active();
	cl1PolygonOffset(value, value);
}

void GL1GraphicContextProvider::set_offset_units(float value)
{
	// TODO: What to do here?
}

void GL1GraphicContextProvider::enable_logic_op(bool enabled)
{
	set_active();
	if (enabled)
	{
		cl1Enable(GL_COLOR_LOGIC_OP);
	}
	else
	{
		cl1Disable(GL_COLOR_LOGIC_OP);
	}
}

void GL1GraphicContextProvider::set_logic_op(LogicOp op)
{
	set_active();
	cl1LogicOp(to_enum(op));
}

void GL1GraphicContextProvider::set_draw_buffer(DrawBuffer buffer)
{
	set_active();
	cl1DrawBuffer( to_enum(buffer) );

}

void GL1GraphicContextProvider::enable_stencil_test(bool enabled){}
void GL1GraphicContextProvider::set_stencil_compare_front(CompareFunction front, int front_ref, int front_mask){}
void GL1GraphicContextProvider::set_stencil_compare_back(CompareFunction back, int back_ref, int back_mask){}
void GL1GraphicContextProvider::set_stencil_write_mask(unsigned char front_facing_mask, unsigned char back_facing_mask){}
void GL1GraphicContextProvider::set_stencil_op_front(StencilOp fail_front, StencilOp pass_depth_fail_front, StencilOp pass_depth_pass_front){}
void GL1GraphicContextProvider::set_stencil_op_back(StencilOp fail_back, StencilOp pass_depth_fail_back, StencilOp pass_depth_pass_back){}

void GL1GraphicContextProvider::enable_depth_test(bool enabled)
{
	set_active();
	if(enabled )
		cl1Enable(GL_DEPTH_TEST);
	else
		cl1Disable(GL_DEPTH_TEST);

}

void GL1GraphicContextProvider::enable_depth_write(bool enabled)
{
	set_active();

	cl1DepthMask(enabled ? 1 : 0);

}

void GL1GraphicContextProvider::set_depth_compare_function(CompareFunction func)
{
	set_active();
	cl1DepthFunc(to_enum(func));

}

void GL1GraphicContextProvider::enable_color_write(bool red, bool green, bool blue, bool alpha)
{
	//set_active();
}

/////////////////////////////////////////////////////////////////////////////
// GL1GraphicContextProvider Implementation:

GLenum GL1GraphicContextProvider::to_enum(DrawBuffer buffer)
{
	switch(buffer)
	{
	case buffer_back: return GL_BACK;
	case buffer_back_left: return GL_BACK_LEFT;
	case buffer_back_right: return GL_BACK_RIGHT;
	case buffer_front: return GL_FRONT;
	case buffer_front_and_back: return GL_FRONT_AND_BACK;
	case buffer_front_left: return GL_FRONT_LEFT;
	case buffer_front_right: return GL_FRONT_RIGHT;
	case buffer_left: return GL_LEFT;
	case buffer_none: return GL_NONE;
	case buffer_right: return GL_RIGHT;
	default: return GL_BACK;
	}
}

GLenum GL1GraphicContextProvider::to_enum(CompareFunction func)
{
	switch( func )
	{
	case compare_never: return GL_NEVER;
	case compare_less: return GL_LESS;
	case compare_lequal: return GL_LEQUAL; 
	case compare_greater: return GL_GREATER; 
	case compare_gequal: return GL_GEQUAL; 
	case compare_equal: return GL_EQUAL; 
	case compare_notequal: return GL_NOTEQUAL; 
	case compare_always: return GL_ALWAYS; 
	default: return GL_LEQUAL;
	}
}

GLenum GL1GraphicContextProvider::to_enum(StencilOp op)
{
	switch( op )
	{
	case stencil_decr: return GL_DECR;
	case stencil_decr_wrap: return GL_DECR_WRAP;
	case stencil_incr: return GL_INCR;
	case stencil_incr_wrap: return GL_INCR_WRAP;
	case stencil_invert: return GL_INVERT;
	case stencil_keep: return GL_KEEP;
	case stencil_replace: return GL_REPLACE;
	case stencil_zero: return GL_ZERO;	
	default: return GL_KEEP;
	}
}

GLenum GL1GraphicContextProvider::to_enum(CullMode mode)
{
	switch( mode )
	{
	case cull_front: return GL_FRONT;
	case cull_back: return GL_BACK;
	case cull_front_and_back: return GL_FRONT_AND_BACK;
	default: return GL_BACK;
	}
};

GLenum GL1GraphicContextProvider::to_enum(FillMode mode)
{
	switch( mode )
	{
	case fill_point: return GL_POINT;
	case fill_line: return GL_LINE;
	case fill_polygon: return GL_FILL;
	default: return GL_FILL;
	}
};

GLenum GL1GraphicContextProvider::to_enum(BlendFunc func)
{
	switch( func )
	{
	case blend_zero: return GL_ZERO;
	case blend_one: return GL_ONE;
	case blend_dest_color: return GL_DST_COLOR;
	case blend_src_color: return GL_SRC_COLOR;
	case blend_one_minus_dest_color: return GL_ONE_MINUS_DST_COLOR;
	case blend_one_minus_src_color: return GL_ONE_MINUS_SRC_COLOR;
	case blend_src_alpha: return GL_SRC_ALPHA;
	case blend_one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
	case blend_dest_alpha: return GL_DST_ALPHA;
	case blend_one_minus_dest_alpha: return GL_ONE_MINUS_DST_ALPHA;
	case blend_src_alpha_saturate: return GL_SRC_ALPHA_SATURATE;
	case blend_constant_color: return GL_CONSTANT_COLOR;
	case blend_one_minus_constant_color: return GL_ONE_MINUS_CONSTANT_COLOR;
	case blend_constant_alpha: return GL_CONSTANT_ALPHA;
	case blend_one_minus_constant_alpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
	default: return GL_BLEND_SRC;
	}
}


GLenum GL1GraphicContextProvider::to_enum(BlendEquation eq)
{
	switch( eq )
	{
	case equation_add: return GL_FUNC_ADD;
	case equation_subtract: return GL_FUNC_SUBTRACT;
	case equation_reverse_subtract: return GL_FUNC_REVERSE_SUBTRACT;
	case equation_min: return GL_MIN;
	case equation_max: return GL_MAX;
	default: return GL_FUNC_ADD;
	}
};

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitSource value)
{
	switch( value )
	{
	case cl_source_texture: return GL_TEXTURE;
	case cl_source_texture0: return GL_TEXTURE0;
	case cl_source_texture1: return GL_TEXTURE1;
	case cl_source_texture2: return GL_TEXTURE2;
	case cl_source_texture3: return GL_TEXTURE3;
	case cl_source_texture4: return GL_TEXTURE4;
	case cl_source_texture5: return GL_TEXTURE5;
	case cl_source_texture6: return GL_TEXTURE6;
	case cl_source_texture7: return GL_TEXTURE7;
	case cl_source_texture8: return GL_TEXTURE8;
	case cl_source_texture9: return GL_TEXTURE9;
	case cl_source_texture10: return GL_TEXTURE10;
	case cl_source_texture11: return GL_TEXTURE11;
	case cl_source_texture12: return GL_TEXTURE12;
	case cl_source_texture13: return GL_TEXTURE13;
	case cl_source_texture14: return GL_TEXTURE14;
	case cl_source_texture15: return GL_TEXTURE15;
	case cl_source_constant: return GL_CONSTANT;
	case cl_source_primary_color: return GL_PRIMARY_COLOR;
	case cl_source_previous: return GL_PREVIOUS;
	default: return GL_TEXTURE;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitRGBOperand value)
{
	switch( value )
	{
	case cl_rgb_operand_src_color: return GL_SRC_COLOR;
	case cl_rgb_operand_one_minus_src_color: return GL_ONE_MINUS_SRC_COLOR;
	case cl_rgb_operand_src_alpha: return GL_SRC_ALPHA;
	case cl_rgb_operand_one_minus_src_alpha: return GL_ONE_MINUS_SRC_ALPHA;
	default: return GL_SRC_COLOR;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitAlphaOperand value)
{
	if( value == cl_alpha_operand_src_alpha )
		return GL_SRC_ALPHA_SATURATE;

	return GL_ONE_MINUS_SRC_ALPHA;
}

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitAlphaMode value)
{
	switch(value)
	{
	case cl_combinemode_alpha_replace: return GL_REPLACE;
	case cl_combinemode_alpha_modulate: return GL_MODULATE;
	case cl_combinemode_alpha_add: return GL_ADD;
	case cl_combinemode_alpha_add_signed: return GL_ADD_SIGNED;
	case cl_combinemode_alpha_interpolate: return GL_INTERPOLATE;
	case cl_combinemode_alpha_subtract: return GL_SUBTRACT;
	default: return GL_REPLACE;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitRGBMode value)
{
	switch(value)
	{
	case cl_combinemode_rgb_replace: return GL_REPLACE;
	case cl_combinemode_rgb_modulate: return GL_MODULATE;
	case cl_combinemode_rgb_add: return GL_ADD;
	case cl_combinemode_rgb_add_signed: return GL_ADD_SIGNED;
	case cl_combinemode_rgb_interpolate: return GL_INTERPOLATE;
	case cl_combinemode_rgb_subtract: return GL_SUBTRACT;
	case cl_combinemode_rgb_dot3_rgb: return GL_DOT3_RGB;
	case cl_combinemode_rgb_dot3_rgba: return GL_DOT3_RGBA;
	default: return GL_REPLACE;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum TextureUnitMode value)
{
	switch(value)
	{
	case cl_unitmode_replace: return GL_REPLACE;
	case cl_unitmode_modulate: return GL_MODULATE;
	case cl_unitmode_decal: return GL_DECAL;
	case cl_unitmode_blend: return GL_BLEND;
	case cl_unitmode_add: return GL_ADD;
	case cl_unitmode_combine: return GL_COMBINE;
	default: return GL_REPLACE;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum VertexAttributeDataType value)
{
	switch(value)
	{
	case type_unsigned_byte:
		return GL_UNSIGNED_BYTE;
	case type_unsigned_short:
		return GL_UNSIGNED_SHORT;
	case type_unsigned_int:
		return GL_UNSIGNED_INT;
	case type_byte:
		return GL_BYTE;
	case type_short:
		return GL_SHORT;
	case type_int:
		return GL_INT;
	case type_float:
		return GL_FLOAT;
	default:
		return 0;
	}
}

GLenum GL1GraphicContextProvider::to_enum(enum PrimitivesType value)
{
	GLenum gl_mode = 0;
	switch (value)
	{
	case type_points: gl_mode = GL_POINTS; break;
	case type_line_strip: gl_mode = GL_LINE_STRIP; break;
	case type_line_loop: gl_mode = GL_LINE_LOOP; break;
	case type_lines: gl_mode = GL_LINES; break;
	case type_triangle_strip: gl_mode = GL_TRIANGLE_STRIP; break;
	case type_triangle_fan: gl_mode = GL_TRIANGLE_FAN; break;
	case type_triangles: gl_mode = GL_TRIANGLES; break;
	}
	return gl_mode;
}

GLenum GL1GraphicContextProvider::to_enum(enum LogicOp op)
{
	GLenum gl_op = 0;
	switch (op)
	{
		case logic_clear: gl_op = GL_CLEAR; break;  
		case logic_and: gl_op = GL_AND; break;
		case logic_and_reverse: gl_op = GL_AND_REVERSE; break;
		case logic_copy: gl_op = GL_COPY; break;
		case logic_and_inverted: gl_op = GL_AND_INVERTED; break;
		case logic_noop: gl_op = GL_NOOP; break;
		case logic_xor: gl_op = GL_XOR; break;
		case logic_or: gl_op = GL_OR; break;
		case logic_nor: gl_op = GL_NOR; break;
		case logic_equiv: gl_op = GL_EQUIV; break;
		case logic_invert: gl_op = GL_INVERT; break;
		case logic_or_reverse: gl_op = GL_OR_REVERSE; break;
		case logic_copy_inverted: gl_op = GL_COPY_INVERTED; break;
		case logic_or_inverted: gl_op = GL_OR_INVERTED; break;
		case logic_nand: gl_op = GL_NAND; break;
		case logic_set: gl_op = GL_SET; break;
		default: break;
	}
	return gl_op;
}

void GL1GraphicContextProvider::set_primitive_texture( int texture_index, PrimitivesArrayProvider::VertexData &array_texture, int offset, int num_vertices, int total_vertices)
{
	GL1TextureProvider *texture;
	if ( (texture_index <0) || (texture_index >= max_texture_coords) )
	{
		texture = NULL;		// Ignore invalid texture index's
	}
	else
	{
		texture = selected_textures[texture_index].texture;
	}

	if (texture)
	{
		if (cl1ActiveTexture != 0)
			cl1ActiveTexture( GL_TEXTURE0 + texture_index );

		cl1Enable(texture->get_texture_type());
		cl1BindTexture(texture->get_texture_type(), texture->get_handle());

		if (cl1ClientActiveTexture)
			cl1ClientActiveTexture(GL_TEXTURE0 + texture_index );

		cl1EnableClientState(GL_TEXTURE_COORD_ARRAY);

		if (texture->is_power_of_two_texture() || (num_vertices==0))
		{
			GL1VertexArrayBufferProvider *vertex_array_ptr = static_cast<GL1VertexArrayBufferProvider *>(array_texture.array_provider);
			if (!vertex_array_ptr)
				throw Exception("Invalid BindBuffer Provider");

			const char *data_ptr = ((const char *) vertex_array_ptr->get_data()) + array_texture.offset;

			cl1TexCoordPointer(array_texture.size, to_enum(array_texture.type),  array_texture.stride,  data_ptr);
		}
		else
		{
			// A hack to handle non-power-of-two textures
			texture->transform_coordinate(array_texture, transformed_coords, offset, num_vertices, total_vertices);
			cl1TexCoordPointer(array_texture.size, GL_FLOAT, 0,  &transformed_coords[0]);
		}
	}
	else
	{
		//Disabled this exception because the sprite render batcher does this intentionally
		//throw Exception("Attempt to draw a texture that was not selected in the GL1 target");
	}

}

void GL1GraphicContextProvider::reset_primitive_texture( int texture_index)
{
	if ( (texture_index <0) || (texture_index >= max_texture_coords) )
	{
		return;
	}

	GL1TextureProvider *texture = selected_textures[texture_index].texture;
	if (texture)
	{
		if (cl1ActiveTexture != 0)
			cl1ActiveTexture( GL_TEXTURE0 + texture_index );

		cl1BindTexture(GL_TEXTURE_2D, 0);
		cl1Disable(GL_TEXTURE_2D);
	}

}

void GL1GraphicContextProvider::reset_primitive_texture_all()
{
	for (int cnt=0; cnt < max_texture_coords; cnt++)
	{
		GL1TextureProvider *texture = selected_textures[cnt].texture;
		if (texture)
		{
			if (cl1ActiveTexture != 0)
				cl1ActiveTexture( GL_TEXTURE0 + cnt );
	
			cl1BindTexture(GL_TEXTURE_2D, 0);
			cl1Disable(GL_TEXTURE_2D);
		}
	}
}

}
