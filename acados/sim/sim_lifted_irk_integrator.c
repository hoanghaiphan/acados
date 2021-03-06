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

#include "acados/sim/sim_lifted_irk_integrator.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "blasfeo/include/blasfeo_target.h"
#include "blasfeo/include/blasfeo_common.h"
#include "blasfeo/include/blasfeo_d_aux.h"
#include "blasfeo/include/blasfeo_d_blas.h"
#include "blasfeo/include/blasfeo_d_kernel.h"
#include "blasfeo/include/blasfeo_i_aux.h"

#include "acados/utils/print.h"

#if TRIPLE_LOOP

#if CODE_GENERATION
#define DIM 6       // num_stages*NX
#define DIM_RHS 9   // NX+NU
#endif

real_t LU_system_ACADO(real_t* const A, int* const perm, int dim, int* nswaps) {
    real_t det;
    real_t swap;
    real_t valueMax;

#if !CODE_GENERATION
    int DIM = dim;
#else
    dim += 0;
#endif

    int i, j, k;
    int indexMax;
    int intSwap;

    for (i = 0; i < DIM; ++i) {
        perm[i] = i;
    }
    det = 1.0000000000000000e+00;
    for (i=0; i < (DIM-1); i++) {
        indexMax = i;
        valueMax = fabs(A[i*DIM+i]);
        for (j=(i+1); j < DIM; j++) {
            swap = fabs(A[i*DIM+j]);
            if (swap > valueMax) {
                indexMax = j;
                valueMax = swap;
            }
        }
        if (indexMax > i) {
            nswaps[0] += 1;
            for (k = 0; k < DIM; ++k) {
                swap = A[k*DIM+i];
                A[k*DIM+i] = A[k*DIM+indexMax];
                A[k*DIM+indexMax] = swap;
            }
            intSwap = perm[i];
            perm[i] = perm[indexMax];
            perm[indexMax] = intSwap;
        }
//        det *= A[i*DIM+i];
        for (j=i+1; j < DIM; j++) {
            A[i*DIM+j] = -A[i*DIM+j]/A[i*DIM+i];
            for (k=i+1; k < DIM; k++) {
                A[k*DIM+j] += A[i*DIM+j] * A[k*DIM+i];
            }
        }
    }
//    det *= A[DIM*DIM-1];
//    det = fabs(det);
    return det;
}

real_t solve_system_ACADO(real_t* const A, real_t* const b, int* const perm, int dim, int dim2) {
    int i, j, k;
    int index1;

#if !CODE_GENERATION
    int DIM = dim;
    int DIM_RHS = dim2;
#else
    dim += 0;
    dim2 += 0;
#endif
    real_t bPerm[DIM*DIM_RHS];
    real_t tmp_var;

    for (i = 0; i < DIM; ++i) {
        index1 = perm[i];
        for (j = 0; j < DIM_RHS; ++j) {
            bPerm[j*DIM+i] = b[j*DIM+index1];
        }
    }
    for (j = 1; j < DIM; ++j) {
        for (i = 0; i < j; ++i) {
            tmp_var = A[i*DIM+j];
            for (k = 0; k < DIM_RHS; ++k) {
                bPerm[k*DIM+j] += tmp_var*bPerm[k*DIM+i];
            }
        }
    }
    for (i = DIM-1; -1 < i; --i) {
        for (j = DIM-1; i < j; --j) {
            tmp_var = A[j*DIM+i];
            for (k = 0; k < DIM_RHS; ++k) {
                bPerm[k*DIM+i] -= tmp_var*bPerm[k*DIM+j];
            }
        }
        tmp_var = 1.0/A[i*(DIM+1)];
        for (k = 0; k < DIM_RHS; ++k) {
            bPerm[k*DIM+i] = tmp_var*bPerm[k*DIM+i];
        }
    }
    for (k = 0; k < DIM*DIM_RHS; ++k) {
        b[k] = bPerm[k];
    }
    return 0;
}

