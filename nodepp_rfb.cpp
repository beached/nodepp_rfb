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
#include "lib_net_server.h"


namespace daw { 
	namespace rfb {
		namespace impl {
			template<typename Collection, typename Func>
			void process_all( Collection & values, Func f ) {
				while( !values.empty( ) ) {
					f( values.back( ) );
					values.pop_back( );
				}
			}

			struct Update {
				uint16_t x;
				uint16_t y;
				uint16_t width;
				uint16_t height;
			};	// struct Update

			class RFBServerImpl { 
				uint16_t m_width;
				uint16_t m_height;
				uint8_t m_bit_depth;
				std::vector<uint8_t> m_buffer;
				std::vector<Update> m_updates;
				daw::nodepp::lib::net::NetServer m_server;

				void setup_callbacks( ) {
					m_server->on_connection( [&]( daw::nodepp::lib::net::NetSocketStream socket ) {
						auto callback_id = m_server->emitter( )->add_listener( "send_buffer", [socket]( std::shared_ptr<daw::nodepp::base::data_t> buffer ) {
							socket->write_async( *buffer );
						} );
						socket->emitter( )->on( "close", [callback_id, this]( ) {
							this->m_server->emitter( )->remove_listener( "send_buffer", callback_id );
						} );

						socket << "RFB 003.003\n";
					} );
				}

			public:
				RFBServerImpl( uint16_t width, uint16_t height, uint8_t bit_depth, daw::nodepp::base::EventEmitter emitter ):
					m_width{ width },
					m_height{ height },
					m_bit_depth{ bit_depth },
					m_buffer( static_cast<size_t>(width*height*bit_depth), 0 ),
					m_updates( ),
					m_server( daw::nodepp::lib::net::create_net_server( std::move( emitter ) ) ) {
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
					m_updates.push_back( { x1, y1, static_cast<uint16_t>(x2 - x1), static_cast<uint16_t>(y2 - y1) } );
					return result;
				}

				BoxReadOnly get_read_only_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) const {
					assert( y2 >= y1 );
					assert( x2 >= x1 );
					BoxReadOnly result;
					result.reserve( static_cast<size_t>(y2 - y1) );

					auto const width = x2 - x1;
					for( size_t n = y1; n < y2; ++n ) {
						auto p1 = m_buffer.data( ) + (m_width*n) + x1;
						auto rng = daw::range::make_range<uint8_t const *>( p1, p1 + width );
						result.push_back( rng );
					}
					return result;
				}

			private:
				template<typename T>
				static std::vector<unsigned char> to_bytes( T const & value ) {
					auto const N = sizeof( T );
					std::vector<unsigned char> result( N );
					*(reinterpret_cast<T *>(result.data( ))) = value;
					return result;
				}

				template<typename T, typename U>
				static void append( T & destination, U const & source ) {
					std::copy( source.begin( ), source.end( ), std::back_inserter( destination ) );
				}

				void send_all( std::shared_ptr<daw::nodepp::base::data_t> buffer ) {
					m_server->emitter( )->emit( "send_buffer", buffer );
				}
				
			public:
				void update( ) {
					auto buffer = std::make_shared<daw::nodepp::base::data_t>( );
					buffer->push_back( 0 );	// Message Type, FrameBufferUpdate
					buffer->push_back( 0 );	// Padding
					append( *buffer, to_bytes( static_cast<uint16_t>(m_updates.size( )) ) );
					impl::process_all( m_updates, [&]( auto const & u ) {						
						append( *buffer, to_bytes( u ) );
						buffer->push_back( 0 );	// Encoding type RAW
						for( size_t row = u.y; row < u.y + u.height; ++row ) {
							append( *buffer, daw::range::make_range( m_buffer.begin( ) + (u.y*m_width) + u.x, m_buffer.begin( ) + ((u.y + u.height)*m_width) + u.x + u.width ) );
						}
					} );
					send_all( buffer );
				}

				void on_key_event( std::function<void( bool key_down, uint32_t key )> callback ) {
					m_server->emitter( )->on( "on_key_event", std::move( callback ) );
				}

				void on_pointer_event( std::function<void( ButtonMask buttons, uint16_t x_position, uint16_t y_position )> callback ) {
					m_server->emitter( )->on( "on_pointer_event", std::move( callback ) );
				}
				void on_client_clipboard_text( std::function<void( boost::string_ref text )> callback ) {
					m_server->emitter( )->on( "on_clipboard_text", std::move( callback ) );
				}

				void send_clipboard_text( boost::string_ref text ) {
					assert( text.size( ) <= std::numeric_limits<uint32_t>::max( ) );
					auto buffer = std::make_shared<daw::nodepp::base::data_t>( );
					buffer->push_back( 0 );	// Message Type, ServerCutText
					buffer->push_back( 0 );	// Padding
					buffer->push_back( 0 );	// Padding
					buffer->push_back( 0 );	// Padding
					
					append( *buffer, to_bytes( static_cast<uint32_t>(text.size( )) ) );
					append( *buffer, text );
					send_all( buffer );
				}

				void send_bell( ) {
					auto buffer = std::make_shared<daw::nodepp::base::data_t>( 1, 2 );
					send_all( buffer );
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
			m_impl->on_key_event( std::move( callback ) );
		}

		void RFBServer::on_pointer_event( std::function<void( ButtonMask buttons, uint16_t x_position, uint16_t y_position )> callback ) {
			m_impl->on_pointer_event( std::move( callback ) );
		}

		void RFBServer::on_client_clipboard_text( std::function<void( boost::string_ref text )> callback ) {
			m_impl->on_client_clipboard_text( std::move( callback ) );
		}

		void RFBServer::send_clipboard_text( boost::string_ref text ) {
			m_impl->send_clipboard_text( text );
		}

		void RFBServer::send_bell( ) {
			m_impl->send_bell( );
		}

		Box RFBServer::get_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) {
			return m_impl->get_area( x1, y1, x2, y2 );
		}

		BoxReadOnly RFBServer::get_readonly_area( uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2 ) const {
			return m_impl->get_read_only_area( x1, y1, x2, y2 );
		}

		void RFBServer::update( ) {
		}
	}	// namespace rfb
}    // namespace daw
