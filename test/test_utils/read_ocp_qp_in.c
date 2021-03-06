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

#include "test/test_utils/read_ocp_qp_in.h"

#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "blasfeo/include/blasfeo_target.h"
#include "blasfeo/include/blasfeo_common.h"
#include "blasfeo/include/blasfeo_d_aux.h"
#include "blasfeo/include/blasfeo_i_aux.h"

#include "acados/utils/allocate_ocp_qp.h"

static void transpose_matrix(real_t *mat, int m, int n) {
    int_t ii, jj;
    real_t *tmp;
    d_zeros(&tmp, m, n);

    for (jj = 0; jj < n; jj++) {
        for (ii = 0; ii < m; ii++) {
             tmp[ii*n+jj] = mat[jj*m + ii];
        }
    }
    for (ii = 0; ii < m*n; ii++) mat[ii] = tmp[ii];
    d_free(tmp);
}


int_t read_int_vector_from_txt(int_t *vec, int_t n, const char *fname) {
    int_t status, ii;
    FILE *myFile;
    myFile = fopen(fname, "r");

    if (myFile == NULL) {
        printf("Error Reading File %s ! ! ! ! ! ! ! ! !\n", fname);
        return -1;
    }

    for (ii = 0; ii < n; ii++) {
        status = fscanf(myFile, "%d,", &vec[ii]);
        assert(status >= 0);
    }

    fclose(myFile);

    return 0;
}


int_t read_double_vector_from_txt(real_t *vec, int_t n, const char *fname) {
    int_t status, ii;
    FILE *myFile;
    myFile = fopen(fname, "r");

    if (myFile == NULL) {
        printf("Error Reading File %s ! ! ! ! ! ! ! ! !\n", fname);
        return -1;
    }

    for (ii = 0; ii < n; ii++) {
         status = fscanf(myFile, "%lf,", &vec[ii]);
         assert(status >= 0);
    }

    fclose(myFile);

    return 0;
}


int_t read_double_matrix_from_txt(real_t *mat, int_t m, int_t n, const char *fname) {
    int_t status;
    status = read_double_vector_from_txt(mat, m*n, fname);
    transpose_matrix(mat, n, m);
    return status;
}


static void read_ocp_qp_in_N(int_t *N, const char *fpath) {
    char fname[256];
    int status;
    snprintf(fname, sizeof(fname), "%s%s", fpath, "N.txt");
    status = read_int_vector_from_txt(N, 1, fname);
    assert(status == 0);
    assert(*N > 0);
}


static void read_ocp_qp_in_nx(int_t **nx, int_t N, const char *fpath) {
    int_t kk, status;
    char fname[256];

    snprintf(fname, sizeof(fname), "%s%s", fpath, "nx.txt");
    status = read_int_vector_from_txt(*nx, N+1, fname);
    assert(status == 0);
    for (kk = 0; kk < N+1; kk++) assert(nx[0][kk] >= 0);
}


static void read_ocp_qp_in_nu(int_t **nu, int_t N, const char *fpath) {
    int_t kk, status;
    char fname[256];

    snprintf(fname, sizeof(fname), "%s%s", fpath, "nu.txt");
    status = read_int_vector_from_txt(*nu, N, fname);
    assert(status == 0);
    for (kk = 0; kk < N; kk++) assert(nu[0][kk] >= 0);
}


static void read_ocp_qp_in_nb(int_t **nb, int_t N, const char *fpath) {
    int_t kk, status;
    char fname[256];

    snprintf(fname, sizeof(fname), "%s%s", fpath, "nb.txt");
    status = read_int_vector_from_txt(*nb, N+1, fname);
    assert(status == 0);
    for (kk = 0; kk < N+1; kk++) assert(nb[0][kk] >= 0);
}


static void read_ocp_qp_in_nc(int_t **nc, int_t N, const char *fpath) {
    int_t kk, status;
    char fname[256];

    snprintf(fname, sizeof(fname), "%s%s", fpath, "nc.txt");
    status = read_int_vector_from_txt(*nc, N+1, fname);
    assert(status == 0);
    for (kk = 0; kk < N+1; kk++) assert(nc[0][kk] >= 0);
}