real_t solve_system_trans_ACADO(real_t* const A, real_t* const b,
        int* const perm, int dim, int dim2) {
    int i, j, k;
    int index1;

#if !CODE_GENERATION
    int DIM = dim;
    int DIM_RHS = dim2;
#else
    dim += 0;
    dim2 += 0;
#endif
    real_t bPerm[DIM*DIM_RHS];
    real_t tmp_var;


    for (k = 0; k < DIM*DIM_RHS; ++k) {
        bPerm[k] = b[k];
    }
    for (i = 0; i < DIM; i++) {
        for (j = 0; j < i; j++) {
            tmp_var = A[i*DIM+j];
            for (k = 0; k < DIM_RHS; ++k) {
                bPerm[k*DIM+i] -= tmp_var*bPerm[k*DIM+j];
            }
        }
        tmp_var = 1.0/A[i*(DIM+1)];
        for (k = 0; k < DIM_RHS; ++k) {
            bPerm[k*DIM+i] = tmp_var*bPerm[k*DIM+i];
        }
    }
    for (j = DIM-1; j > -1; --j) {
        for (i = DIM-1; i > j; --i) {
            tmp_var = A[j*DIM+i];
            for (k = 0; k < DIM_RHS; ++k) {
                bPerm[k*DIM+j] += tmp_var*bPerm[k*DIM+i];
            }
        }
    }
    for (i = 0; i < DIM; ++i) {
        index1 = perm[i];
        for (j = 0; j < DIM_RHS; ++j) {
            b[j*DIM+index1] = bPerm[j*DIM+i];
        }
    }
    return 0;
}


#endif

