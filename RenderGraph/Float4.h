#pragma once
#pragma warning(disable:4201)  
#include <cmath>

#define E2I_x m[0]
#define E2I_y m[1]
#define E2I_z m[2]
#define E2I_w m[3]
#define E2I_0 0.0f
#define E2I_1 1.0f

#define E2C_x r
#define E2C_y g
#define E2C_z b
#define E2C_w a
#define E2C_0 0
#define E2C_1 1

#define SWIZZLE4_ACCESSOR_INTERNAL(m0, m1, m2, m3) _##m0##m1##m2##m3
#define SWIZZLE4_ACCESSOR(m0, m1, m2, m3) SWIZZLE4_ACCESSOR_INTERNAL(m0, m1, m2, m3), SWIZZLE4_ACCESSOR_INTERNAL(E2C_##m0, E2C_##m1, E2C_##m2, E2C_##m3)

#define SWIZZLE4_OP(Op, m0, m1, m2, m3)											\
			inline Float4 operator Op(const Float4& other)						\
			{																	\
				this->m0 Op other.m[0];											\
				this->m1 Op other.m[1];											\
				this->m2 Op other.m[2];											\
				this->m3 Op other.m[3];											\
				return Float4(m0, m1, m2, m3);									\
			}	

#define SWIZZLE4_RO_INTERNAL(i0, i1, i2, i3)									\
			inline operator Float4() const										\
			{																	\
				return Float4(i0, i1, i2, i3);									\
			}																												

#define SWIZZLE4_RO(m0, m1, m2, m3)												\
		struct alignas(16) SwizzleType_##m0##m1##m2##m3							\
		{																		\
			SWIZZLE4_RO_INTERNAL(E2I_##m0, E2I_##m1, E2I_##m2, E2I_##m3)		\
		protected:																\
			alignas(16) float m[4];												\
		} SWIZZLE4_ACCESSOR(m0, m1, m2, m3);

#define SWIZZLE4_RW_INTERNAL(i0, i1, i2, i3)									\
			inline operator Float4() const										\
			{																	\
				return Float4(i0, i1, i2, i3);									\
			}																	\
			SWIZZLE4_OP( =, i0, i1, i2, i3)										\
			SWIZZLE4_OP(+=, i0, i1, i2, i3)										\
			SWIZZLE4_OP(-=, i0, i1, i2, i3)										\
			SWIZZLE4_OP(*=, i0, i1, i2, i3)										\
			SWIZZLE4_OP(/=, i0, i1, i2, i3)										\

#define SWIZZLE4_RW(m0, m1, m2, m3)												\
		struct alignas(16) SwizzleType_##m0##m1##m2##m3							\
		{																		\
			SWIZZLE4_RW_INTERNAL(E2I_##m0, E2I_##m1, E2I_##m2, E2I_##m3)		\
		protected:																\
			alignas(16) float m[4];												\
		} SWIZZLE4_ACCESSOR(m0, m1, m2, m3);														

#define SWIZZLE4_PERM2_INTERNAL(m0, m1)											\
\
		SWIZZLE4_RO(m0, m0, m0, m1)												\
		SWIZZLE4_RO(m0, m0, m1, m0)												\
		SWIZZLE4_RO(m0, m0, m1, m1)												\
\
		SWIZZLE4_RO(m0, m1, m0, m0)												\
		SWIZZLE4_RO(m0, m1, m0, m1)												\
		SWIZZLE4_RO(m0, m1, m1, m0)												\
		SWIZZLE4_RO(m0, m1, m1, m1)

#define SWIZZLE4_PERM2(m0, m1)													\
\
		SWIZZLE4_PERM2_INTERNAL(m0, m1)											\
		SWIZZLE4_PERM2_INTERNAL(m1, m0)
		
#define SWIZZLE4_PERM3_INTERNAL(m0, m1, m2)										\
\
		SWIZZLE4_RO(m0, m0, m1, m2)												\
		SWIZZLE4_RO(m0, m0, m2, m1)												\
\
		SWIZZLE4_RO(m0, m1, m0, m2)												\
		SWIZZLE4_RO(m0, m1, m1, m2)												\
		SWIZZLE4_RO(m0, m1, m2, m0)												\
		SWIZZLE4_RO(m0, m1, m2, m1)												\
		SWIZZLE4_RO(m0, m1, m2, m2)												\
\
		SWIZZLE4_RO(m0, m2, m0, m1)												\
		SWIZZLE4_RO(m0, m2, m1, m0)												\
		SWIZZLE4_RO(m0, m2, m1, m1)												\
		SWIZZLE4_RO(m0, m2, m1, m2)												\
		SWIZZLE4_RO(m0, m2, m2, m1)												\

#define SWIZZLE4_PERM3(m0, m1, m2)												\
\
		SWIZZLE4_PERM3_INTERNAL(m0, m1, m2)										\
		SWIZZLE4_PERM3_INTERNAL(m1, m0, m2)										\
		SWIZZLE4_PERM3_INTERNAL(m2, m1, m0)										\

#define SWIZZLE4_PERM4_INTERNAL(MD, m0, m1, m2, m3)								\
\
		SWIZZLE4_##MD(m0, m1, m2, m3)											\
		SWIZZLE4_##MD(m1, m2, m3, m0)											\
		SWIZZLE4_##MD(m2, m3, m0, m1)											\
		SWIZZLE4_##MD(m3, m0, m1, m2)											\

#define SWIZZLE4_PERM4(MD, m0, m1, m2, m3)										\
\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m1, m2, m3)								\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m1, m3, m2)								\
\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m2, m3, m1)								\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m2, m1, m3)								\
\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m3, m2, m1)								\
		SWIZZLE4_PERM4_INTERNAL(MD, m0, m3, m1, m2)								\

