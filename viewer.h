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

object_fn( window, draw )
{
	canvas_clear( this->buffer );
	canvas_draw_canvas_safe( this->buffer, image, ( i2( this->buffer.size.w ) - i2( image.size.w ) ) / 2, ( i2( this->buffer.size.h ) - i2( image.size.h ) ) / 2 );
}

start
{
	if( start_parameters_count <= 1 )
	{
		//
	}

	const byte const_ref input = start_parameters[ 1 ];

	byte ref path_ext = path_get_extension( input );

	i4 width = 1;
	i4 height = 1;

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

	new_window( width, height );
	window_set_draw_fn( current_window, window_draw );
	window_set_buffer_max( current_window, width, height );
}
