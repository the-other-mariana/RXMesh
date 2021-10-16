#pragma once

#include <fstream>
#include <memory>
#include <unordered_map>
#include <vector>
#include "rxmesh/patch_info.h"
#include "rxmesh/patcher/patcher.h"
#include "rxmesh/rxmesh_context.h"
#include "rxmesh/rxmesh_types.h"
#include "rxmesh/util/log.h"
#include "rxmesh/util/macros.h"
#include "rxmesh/util/util.h"

class RXMeshTest;

namespace rxmesh {

class RXMesh
{
   public:
    /**
     * @brief Export the mesh to obj file
     *
     * @tparam VertT Lambda function type [inferred]
     * @param filename the output file
     * @param getCoords lambda function that takes two uint32_t/int parameters
     * and return a real number. The first parameter is the vertex id. The
     * second parameter is dimension (0,1, or 2)
     */
    template <typename VertT>
    void export_obj(const std::string& filename, VertT getCoords)
    {
        std::string  fn = STRINGIFY(OUTPUT_DIR) + filename;
        std::fstream file(fn, std::ios::out);
        file.precision(30);

        // write vertices
        for (uint32_t v = 0; v < m_num_vertices; ++v) {
            uint32_t v_id = v;

            file << "v  ";
            for (uint32_t i = 0; i < 3; ++i) {
                file << getCoords(v_id, i) << "  ";
            }
            file << std::endl;
        }
        // write connectivity
        write_connectivity(file);
        file.close();
    }


    uint32_t get_num_vertices() const
    {
        return m_num_vertices;
    }
    uint32_t get_num_edges() const
    {
        return m_num_edges;
    }
    uint32_t get_num_faces() const
    {
        return m_num_faces;
    }

    uint32_t get_max_valence() const
    {
        return m_max_valence;
    }

    uint32_t get_max_edge_incident_faces() const
    {
        return m_max_edge_incident_faces;
    }

    uint32_t get_max_edge_adjacent_faces() const
    {
        return m_max_face_adjacent_faces;
    }

    const RXMeshContext& get_context() const
    {
        return m_rxmesh_context;
    }

    bool is_edge_manifold() const
    {
        return m_is_input_edge_manifold;
    }

    bool is_closed() const
    {
        return m_is_input_closed;
    }

    uint32_t get_patch_size() const
    {
        return m_patch_size;
    }

    uint32_t get_num_patches() const
    {
        return m_num_patches;
    }

    uint32_t get_num_components() const
    {
        return m_patcher->get_num_components();
    }

    void get_max_min_avg_patch_size(uint32_t& min_p,
                                    uint32_t& max_p,
                                    uint32_t& avg_p) const
    {
        return m_patcher->get_max_min_avg_patch_size(min_p, max_p, avg_p);
    }

    double get_ribbon_overhead() const
    {
        return m_patcher->get_ribbon_overhead();
    }

    uint32_t get_per_patch_max_vertices() const
    {
        return m_max_vertices_per_patch;
    }

    uint32_t get_per_patch_max_edges() const
    {
        return m_max_edges_per_patch;
    }

    uint32_t get_per_patch_max_faces() const
    {
        return m_max_faces_per_patch;
    }

    float get_patching_time() const
    {
        return m_patcher->get_patching_time();
    }

    uint32_t get_num_lloyd_run() const
    {
        return m_patcher->get_num_lloyd_run();
    }

    uint32_t get_edge_id(const uint32_t v0, const uint32_t v1) const;

    const std::unique_ptr<patcher::Patcher>& get_patcher() const
    {
        return m_patcher;
    };

   protected:
    virtual ~RXMesh();

    RXMesh(const RXMesh&) = delete;

    RXMesh(const std::vector<std::vector<uint32_t>>& fv,
           const bool                                quite = true);

    /**
     * @brief build different supporting data structure used to build RXMesh
     *
     * Set the number of vertices, edges, and faces, populate edge_map (which
     * takes two connected vertices and returns their edge id), build
     * face-incident-faces data structure (used to in creating patches). This is
     * done using a single pass over FV
     *
     * @param fv input face incident vertices
     * @param ef output edge incident faces
     * @param ef output face adjacent faces
     */
    void build_supporting_structures(
        const std::vector<std::vector<uint32_t>>& fv,
        std::vector<std::vector<uint32_t>>&       ef,
        std::vector<uint32_t>&                    ff_offset,
        std::vector<uint32_t>&                    ff_values);