#define SWIZZLE4																\
		SWIZZLE4_RO(x, x, x, x);												\
		SWIZZLE4_RO(y, y, y, y);												\
		SWIZZLE4_RO(z, z, z, z);												\
		SWIZZLE4_RO(w, w, w, w);												\
		SWIZZLE4_RO(0, 0, 0, 0);												\
		SWIZZLE4_RO(1, 1, 1, 1);												\
																				\
		SWIZZLE4_PERM2(x, y);													\
		SWIZZLE4_PERM2(x, z);													\
		SWIZZLE4_PERM2(x, w);													\
		SWIZZLE4_PERM2(x, 0);													\
		SWIZZLE4_PERM2(x, 1);													\
																				\
		SWIZZLE4_PERM2(y, z);													\
		SWIZZLE4_PERM2(y, w);													\
		SWIZZLE4_PERM2(y, 0);													\
		SWIZZLE4_PERM2(y, 1);													\
																				\
		SWIZZLE4_PERM2(z, w);													\
		SWIZZLE4_PERM2(z, 0);													\
		SWIZZLE4_PERM2(z, 1);													\
		SWIZZLE4_PERM2(w, 0);													\
		SWIZZLE4_PERM2(w, 1);													\
																				\
		SWIZZLE4_PERM2(0, 1);													\
																				\
		SWIZZLE4_PERM3(x, y, z);												\
		SWIZZLE4_PERM3(x, y, w);												\
		SWIZZLE4_PERM3(x, z, w);												\
		SWIZZLE4_PERM3(y, z, w);												\
																				\
		SWIZZLE4_PERM3(x, y, 0);												\
		SWIZZLE4_PERM3(x, z, 0);												\
		SWIZZLE4_PERM3(y, z, 0);												\
		SWIZZLE4_PERM3(x, w, 0);												\
																				\
		SWIZZLE4_PERM3(y, w, 0);												\
		SWIZZLE4_PERM3(z, w, 0);												\
		SWIZZLE4_PERM3(x, y, 1);												\
		SWIZZLE4_PERM3(x, z, 1);												\
						 														\
		SWIZZLE4_PERM3(y, z, 1);												\
		SWIZZLE4_PERM3(x, w, 1);												\
		SWIZZLE4_PERM3(y, w, 1);												\
		SWIZZLE4_PERM3(z, w, 1);												\
																				\
		SWIZZLE4_PERM3(x, 0, 1);												\
		SWIZZLE4_PERM3(y, 0, 1);												\
		SWIZZLE4_PERM3(z, 0, 1);												\
		SWIZZLE4_PERM3(w, 0, 1);												\
																				\
		SWIZZLE4_PERM4(RO, x, y, z, 0);											\
		SWIZZLE4_PERM4(RO, x, y, w, 0);											\
		SWIZZLE4_PERM4(RO, x, z, w, 0);											\
		SWIZZLE4_PERM4(RO, y, z, w, 0);											\
																				\
		SWIZZLE4_PERM4(RO, x, y, z, 1);											\
		SWIZZLE4_PERM4(RO, x, y, w, 1);											\
		SWIZZLE4_PERM4(RO, x, z, w, 1);											\
		SWIZZLE4_PERM4(RO, y, z, w, 1);											\
																				\
		SWIZZLE4_PERM4(RO, x, y, 0, 1);											\
		SWIZZLE4_PERM4(RO, x, z, 0, 1);											\
		SWIZZLE4_PERM4(RO, y, z, 0, 1);											\
		SWIZZLE4_PERM4(RO, x, w, 0, 1);											\
																				\
		SWIZZLE4_PERM4(RO, y, w, 0, 1);											\
		SWIZZLE4_PERM4(RO, z, w, 0, 1);											\
																				\
		SWIZZLE4_PERM4(RW, x, y, z, w);

