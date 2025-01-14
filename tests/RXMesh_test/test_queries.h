#include <vector>
#include "gtest/gtest.h"
#include "query.cuh"
#include "rxmesh/rxmesh_attribute.h"
#include "rxmesh/rxmesh_static.h"
#include "rxmesh/rxmesh_util.h"
#include "rxmesh/util/import_obj.h"
#include "rxmesh/util/math.h"
#include "rxmesh/util/report.h"
#include "rxmesh_test.h"
using namespace RXMESH;

/**
 * launcher()
 */
template <uint32_t blockThreads>
float launcher(const RXMeshContext&       context,
               const Op                   op,
               RXMeshAttribute<uint32_t>& input_container,
               RXMeshAttribute<uint32_t>& output_container,
               LaunchBox<blockThreads>&   launch_box,
               const bool                 oriented = false)
{
    CUDA_ERROR(cudaProfilerStart());
    GPUTimer timer;
    timer.start();

    switch (op) {
        case Op::VV:
            query<Op::VV, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container, oriented);
            break;
        case Op::VE:
            query<Op::VE, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::VF:
            query<Op::VF, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::EV:
            query<Op::EV, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::EE:
            RXMESH_ERROR(
                "RXMeshStatic::launcher_no_src() Op::EE is not "
                "supported!!");
            break;
        case Op::EF:
            query<Op::EF, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::FV:
            query<Op::FV, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::FE:
            query<Op::FE, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
        case Op::FF:
            query<Op::FF, blockThreads>
                <<<launch_box.blocks, blockThreads,
                   launch_box.smem_bytes_dyn>>>(context, input_container,
                                                output_container);
            break;
    }

    timer.stop();
    CUDA_ERROR(cudaDeviceSynchronize());
    CUDA_ERROR(cudaGetLastError());
    CUDA_ERROR(cudaProfilerStop());
    return timer.elapsed_millis();
}

/**
 * calc_fixed_offset()
 */
template <uint32_t patchSize>
inline uint32_t max_output_per_element(const RXMeshStatic<patchSize>& rxmesh,
                                       const Op&                      op)
{
    if (op == Op::EV) {
        return 2;
    } else if (op == Op::EF) {
        return rxmesh.get_max_edge_incident_faces();
    } else if (op == Op::FV || op == Op::FE) {
        return rxmesh.get_face_degree();
    } else if (op == Op::FF) {
        return rxmesh.get_max_edge_adjacent_faces();
    } else if (op == Op::VV || op == Op::VE || op == Op::VF) {
        return rxmesh.get_max_valence();
    } else {
        RXMESH_ERROR("calc_fixed_offset() Invalid op " + op_to_string(op));
        return -1u;
    }    
}

TEST(RXMesh, Oriented_VV)
{

    // Select device
    cuda_query(rxmesh_args.device_id, rxmesh_args.quite);

    std::vector<std::vector<uint32_t>> Faces;

    ASSERT_TRUE(import_obj(STRINGIFY(INPUT_DIR) "cube.obj", Verts, Faces, true));

    // Instantiate RXMesh Static
    RXMeshStatic<PATCH_SIZE> rxmesh_static(Faces, Verts, false, rxmesh_args.quite);

    EXPECT_TRUE(rxmesh_static.is_closed())
        << " Can't generate oriented VV for input with boundaries";

    // input/output container
    RXMeshAttribute<uint32_t> input_container;
    input_container.init(rxmesh_static.get_num_vertices(), 1u, RXMESH::DEVICE,
                         RXMESH::AoS, false, false);

    RXMeshAttribute<uint32_t> output_container;
    output_container.init(rxmesh_static.get_num_vertices(),
                          max_output_per_element(rxmesh_static, Op::VV) + 1,
                          RXMESH::DEVICE, RXMESH::SoA, false, false);

    // launch box
    LaunchBox<256> launch_box;
    rxmesh_static.prepare_launch_box(Op::VV, launch_box, false, true);

    // launch query
    float tt = launcher(rxmesh_static.get_context(), Op::VV, input_container,
                        output_container, launch_box, true);


    // move containers to the CPU for testing
    output_container.move(RXMESH::DEVICE, RXMESH::HOST);
    input_container.move(RXMESH::DEVICE, RXMESH::HOST);

    RXMeshTest tester(true);
    EXPECT_TRUE(tester.run_query_verifier(rxmesh_static, Op::VV,
                                          input_container, output_container));

    // Make sure orientation is accurate
    // for the cube, all angle are either 45 or 90

    for (uint32_t v = 0; v < rxmesh_static.get_num_vertices(); ++v) {

        uint32_t vertex = input_container(v);

        uint32_t v_0 = output_container(v, output_container(v, 0));
        for (uint32_t i = 1; i < output_container(v, 0); ++i) {

            uint32_t v_1 = output_container(v, i);

            std::vector<dataT> p1{Verts[vertex][0] - Verts[v_0][0],
                                  Verts[vertex][1] - Verts[v_0][1],
                                  Verts[vertex][2] - Verts[v_0][2]};

            std::vector<dataT> p2{Verts[vertex][0] - Verts[v_1][0],
                                  Verts[vertex][1] - Verts[v_1][1],
                                  Verts[vertex][2] - Verts[v_1][2]};
            dataT              dot_pro = dot(p1, p2);
            dataT              theta =
                std::acos(dot_pro / (vector_length(p1[0], p1[1], p1[2]) *
                                     vector_length(p2[0], p2[1], p2[2])));
            theta = (theta * 180) / 3.14159265;
            EXPECT_TRUE(std::abs(theta - 90) < 0.0001 ||
                        std::abs(theta - 45) < 0.0001);
            v_0 = v_1;
        }
    }


    input_container.release();
    output_container.release();
}