void sim_lifted_irk(const sim_in *in, sim_out *out,
        void *mem_, void *work_ ) {
    int_t nx = in->nx;
    int_t nu = in->nu;
    sim_RK_opts *opts = in->opts;
    int_t num_stages = opts->num_stages;
    int_t i, s1, s2, j, istep;
    sim_lifted_irk_memory *mem = (sim_lifted_irk_memory*) mem_;
    sim_lifted_irk_workspace *work = (sim_lifted_irk_workspace*) work_;
#if WARM_SWAP
    int_t *ipiv_old = mem->ipiv;  // pivoting vector
#endif
#if !TRIPLE_LOOP || WARM_SWAP
    int_t iswap;
#endif
    int_t *ipiv_tmp = work->ipiv_tmp;
    real_t H_INT = in->step;
    int_t NSTEPS = in->nSteps;
    int_t NF = in->nsens_forw;

    real_t *A_mat = opts->A_mat;
    real_t *b_vec = opts->b_vec;
//    real_t *c_vec = opts->c_vec;

//    print_matrix("stdout", A_mat, num_stages, num_stages);

    real_t *VDE_tmp = work->VDE_tmp;
    real_t *out_tmp = work->out_tmp;
    real_t *rhs_in = work->rhs_in;

    real_t *K_traj = mem->K_traj;
    real_t *DK_traj = mem->DK_traj;

    int *ipiv = work->ipiv;  // pivoting vector
    real_t *sys_mat = work->sys_mat;
    real_t *sys_sol = work->sys_sol;
#if !TRIPLE_LOOP
    struct d_strmat *str_mat = work->str_mat;
#if defined(LA_HIGH_PERFORMANCE)
#if TRANSPOSED
    struct d_strmat *str_sol_t = work->str_sol_t;
#else  // NOT TRANSPOSED
    struct d_strmat *str_sol = work->str_sol;
#endif  // TRANSPOSED
#else   // LA_BLAS | LA_REFERENCE
    struct d_strmat *str_sol = work->str_sol;
#endif  // LA_BLASEO
#endif  // !TRIPLE_LOOP

    acado_timer timer, timer_la, timer_ad;
    real_t timing_la = 0.0;
    real_t timing_ad = 0.0;

    acado_tic(&timer);

    for (i = 0; i < nx; i++) out_tmp[i] = in->x[i];
    for (i = 0; i < nx*NF; i++) out_tmp[nx+i] = in->S_forw[i];  // sensitivities

    for (i = 0; i < nu; i++) rhs_in[nx*(1+NF)+i] = in->u[i];

    // Newton step of the collocation variables with respect to the inputs:
    if (NF == (nx+nu)) {
        for (istep = 0; istep < NSTEPS; istep++) {
            for (s1 = 0; s1 < num_stages; s1++) {
                for (j = 0; j < nx; j++) {  // step in X
                    for (i = 0; i < nx; i++) {
                        K_traj[(istep*num_stages+s1)*nx+i] +=
                                DK_traj[(istep*num_stages+s1)*nx*(nx+nu)+j*nx+i]*
                                (in->x[j]-mem->x[j]);  // RK step
                    }
                    mem->x[j] = in->x[j];
                }
                for (j = 0; j < nu; j++) {  // step in U
                    for (i = 0; i < nx; i++) {
                        K_traj[(istep*num_stages+s1)*nx+i] +=
                                DK_traj[(istep*num_stages+s1)*nx*(nx+nu)+(nx+j)*nx+i]*
                                (in->u[j]-mem->u[j]);  // RK step
                    }
                    mem->u[j] = in->u[j];
                }
            }
        }
    }

    for (istep = 0; istep < NSTEPS; istep++) {
        // form exact linear system matrix (explicit ODE case):
        for (i = 0; i < num_stages*nx*num_stages*nx; i++) sys_mat[i] = 0.0;
#if WARM_SWAP
        for (i = 0; i < num_stages*nx; i++) {
            iswap = ipiv_old[i];
            sys_mat[i*num_stages*nx+iswap] = 1.0;  // identity matrix
        }
#else
        for (i = 0; i < num_stages*nx; i++ ) sys_mat[i*(num_stages*nx+1)] = 1.0;  // identity matrix
#endif

        for (s1 = 0; s1 < num_stages; s1++) {
            for (i = 0; i < nx; i++) {
                rhs_in[i] = out_tmp[i];
            }
            for (i = 0; i < nu; i++) rhs_in[nx+i] = in->u[i];
            for (s2 = 0; s2 < num_stages; s2++) {
                for (i = 0; i < nx; i++) {
                    rhs_in[i] += H_INT*A_mat[s2*num_stages+s1]*K_traj[istep*num_stages*nx+s2*nx+i];
                }
            }
            acado_tic(&timer_ad);
            in->jac_fun(rhs_in, VDE_tmp);  // k evaluation
            timing_ad += acado_toc(&timer_ad);

            // put VDE_tmp in sys_mat:
            for (s2 = 0; s2 < num_stages; s2++) {
#if WARM_SWAP
                for (i = 0; i < nx; i++) {
                    iswap = ipiv_old[s1*nx+i];
                    for (j = 0; j < nx; j++) {
                        sys_mat[(s2*nx+j)*num_stages*nx+iswap] -=
                                H_INT*A_mat[s2*num_stages+s1]*VDE_tmp[nx+j*nx+i];
                    }
                }
#else
                for (j = 0; j < nx; j++) {
                    for (i = 0; i < nx; i++) {
                        sys_mat[(s2*nx+j)*num_stages*nx+s1*nx+i] -=
                                H_INT*A_mat[s2*num_stages+s1]*VDE_tmp[nx+j*nx+i];
                    }
                }
#endif
            }
        }

        acado_tic(&timer_la);
#if TRIPLE_LOOP
        LU_system_ACADO(sys_mat, ipiv, num_stages*nx, &mem->nswaps);
#else   // TRIPLE_LOOP
        // ---- BLASFEO: LU factorization ----
#if defined(LA_HIGH_PERFORMANCE)
        d_cvt_mat2strmat(num_stages*nx, num_stages*nx, sys_mat, num_stages*nx,
                str_mat, 0, 0);  // mat2strmat
#endif  // LA_BLAS | LA_REFERENCE
        dgetrf_libstr(num_stages*nx, num_stages*nx, str_mat, 0, 0, str_mat, 0,
                0, ipiv);  // Gauss elimination
        // ---- BLASFEO: LU factorization ----
#endif   // TRIPLE_LOOP
        timing_la += acado_toc(&timer_la);

        for (s1 = 0; s1 < num_stages; s1++) {
            for (i = 0; i < nx*(1+NF); i++) {
                rhs_in[i] = out_tmp[i];
            }
            for (s2 = 0; s2 < num_stages; s2++) {
                for (i = 0; i < nx; i++) {
                    rhs_in[i] += H_INT*A_mat[s2*num_stages+s1]*K_traj[istep*num_stages*nx+s2*nx+i];
                }
            }
            acado_tic(&timer_ad);
            in->VDE_forw(rhs_in, VDE_tmp);  // k evaluation
            timing_ad += acado_toc(&timer_ad);

            // put VDE_tmp in sys_sol:
#if WARM_SWAP
            for (i = 0; i < nx; i++) {
                iswap = ipiv_old[s1*nx+i];
                for (j = 0; j < 1+NF; j++) {
                    sys_sol[j*num_stages*nx+iswap] = VDE_tmp[j*nx+i];
                }
                sys_sol[iswap] -= K_traj[istep*num_stages*nx+s1*nx+i];
            }
#else
            for (j = 0; j < 1+NF; j++) {
                for (i = 0; i < nx; i++) {
                    sys_sol[j*num_stages*nx+s1*nx+i] = VDE_tmp[j*nx+i];
                }
            }
            for (i = 0; i < nx; i++) {
                sys_sol[s1*nx+i] -= K_traj[istep*num_stages*nx+s1*nx+i];
            }
#endif
        }

        acado_tic(&timer_la);
#if TRIPLE_LOOP
        solve_system_ACADO(sys_mat, sys_sol, ipiv, num_stages*nx, 1+NF);
#else  // TRIPLE_LOOP
#if defined(LA_HIGH_PERFORMANCE)
#if TRANSPOSED
        // ---- BLASFEO: row transformations + backsolve ----
        d_cvt_tran_mat2strmat(num_stages*nx, 1+NF, sys_sol, num_stages*nx,
                str_sol_t, 0, 0);  // mat2strmat
        dcolpe_libstr(num_stages*nx, ipiv, str_sol_t);  // col permutations
        dtrsm_rltu_libstr(1+NF, num_stages*nx, 1.0, str_mat, 0, 0, str_sol_t,
                0, 0, str_sol_t, 0, 0);  // L backsolve
        dtrsm_rutn_libstr(1+NF, num_stages*nx, 1.0, str_mat, 0, 0, str_sol_t,
                0, 0, str_sol_t, 0, 0);  // U backsolve
        d_cvt_tran_strmat2mat(1+NF, num_stages*nx, str_sol_t, 0, 0, sys_sol,
                num_stages*nx);  // strmat2mat
        // ---- BLASFEO: row transformations + backsolve ----
#else   // NOT TRANSPOSED
        // ---- BLASFEO: row transformations + backsolve ----
        d_cvt_mat2strmat(num_stages*nx, 1+NF, sys_sol, num_stages*nx,
                str_sol, 0, 0);  // mat2strmat
        drowpe_libstr(num_stages*nx, ipiv, str_sol);  // row permutations
        dtrsm_llnu_libstr(num_stages*nx, 1+NF, 1.0, str_mat, 0, 0, str_sol,
                0, 0, str_sol, 0, 0);  // L backsolve
        dtrsm_lunn_libstr(num_stages*nx, 1+NF, 1.0, str_mat, 0, 0, str_sol,
                0, 0, str_sol, 0, 0);  // U backsolve
        d_cvt_strmat2mat(num_stages*nx, 1+NF, str_sol, 0, 0, sys_sol,
                num_stages*nx);  // strmat2mat
        // ---- BLASFEO: row transformations + backsolve ----
#endif  // TRANSPOSED
#else   // LA_BLAS | LA_REFERENCE
        // ---- BLASFEO: row transformations + backsolve ----
        drowpe_libstr(num_stages*nx, ipiv, str_sol);  // row permutations
        dtrsm_llnu_libstr(num_stages*nx, 1+NF, 1.0, str_mat, 0, 0, str_sol,
                0, 0, str_sol, 0, 0);  // L backsolve
        dtrsm_lunn_libstr(num_stages*nx, 1+NF, 1.0, str_mat, 0, 0, str_sol,
                0, 0, str_sol, 0, 0);  // U backsolve
        // ---- BLASFEO: row transformations + backsolve ----
#endif  // LA_BLAFEO
#endif  // TRIPLE_LOOP
// #if WARM_SWAP
#if TRIPLE_LOOP
        for (i = 0; i < num_stages*nx; i++) ipiv_tmp[i] = ipiv[i];
#else
        for (i = 0; i < num_stages*nx; i++) ipiv_tmp[i] = i;
        for (i = 0; i < num_stages*nx; i++) {
            j = ipiv[i];
            if (j != i) {
                mem->nswaps += 1;
                iswap = ipiv_tmp[i];
                ipiv_tmp[i] = ipiv_tmp[j];
                ipiv_tmp[j] = iswap;
            }
        }
#endif

#if WARM_SWAP

//        fprintf(stdout, "ipiv_old: ");
//        for (i = 0; i < num_stages*nx; i++) fprintf(stdout, "%d ", ipiv_old[i]);
//        fprintf(stdout, "\n");
//        fprintf(stdout, "ipiv: ");
//        for (i = 0; i < num_stages*nx; i++) fprintf(stdout, "%d ", ipiv_tmp[i]);
//        fprintf(stdout, "\n");
        for (i = 0; i < num_stages*nx; i++) {
            ipiv_tmp[i] = ipiv_old[ipiv_tmp[i]];
        }
//        fprintf(stdout, "ipiv_tmp: ");
//        for (i = 0; i < num_stages*nx; i++) fprintf(stdout, "%d ", ipiv_tmp[i]);
//        fprintf(stdout, "\n");
        for (i = 0; i < num_stages*nx; i++) {
            ipiv_old[i] = ipiv_tmp[i];
        }
#endif
        timing_la += acado_toc(&timer_la);

        // Newton step of the collocation variables
        for (i = 0; i < num_stages*nx; i++) {
            K_traj[istep*num_stages*nx+i] += sys_sol[i];
        }
        for (s1 = 0; s1 < num_stages; s1++) {
            for (i = 0; i < nx; i++) {
                out_tmp[i] += H_INT*b_vec[s1]*K_traj[istep*num_stages*nx+s1*nx+i];  // RK step
            }
        }

        // Sensitivities collocation variables
        for (s1 = 0; s1 < num_stages; s1++) {
            for (j = 0; j < NF; j++) {
                for (i = 0; i < nx; i++) {
                    DK_traj[(istep*num_stages+s1)*nx*NF+j*nx+i] =
                            sys_sol[(j+1)*num_stages*nx+s1*nx+i];
                }
            }
        }
        for (s1 = 0; s1 < num_stages; s1++) {
            for (i = 0; i < nx*NF; i++) {
                out_tmp[nx+i] += H_INT*b_vec[s1]*
                        DK_traj[(istep*num_stages+s1)*nx*NF+i];  // RK step
            }
        }
    }
    for (i = 0; i < nx; i++)    out->xn[i] = out_tmp[i];
    for (i = 0; i < nx*NF; i++) out->S_forw[i] = out_tmp[nx+i];

    out->info->CPUtime = acado_toc(&timer);
    out->info->LAtime = timing_la;
    out->info->ADtime = timing_ad;
}