#define VECTOR_ASSIGN_OP(Op) SWIZZLE4_OP(Op, m[0], m[1], m[2], m[3])

#define VECTOR_MATH_OP(Op)														\
			inline Float4 operator Op(const Float4& other) const 				\
			{																	\
				return Float4													\
				(																\
					m[0] Op other.m[0],											\
					m[1] Op other.m[1],											\
					m[2] Op other.m[2],											\
					m[3] Op other.m[3]											\
				);																\
			}

struct alignas(16) Float4
{
	Float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	union alignas(16)
	{
		alignas(16) float m[4];
		struct alignas(16)
		{
			float x;
			float y;
			float z;
			float w;
		};
		SWIZZLE4;
	};

	VECTOR_ASSIGN_OP( =);
	VECTOR_ASSIGN_OP(+=);
	VECTOR_ASSIGN_OP(-=);
	VECTOR_ASSIGN_OP(*=);
	VECTOR_ASSIGN_OP(/=);

	VECTOR_MATH_OP(+);
	VECTOR_MATH_OP(-);
	VECTOR_MATH_OP(*);
	VECTOR_MATH_OP(/);

	float operator[](size_t i)
	{
		return m[i];
	}

	float SquaredLength() const
	{
		return m[0] * m[0] + m[1] * m[1] + m[2] * m[2] + m[3] * m[3];
	}

	float Length()
	{
		return std::sqrtf(SquaredLength());
	}

	Float4 Normalize()
	{
		float rsqrt = 1.0f / Length();
		return Float4(m[0] * rsqrt, m[1] * rsqrt, m[2] * rsqrt, m[3] * rsqrt);
	}
};

inline Float4 operator* (const float& s, const Float4 &v)
{
	return Float4(s * v.m[0], s * v.m[1], s * v.m[2], s * v.m[3]);
}

inline Float4 operator* (const Float4 &v, const float& s)
{
	return Float4(s * v.m[0], s * v.m[1], s * v.m[2], s * v.m[3]);
}

inline float dot (const Float4 &v0, const Float4 &v1)
{
	return v0.m[0] * v1.m[0] + v0.m[1] * v1.m[1] + v0.m[2] * v1.m[2] + v0.m[3] * v1.m[3];
}

#undef E2I_x
#undef E2I_y
#undef E2I_z
#undef E2I_w
#undef E2I_0
#undef E2I_1
#undef E2C_x
#undef E2C_y
#undef E2C_z
#undef E2C_w
#undef E2C_0
#undef E2C_1
#undef SWIZZLE4_ACCESSOR_INTERNAL
#undef SWIZZLE4_ACCESSOR
#undef SWIZZLE4_OP
#undef SWIZZLE4_RO_INTERNAL
#undef SWIZZLE4_RO
#undef SWIZZLE4_RW_INTERNAL
#undef SWIZZLE4_RW
#undef SWIZZLE4_PERM2_INTERNAL
#undef SWIZZLE4_PERM2
#undef SWIZZLE4_PERM3_INTERNAL
#undef SWIZZLE4_PERM3
#undef SWIZZLE4_PERM4_INTERNAL
#undef SWIZZLE4_PERM4
#undef SWIZZLE4	
#undef VECTOR_OP