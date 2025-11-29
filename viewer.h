////////////////////////////////////////////////////////////////
//
//  viewer
//
//  author(s):
//  ENDESGA - https://x.com/ENDESGA | https://bsky.app/profile/endesga.bsky.social
//
//  https://github.com/H-language/viewer
//  2025 - CC0 - FOSS forever
//

////////////////////////////////
/// include(s)

#define PEP_IMPLEMENTATION
#include <pep.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_PSD
#ifdef __TINYC__
	#define STBI_NO_SIMD
#endif
#include <stb_image.h>

#include <C7H16.h>

////////////////////////////////
/// version

#define VIEWER_VERSION_MAJOR 0
#define VIEWER_VERSION_MINOR 1
#define VIEWER_VERSION_PATCH 0
#define VIEWER_VERSION AS_BYTES( VIEWER_VERSION_MAJOR ) "." AS_BYTES( VIEWER_VERSION_MINOR ) "." AS_BYTES( VIEWER_VERSION_PATCH )

////////////////////////////////////////////////////////////////

global canvas image;
global window_canvas main_window_canvas = nothing;

object_fn( window, main_resize )
{
	with( main_window_canvas->scaling )
	{
		when( scaling_rational_fit, scaling_integer_fit_floor, scaling_integer_fit_round, scaling_integer_fit_ceil, scaling_rational_fill, scaling_integer_fill_floor, scaling_integer_fill_round, scaling_integer_fill_ceil )
		{
			window_canvas_center( main_window_canvas );
			window_canvas_clamp( main_window_canvas );
			skip;
		}

		other skip;
	}
}

object_fn( window, main_tick )
{
	perm i4 window_drag_start_x;
	perm i4 window_drag_start_y;
	perm i4 window_start_x;
	perm i4 window_start_y;
	perm i4 canvas_drag_start_x;
	perm i4 canvas_drag_start_y;
	perm i4x2 canvas_offset_start;

	temp flag refresh = no;
	temp i4 const display_w = display_get_width();
	temp i4 const display_h = display_get_height();

	if( mouse_pressed( left ) )
	{
		display_get_mouse_position( ref_of( window_drag_start_x ), ref_of( window_drag_start_y ) );
		window_get_position( this, ref_of( window_start_x ), ref_of( window_start_y ) );
	}
	else if( mouse_held( left ) )
	{
		i4 screen_mx,
		screen_my;
		display_get_mouse_position( ref_of( screen_mx ), ref_of( screen_my ) );
		window_set_position( this, window_start_x + screen_mx - window_drag_start_x, window_start_y + screen_my - window_drag_start_y );
	}

	if( main_window_canvas->scaling isnt scaling_rational_stretch )
	{
		if( mouse_pressed( middle ) )
		{
			canvas_drag_start_x = this->mouse_x;
			canvas_drag_start_y = this->mouse_y;
			canvas_offset_start = main_window_canvas->pos;
			main_window_canvas->scaling = scaling_manual;
		}
		else if( mouse_held( middle ) )
		{
			main_window_canvas->pos.x = canvas_offset_start.x + this->mouse_x - canvas_drag_start_x;
			main_window_canvas->pos.y = canvas_offset_start.y + this->mouse_y - canvas_drag_start_y;
			refresh = yes;
		}
	}

	if( this->scroll.y isnt 0 )
	{
		window_canvas_zoom( main_window_canvas, this->mouse_x, this->mouse_y, r4_pow( 1.2, this->scroll.y ) );
		refresh = yes;
	}

	if( key_pressed( 1 ) )
	{
		main_window_canvas->scaling = scaling_manual;
		refresh = yes;
	}
	else if( key_pressed( 2 ) )
	{
		main_window_canvas->scaling = scaling_rational_fit;
		_window_resize( this );
		refresh = yes;
	}
	else if( key_pressed( 3 ) )
	{
		main_window_canvas->scaling = scaling_rational_fill;
		_window_resize( this );
		refresh = yes;
	}
	else if( key_pressed( 4 ) )
	{
		main_window_canvas->scaling = scaling_rational_stretch;
		_window_resize( this );
		refresh = yes;
	}

	if( key_pressed( escape ) )
	{
		this->close = yes;
		out;
	}

	if( key_pressed( tab ) )
	{
		window_toggle_border( this );
		refresh = yes;
	}

	if( key_pressed( enter ) )
	{
		window_set_position( this, 0, 0 );
		window_set_size( this, display_w, display_h );
		_window_resize( this );
		refresh = yes;
	}

	if( key_pressed( backspace ) )
	{
		if( main_window_canvas->scaling is scaling_rational_stretch and (this->size.w is display_w and this->size.h is display_h) )
		{
			temp i4 const new_w = i4_min( main_window_canvas->canvas->size.w, display_w );
			temp i4 const new_h = i4_min( main_window_canvas->canvas->size.h, display_h );
			window_set_position( this, ( display_w - new_w ) / 2, ( display_h - new_h ) / 2 );
			window_set_size( this, new_w, new_h );
		}
		else
		{
			i4 win_x;
			i4 win_y;
			window_get_position( this, ref_of( win_x ), ref_of( win_y ) );

			n4 scaled_w;
			n4 scaled_h;
			window_canvas_get_scaled_size( main_window_canvas, ref_of( scaled_w ), ref_of( scaled_h ) );

			temp i4 const new_w = i4_min( scaled_w, display_w );
			temp i4 const new_h = i4_min( scaled_h, display_h );
			temp i4 const new_x = i4_clamp( win_x + ( this->size.w - new_w ) / 2, 0, display_w - new_w );
			temp i4 const new_y = i4_clamp( win_y + ( this->size.h - new_h ) / 2, 0, display_h - new_h );

			window_set_position( this, new_x, new_y );
			window_set_size( this, new_w, new_h );

			main_window_canvas->pos = i4x2();
		}

		_window_resize( this );
		refresh = yes;
	}

	if( refresh )
	{
		window_canvas_clamp( main_window_canvas );
		window_refresh( this );
	}
}

