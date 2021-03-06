#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

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

// system headers
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
// flush denormals to zero
#if defined(TARGET_X64_AVX2) || defined(TARGET_X64_AVX) ||  \
    defined(TARGET_X64_SSE3) || defined(TARGET_X86_ATOM) || \
    defined(TARGET_AMD_SSE3)
#include <xmmintrin.h>  // needed to flush to zero sub-normals with _MM_SET_FLUSH_ZERO_MODE (_MM_FLUSH_ZERO_ON); in the main()
#endif

#include "aux_d.h"

#include "sim_erk_integrator.h"
#include "types.h"
#include "ocp_qp_common.h"
#include "ocp_qp_hpmpc.h"
#include "timing.h"
#include "tools.h"
#include "Chen_model.h"

// define IP solver arguments && number of repetitions
#define NREP 1000
#define MAXITER 10
#define TOL 1e-8
#define MINSTEP 1e-8

#define NN 13
#define NX 2
#define NU 1


#ifdef DEBUG
static void print_states_controls(real_t *w, int_t N) {
    printf("node\tx\t\t\t\tu\n");
    for (int_t i = 0; i < N; i++) {
        printf("%4d\t%+e %+e\t%+e\n", i, w[i*(NX+NU)], w[i*(NX+NU)+1], w[i*(NX+NU)+2]);
    }
    printf("%4d\t%+e %+e\n", N, w[N*(NX+NU)], w[N*(NX+NU)+1]);
}
#endif  // DEBUG

static void shift_states(real_t *w, real_t *x_end, int_t N) {
    for (int_t i = 0; i < N; i++) {
        for (int_t j = 0; j < NX; j++) w[i*(NX+NU)+j] = w[(i+1)*(NX+NU)+j];
    }
    for (int_t j = 0; j < NX; j++) w[N*(NX+NU)+j] = x_end[j];
}

static void shift_controls(real_t *w, real_t *u_end, int_t N) {
    for (int_t i = 0; i < N-1; i++) {
        for (int_t j = 0; j < NU; j++) w[i*(NX+NU)+NX+j] = w[(i+1)*(NX+NU)+NX+j];
    }
    for (int_t j = 0; j < NU; j++) w[(N-1)*(NX+NU)+NX+j] = u_end[j];
}

