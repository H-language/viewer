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

object_fn( window, main_tick )
{
	temp flag refresh = no;
	perm i4 drag_start_x;
	perm i4 drag_start_y;
	perm i4 drag_offset_start_x;
	perm i4 drag_offset_start_y;
	perm i4 window_drag_start_x;
	perm i4 window_drag_start_y;
	perm i4 window_start_x;
	perm i4 window_start_y;

	temp i4 mx = this->mouse_x;
	temp i4 my = this->mouse_y;

	temp i4 const display_w = display_get_width();
	temp i4 const display_h = display_get_height();

	temp scaling_mode const scaling = main_window_canvas->scaling;
	temp const i4 scaled_w = i4( r4_round( r4( main_window_canvas->canvas->size.w ) * main_window_canvas->scale.w ) );
	temp const i4 scaled_h = i4( r4_round( r4( main_window_canvas->canvas->size.h ) * main_window_canvas->scale.h ) );

	if( scaling isnt scaling_rational_stretch )
	{
		if( mouse_pressed( middle ) )
		{
			drag_start_x = mx;
			drag_start_y = my;
			drag_offset_start_x = main_window_canvas->pos.x;
			drag_offset_start_y = main_window_canvas->pos.y;
		}
		else if( mouse_held( middle ) )
		{
			main_window_canvas->pos.x = drag_offset_start_x + ( mx - drag_start_x );
			main_window_canvas->pos.y = drag_offset_start_y + ( my - drag_start_y );
			refresh = yes;
		}
	}

	if( mouse_pressed( right ) )
	{
		display_get_mouse_position( ref_of( window_drag_start_x ), ref_of( window_drag_start_y ) );
		window_get_position( this, ref_of( window_start_x ), ref_of( window_start_y ) );
	}
	else if( mouse_held( right ) )
	{
		i4 screen_mx,
		screen_my;
		display_get_mouse_position( ref_of( screen_mx ), ref_of( screen_my ) );
		window_set_position( this, window_start_x + ( screen_mx - window_drag_start_x ), window_start_y + ( screen_my - window_drag_start_y ) );
	}

	if( scaling is scaling_manual and this->scroll.y isnt 0 )
	{
		temp r4 const scale_factor = r4_pow( 1.2, this->scroll.y );
		temp r4 const new_scale = r4_clamp( main_window_canvas->scale.w * scale_factor, 0.2, 200.0 );
		temp r4 const actual_factor = new_scale / main_window_canvas->scale.w;

		main_window_canvas->pos.x = i4( r4_round( r4( mx ) - r4( mx - main_window_canvas->pos.x ) * actual_factor ) );
		main_window_canvas->pos.y = i4( r4_round( r4( my ) - r4( my - main_window_canvas->pos.y ) * actual_factor ) );

		main_window_canvas->scale.w = new_scale;
		main_window_canvas->scale.h = new_scale;
		refresh = yes;
	}

	with( scaling )
	{
		when( scaling_rational_fit, scaling_integer_fit_floor, scaling_integer_fit_round, scaling_integer_fit_ceil )
		{
			main_window_canvas->pos.x = i4_clamp( main_window_canvas->pos.x, 0, this->size.w - scaled_w );
			main_window_canvas->pos.y = i4_clamp( main_window_canvas->pos.y, 0, this->size.h - scaled_h );
			skip;
		}

		when( scaling_rational_fill, scaling_integer_fill_floor, scaling_integer_fill_round, scaling_integer_fill_ceil )
		{
			main_window_canvas->pos.x = i4_clamp( main_window_canvas->pos.x, this->size.w - scaled_w, 0 );
			main_window_canvas->pos.y = i4_clamp( main_window_canvas->pos.y, this->size.h - scaled_h, 0 );
			skip;
		}

		when( scaling_rational_stretch )
		{
			main_window_canvas->pos.x = 0;
			main_window_canvas->pos.y = 0;
			skip;
		}

		when( scaling_manual )
		{
			temp const i4 half = i4_min( this->size.w / 4, this->size.h / 4 );
			main_window_canvas->pos.x = i4_clamp( main_window_canvas->pos.x, -scaled_w + half, this->size.w - half );
			main_window_canvas->pos.y = i4_clamp( main_window_canvas->pos.y, -scaled_h + half, this->size.h - half );
			skip;
		}

		other skip;
	}

	if( key_pressed( 1 ) )
	{
		main_window_canvas->scaling = scaling_manual;
		refresh = yes;
	}
	else if( key_pressed( 2 ) or key_pressed( 3 ) )
	{
		if( key_pressed( 2 ) )
		{
			main_window_canvas->scaling = scaling_rational_fit;
		}
		else
		{
			main_window_canvas->scaling = scaling_rational_fill;
		}

		_window_resize( this );
		temp i4 const new_scaled_w = i4( r4_round( r4( main_window_canvas->canvas->size.w ) * main_window_canvas->scale.w ) );
		temp i4 const new_scaled_h = i4( r4_round( r4( main_window_canvas->canvas->size.h ) * main_window_canvas->scale.h ) );
		main_window_canvas->pos.x = ( this->size.w - new_scaled_w ) / 2;
		main_window_canvas->pos.y = ( this->size.h - new_scaled_h ) / 2;
		refresh = yes;
	}
	else if( key_pressed( 4 ) )
	{
		main_window_canvas->scaling = scaling_rational_stretch;
		_window_resize( this );
		main_window_canvas->pos.x = 0;
		main_window_canvas->pos.y = 0;
		refresh = yes;
	}

	if( key_pressed( escape ) )
	{
		window_toggle_border( this );
		refresh = yes;
	}

	if( key_pressed( tab ) )
	{
		temp flag const fits_exactly = scaled_w is this->size.w and scaled_h is this->size.h;

		i4 win_x;
		i4 win_y;
		window_get_position( this, ref_of( win_x ), ref_of( win_y ) );

		if( fits_exactly )
		{
			window_set_size( this, display_w, display_h );
			window_set_position( this, 0, 0 );
			main_window_canvas->pos.x = ( display_w - scaled_w ) / 2;
			main_window_canvas->pos.y = ( display_h - scaled_h ) / 2;
		}
		else
		{
			temp i4 const new_win_w = i4_min( scaled_w, display_w );
			temp i4 const new_win_h = i4_min( scaled_h, display_h );

			temp i4 new_win_x = win_x + main_window_canvas->pos.x;
			temp i4 new_win_y = win_y + main_window_canvas->pos.y;

			new_win_x = i4_clamp( new_win_x, 0, display_w - new_win_w );
			new_win_y = i4_clamp( new_win_y, 0, display_h - new_win_h );

			window_set_size( this, new_win_w, new_win_h );
			window_set_position( this, new_win_x, new_win_y );

			main_window_canvas->pos.x = ( new_win_w - scaled_w ) / 2;
			main_window_canvas->pos.y = ( new_win_h - scaled_h ) / 2;
		}

		refresh = yes;
	}

	if( refresh ) window_refresh( this );
}