start
{
	i4 width = 1;
	i4 height = 1;

	if( start_parameters_count <= 1 )
	{
		exit( failure );
	}
	else
	{
		byte ref const input = to( byte ref const, start_parameters[ 1 ] );

		byte ref path_ext = path_get_extension( input );

		if( bytes_compare( path_ext, "pep", 4 ) is 0 )
		{
			pep loaded_pep = pep_load( input );
			image = new_object( canvas );
			image->pixels = to( pixel ref, pep_decompress( ref_of( loaded_pep ), pep_bgra, 0 ) );
			pep_free( ref_of( loaded_pep ) );
			image->size.w = loaded_pep.width;
			image->size.h = loaded_pep.height;
			width = image->size.w;
			height = image->size.h;
		}
		else
		{
			i4 channels;
			temp byte const ref const loaded_bytes = to( byte const ref const, stbi_load( input, ref_of( width ), ref_of( height ), ref_of( channels ), 4 ) );
			image = new_canvas( width, height );
			iter( index, width * height )
			{
				temp const int byte_index = index << 2;
				canvas_set_pixel_index( image, index, pixel( loaded_bytes[ byte_index ], loaded_bytes[ byte_index + 1 ], loaded_bytes[ byte_index + 2 ], loaded_bytes[ byte_index + 3 ] ) );
			}
		}
	}

	//

	windows_fps_tick = 0;
	windows_fps_draw = 0;

	temp i4 const display_w = display_get_width();
	temp i4 const display_h = display_get_height();

	temp r4 scale = 1.0;
	if( width > display_w or height > display_h )
	{
		temp r4 const scale_w = r4( display_w ) / r4( width );
		temp r4 const scale_h = r4( display_h ) / r4( height );
		scale = r4_min( scale_w, scale_h );
	}

	temp i4 const win_w = i4( r4_round( r4( width ) * scale ) );
	temp i4 const win_h = i4( r4_round( r4( height ) * scale ) );

	new_window( pick( start_parameters[ 1 ] isnt nothing, path_get_name( start_parameters[ 1 ] ), "viewer" ), win_w, win_h );
	window_set_fn_resize( current_window, window_main_resize );
	window_set_fn_tick( current_window, window_main_tick );
	current_window->clear_before_present = yes;

	main_window_canvas = new_window_canvas( image, sizing_fixed, scaling_rational_fit, nothing );
	main_window_canvas->scale.w = scale;
	main_window_canvas->scale.h = scale;
	window_add_window_canvas( current_window, main_window_canvas );
}
