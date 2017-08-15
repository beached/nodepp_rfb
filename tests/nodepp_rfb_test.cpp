// The MIT License (MIT)
//
// Copyright (c) 2014-2017 Darrell Wright
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <chrono>
#include <cstdlib>
#include <random>
#include <thread>

#include "nodepp_rfb.h"

void draw_rectagle( daw::rfb::RFBServer &srv, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                    daw::rfb::Colour colour ) {
	auto box = srv.get_area( x1, y1, x2, y2 );
	for( auto &row : box ) {
		auto r2 = daw::range::make_range( reinterpret_cast<daw::rfb::Colour *>( row.begin( ) ),
		                                  reinterpret_cast<daw::rfb::Colour *>( row.end( ) ) );
		for( auto &cell : r2 ) {
			cell = colour;
		}
	}
}

int main( int, char ** ) {

	daw::rfb::RFBServer server{640, 480, daw::rfb::BitDepth::eight};

	auto th = std::thread( [&]( ) {
		std::random_device rd;
		std::mt19937 gen{rd( )};
		std::uniform_int_distribution<> dist_x{0, server.width( ) - 1};
		std::uniform_int_distribution<> dist_y{0, server.height( ) - 1};
		std::uniform_int_distribution<> dist_c{0, 255};

		while( true ) {
			auto x = static_cast<uint16_t>( dist_x( gen ) );
			auto y = static_cast<uint16_t>( dist_y( gen ) );
			uint16_t width = 20;
			uint16_t height = 20;
			if( x + width >= server.width( ) ) {
				width = ( server.width( ) - x ) - 1;
			}
			if( y + height >= server.height( ) ) {
				height = ( server.height( ) - y ) - 1;
			}
			draw_rectagle( server, x, y, x + width, y + height,
			               {static_cast<uint8_t>( dist_c( gen ) ), static_cast<uint8_t>( dist_c( gen ) ),
			                static_cast<uint8_t>( dist_c( gen ) ), 0} );
			std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
		}
	} );
	server.listen( 1234, daw::nodepp::lib::net::ip_version::ipv4 );

	return EXIT_SUCCESS;
}
