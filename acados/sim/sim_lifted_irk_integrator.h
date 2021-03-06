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

#ifndef ACADOS_SIM_SIM_LIFTED_IRK_INTEGRATOR_H_
#define ACADOS_SIM_SIM_LIFTED_IRK_INTEGRATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "acados/sim/sim_collocation.h"
#include "acados/sim/sim_rk_common.h"
#include "acados/utils/types.h"

#define TRANSPOSED 1
#define TRIPLE_LOOP 1
#define CODE_GENERATION 0
#define WARM_SWAP 0

typedef struct sim_lifted_irk_workspace_ {
    real_t *rhs_in;
    real_t *VDE_tmp;
    real_t *out_tmp;
    int_t *ipiv;
    int_t *ipiv_tmp;
    real_t *sys_mat;
    real_t *sys_sol;
    struct d_strmat *str_mat;
    struct d_strmat *str_sol;
    struct d_strmat *str_sol_t;
} sim_lifted_irk_workspace;

typedef struct sim_lifted_irk_memory_ {
    real_t *K_traj;
    real_t *DK_traj;
    real_t *x;
    real_t *u;
    int_t nswaps;
    int *ipiv;
} sim_lifted_irk_memory;


void sim_lifted_irk(const sim_in *in, sim_out *out,
        void *mem, void *work);

void sim_lifted_irk_create_workspace(const sim_in *in,
        sim_lifted_irk_workspace *work);

void sim_lifted_irk_create_memory(const sim_in *in,
        sim_lifted_irk_memory *mem);

void sim_irk_create_opts(int_t num_stages, const char* name, sim_RK_opts *opts);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // ACADOS_SIM_SIM_LIFTED_IRK_INTEGRATOR_H_
