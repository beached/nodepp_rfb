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


#include <vector>

#include "nodepp_rfb.h"



namespace daw { 
	namespace rfb {
		namespace impl {
			class RFBServerImpl { 
				uint16_t m_width;
				uint16_t m_height;
				uint8_t m_bit_depth;
				std::vector<uint8_t> m_buffer;
			public:
				RFBServerImpl( uint16_t width, uint16_t height, uint8_t bit_depth, daw::nodepp::base::EventEmitter emitter ):
					m_width{ width },
					m_height{ height },
					m_bit_depth{ bit_depth },
					m_buffer( static_cast<size_t>(width*height*bit_depth), 0 ) {
				}
				
				uint16_t width( ) const {
					return m_width;
				}

				uint16_t height( ) const {
					return m_height;
				}

				Box get_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) {
					assert( y2 >= y1 );
					assert( x2 >= x1 );
					Box result;
					result.reserve( static_cast<size_t>(y2 - y1) );

					auto const width = x2 - x1;
					for( size_t n = y1; n < y2; ++n ) {
						auto p1 = m_buffer.data( ) + (m_width*n) + x1;
						auto rng = daw::range::make_range( p1, p1 + width );
						result.push_back( rng );
					}
					return result;
				}
			};	// class RFBServerImpl
		}	// namespace impl
		namespace {
			constexpr uint8_t get_bit_depth( BitDepth::values bit_depth ) {
				return bit_depth == BitDepth::eight ? 8 : bit_depth == BitDepth::sixteen ? 16 : 32;
			}
		}	// namespace anonymous

		RFBServer::RFBServer( uint16_t width, uint16_t height, BitDepth::values depth, daw::nodepp::base::EventEmitter emitter ):
			m_impl( std::make_unique<impl::RFBServerImpl>( width, height, get_bit_depth( depth ), std::move( emitter ) ) ) {

		}

		RFBServer::~RFBServer( ) { }	// Empty but cannot use default.  The destructor on std::unique_ptr needs to know the full info for RFBServerImpl

		RFBServer::RFBServer( RFBServer && other ): m_impl( std::move( other.m_impl ) ) { }
		
		RFBServer & RFBServer::operator=( RFBServer && rhs ) {
			if( this != &rhs ) {
				m_impl = std::move( rhs.m_impl );
			}
			return *this;
		}

		uint16_t RFBServer::width( ) const {
			return m_impl->width( );
		}

		uint16_t RFBServer::height( ) const {
			return m_impl->height( );
		}

		void RFBServer::listen( uint16_t port ) {

		}

		void RFBServer::close( ) {

		}

		void RFBServer::on_key_event( std::function<void( bool key_down, uint32_t key )> callback ) {
		}

		void RFBServer::on_pointer_event( std::function<void( ButtonMask buttons, uint16_t x_position, uint16_t y_position )> callback ) {
		}

		void RFBServer::on_client_clipboard_text( std::function<void( boost::string_ref text )> callback ) {
		}

		Box RFBServer::get_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) {
			return m_impl->get_area( x1, y1, x2, y2 );
		}

		void RFBServer::update( ) {
		}
	}	// namespace rfb
}    // namespace daw
