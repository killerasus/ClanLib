/*
**  ClanLib SDK
**  Copyright (c) 1997-2015 The ClanLib Team
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

#include "view_tree.h"
#include "../../Display/Window/display_window.h"
#include "../../Display/Window/keys.h"

namespace clan
{
	class InputEvent;
	class TextureWindow_Impl;

	class TextureWindow : public ViewTree
	{
	public:
		TextureWindow(Canvas &canvas);

		/// \brief Set position and size within the canvas
		///
		/// \param rect = position and size.
		void set_viewport(const Rectf &rect);

		/// \brief Returns the position and size within the canvas
		Rectf get_viewport() const;

		void set_background_color(const Colorf &background_color = Colorf::transparent);
		void set_clear_background(bool enable = true);

		/// \brief Always redraw the entire gui
		void set_always_render(bool enable = true);

		void update();

		/// \brief Set the window to use to automatically set the cursor (including mouse capture)
		///
		/// Default = Don't change the cursor
		void set_cursor_window(const DisplayWindow &cursor_window);

		/// \brief Set the window to use to automatically handle window events
		///
		/// Default = You need to call the user defined events instead (on_...)
		/// \param event_window = The window to use
		/// \param transform_mouse_matrix = Matrix to use to transform the mouse coordinates
		void set_event_window(const DisplayWindow &event_window, const Mat4f &transform_mouse_matrix = Mat4f::identity());

		/// User defined events. Call these if set_event_window() is not used
		void on_window_close();
		void on_lost_focus();
		void on_got_focus();
		void on_key_down(const clan::InputEvent &);
		void on_key_up(const clan::InputEvent &);
		void on_mouse_down(const clan::InputEvent &);
		void on_mouse_dblclk(const clan::InputEvent &);
		void on_mouse_up(const clan::InputEvent &);
		void on_mouse_move(const clan::InputEvent &);

		DisplayWindow get_display_window() override { return DisplayWindow(); }
		Canvas get_canvas() const override;

	protected:
		void set_needs_render() override;

		Pointf client_to_screen_pos(const Pointf &pos) override;
		Pointf screen_to_client_pos(const Pointf &pos) override;

		void present_popup(const Pointf &pos, const std::shared_ptr<ViewController> &popup) override;
		void dismiss_popup() override;

		void present_modal(const std::string &title, const std::shared_ptr<ViewController> &modal) override;
		void dismiss_modal() override;

	private:
		std::shared_ptr<TextureWindow_Impl> impl;

		friend class TextureWindow_Impl;
	};
}