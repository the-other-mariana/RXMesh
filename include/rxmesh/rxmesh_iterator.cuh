#pragma once
#include <stdint.h>
#include "rxmesh/rxmesh_types.h"

namespace rxmesh {

struct RXMeshIterator
{
    __device__ RXMeshIterator(const uint16_t  local_id,
                              const uint16_t* patch_output,
                              const uint16_t* patch_offset,
                              const uint32_t* output_ltog_map,
                              const uint32_t  offset_size,
                              const uint32_t  num_src_in_patch,
                              int             shift = 0)
        : m_patch_output(patch_output),
          m_patch_offset(patch_offset),
          m_output_ltog_map(output_ltog_map),
          m_num_src_in_patch(num_src_in_patch),
          m_shift(shift)
    {
        set(local_id, offset_size);
    }

    RXMeshIterator(const RXMeshIterator& orig) = default;

    __device__ uint16_t local_id() const
    {
        return m_local_id;
    }

    __device__ uint16_t size() const
    {
        return m_end - m_begin;
    }

    __device__ uint16_t neighbour_local_id(uint32_t i) const
    {
        return m_patch_output[m_begin + i];
    }

    __device__ uint32_t operator[](const uint32_t i) const
    {
        assert(m_patch_output);
        assert(m_output_ltog_map);
        assert(i + m_begin < m_end);
        return m_output_ltog_map[((m_patch_output[m_begin + i]) >> m_shift)];
    }

    __device__ uint32_t operator*() const
    {
        assert(m_patch_output);
        assert(m_output_ltog_map);
        return ((*this)[m_current]);
    }

    __device__ uint32_t back() const
    {
        return ((*this)[size() - 1]);
    }

    __device__ uint32_t front() const
    {
        return ((*this)[0]);
    }

    __device__ RXMeshIterator& operator++()
    {
        // pre
        m_current = (m_current + 1) % size();
        return *this;
    }
    __device__ const RXMeshIterator operator++(int)
    {
        // post
        RXMeshIterator pre(*this);
        m_current = (m_current + 1) % size();
        return pre;
    }

    __device__ RXMeshIterator& operator--()
    {
        // pre
        m_current = (m_current == 0) ? size() - 1 : m_current - 1;
        return *this;
    }

    __device__ const RXMeshIterator operator--(int)
    {
        // post
        RXMeshIterator pre(*this);
        m_current = (m_current == 0) ? size() - 1 : m_current - 1;
        return pre;
    }

    __device__ bool operator==(const RXMeshIterator& rhs) const
    {
        return rhs.m_local_id == m_local_id && rhs.m_current == m_current;
    }

    __device__ bool operator!=(const RXMeshIterator& rhs) const
    {
        return !(*this == rhs);
    }


    // private:
    const uint16_t* m_patch_output;
    const uint16_t* m_patch_offset;
    const uint32_t* m_output_ltog_map;
    uint16_t        m_local_id;
    uint16_t        m_begin;
    uint16_t        m_end;
    uint16_t        m_current;
    int             m_shift;
    uint32_t        m_num_src_in_patch;

    __device__ void set(const uint16_t local_id, const uint32_t offset_size)
    {
        m_current  = 0;
        m_local_id = local_id;
        if (offset_size == 0) {
            m_begin = m_patch_offset[m_local_id];
            m_end   = m_patch_offset[m_local_id + 1];
        } else {
            m_begin = m_local_id * offset_size;
            m_end   = (m_local_id + 1) * offset_size;
        }
        assert(m_end > m_begin);
    }
};

template <typename HandleT, typename LocalT>
struct RXMeshIteratorV1
{
    __device__ RXMeshIteratorV1(const uint16_t  local_id,
                                const LocalT*   patch_output,
                                const uint16_t* patch_offset,
                                const uint32_t  offset_size,
                                const uint32_t  patch_id,
                                int             shift = 0)
        : m_patch_output(patch_output),
          m_patch_offset(patch_offset),
          m_patch_id(patch_id),
          m_shift(shift)
    {
        set(local_id, offset_size);
    }

    RXMeshIteratorV1(const RXMeshIteratorV1& orig) = default;


    __device__ uint16_t size() const
    {
        return m_end - m_begin;
    }

    __device__ HandleT operator[](const uint16_t i) const
    {
        assert(m_patch_output);
        assert(i + m_begin < m_end);
        return {m_patch_id, ((m_patch_output[m_begin + i].id) >> m_shift)};
    }

    __device__ HandleT operator*() const
    {
        assert(m_patch_output);
        assert(m_output_ltog_map);
        return ((*this)[m_current]);
    }

    __device__ HandleT back() const
    {
        return ((*this)[size() - 1]);
    }

    __device__ HandleT front() const
    {
        return ((*this)[0]);
    }

    __device__ RXMeshIteratorV1& operator++()
    {
        // pre
        m_current = (m_current + 1) % size();
        return *this;
    }
    __device__ const RXMeshIteratorV1 operator++(int)
    {
        // post
        RXMeshIteratorV1 pre(*this);
        m_current = (m_current + 1) % size();
        return pre;
    }

    __device__ RXMeshIteratorV1& operator--()
    {
        // pre
        m_current = (m_current == 0) ? size() - 1 : m_current - 1;
        return *this;
    }

    __device__ const RXMeshIteratorV1 operator--(int)
    {
        // post
        RXMeshIteratorV1 pre(*this);
        m_current = (m_current == 0) ? size() - 1 : m_current - 1;
        return pre;
    }

    __device__ bool operator==(const RXMeshIteratorV1& rhs) const
    {
        return rhs.m_local_id == m_local_id && rhs.m_current == m_current;
    }

    __device__ bool operator!=(const RXMeshIteratorV1& rhs) const
    {
        return !(*this == rhs);
    }


   private:
    const LocalT*   m_patch_output;
    const uint16_t* m_patch_offset;
    const uint32_t  m_patch_id;
    uint16_t        m_local_id;
    uint16_t        m_begin;
    uint16_t        m_end;
    uint16_t        m_current;
    int             m_shift;

    __device__ void set(const uint16_t local_id, const uint32_t offset_size)
    {
        m_current  = 0;
        m_local_id = local_id;
        if (offset_size == 0) {
            m_begin = m_patch_offset[m_local_id];
            m_end   = m_patch_offset[m_local_id + 1];
        } else {
            m_begin = m_local_id * offset_size;
            m_end   = (m_local_id + 1) * offset_size;
        }
        assert(m_end > m_begin);
    }
};

using RXMeshVertexIterator = RXMeshIteratorV1<VertexHandle, LocalVertexT>;
using RXMeshEdgeIterator   = RXMeshIteratorV1<EdgeHandle, LocalEdgeT>;
using RXMeshFaceIterator   = RXMeshIteratorV1<FaceHandle, LocalFaceT>;

}  // namespace rxmesh