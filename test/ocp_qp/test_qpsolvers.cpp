/*
 *    This file is part of acados.
 *
 *    acados is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License as published by the Free Software Foundation; either
 *    version 3 of the License, or (at your option) any later version.
 *
 *    acados is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with acados; if not, write to the Free Software Foundation,
 *    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <iostream>
#include <string>
#include <vector>

#include "blasfeo/include/blasfeo_d_aux.h"
#include "catch/include/catch.hpp"
#include "OOQP/include/cQpGenSparse.h"

#include "acados/ocp_qp/ocp_qp_condensing_qpoases.h"
#include "acados/ocp_qp/ocp_qp_hpmpc.h"
#include "acados/ocp_qp/ocp_qp_ooqp.h"
#include "acados/ocp_qp/ocp_qp_qpdunes.h"
#include "acados/utils/allocate_ocp_qp.h"
#include "test/ocp_qp/condensing_test_helper.h"
#include "test/test_utils/read_matrix.h"
#include "test/test_utils/read_ocp_qp_in.h"

using std::vector;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using Eigen::Map;

int_t TEST_OOQP = 1;
real_t TOL_OOQP = 1e-6;
int_t TEST_QPOASES = 1;
real_t TOL_QPOASES = 1e-10;
int_t TEST_QPDUNES = 1;
real_t TOL_QPDUNES = 1e-10;
int_t TEST_HPMPC = 0;
real_t TOL_HPMPC = 1e-10;

static vector<std::string> scenarios = {"LTI", "LTV"};
// TODO(dimitris): add back "ONLY_AFFINE" after fixing problem
vector<std::string> constraints = {"UNCONSTRAINED", "ONLY_BOUNDS", "CONSTRAINED"};

// TODO(dimitris): Clean up octave code
TEST_CASE("Solve random OCP_QP", "[QP solvers]") {
    ocp_qp_in qp_in;
    ocp_qp_out qp_out;

    int_t SET_BOUNDS = 0;
    int_t SET_INEQUALITIES = 0;
    int_t SET_x0 = 1;
    int_t QUIET = 1;

    int return_value;
    VectorXd acados_W, true_W;

    for (std::string constraint : constraints) {
        SECTION(constraint) {
            if (constraint == "CONSTRAINED" || constraint == "ONLY_BOUNDS") SET_BOUNDS = 1;
            if (constraint == "CONSTRAINED" || constraint == "ONLY_AFFINE") SET_INEQUALITIES = 1;

            for (std::string scenario : scenarios) {
                SECTION(scenario) {
                    read_ocp_qp_in(&qp_in, (char*) scenario.c_str(),
                    SET_BOUNDS, SET_INEQUALITIES, SET_x0, QUIET);
                    allocate_ocp_qp_out(&qp_in, &qp_out);

                    // TODO(dimitris): extend to variable dimensions
                    int_t N = qp_in.N;
                    int_t nx = qp_in.nx[0];
                    int_t nu = qp_in.nu[0];

                    // load optimal solution from quadprog
                    if (constraint == "UNCONSTRAINED") {
                        true_W = readMatrixFromFile(scenario +
                            "/w_star_ocp_unconstrained.dat", (N+1)*nx + N*nu, 1);
                    } else if (constraint == "ONLY_BOUNDS") {
                        true_W = readMatrixFromFile(scenario +
                            "/w_star_ocp_bounds.dat", (N+1)*nx + N*nu, 1);
                    } else if (constraint == "ONLY_AFFINE") {
                        true_W = readMatrixFromFile(scenario +
                            "/w_star_ocp_no_bounds.dat", (N+1)*nx + N*nu, 1);
                    } else if (constraint == "CONSTRAINED") {
                        true_W = readMatrixFromFile(scenario +
                            "/w_star_ocp_constrained.dat", (N+1)*nx + N*nu, 1);
                    }
                    if (TEST_QPOASES) {
                        SECTION("qpOASES") {
                            std::cout <<"---> TESTING qpOASES with QP: "<< scenario <<
                                ", " << constraint << std::endl;

                            ocp_qp_condensing_qpoases_args args;
                            args.dummy = 42.0;

                            // TODO(dimitris): also test that qp_in has not changed
                            return_value = \
                                ocp_qp_condensing_qpoases(&qp_in, &qp_out, &args, NULL, NULL);
                            acados_W = Eigen::Map<VectorXd>(qp_out.x[0], (N+1)*nx + N*nu);
                            REQUIRE(return_value == 0);
                            REQUIRE(acados_W.isApprox(true_W, TOL_QPOASES));
                            std::cout <<"---> PASSED " << std::endl;
                        }
                    }
                    if (TEST_QPDUNES) {
                        SECTION("qpDUNES") {
                            std::cout <<"---> TESTING qpDUNES with QP: "<< scenario <<
                                ", " << constraint << std::endl;

                            ocp_qp_qpdunes_args args;
                            ocp_qp_qpdunes_memory mem;
                            void *work;

                            ocp_qp_qpdunes_create_arguments(&args, QPDUNES_DEFAULT_ARGUMENTS);
                            args.options.printLevel = 0;

                            int_t work_space_size =
                                ocp_qp_qpdunes_calculate_workspace_size(&qp_in, &args);
                            work = (void*)malloc(work_space_size);

                            int_t mem_return = ocp_qp_qpdunes_create_memory(&qp_in, &args, &mem);
                            REQUIRE(mem_return == 0);

                            return_value = ocp_qp_qpdunes(&qp_in, &qp_out, &args, &mem, work);
                            acados_W = Eigen::Map<VectorXd>(qp_out.x[0], (N+1)*nx + N*nu);
                            free(work);
                            ocp_qp_qpdunes_free_memory(&mem);
                            REQUIRE(return_value == 0);
                            REQUIRE(acados_W.isApprox(true_W, TOL_OOQP));
                            std::cout <<"---> PASSED " << std::endl;
                        }
                    }
                    if (TEST_OOQP) {
                        SECTION("OOQP") {
                            std::cout <<"---> TESTING OOQP with QP: "<< scenario <<
                                ", " << constraint << std::endl;

                            ocp_qp_ooqp_args args;
                            ocp_qp_ooqp_memory mem;
                            void *work;

                            args.printLevel = 0;
                            args.workspaceMode = 2;  // TODO(dimitris): make this default

                            int_t work_space_size =
                                ocp_qp_ooqp_calculate_workspace_size(&qp_in, &args);
                            work = (void*)malloc(work_space_size);

                            int_t mem_return = ocp_qp_ooqp_create_memory(&qp_in, &args, &mem);
                            REQUIRE(mem_return == 0);

                            return_value = ocp_qp_ooqp(&qp_in, &qp_out, &args, &mem, work);
                            acados_W = Eigen::Map<VectorXd>(qp_out.x[0], (N+1)*nx + N*nu);
                            free(work);
                            ocp_qp_ooqp_free_memory(&mem);
                            REQUIRE(return_value == 0);
                            REQUIRE(acados_W.isApprox(true_W, TOL_OOQP));
                            std::cout <<"---> PASSED " << std::endl;
                        }
                    }
                    if (TEST_HPMPC) {
                        SECTION("HPMPC") {
                            std::cout <<"---> TESTING HPMPC with QP: "<< scenario <<
                                ", " << constraint << std::endl;

                            ocp_qp_hpmpc_args args;
                            args.tol = 1e-12;
                            args.max_iter = 20;
                            args.mu0 = 0.0;
                            args.warm_start = 0;
                            args.N2 = N;
                            double inf_norm_res[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
                            args.inf_norm_res = &inf_norm_res[0];
                            int work_space_size =
                            ocp_qp_hpmpc_workspace_size_bytes(N, (int_t *)qp_in.nx,
                            (int_t *)qp_in.nu, (int_t *)qp_in.nb, (int_t *)qp_in.nc,
                            (int_t **)qp_in.idxb, &args);
                            // printf("\nwork space size: %d bytes\n", work_space_size);
                            void *work = malloc(work_space_size);

                            return_value = ocp_qp_hpmpc(&qp_in, &qp_out, &args, work);
                            acados_W = Eigen::Map<VectorXd>(qp_out.x[0], (N+1)*nx + N*nu);
                            free(work);
                            REQUIRE(return_value == 0);
                            REQUIRE(acados_W.isApprox(true_W, TOL_HPMPC));
                            std::cout <<"---> PASSED " << std::endl;
                        }
                    }
                    // std::cout << "ACADOS output:\n" << acados_W << std::endl;
                    // printf("-------------------\n");
                    // std::cout << "OCTAVE output:\n" << true_W << std::endl;
                    // printf("-------------------\n");
                    // printf("return value = %d\n", return_value);
                    // printf("-------------------\n");
                    free_ocp_qp_in(&qp_in);
                    free_ocp_qp_out(&qp_out);
                }  // END_SECTION_SCENARIOS
            }  // END_FOR_SCENARIOS
        }  // END_SECTION_CONSTRAINTS
    }  // END_FOR_CONSTRAINTS
}  // END_TEST_CASE