start
{
	i4 width = 1;
	i4 height = 1;

	if( start_parameters_count <= 1 )
	{
		width = 200;
		height = 200;
		image = new_canvas( width, height );
		canvas_fill( image, pixel( 0xff, 0, 0x44, 0xff ) );
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

	temp i4 const disp_w = display_get_width();
	temp i4 const disp_h = display_get_height();

	temp r4 scale = 1.0;
	if( width > disp_w or height > disp_h )
	{
		temp r4 const scale_w = r4( disp_w ) / r4( width );
		temp r4 const scale_h = r4( disp_h ) / r4( height );
		scale = r4_min( scale_w, scale_h );
	}

	temp i4 const win_w = i4( r4_round( r4( width ) * scale ) );
	temp i4 const win_h = i4( r4_round( r4( height ) * scale ) );

	new_window( pick( start_parameters[ 1 ] isnt nothing, path_get_name( start_parameters[ 1 ] ), "viewer" ), win_w, win_h );
	window_set_fn_tick( current_window, window_main_tick );
	current_window->clear_before_present = yes;

	main_window_canvas = new_window_canvas( image, sizing_fixed, scaling_manual, nothing );
	main_window_canvas->scale.w = scale;
	main_window_canvas->scale.h = scale;
	window_add_window_canvas( current_window, main_window_canvas );
}