static void read_ocp_qp_in_basic(ocp_qp_in *const in, const char *fpath) {
    int_t kk, N, status;
    char fname[256];
    char stage[16];
    N = in->N;

    for (kk = 0; kk < N; kk++) {
        snprintf(stage, sizeof(stage), "%d", kk);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "Q", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->Q[kk], in->nx[kk], in->nx[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "qv", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->q[kk], in->nx[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "S", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->S[kk], in->nu[kk], in->nx[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "R", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->R[kk], in->nu[kk], in->nu[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "rv", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->r[kk], in->nu[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "A", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->A[kk], in->nx[kk+1], in->nx[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "B", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->B[kk], in->nx[kk+1], in->nu[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "bv", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->b[kk], in->nx[kk+1], fname);
        assert(status == 0);
    }
    snprintf(stage, sizeof(stage), "%d", N);
    snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "Q", kk, ".txt");
    status = read_double_matrix_from_txt((real_t*)in->Q[N], in->nx[N], in->nx[N], fname);
    assert(status == 0);

    snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "qv", kk, ".txt");
    status = read_double_vector_from_txt((real_t*)in->q[N], in->nx[N], fname);
    assert(status == 0);
}


static void read_ocp_qp_in_bounds(ocp_qp_in *const in, const char *fpath) {
    char fname[256];
    char stage[16];
    int_t ii, kk, status;

    for (kk = 0; kk <= in->N; kk++) {
        snprintf(stage, sizeof(stage), "%d", kk);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "lb", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->lb[kk], in->nb[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "ub", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->ub[kk], in->nb[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "idxb", kk, ".txt");
        status = read_int_vector_from_txt((int_t*)in->idxb[kk], in->nb[kk], fname);
        assert(status == 0);
        for (ii = 0; ii < in->nb[kk]; ii++) {
            assert(in->idxb[kk][ii] >= 0);
            if (kk < in->N) assert(in->idxb[kk][ii] < in->nx[kk]+in->nu[kk]);
            if (kk == in->N) assert(in->idxb[kk][ii] < in->nx[kk]);
            assert(in->lb[kk][ii] <= in->ub[kk][ii]);
        }
    }
}


static void read_ocp_qp_in_polyhedral(ocp_qp_in *const in, const char *fpath) {
    char fname[256];
    char stage[16];
    int_t ii, kk, status;

    for (kk = 0; kk <= in->N; kk++) {
        snprintf(stage, sizeof(stage), "%d", kk);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "lc", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->lc[kk], in->nc[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "uc", kk, ".txt");
        status = read_double_vector_from_txt((real_t*)in->uc[kk], in->nc[kk], fname);
        assert(status == 0);

        snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "Cx", kk, ".txt");
        status = read_double_matrix_from_txt((real_t*)in->Cx[kk], in->nc[kk], in->nx[kk], fname);
        assert(status == 0);

        if (kk < in->N) {
            snprintf(fname, sizeof(fname), "%s%s%d%s", fpath, "Cu", kk, ".txt");
            status = read_double_matrix_from_txt((real_t*)in->Cu[kk], in->nc[kk], in->nu[kk],
            fname);
            assert(status == 0);
        }
        for (ii = 0; ii < in->nc[kk]; ii++) assert(in->lc[kk][ii] <= in->uc[kk][ii]);
    }
}


static void read_ocp_qp_in_x0(ocp_qp_in *const in, const char *fpath) {
    char fname[256];
    int ii, status, *ptr;

    snprintf(fname, sizeof(fname), "%s%s", fpath, "x0.txt");
    status = read_double_vector_from_txt((real_t*)in->lb[0], in->nx[0], fname);
    assert(status == 0);
    status = read_double_vector_from_txt((real_t*)in->ub[0], in->nx[0], fname);
    assert(status == 0);

    ptr = (int_t*) in->idxb[0];
    for (ii = 0; ii < in->nx[0]; ii++) ptr[ii] = ii;
}


void print_ocp_qp_in(ocp_qp_in const in) {
    int_t kk, ii;
    int_t N = in.N;
    printf("\n----------------------------------\n");
    printf("LOADED DATA FROM TXT FILES:\n\n");
    printf("N:\n%d\n\n", N);
    printf("nx:\n");
    int_print_mat(1, N+1, (int_t *)in.nx, 1);
    printf("nu:\n");
    int_print_mat(1, N, (int_t *)in.nu, 1);
    printf("nb:\n");
    int_print_mat(1, N+1, (int_t *)in.nb, 1);
    printf("nc:\n");
    int_print_mat(1, N+1, (int_t *)in.nc, 1);
    printf("bounds:\n");
    for (kk = 0; kk <= N; kk++) {
        for (ii = 0; ii < in.nb[kk]; ii++) {
            printf("%2.2f  <=  z_%d[%d]  <=  %2.2f\n",
            in.lb[kk][ii], kk, in.idxb[kk][ii], in.ub[kk][ii]);
        }
    }
    printf("\ninequality bounds:\n");
    for (kk = 0; kk <= N; kk++) {
        for (ii = 0; ii < in.nc[kk]; ii++) {
            printf("%2.2f  <=  [Cx_%d[%d] Cu_%d[%d]]*z_%d  <=  %2.2f\n",
            in.lc[kk][ii], kk, ii, kk, ii, kk, in.uc[kk][ii]);
        }
    }
    printf("\nobjective:");
    for (kk = 0; kk < N; kk++) {
        printf("\nQ[%d] =\n", kk);
        d_print_mat(in.nx[kk], in.nx[kk], (real_t*)in.Q[kk], in.nx[kk]);
        printf("\nR[%d] =\n", kk);
        d_print_mat(in.nu[kk], in.nu[kk], (real_t*)in.R[kk], in.nu[kk]);
        printf("\nS[%d] =\n", kk);
        d_print_mat(in.nu[kk], in.nx[kk], (real_t*)in.S[kk], in.nu[kk]);
        printf("\nq[%d] =\n", kk);
        d_print_mat(in.nx[kk], 1, (real_t*)in.q[kk], in.nx[kk]);
        printf("\nr[%d] =\n", kk);
        d_print_mat(in.nu[kk], 1, (real_t*)in.r[kk], in.nu[kk]);
    }
    printf("\nQ[%d] =\n", kk);
    d_print_mat(in.nx[N], in.nx[N], (real_t*)in.Q[N], in.nx[N]);
    printf("\nq[%d] =\n", kk);
    d_print_mat(in.nx[N], 1, (real_t*)in.q[N], in.nx[N]);

    printf("\nequalities:");
    for (kk = 0; kk < N; kk++) {
        printf("\nA[%d] =\n", kk);
        d_print_mat(in.nx[kk+1], in.nx[kk], (real_t*)in.A[kk], in.nx[kk+1]);
        printf("\nB[%d] =\n", kk);
        d_print_mat(in.nx[kk+1], in.nu[kk], (real_t*)in.B[kk], in.nx[kk+1]);
        printf("\nb[%d] =\n", kk);
        d_print_mat(in.nx[kk+1], 1, (real_t*)in.b[kk], in.nx[kk+1]);
    }
    printf("\ninequalities:");
    for (kk = 0; kk < N; kk++) {
        printf("\nCx[%d] =\n", kk);
        d_print_mat(in.nc[kk], in.nx[kk], (real_t*)in.Cx[kk], in.nc[kk]);
        printf("\nCu[%d] =\n", kk);
        d_print_mat(in.nc[kk], in.nu[kk], (real_t*)in.Cu[kk], in.nc[kk]);
    }
    printf("\nCx[%d] =\n", kk);
    d_print_mat(in.nc[N], in.nx[N], (real_t*)in.Cx[N], in.nc[N]);
    printf("\n----------------------------------\n");
}


void read_ocp_qp_in(ocp_qp_in *const in, const char *fpath_,
    int_t BOUNDS, int_t INEQUALITIES, int_t MPC, int_t QUIET) {
    char fpath[256];
    int_t ii, kk, N, pathLength;
    int_t *nx, *nu, *nb, *nc;

    // adding slash at the end if missing
    pathLength = (int_t) strlen(fpath_);
    // TODO(dimitris): add windows support..
    if (fpath_[pathLength-1] != '/') {
        snprintf(fpath, sizeof(fpath), "%s%c", fpath_, '/');
    } else {
        snprintf(fpath, sizeof(fpath), "%s", fpath_);
    }

    read_ocp_qp_in_N(&N, fpath);

    int_zeros(&nu, N+1, 1);
    int_zeros(&nx, N+1, 1);
    int_zeros(&nb, N+1, 1);
    int_zeros(&nc, N+1, 1);

    read_ocp_qp_in_nx(&nx, N, fpath);
    read_ocp_qp_in_nu(&nu, N, fpath);

    if (BOUNDS) {
        read_ocp_qp_in_nb(&nb, N, fpath);
        for (kk = 0; kk < N; kk++) assert(nb[kk] <= nx[kk]+nu[kk]);
        assert(nb[N] <= nx[N]);
    }
    if (INEQUALITIES) read_ocp_qp_in_nc(&nc, N, fpath);
    if (MPC && !BOUNDS) nb[0] = nx[0];

    allocate_ocp_qp_in(N, nx, nu, nb, nc, in);
    read_ocp_qp_in_basic(in, fpath);
    if (BOUNDS) read_ocp_qp_in_bounds(in, fpath);
    if (INEQUALITIES) read_ocp_qp_in_polyhedral(in, fpath);
    if (MPC) read_ocp_qp_in_x0(in, fpath);  // NOTE: call it always AFTER the bounds
    if (BOUNDS && MPC) {
        for (ii = 0; ii < in->nx[0]; ii++) {
            if (in->idxb[0][ii] != ii) {
                printf("\nERROR: Not implemented yet!\n");
                printf("If BOUNDS == 1 & MPC == 1, x0 must be bounded: idxb[0][0:nx-1] = 0:nx-1\n");
                assert(0 == 1);
            }
        }
    }

    if (!QUIET) print_ocp_qp_in(*in);

    int_free(nx);
    int_free(nu);
    int_free(nb);
    int_free(nc);
}
