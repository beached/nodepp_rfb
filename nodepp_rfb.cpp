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

#include <iostream>
#include <thread>
#include <vector>

#include <daw/nodepp/lib_net_server.h>
#include "nodepp_rfb.h"
#include "rfb_messages.h"


namespace daw {
	namespace rfb {
		namespace impl {
			namespace {
				constexpr uint8_t get_bit_depth( BitDepth::values bit_depth ) {
					return bit_depth == BitDepth::eight ? 8 : bit_depth == BitDepth::sixteen ? 16 : 32;
				}

				template<typename Collection, typename Func>
				void process_all( Collection && values, Func f ) {
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

				ServerInitialisationMsg create_server_initialization_message( uint16_t width, uint16_t height, uint8_t depth ) {
					ServerInitialisationMsg result;
					memset( &result, 0, sizeof( ServerInitialisationMsg ) );
					result.width = width;
					result.height = height;
					result.pixel_format.bpp = depth;
					result.pixel_format.depth = depth;
					result.pixel_format.true_colour_flag = true;
					result.pixel_format.red_max = 255;
					result.pixel_format.blue_max = 255;
					result.pixel_format.green_max = 255;
					return result;
				}

				ButtonMask create_button_mask( uint8_t mask ) {
					return *(reinterpret_cast<ButtonMask *>(&mask));
				}

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

				template<typename T>
				static void append( T & destination, std::string const & source ) {
					append( destination, to_bytes( source.size( ) ) );
					std::copy( source.begin( ), source.end( ), std::back_inserter( destination ) );
				}

				template<typename T>
				static void append( T & destination, boost::string_ref source ) {
					append( destination, to_bytes( source.size( ) ) );
					std::copy( source.begin( ), source.end( ), std::back_inserter( destination ) );
				}

				bool validate_fixed_buffer( std::shared_ptr<daw::nodepp::base::data_t> & buffer, size_t size ) {
					auto result = static_cast<bool>(buffer);
					result &= buffer->size( ) == size;
					return result;
				}

				template<typename T>
				bool as_bool( T const value ) {
					static_assert(::daw::traits::is_integral_v<T>, "Parameter to as_bool must be an Integral type");
					return value != 0;
				}
			}	// namespace anonymous

			class RFBServerImpl final {
				uint16_t m_width;
				uint16_t m_height;
				uint8_t m_bit_depth;
				std::vector<uint8_t> m_buffer;
				std::vector<Update> m_updates;
				daw::nodepp::lib::net::NetServer m_server;
				std::thread m_service_thread;

				
				void send_all( std::shared_ptr<daw::nodepp::base::data_t> buffer ) {
					assert( buffer );
					m_server->emitter( )->emit( "send_buffer", buffer );
				}

				bool recv_client_initialization_msg( daw::nodepp::lib::net::NetSocketStream & socket, std::shared_ptr<daw::nodepp::base::data_t> data_buffer, int64_t callback_id ) {
					if( validate_fixed_buffer( data_buffer, 1 ) ) {
						if( !as_bool( (*data_buffer)[0] ) ) {
							this->m_server->emitter( )->emit( "close_all", callback_id );
						}
						return true;
					}
					return false;
				}

