/*
 * Copyright (c) 2008-2017, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#pragma once

#include "NvFlexTypes.h"

#include <new>

namespace NvFlex
{

	template<class T, NvFlexUint staticCapacity>
	struct VectorCached : public Allocable
	{
		static const NvFlexUint s_staticCapacity = staticCapacity;

		T* m_data;
		NvFlexUint m_capacity;
		T m_cache[staticCapacity];
		NvFlexUint m_size = 0u;

		T* allocate(NvFlexUint capacity)
		{
			T* ptr = (T*)operator new[](capacity*sizeof(T));
			for (NvFlexUint i = 0; i < capacity; i++)
			{
				new(ptr + i) T;
			}
			return ptr;
		}

		void cleanup(T* data, NvFlexUint capacity)
		{
			if (m_cache != data)
			{
				for (NvFlexUint i = 0; i < capacity; i++)
				{
					data[i].~T();
				}
				operator delete[](data);
			}
		}

		VectorCached(NvFlexUint capacity)
		{
			m_capacity = capacity;
			if (capacity > staticCapacity)
			{
				m_data = allocate(capacity);
			}
			else
			{
				m_data = m_cache;
			}
		}

		~VectorCached()
		{
			if (m_capacity > staticCapacity)
			{
				cleanup(m_data, m_capacity);
			}
			m_data = nullptr;
		}

		T& operator[](unsigned int idx)
		{
			return m_data[idx];
		}

		NvFlexUint allocateBack()
		{
			// resize if needed
			if (m_size + 1 > m_capacity)
			{
				NvFlexUint capacity = 2 * m_capacity;
				T* newData = allocate(capacity);
				// copy to new
				for (NvFlexUint i = 0; i < m_size; i++)
				{
					new(&newData[i]) T(m_data[i]);
				}
				// cleanup old
				cleanup(m_data, m_capacity);
				// commit new
				m_data = newData;
				m_capacity = capacity;
			}
			m_size++;
			return m_size - 1;
		}

		void reserve(NvFlexUint capacity)
		{
			if (capacity > m_capacity)
			{
				T* newData = allocate(capacity);
				// copy to new
				for (NvFlexUint i = 0; i < m_size; i++)
				{
					new(&newData[i]) T(m_data[i]);
				}
				// cleanup old
				cleanup(m_data, m_capacity);
				// commit new
				m_data = newData;
				m_capacity = capacity;
			}
		}
	};

	template <class T>
	struct Image3D : public Allocable
	{
		T* m_data = nullptr;
		NvFlexDim m_dim = { 0u, 0u, 0u };
		NvFlexUint m_capacity = 0u;

		Image3D() {}

		void init(NvFlexDim dim)
		{
			m_dim = dim;
			m_capacity = m_dim.x * m_dim.y * m_dim.z;

			T* ptr = (T*)operator new[](m_capacity*sizeof(T));
			for (NvFlexUint i = 0; i < m_capacity; i++)
			{
				new(ptr + i) T;
			}
			m_data = ptr;
		}

		~Image3D()
		{
			if (m_data)
			{
				for (NvFlexUint i = 0; i < m_capacity; i++)
				{
					m_data[i].~T();
				}
				operator delete[](m_data);
				m_data = nullptr;
			}
		}

		T& operator[](unsigned int idx)
		{
			return m_data[idx];
		}

		T& operator()(NvFlexUint i, NvFlexUint j, NvFlexUint k)
		{
			return m_data[(k*m_dim.y + j) * m_dim.x + i];
		}
	};

	template <class T>
	struct Image1D : public Allocable
	{
		T* m_data = nullptr;
		NvFlexUint m_dim = 0u;
		NvFlexUint m_capacity = 0u;

		Image1D() {}

		void init(NvFlexUint dim)
		{
			m_dim = dim;
			m_capacity = m_dim;

			T* ptr = (T*)operator new[](m_capacity*sizeof(T));
			for (NvFlexUint i = 0; i < m_capacity; i++)
			{
				new(ptr + i) T;
			}
			m_data = ptr;
		}

		~Image1D()
		{
			if (m_data)
			{
				for (NvFlexUint i = 0; i < m_capacity; i++)
				{
					m_data[i].~T();
				}
				operator delete[](m_data);
				m_data = nullptr;
			}
		}

		T& operator[](unsigned int idx)
		{
			return m_data[idx];
		}
	};

}