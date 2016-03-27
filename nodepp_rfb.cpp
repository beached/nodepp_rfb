// The MIT License (MIT)
//
// Copyright (c) 2014-2015 Darrell Wright
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

#include "nodepp_rfb.h"

namespace daw { 
	namespace rfb {
		RFBServer::RFBServer( uint16_t port, uint16_t width, uint16_t height, Depth depth, daw::nodepp::base::EventEmitter emitter ):
			m_server{ daw::nodepp::lib::net::create_net_server( std::move( emitter ) ) },
			m_frame_buffer_mutex{ },
			m_dimensions{ std::make_pair( width, height ) },
			m_framebuffers{ std::make_pair( std::vector<uint8_t>( width*height*depth, 0 ), std::vector<uint8_t>( width*height*depth, 0 ) ) },
			m_current_framebuffer{ &m_framebuffers.first } {

			m_server->on_connection( []( auto socket ) {
				socket << "RFB 003.003\n";
				
			} );

			m_server->listen( port );
		}
	}	// namespace rfb
}    // namespace daw