TEST(RXMesh, Queries)
{
    if (rxmesh_args.shuffle) {
        ASSERT_FALSE(rxmesh_args.sort)
            << " cannot shuffle and sort at the same time!";
    }
    if (rxmesh_args.sort) {
        ASSERT_FALSE(rxmesh_args.shuffle)
            << " cannot shuffle and sort at the same time!";
    }


    bool oriented = false;

    // Select device
    cuda_query(rxmesh_args.device_id, rxmesh_args.quite);

    std::vector<std::vector<uint32_t>> Faces;

    ASSERT_TRUE(import_obj(rxmesh_args.obj_file_name, Verts, Faces,
                    rxmesh_args.quite));

    if (rxmesh_args.shuffle) {
        shuffle_obj(Faces, Verts);
    }

    // RXMesh
    RXMeshStatic<PATCH_SIZE> rxmesh_static(Faces, Verts, rxmesh_args.sort,
                               rxmesh_args.quite);


    // Report
    Report report;
    report = Report("QueryTest_RXMesh");
    report.command_line(rxmesh_args.argc, rxmesh_args.argv);
    report.device();
    report.system();
    report.model_data(rxmesh_args.obj_file_name, rxmesh_static);
    report.add_member("method", std::string("RXMesh"));

    std::string order = "default";
    if (rxmesh_args.shuffle) {
        order = "shuffle";
    } else if (rxmesh_args.sort) {
        order = "sorted";
    }
    report.add_member("input_order", order);

    // Tester to verify all queries
    ::RXMeshTest tester(true);
    EXPECT_TRUE(tester.run_ltog_mapping_test(rxmesh_static))
        << "Local-to-global mapping test failed";

    // adding query that we want to test
    std::vector<Op> ops = {Op::VV, Op::VE, Op::VF,  //
                           Op::FV, Op::FE, Op::FF,  //
                           Op::EV, Op::EF};


    for (auto& ops_it : ops) {

        // Input and output element type
        ELEMENT source_ele(ELEMENT::VERTEX), output_ele(ELEMENT::VERTEX);
        io_elements(ops_it, source_ele, output_ele);

        // Input size
        uint32_t input_size =
            (source_ele == ELEMENT::VERTEX) ?
                rxmesh_static.get_num_vertices() :
                ((source_ele == ELEMENT::EDGE) ? rxmesh_static.get_num_edges() :
                                                 rxmesh_static.get_num_faces());

        // input/output container
        RXMeshAttribute<uint32_t> input_container;
        input_container.init(input_size, 1u, RXMESH::DEVICE, RXMESH::AoS, false,
                             false);

        // allocate output container
        // for each mesh element, we reserve the maximum possible output based
        // on the operation (ops_it). The +1 is used to store the size of the
        // output for operations that output variable outputs per elements
        // (e.g., VV)
        RXMeshAttribute<uint32_t> output_container;
        output_container.init(input_size,
                              max_output_per_element(rxmesh_static, ops_it) + 1,
                              RXMESH::DEVICE, RXMESH::SoA, false, false);

        // launch box
        LaunchBox<256> launch_box;
        rxmesh_static.prepare_launch_box(ops_it, launch_box, false, oriented);

        // test data
        TestData td;
        td.test_name = op_to_string(ops_it);
        td.num_threads = launch_box.num_threads;
        td.num_blocks = launch_box.blocks;
        td.dyn_smem = launch_box.smem_bytes_dyn;
        td.static_smem = launch_box.smem_bytes_static;


        float total_time = 0;
        for (uint32_t itr = 0; itr < rxmesh_args.num_run; itr++) {

            output_container.reset(INVALID32, RXMESH::DEVICE);
            input_container.reset(INVALID32, RXMESH::DEVICE);

            // launch query
            float tt =
                launcher(rxmesh_static.get_context(), ops_it, input_container,
                         output_container, launch_box, oriented);
            total_time += tt;
            td.time_ms.push_back(tt);
        }

        // move containers to the CPU for testing
        output_container.move(RXMESH::DEVICE, RXMESH::HOST);
        input_container.move(RXMESH::DEVICE, RXMESH::HOST);


        // verify
        bool passed = tester.run_query_verifier(
            rxmesh_static, ops_it, input_container, output_container);

        td.passed.push_back(passed);
        EXPECT_TRUE(passed) << "Testing: " << td.test_name;

        report.add_test(td);
        if (!rxmesh_args.quite) {
            RXMESH_TRACE(" {} {} time = {} (ms)", td.test_name.c_str(),
                         (passed ? " passed " : " failed "),
                         total_time / float(rxmesh_args.num_run));
        }

        input_container.release();
        output_container.release();
    }


    report.write(
        rxmesh_args.output_folder + "/rxmesh/" + order,
        "QueryTest_RXMesh_" + extract_file_name(rxmesh_args.obj_file_name));
}
