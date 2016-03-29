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

#pragma once

#include <boost/utility/string_ref.hpp>
#include <cstdint>
#include <string>
#include <vector>

#include "base_event_emitter.h"
#include "daw_range.h"

namespace daw { 
	namespace rfb {

		namespace impl {
			class RFBServerImpl;
		}	// namespace impl

		namespace BitDepth {
			enum values: uint8_t { eight = 8, sixteen = 16, thirtytwo = 32 };
		}	// namespace BitDepth

		union ButtonMask {
			uint8_t value;
			struct {
				bool button_1 : 1;
				bool button_2 : 1;
				bool button_3 : 1;
				bool button_4 : 1;
				bool button_5 : 1;
				bool button_6 : 1;
				bool button_7 : 1;
				bool button_8 : 1;
			} button;
		};	// union ButtonMask

		struct Colour {
			uint8_t red;
			uint8_t green;
			uint8_t blue;
			uint8_t padding;
		};

		using Box = std::vector<daw::range::Range<uint8_t *>>;
		using BoxReadOnly = std::vector<daw::range::Range<uint8_t const *>>;

		class RFBServer {
			std::unique_ptr<impl::RFBServerImpl> m_impl;
		public:	
			RFBServer( ) = delete;
			RFBServer( RFBServer const & ) = delete;
			RFBServer & operator=( RFBServer const & ) = delete;

			RFBServer( uint16_t width, uint16_t height, BitDepth::values depth, daw::nodepp::base::EventEmitter emitter = daw::nodepp::base::create_event_emitter( ) );
			~RFBServer( );
			RFBServer( RFBServer && other );
			RFBServer & operator=( RFBServer && rhs );
			

			uint16_t width( ) const;
			uint16_t height( ) const;			

			void listen( uint16_t port );
			void close( );

			void on_key_event( std::function<void( bool key_down, uint32_t key )> callback );
			void on_pointer_event( std::function<void( ButtonMask buttons, uint16_t x_position, uint16_t y_position )> callback );
			void on_client_clipboard_text( std::function<void( boost::string_ref text )> callback );

			void send_clipboard_text( boost::string_ref text );
			void send_bell( );
			//////////////////////////////////////////////////////////////////////////
			/// Summary: get a bounded area that will later be updated to the client
			Box get_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 );
			BoxReadOnly get_readonly_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) const;

			//////////////////////////////////////////////////////////////////////////
			/// Summary: send all updated areas to client
			void update( );
		};	// class RFBServer
	}	// namespace rfb
}    // namespace daw
