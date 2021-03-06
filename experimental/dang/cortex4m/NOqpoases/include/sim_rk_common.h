/*    /home/dang/acados/acados/sim/sim_rk_common.h
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

#ifndef ACADOS_SIM_SIM_RK_COMMON_H_
#define ACADOS_SIM_SIM_RK_COMMON_H_

#include "sim_common.h"
#include "types.h"

typedef struct sim_RK_opts_ {
    int_t num_stages;
    real_t *A_mat;
    real_t *c_vec;
    real_t *b_vec;
} sim_RK_opts;

#endif  // ACADOS_SIM_SIM_RK_COMMON_H_
