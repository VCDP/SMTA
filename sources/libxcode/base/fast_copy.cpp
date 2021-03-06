/*
 * 
 *   Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
 * 
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 * 
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *   Contact Information:
 *   Intel Corporation
 * 
 * 
 *  version: SMTA.A.0.9.4-2
 */

#include "fast_copy.h"
#include <stdio.h>

void *fast_copy( void *dst, size_t dst_size, void *src, size_t src_size )
{
    size_t aligned;
    size_t remain;
    size_t i, round;
    __m128i x0, x1, x2, x3, x4, x5, x6, x7;
    __m128i *pDst, *pSrc;
    size_t size = src_size;

    if ( dst == NULL || src == NULL ) {
        return NULL;
    }

    aligned = (((size_t) dst) | ((size_t) src)) & 0x0F;
    if ( aligned != 0 ) {
        //      printf( "Normal copy from 0x%x to 0x%x\n", src, dst );
        size = dst_size >= src_size ? src_size : dst_size;
        return memcpy( dst, src, size );
    }

    pDst = (__m128i *) dst;
    pSrc = (__m128i *) src;
    remain = size & 0x7F;
    round = size >> 7;
    _mm_mfence();

    for ( i = 0; i < round; i++ ) {
        x0 = _mm_stream_load_si128( pSrc + 0 );
        x1 = _mm_stream_load_si128( pSrc + 1 );
        x2 = _mm_stream_load_si128( pSrc + 2 );
        x3 = _mm_stream_load_si128( pSrc + 3 );
        x4 = _mm_stream_load_si128( pSrc + 4 );
        x5 = _mm_stream_load_si128( pSrc + 5 );
        x6 = _mm_stream_load_si128( pSrc + 6 );
        x7 = _mm_stream_load_si128( pSrc + 7 );
        /*
        __asm__ __volatile__( "movntdqa (%1), %0":"=x"(x0):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 16(%1), %0":"=x"(x1):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 32(%1), %0":"=x"(x2):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 48(%1), %0":"=x"(x3):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 64(%1), %0":"=x"(x4):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 80(%1), %0":"=x"(x5):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 96(%1), %0":"=x"(x6):"g"(pSrc) );
        __asm__ __volatile__( "movntdqa 112(%1), %0":"=x"(x7):"g"(pSrc) );
        */
        _mm_store_si128( pDst + 0, x0 );
        _mm_store_si128( pDst + 1, x1 );
        _mm_store_si128( pDst + 2, x2 );
        _mm_store_si128( pDst + 3, x3 );
        _mm_store_si128( pDst + 4, x4 );
        _mm_store_si128( pDst + 5, x5 );
        _mm_store_si128( pDst + 6, x6 );
        _mm_store_si128( pDst + 7, x7 );
        pSrc += 8;
        pDst += 8;
    }

    if ( remain >= 16 ) {
        size = remain;
        remain = size & 0xF;
        round = size >> 4;

        for ( i = 0; i < round; i++ ) {
            //__asm__ ( "movntdqa (%1), %0":"=x"(x0):"g"(pSrc) );
            x0 = _mm_stream_load_si128( pSrc + 0 );
            pSrc += 1;
            _mm_store_si128( pDst, x0 );
            pDst += 1;
        }
    }

    if ( remain > 0 ) {
        char *ps = (char *)(pSrc);
        char *pd = (char *)(pDst);

        for ( i = 0; i < remain; i++ ) {
            pd[i] = ps[i];
        }
    }

    return dst;
}

