#pragma once

#include "rxmesh/kernels/rxmesh_query_dispatcher.cuh"
#include "rxmesh/rxmesh_attribute.h"
#include "rxmesh/rxmesh_context.h"
#include "rxmesh/util/math.h"
#include "rxmesh/util/vector.h"
/**
 * vertex_normal()
 */
template <typename T, uint32_t blockThreads>
__launch_bounds__(blockThreads, 6) __global__
    static void compute_vertex_normal(const RXMESH::RXMeshContext context,
                                      RXMESH::RXMeshAttribute<T>  coords,
                                      RXMESH::RXMeshAttribute<T>  normals)
{
    using namespace RXMESH;
    auto vn_lambda = [&](uint32_t face_id, RXMeshIterator& fv) {
        // get the face's three vertices coordinates
        Vector<3, T> c0(coords(fv[0], 0), coords(fv[0], 1), coords(fv[0], 2));
        Vector<3, T> c1(coords(fv[1], 0), coords(fv[1], 1), coords(fv[1], 2));
        Vector<3, T> c2(coords(fv[2], 0), coords(fv[2], 1), coords(fv[2], 2));

        // compute the face normal
        Vector<3, T> n = cross(c1 - c0, c2 - c0);

        // the three edges length
        Vector<3, T> l(dist2(c0, c1), dist2(c1, c2), dist2(c2, c0));

        // add the face's normal to its vertices
        for (uint32_t v = 0; v < 3; ++v) {      // for every vertex in this face
            for (uint32_t i = 0; i < 3; ++i) {  // for the vertex 3 coordinates
                atomicAdd(&normals(fv[v], i), n[i] / (l[v] + l[(v + 2) % 3]));
            }
        }
    };

    query_block_dispatcher<Op::FV, blockThreads>(context, vn_lambda);
}