				void setup_callbacks( ) {
					m_server->on_connection( [&, &srv=m_server]( auto socket ) {
						std::cout << "Connection from: " << socket->remote_address( ) << ":" << socket->remote_port( ) << std::endl;
						// Setup send_buffer callback on server.  This is registered by all sockets so that updated areas
						// can be sent to all clients
						auto send_buffer_callback_id = srv->emitter( )->add_listener( "send_buffer", [s=socket]( std::shared_ptr<daw::nodepp::base::data_t> buffer ) mutable {
							s->write_async( *buffer );
						} );

						//TODO:****************DAW**********HERE***********
						// The close_all callback will close all vnc sessions but the one specified.  This is used when
						// a client connects and requests that no other clients share the session
						auto close_all_callback_id = srv->emitter( )->add_listener( "close_all", [test_callback_id=send_buffer_callback_id, s=socket]( int64_t current_callback_id ) mutable {
							if( test_callback_id != current_callback_id ) {
								s->close( );
							}
						} );

						// When socket is closed, remove registered callbacks in server
						socket->emitter( )->on( "close", [&srv, send_buffer_callback_id, close_all_callback_id]( ) {
							srv->emitter( )->remove_listener( "send_buffer", send_buffer_callback_id );
							srv->emitter( )->remove_listener( "close_all", close_all_callback_id );
						} );

						// We have sent the server version, now validate client version
						socket->on_next_data_received( [&, s1=socket, send_buffer_callback_id]( std::shared_ptr<daw::nodepp::base::data_t> buffer1, bool ) mutable {
							if( !revc_client_version_msg( socket, buffer1 ) ) {
								socket->close( );
								return;
							}

							// Authentication message is sent
							socket->on_next_data_received( [socket, this, send_buffer_callback_id]( std::shared_ptr<daw::nodepp::base::data_t> buffer2, bool ) mutable {
								// Client Initialization Message expected, data buffer should have 1 value
								if( !recv_client_initialization_msg( socket, buffer2, send_buffer_callback_id ) ) {
									socket->close( );
									return;
								}

								// Server Initialization Sent, main reception loop
								socket->on_data_received( [socket, this, send_buffer_callback_id]( std::shared_ptr<daw::nodepp::base::data_t> buffer3, bool ) mutable {
									// Main Receive Loop
									parse_client_msg( socket, buffer3 );
									return;
									socket->read_async( );
								} );
								send_server_initialization_msg( socket );
								socket->read_async( );
							} );
							send_authentication_msg( socket );
							socket->read_async( );
						} );
						send_server_version_msg( socket );
						socket->read_async( );
					} );

				}

				void parse_client_msg( daw::nodepp::lib::net::NetSocketStream socket, std::shared_ptr<daw::nodepp::base::data_t> buffer ) {
					if( !buffer && buffer->size( ) < 1 ) {
						socket->close( );
						return;
					}
					auto const & message_type = buffer->front( );
					switch( message_type ) {
					case 0:	// SetPixelFormat
						break;
					case 1:	// FixColourMapEntries
						break;
					case 2:	// SetEncodings
						break;
					case 3:
					{	// FramebufferUpdateRequest
						if( buffer->size( ) < sizeof( ClientFrameBufferUpdateRequestMsg ) ) {
							socket->close( );
						}
						auto req = nodepp::base::from_data_t_to_value<ClientFrameBufferUpdateRequestMsg>( *buffer );
						add_update_request( req.x, req.y, req.width, req.height );
						update( );
					}
					break;
					case 4:
					{ // KeyEvent
						if( buffer->size( ) < sizeof( ClientKeyEventMsg ) ) {
							socket->close( );
						}
						auto req = nodepp::base::from_data_t_to_value<ClientKeyEventMsg>( *buffer );
						emit_key_event( as_bool( req.down_flag ), req.key );
					}
					break;
					case 5:
					{	// PointerEvent
						if( buffer->size( ) < sizeof( ClientPointerEventMsg ) ) {
							socket->close( );
						}
						auto req = nodepp::base::from_data_t_to_value<ClientPointerEventMsg>( *buffer );
						emit_pointer_event( create_button_mask( req.button_mask ), req.x, req.y );
					}
					break;
					case 6:
					{	// ClientCutText
						if( buffer->size( ) < 8 ) {
							socket->close( );
						}
						auto len = nodepp::base::from_data_t_to_value<uint32_t>( *buffer, 4 );
						//auto len = *(reinterpret_cast<uint32_t *>(buffer->data( ) + 4));
						if( buffer->size( ) < 8 + len ) {
							// Verify buffer is long enough and we don't overflow
							socket->close( );
						}
						boost::string_ref text { buffer->data( ) + 8, len };
						emit_client_clipboard_text( text );
					}
					break;
					}
				}

				void send_server_version_msg( daw::nodepp::lib::net::NetSocketStream socket ) {
					std::string const rfb_version = "RFB 003.003\n";
					socket << rfb_version;
				}

				bool revc_client_version_msg( daw::nodepp::lib::net::NetSocketStream socket, std::shared_ptr<daw::nodepp::base::data_t> data_buffer ) {
					auto result = validate_fixed_buffer( data_buffer, 12 );

					std::string const expected_msg = "RFB 003.003\n";

					if( !std::equal( expected_msg.begin( ), expected_msg.end( ), data_buffer->begin( ) ) ) {
						result = false;
						auto msg = std::make_shared<daw::nodepp::base::data_t>( );
						append( *msg, to_bytes( static_cast<uint32_t>(0) ) );	// Authentication Scheme 0, Connection Failed
						std::string const err_msg = "Unsupported version, only 3.3 is supported";
						append( *msg, err_msg );
						socket->write_async( *msg );
					}
					return result;
				}