void sim_lifted_irk_create_workspace(const sim_in *in,
        sim_lifted_irk_workspace *work) {
    int_t nx = in->nx;
    int_t nu = in->nu;
    sim_RK_opts *opts = in->opts;
    int_t num_stages = opts->num_stages;
    int_t NF = in->nsens_forw;

    work->rhs_in = malloc(sizeof(*work->rhs_in) * (nx*(1+NF)+nu));
    work->VDE_tmp = malloc(sizeof(*work->VDE_tmp) * (nx*(1+NF)));
    work->out_tmp = malloc(sizeof(*work->out_tmp) * (nx*(1+NF)));
    work->ipiv = malloc(sizeof(*work->ipiv) * (num_stages*nx));
    work->sys_mat = malloc(sizeof(*work->sys_mat) * (num_stages*nx)*(num_stages*nx));
    work->sys_sol = malloc(sizeof(*work->sys_sol) * (num_stages*nx)*(1+NF));

// #if WARM_SWAP
    work->ipiv_tmp = malloc(sizeof(*work->ipiv_tmp) * (num_stages*nx));
// #endif

#if !TRIPLE_LOOP

#if defined(LA_HIGH_PERFORMANCE)
    // matrices in matrix struct format:
    int size_strmat = 0;
    size_strmat += d_size_strmat(num_stages*nx, num_stages*nx);
#if TRANSPOSED
    size_strmat += d_size_strmat(1+NF, num_stages*nx);
#else   // NOT TRANSPOSED
    size_strmat += d_size_strmat(num_stages*nx, 1+NF);
#endif  // TRANSPOSED

    // accocate memory
    void *memory_strmat;
    v_zeros_align(&memory_strmat, size_strmat);
    char *ptr_memory_strmat = (char *) memory_strmat;

    d_create_strmat(num_stages*nx, num_stages*nx, work->str_mat, ptr_memory_strmat);
    ptr_memory_strmat += work->str_mat->memory_size;
#if TRANSPOSED
    d_create_strmat(1+NF, num_stages*nx, work->str_sol_t, ptr_memory_strmat);
    ptr_memory_strmat += work->str_sol_t->memory_size;
#else   // NOT TRANSPOSED
    d_create_strmat(num_stages*nx, 1+NF, work->str_sol, ptr_memory_strmat);
    ptr_memory_strmat += work->str_sol->memory_size;
#endif  // TRANSPOSED

#elif defined(LA_REFERENCE)

    //  pointer to column-major matrix
    d_create_strmat(num_stages*nx, num_stages*nx, work->str_mat, work->sys_mat);
    d_create_strmat(num_stages*nx, 1+NF, work->str_sol, work->sys_sol);

    // allocate new memory only for the diagonal
    int size_strmat = 0;
    size_strmat += d_size_diag_strmat(num_stages*nx, num_stages*nx);

    void *memory_strmat = malloc(size_strmat);
//    void *memory_strmat;
//    v_zeros_align(&memory_strmat, size_strmat);
    char *ptr_memory_strmat = (char *) memory_strmat;

    d_cast_diag_mat2strmat((double *) ptr_memory_strmat, work->str_mat);
//    ptr_memory_strmat += d_size_diag_strmat(num_stages*nx, num_stages*nx);

#else  // LA_BLAS

    // not allocate new memory: point to column-major matrix
    d_create_strmat(num_stages*nx, num_stages*nx, work->str_mat, work->sys_mat);
    d_create_strmat(num_stages*nx, 1+NF, work->str_sol, work->sys_sol);

#endif  // LA_HIGH_PERFORMANCE

#endif  // !TRIPLE_LOOP
}


