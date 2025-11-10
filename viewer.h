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

object_fn( window, input )
{
	temp flag refresh = no;
	perm i4 drag_start_x;
	perm i4 drag_start_y;
	perm i4 drag_offset_start_x;
	perm i4 drag_offset_start_y;

	temp i4 mx = this->mouse_x;
	temp i4 my = this->mouse_y;

	if( mouse_left_pressed() )
	{
		drag_start_x = mx;
		drag_start_y = my;
		drag_offset_start_x = this->draw_offset.x;
		drag_offset_start_y = this->draw_offset.y;
	}
	else if( mouse_left_held() )
	{
		this->draw_offset.x = drag_offset_start_x + ( mx - drag_start_x );
		this->draw_offset.y = drag_offset_start_y + ( my - drag_start_y );
		refresh = yes;
	}

	temp const i2 scaled_w = this->buffer.size.w * this->scale;
	temp const i2 scaled_h = this->buffer.size.h * this->scale;

	temp const i2 delta_w = ( ( i2( this->buffer.size.w ) - i2( image.size.w ) + 1 ) / 2 ) * this->scale;
	temp const i2 delta_h = ( ( i2( this->buffer.size.h ) - i2( image.size.h ) + 1 ) / 2 ) * this->scale;

	temp const i2 half_w = this->size_target.w / 2;
	temp const i2 half_h = this->size_target.h / 2;

	this->draw_offset.x = i2_clamp( this->draw_offset.x, -scaled_w + half_w + delta_w, this->size_target.w - half_w - delta_w );
	this->draw_offset.y = i2_clamp( this->draw_offset.y, -scaled_h + half_h + delta_h, this->size_target.h - half_h - delta_h );

	if( refresh ) window_refresh( this );
}

object_fn( window, draw )
{
	canvas_clear( this->buffer );
	canvas_fill( this->buffer, pixel(0x77,0,0x22,0xff));
	canvas_draw_canvas_safe( this->buffer, image, ( i2( this->buffer.size.w ) - i2( image.size.w ) + 1 ) / 2, ( i2( this->buffer.size.h ) - i2( image.size.h ) + 1 ) / 2 );
}

start
{
	i4 width = 1;
	i4 height = 1;

	if( start_parameters_count <= 1 )
	{
		width = 200;
		height = 200;
		image = canvas( width, height );
		canvas_fill( image, pixel( 0xff, 0, 0x44, 0xff ) );
	}
	else
	{
		const byte const_ref input = start_parameters[ 1 ];

		byte ref path_ext = path_get_extension( input );

		if( bytes_compare( path_ext, "pep", 4 ) is 0 )
		{
			pep loaded_pep = pep_load( input );
			image.pixels = to( pixel ref, pep_decompress( ref_of( loaded_pep ), pep_bgra, 0 ) );
			pep_free( ref_of( loaded_pep ) );
			image.size.w = loaded_pep.width;
			image.size.h = loaded_pep.height;
			width = image.size.w;
			height = image.size.h;
		}
		else
		{
			i4 channels;
			temp const byte const_ref loaded_bytes = to( const byte const_ref, stbi_load( input, ref_of( width ), ref_of( height ), ref_of( channels ), 4 ) );
			image = canvas( width, height );
			iter( index, width * height )
			{
				temp const int byte_index = index << 2;
				canvas_draw_pixel_index( image, index, pixel( loaded_bytes[ byte_index ], loaded_bytes[ byte_index + 1 ], loaded_bytes[ byte_index + 2 ], loaded_bytes[ byte_index + 3 ] ) );
			}
		}
	}

	new_window( width, height );
	window_set_draw_fn( current_window, window_draw );
	window_set_input_fn( current_window, window_input );
	window_set_buffer_max( current_window, width, height );
}