				void send_server_initialization_msg( daw::nodepp::lib::net::NetSocketStream socket ) {
					auto msg = std::make_shared<daw::nodepp::base::data_t>( );
					append( *msg, to_bytes( create_server_initialization_message( m_width, m_height, m_bit_depth ) ) );
					append( *msg, boost::string_ref( "Test RFB Service" ) );	// Add title length and title values
					socket->write_async( *msg );	// Send msg
				}

				void send_authentication_msg( daw::nodepp::lib::net::NetSocketStream socket ) {
					auto msg = std::make_shared<daw::nodepp::base::data_t>( );
					append( *msg, to_bytes( static_cast<uint32_t>(1) ) );	// Authentication Scheme 1, No Auth
					socket->write_async( *msg );
				}


			public:
				RFBServerImpl( uint16_t width, uint16_t height, uint8_t bit_depth, daw::nodepp::base::EventEmitter emitter ):
					m_width { width },
					m_height { height },
					m_bit_depth { bit_depth },
					m_buffer( static_cast<size_t>(width*height*(bit_depth == 8 ? 1 : bit_depth == 16 ? 2 : 4)), 0 ),
					m_updates( ),
					m_server( daw::nodepp::lib::net::create_net_server_nossl( std::move( emitter ) ) ),
					m_service_thread( ) {
					setup_callbacks( );
				}

				uint16_t width( ) const {
					return m_width;
				}

				uint16_t height( ) const {
					return m_height;
				}

				void add_update_request( uint16_t x, uint16_t y, uint16_t width, uint16_t height ) {
					m_updates.push_back( { x, y, width, height } );
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
					add_update_request( x1, y1, static_cast<uint16_t>(x2 - x1), static_cast<uint16_t>(y2 - y1) );
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

				void emit_key_event( bool key_down, uint32_t key ) {
					m_server->emitter( )->emit( "on_key_event", key_down, key );
				}

				void on_pointer_event( std::function<void( ButtonMask buttons, uint16_t x_position, uint16_t y_position )> callback ) {
					m_server->emitter( )->on( "on_pointer_event", std::move( callback ) );
				}

				void emit_pointer_event( ButtonMask buttons, uint16_t x_position, uint16_t y_position ) {
					m_server->emitter( )->emit( "on_pointer_event", buttons, x_position, y_position );
				}

				void on_client_clipboard_text( std::function<void( boost::string_ref text )> callback ) {
					m_server->emitter( )->on( "on_clipboard_text", std::move( callback ) );
				}

				void emit_client_clipboard_text( boost::string_ref text ) {
					m_server->emitter( )->emit( "on_clipboard_text", text );
				}

				void send_clipboard_text( boost::string_ref text ) {
					assert( text.size( ) <= std::numeric_limits<uint32_t>::max( ) );
					auto buffer = std::make_shared<daw::nodepp::base::data_t>( );
					buffer->push_back( 0 );	// Message Type, ServerCutText
					buffer->push_back( 0 );	// Padding
					buffer->push_back( 0 );	// Padding
					buffer->push_back( 0 );	// Padding

					append( *buffer, text );

					send_all( buffer );
				}

				void send_bell( ) {
					auto buffer = std::make_shared<daw::nodepp::base::data_t>( 1, 2 );
					send_all( buffer );
				}

				void listen( uint16_t port ) {
					m_server->listen( port );
					//m_service_thread = std::thread( []( ) {
					daw::nodepp::base::start_service( daw::nodepp::base::StartServiceMode::Single );
					//} );
					}

				void close( ) {
					daw::nodepp::base::ServiceHandle::stop( );
					m_service_thread.join( );
				}

			};	// class RFBServerImpl
		}	// namespace impl

		RFBServer::RFBServer( uint16_t width, uint16_t height, BitDepth::values depth, daw::nodepp::base::EventEmitter emitter ):
			m_impl( std::make_unique<impl::RFBServerImpl>( width, height, impl::get_bit_depth( depth ), std::move( emitter ) ) ) { }

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
			m_impl->listen( port );
		}

		void RFBServer::close( ) {
			m_impl->close( );

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
			m_impl->update( );
		}
	}	// namespace rfb
}    // namespace daw