    /**
     * @brief Calculate various statistics for the input mesh
     *
     * Calculate max valence, max edge incident faces, max face adjacent faces,
     * if the input is closed, if the input is edge manifold, and max number of
     * vertices/edges/faces per patch
     *
     * @param fv input face incident vertices
     * @param ef input edge incident faces
     */
    void calc_statistics(const std::vector<std::vector<uint32_t>>& fv,
                         const std::vector<std::vector<uint32_t>>& ef);

    void build(const std::vector<std::vector<uint32_t>>& fv);
    void build_single_patch(const std::vector<std::vector<uint32_t>>& fv,
                            const uint32_t                            patch_id);

    void build_single_patch_ltog(const std::vector<std::vector<uint32_t>>& fv,
                                 const uint32_t patch_id);

    void build_single_patch_topology(
        const std::vector<std::vector<uint32_t>>& fv,
        const uint32_t                            patch_id);


    void move_to_device();

    void build_device();

    uint32_t get_edge_id(const std::pair<uint32_t, uint32_t>& edge) const;

    virtual void write_connectivity(std::fstream& file) const;

    // our friend tester class
    friend class ::RXMeshTest;

    RXMeshContext m_rxmesh_context;

    uint32_t m_num_edges, m_num_faces, m_num_vertices, m_max_valence,
        m_max_edge_incident_faces, m_max_face_adjacent_faces;

    // patches
    uint32_t       m_num_patches;
    const uint32_t m_patch_size;
    bool           m_is_input_edge_manifold;
    bool           m_is_input_closed;
    bool           m_quite;

    std::unordered_map<std::pair<uint32_t, uint32_t>,
                       uint32_t,
                       detail::edge_key_hash>
        m_edges_map;

    // pointer to the patcher class responsible for everything related to
    // patching the mesh into small pieces
    std::unique_ptr<patcher::Patcher> m_patcher;


    // Patch sub-matrices

    // Host
    uint32_t m_max_vertices_per_patch, m_max_edges_per_patch,
        m_max_faces_per_patch;

    //** main incident relations
    std::vector<std::vector<uint16_t>> m_h_patches_edges;
    std::vector<std::vector<uint16_t>> m_h_patches_faces;
    //.x edge address
    //.y edge size
    //.z face address
    //.w face size
    std::vector<uint4> m_h_ad_size;

    // the number of owned mesh elements per patch
    std::vector<uint16_t> m_h_num_owned_f, m_h_num_owned_e, m_h_num_owned_v;

    //** mappings
    // local to global map for (v)ertices (e)dges and (f)aces
    std::vector<std::vector<uint32_t>> m_h_patches_ltog_v;
    std::vector<std::vector<uint32_t>> m_h_patches_ltog_e;
    std::vector<std::vector<uint32_t>> m_h_patches_ltog_f;

    // storing the start id(x) and element count(y)
    std::vector<uint2> m_h_ad_size_ltog_v, m_h_ad_size_ltog_e,
        m_h_ad_size_ltog_f;


    // Device
    // Each device pointer points to a long array that holds specific data
    // separated by patch id
    //       ____________ _____________ ____________
    //      |____________|_____________|____________|
    //           ^^            ^^            ^^
    //      patch 1 data  patch 2 data   patch 3 data

    // We store the starting id and the size of mesh elements for each patch
    // in m_d_ad_size_ltog_MESHELE (ad for address) where MESHELE could be
    // v,e, or f. This is for the mapping pointers
    // For incidence pointers, we only need store the starting id

    // mapping
    uint32_t *m_d_patches_ltog_v, *m_d_patches_ltog_e, *m_d_patches_ltog_f;
    uint2 *   m_d_ad_size_ltog_v, *m_d_ad_size_ltog_e, *m_d_ad_size_ltog_f;

    // incidence
    uint16_t *m_d_patches_edges, *m_d_patches_faces;


    //.x edge address
    //.y edge size
    //.z face address
    //.w face size
    uint4* m_d_ad_size;

    // the number of owned mesh elements per patch
    uint16_t *m_d_num_owned_f, *m_d_num_owned_e, *m_d_num_owned_v;


    PatchInfo* m_patches_info;
};
}  // namespace rxmesh
