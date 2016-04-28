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
#include <cstring>
#include <cstdint>

namespace daw {
	namespace rfb {
		namespace impl {
			struct ServerInitialisationMsg {
				uint16_t width;
				uint16_t height;
				struct {
					uint8_t bpp;
					uint8_t depth;
					uint8_t big_endian_flag;
					uint8_t true_colour_flag;
					uint16_t red_max;
					uint16_t green_max;
					uint16_t blue_max;
					uint16_t red_shift;
					uint16_t green_shift;
					uint16_t blue_shift;
					uint8_t padding[3];
				} pixel_format;
				// Send name length/name after

			};	// struct ServerInitialisation

			struct ClientFrameBufferUpdateRequestMsg {
				uint8_t message_type;	// Always 3
				uint8_t incremental;
				uint16_t x;
				uint16_t y;
				uint16_t width;
				uint16_t height;
			};	// struct FramebufferUpdateRequest

			struct ClientKeyEventMsg {
				uint8_t message_type;	// Always 4
				uint8_t down_flag;
				uint16_t padding;
				uint32_t key;
			};	// struct ClientKeyEventMsg

			struct ClientPointerEventMsg {
				uint8_t message_type;	// Always 5
				uint8_t button_mask;
				uint16_t x;
				uint16_t y;
			};	// struct ClientPointerEventMsg

		}	// namespace impl
	}	// namespace rfb

}	// namespace daw

