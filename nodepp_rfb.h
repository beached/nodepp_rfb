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

#include <cstdint>

#include "base_event_emitter.h"

namespace daw { 
	namespace rfb {

		namespace impl {
			class RFBServerImpl;
		}	// namespace impl

		class RFBServer {
			std::unique_ptr<impl::RFBServerImpl> m_impl;
		public:
			struct BitDepth {
				enum values: uint8_t { eight = 8, sixteen = 16, twentyfour = 24 };
			};

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

		};	// class RFBServer
	}	// namespace rfb
}    // namespace daw