// Simple SQP example for acados
int main() {
    // Problem data
    printf('11111');
    int_t   N                   = NN;
    real_t  x0[NX]              = {0.05, 0};
    real_t  w[NN*(NX+NU)+NX]    = {0};  // States and controls stacked
    real_t  Q[NX*NX]            = {0};
    real_t  R[NU*NU]            = {0};
    real_t  xref[NX]            = {0};
    real_t  uref[NX]            = {0};
    int_t   max_sqp_iters       = 1;
    int_t   max_iters           = 10;
    real_t  x_end[NX]           = {0};
    real_t  u_end[NU]           = {0};

    for (int_t i = 0; i < NX; i++) Q[i*(NX+1)] = 1.0;
    for (int_t i = 0; i < NU; i++) R[i*(NU+1)] = 0.05;

    // Integrator structs
    real_t T = 0.01;
    sim_in  sim_in;
    sim_out sim_out;
    sim_in.nSteps = 10;
    sim_in.step = T/sim_in.nSteps;
    sim_in.VDE_forw = &VDE_fun;
    sim_in.nx = NX;
    sim_in.nu = NU;

    sim_in.sens_forw = true;
    sim_in.sens_adj = false;
    sim_in.sens_hess = false;
    sim_in.nsens_forw = NX+NU;

    sim_in.x = malloc(sizeof(*sim_in.x) * (NX));
    sim_in.u = malloc(sizeof(*sim_in.u) * (NU));
    sim_in.S_forw = malloc(sizeof(*sim_in.S_forw) * (NX*(NX+NU)));
    for (int_t i = 0; i < NX*(NX+NU); i++) sim_in.S_forw[i] = 0.0;
    for (int_t i = 0; i < NX; i++) sim_in.S_forw[i*(NX+1)] = 1.0;

    sim_out.xn = malloc(sizeof(*sim_out.xn) * (NX));
    sim_out.S_forw = malloc(sizeof(*sim_out.S_forw) * (NX*(NX+NU)));

    sim_info erk_info;
    sim_out.info = &erk_info;

    sim_erk_workspace erk_work;
    sim_RK_opts rk_opts;
    sim_erk_create_opts(4, &rk_opts);
    sim_erk_create_workspace(&sim_in, &erk_work);

    int_t nx[NN+1] = {0};
    int_t nu[NN] = {0};
    int_t nb[NN+1] = {0};
    int_t nc[NN+1] = {0};
    for (int_t i = 0; i < N; i++) {
        nx[i] = NX;
        nu[i] = NU;
    }
    nx[N] = NX;

    real_t *pA[N];
    real_t *pB[N];
    real_t *pb[N];
    real_t *pQ[N+1];
    real_t *pS[N];
    real_t *pR[N];
    real_t *pq[N+1];
    real_t *pr[N];
    real_t *px[N+1];
    real_t *pu[N];
    real_t *px0[1];
    int    *hidxb[N+1];
    real_t *pC[N + 1];
    real_t *pD[N];
    real_t *plg[N + 1];
    real_t *pug[N + 1];

    /************************************************
    * box constraints
    ************************************************/
    int ii, jj;

    nb[0] = 0;
    for (ii = 1; ii < N; ii++) nb[ii] = 0;
    nb[N] = 0;

    int *idxb0;
    int_zeros(&idxb0, nb[0], 1);
    idxb0[0] = 0;
    idxb0[1] = 1;


    int *idxb1;
    int_zeros(&idxb1, nb[1], 1);

    int *idxbN;
    int_zeros(&idxbN, nb[N], 1);

    for (ii = 1; ii < N; ii++) {
        hidxb[ii] = idxb1;
    }

    hidxb[N] = idxbN;

    d_zeros(&px0[0], nx[0], 1);
    for (int_t i = 0; i < N; i++) {
        d_zeros(&pA[i], nx[i+1], nx[i]);
        d_zeros(&pB[i], nx[i+1], nu[i]);
        d_zeros(&pb[i], nx[i+1], 1);
        d_zeros(&pS[i], nu[i], nx[i]);
        d_zeros(&pq[i], nx[i], 1);
        d_zeros(&pr[i], nu[i], 1);
        d_zeros(&px[i], nx[i], 1);
        d_zeros(&pu[i], nu[i], 1);
    }
    d_zeros(&pq[N], nx[N], 1);
    d_zeros(&px[N], nx[N], 1);
    // hidxb[N] = idxbN;

    nx[0] = 0;
    //    d_print_mat(nx, 1, b, nx);
    //    d_print_mat(nx, 1, b0, nx);

    // then A0 is a matrix of size 0x0
    double *A0;
    d_zeros(&A0, 0, 0);

    int ng = 0;  // 4;  // number of general constraints
    int ngN = 0;  // 4;  // number of general constraints at the last stage

    int ngg[N + 1];
    for (ii = 0; ii < N; ii++) ngg[ii] = ng;
    ngg[N] = ngN;

    /************************************************
    * general constraints
    ************************************************/

    double *C0;
    d_zeros(&C0, 0, NX);
    double *D0;
    d_zeros(&D0, 0, NU);
    double *lg0;
    d_zeros(&lg0, 0, 1);
    double *ug0;
    d_zeros(&ug0, 0, 1);

    double *C;
    d_zeros(&C, 0, NX);
    double *D;
    d_zeros(&D, 0, NU);
    double *lg;
    d_zeros(&lg, 0, 1);
    double *ug;
    d_zeros(&ug, 0, 1);

    double *CN;
    d_zeros(&CN, ngN, NX);
    double *lgN;
    d_zeros(&lgN, ngN, 1);  // force all states to 0 at the last stage
    double *ugN;
    d_zeros(&ugN, ngN, 1);  // force all states to 0 at the last stage

    pC[0] = C0;
    pD[0] = D0;
    plg[0] = lg0;
    pug[0] = ug0;
    real_t *ppi[N];
    real_t *plam[N+1];

    ii = 0;
    d_zeros(&ppi[ii], nx[ii+1], 1);
    d_zeros(&plam[ii], 2*nb[ii]+2*nb[ii], 1);
    for (ii = 1; ii < N; ii++) {
        pC[ii] = C;
        pD[ii] = D;
        plg[ii] = lg;
        pug[ii] = ug;
        d_zeros(&ppi[ii], nx[ii+1], 1);
        d_zeros(&plam[ii], 2*nb[ii]+2*nb[ii], 1);
    }
    pC[N] = CN;
    plg[N] = lgN;
    pug[N] = ugN;

    /************************************************
    * solver arguments
    ************************************************/

    // solver arguments
    ocp_qp_hpmpc_args hpmpc_args;
    hpmpc_args.tol = TOL;
    hpmpc_args.max_iter = MAXITER;
//  hpmpc_args.min_step = MINSTEP;
    hpmpc_args.mu0 = 0.0;
//  hpmpc_args.sigma_min = 1e-3;
    hpmpc_args.warm_start = 0;
    hpmpc_args.N2 = N;

    /************************************************
    * work space
    ************************************************/

    int work_space_size = ocp_qp_hpmpc_workspace_size_bytes(N, nx, nu, nb, ngg, hidxb, &hpmpc_args);
    printf("\nwork space size: %d bytes\n", work_space_size);

    // void *workspace = malloc(work_space_size);
    void *workspace = malloc(500000);

    // double workspace[500000];
    // Allocate OCP QP variables
    ocp_qp_in qp_in;
    qp_in.N = N;
    ocp_qp_out qp_out;
    qp_in.nx = nx;
    qp_in.nu = nu;
    qp_in.nb = nb;
    qp_in.nc = nc;
    for (int_t i = 0; i < N; i++) {
        pQ[i] = Q;
        pR[i] = R;
    }
    pQ[N] = Q;
    qp_in.Q = (const real_t **) pQ;
    qp_in.S = (const real_t **) pS;
    qp_in.R = (const real_t **) pR;
    qp_in.q = (const real_t **) pq;
    qp_in.r = (const real_t **) pr;
    qp_in.A = (const real_t **) pA;
    qp_in.B = (const real_t **) pB;
    qp_in.b = (const real_t **) pb;
    // qp_in.lb = (const real_t **) px0;
    // qp_in.ub = (const real_t **) px0;
    // qp_in.idxb = (const int_t **) hidxb;
    qp_in.Cx = (const real_t **) pC;
    qp_in.Cu = (const real_t **) pD;
    qp_in.lc = (const real_t **) plg;
    qp_in.uc = (const real_t **) pug;

    qp_out.x = px;
    qp_out.u = pu;
    qp_out.pi = ppi;
    qp_out.lam = plam;

    acado_timer timer;
    real_t timings = 0;
    for (int_t iter = 0; iter < max_iters; iter++) {
        // printf("\n------ ITERATION %d ------\n", iter);
        acado_tic(&timer);
        for (int_t sqp_iter = 0; sqp_iter < max_sqp_iters; sqp_iter++) {
            for (int_t i = 0; i < N; i++) {
                // Pass state and control to integrator
                for (int_t j = 0; j < NX; j++) sim_in.x[j] = w[i*(NX+NU)+j];
                for (int_t j = 0; j < NU; j++) sim_in.u[j] = w[i*(NX+NU)+NX+j];
                sim_erk(&sim_in, &sim_out, &rk_opts, &erk_work);

                // Construct QP matrices
                for (int_t j = 0; j < NX; j++) {
                    pq[i][j] = Q[j*(NX+1)]*(w[i*(NX+NU)+j]-xref[j]);
                }

                for (int_t j = 0; j < NU; j++) {
                    pr[i][j] = R[j*(NX+1)]*(w[i*(NX+NU)+NX+j]-uref[j]);
                }

                for (int_t j = 0; j < NX; j++) {
                    pb[i][j] = sim_out.xn[j] - w[(i+1)*(NX+NU)+j];
                    for (int_t k = 0; k < NX; k++) pA[i][j*NX+k] = sim_out.S_forw[j*(NX)+k];
                }

                for (int_t j = 0; j < NU; j++)
                    for (int_t k = 0; k < NX; k++) pB[i][j*NX+k] = sim_out.S_forw[NX*NX + NX*j+k];
            }

            dgemv_n_3l(NX, NX, pA[0], NX, x0, pb[0]);

            for (int_t j = 0; j < NX; j++) {
                pq[N][j] = Q[j]*(w[N*(NX+NU)+j]-xref[j]);
            }
            int status = ocp_qp_hpmpc(&qp_in, &qp_out, &hpmpc_args, workspace);
            // int status = 0;
            // printf("hpmpc_status=%i\n",status);
            if (status == 1) printf("status = ACADOS_MAXITER\n");

            if (status == 2) printf("status = ACADOS_MINSTEP\n");

            for (int_t i = 0; i < N; i++) {
                for (int_t j = 0; j < NX; j++) w[i*(NX+NU)+j] += qp_out.x[i][j];
                for (int_t j = 0; j < NU; j++) w[i*(NX+NU)+NX+j] += qp_out.u[i][j];
            }
            for (int_t j = 0; j < NX; j++) w[N*(NX+NU)+j] += qp_out.x[N][j];
        }
        for (int_t i = 0; i < NX; i++) x0[i] = w[NX+NU+i];
        shift_states(w, x_end, N);
        shift_controls(w, u_end, N);
        timings += acado_toc(&timer);
    }
    #ifdef DEBUG
    print_states_controls(&w[0], N);
    #endif  // DEBUG
    printf("Average of %.3f ms per iteration.\n", 1e3*timings/max_iters);
    // free(workspace);
    return 0;
}

void app_main(void)
{
    nvs_flash_init();
    // tcpip_adapter_init();
    // ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    // ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    // ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    // wifi_config_t sta_config = {
    //     .sta = {
    //         .ssid = "access_point_name",
    //         .password = "password",
    //         .bssid_set = false
    //     }
    // };
    // ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &sta_config) );
    // ESP_ERROR_CHECK( esp_wifi_start() );
    // ESP_ERROR_CHECK( esp_wifi_connect() );
    xTaskCreate(main, "main", 10*1024, NULL, 5, NULL);
    // xTaskCreate(main_memory_task, "main_memory", 10*1024, NULL, 5, NULL);
}
