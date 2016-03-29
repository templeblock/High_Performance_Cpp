/*
 * vectorization.h
 *
 *  Created on: 16 mars 2016
 *      Author: gnthibault
 */

#ifndef VECTORIZATION_H_
#define VECTORIZATION_H_

//STL
#include <vector>
#include <exception>

//Boost
#include <boost/align/aligned_allocator.hpp>

/*
 * Include for x86 intrinsics
 * Documentation for the various intrinsics can be found on
 * https://software.intel.com/sites/landingpage/IntrinsicsGuide/
 */
#ifdef USE_SSE
	#include "xmmintrin.h"
	#include "emmintrin.h"
#elif defined USE_AVX
	#include "immintrin.h"
#elif defined USE_NEON
	#include <arm_neon.h>
#endif

//Too dangerous to define default implementation
template<typename T, class VecT>
class VectorizedMemOp
{
public:
	static VecT load( const T* ptr )
	{
		return *ptr;
	}
	static void store( float* ptr, VecT value)
	{
		*ptr = value;
	}
};

#ifdef USE_SSE
	template<>
	class VectorizedMemOp<float,__m128>
	{
	public:
		static __m128 load( const float* ptr )
		{
			return _mm_load_ps( ptr );
		}
		static void store( float* ptr, __m128 value)
		{
			_mm_store_ps( ptr, value );
		}
	};
#elif defined USE_AVX
	template<>
	class VectorizedMemOp<float,__m256>
	{
	public:
		static __m256 load( const float* ptr )
		{
			return _mm256_load_ps( ptr );
		}
		static void store( float* ptr, __m256 value)
		{
			_mm256_store_ps( ptr, value );
		}
	};
#elif defined USE_NEON
	template<>
	class VectorizedMemOp<float,float32x4_t>
	{
	public:
		static float32x4_t load( const float* ptr )
		{
			return vld1q_f32( ptr );
		}
		static void store( float* ptr, float32x4_t value)
		{
			vst1q_f32( ptr, value );
		}
	};
#endif

#ifdef USE_SSE
	/*
	 * No generic implementation for this function for now
	 * Used in the dot product example
	 * Sum all 4 float elements of a 128 bits vector into 1 value
	 * To do so, we use a butterfly like summation pattern
	 */
	float sumFloat(__m128 value)
	{
		/*
		 * Shuffle the input vector such that we have 1,0,3,2
		 * This is equivalent to a pairwise swap where the first
		 * two elements are swapped with the next two
		 */
		__m128 shufl = _mm_shuffle_ps(value,value, _MM_SHUFFLE(1,0,3,2));

		//Sum both values
		shufl = _mm_add_ps(value, shufl);
		//shufl = |3|2|1|0| + |1|0|3|2| = |3+1|2+0|1+3|0+2|

		/*
		 * Second shuffle 2,3,0,1
		 * This is equivalent to 1 by 1 swap between every
		 * two neighboring elements from the first swap
		 */
		__m128 shufl2 = _mm_shuffle_ps(shufl,shufl, _MM_SHUFFLE(2,3,0,1));
		//shufl2 = |2+0|3+1|0+2|1+3|


		//Sum both values
		shufl = _mm_add_ps(shufl, shufl2);
		//shufl = |3+1|2+0|1+3|0+2| + |2+0|3+1|0+2|1+3|

		//Extract the right value
		return _mm_cvtss_f32 (shufl);
		//return _mm_extract_ps (shufl a, 0);
	}
#endif

//Perform left and right shift
template<typename T, class VecT, unsigned char SHIFT>
class VectorizedShift
{
public:
	//Defaulted implementation for scalar type: no shift
	constexpr static VecT LeftShift( VecT input )
	{
		return SHIFT != 0 ? 0 : input;
	}
	//Defaulted implementation for scalar type: no shift
	constexpr static VecT RightShift( VecT input )
	{
		return SHIFT != 0 ? 0 : input;
	}
};

#ifdef USE_SSE
	template<unsigned char SHIFT>
	class VectorizedShift<float,__m128,SHIFT>
	{
	public:
		constexpr static __m128 LeftShift( __m128 input )
		{
			return (__m128)_mm_slli_si128( (__m128i)input, SHIFT );
		}
		constexpr static __m128 RightShift( __m128 input )
		{
			return (__m128)_mm_srli_si128( (__m128i)input, SHIFT );
		}
	};
#endif


// forward-declaration to allow use in Iter
template<typename T> class SimdVec;

//Specialize Packed types when they exist
template<typename T> struct PackedType { typedef T type; };//Default packed type is... not packed

#ifdef USE_SSE
	template<> struct PackedType<float> { using type = __m128; };
#elif defined USE_AVX
	template<> struct PackedType<float> { using type = __m256; };
#elif defined USE_NEON
	template<> struct PackedType<float> { using type = float32x4_t; };
#endif
template<typename T> using PackType = typename PackedType<T>::type;

#endif /* VECTORIZATION_H_ */