void sim_lifted_irk_create_memory(const sim_in *in,
        sim_lifted_irk_memory *mem) {
    int_t i;
    int_t nx = in->nx;
    int_t nu = in->nu;
    int_t nSteps = in->nSteps;
    sim_RK_opts *opts = in->opts;
    int_t num_stages = opts->num_stages;
    int_t NF = in->nsens_forw;

    mem->K_traj = malloc(sizeof(*mem->K_traj) * (nSteps*num_stages*nx));
    mem->DK_traj = malloc(sizeof(*mem->DK_traj) * (nSteps*num_stages*nx*NF));
    mem->x = calloc(nx, sizeof(*mem->x));
    mem->u = calloc(nu, sizeof(*mem->u));

#if WARM_SWAP
    mem->ipiv = malloc(sizeof(*mem->ipiv) * (num_stages*nx));
    for ( i = 0; i < num_stages*nx; i++ ) mem->ipiv[i] = i;
#endif

    for ( i = 0; i < nSteps*num_stages*nx; i++ ) mem->K_traj[i] = 0.0;
    for ( i = 0; i < nSteps*num_stages*nx*NF; i++ ) mem->DK_traj[i] = 0.0;
}


void sim_irk_create_opts(const int_t num_stages, const char* name, sim_RK_opts *opts) {
    opts->num_stages = num_stages;
    opts->A_mat = malloc(sizeof(*opts->A_mat) * (num_stages*num_stages));
    opts->b_vec = malloc(sizeof(*opts->b_vec) * (num_stages));
    opts->c_vec = malloc(sizeof(*opts->c_vec) * (num_stages));

    if ( strcmp(name, "Gauss") == 0 ) {  // GAUSS METHODS
        get_Gauss_nodes(opts->num_stages, opts->c_vec);
        create_Butcher_table(opts->num_stages, opts->c_vec, opts->b_vec, opts->A_mat);
    } else if ( strcmp(name, "Radau") == 0 ) {
        // TODO(rien): add Radau IIA collocation schemes
//        get_Radau_nodes(opts->num_stages, opts->c_vec);
        create_Butcher_table(opts->num_stages, opts->c_vec, opts->b_vec, opts->A_mat);
    } else {
        // throw error somehow?
    }
//    print_matrix("stdout", opts->c_vec, num_stages, 1);
//    print_matrix("stdout", opts->A_mat, num_stages, num_stages);
//    print_matrix("stdout", opts->b_vec, num_stages, 1);
}
