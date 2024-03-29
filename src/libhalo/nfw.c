#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_fit.h>
#include <gsl/gsl_multifit.h>
#include <gsl/gsl_multifit_nlin.h>

#include "../libmath/math.h"
#include "../general_def.h"

#include "halo.h"

#ifdef WITH_MPI
#include "../libparallel/general.h"
#endif


/*
 *  Function declaration
 */
double nfw_drs(double, double, double);
double nfw_drho0(double, double, double);

int nfw_f(const gsl_vector*, void *, gsl_vector*);
int d_nfw_f(const gsl_vector*, void *, gsl_matrix*);
int fd_nfw_f(const gsl_vector*, void *, gsl_vector*, gsl_matrix*);
void best_fit_nfw(double, double, int, double*, double*, double*);

void fit_halo_profile(struct halo *);


/*
 *  Function implementations
 */
void fit_and_store_nfw_parameters()
{
	int k=0; 
	int nHaloes;

	struct halo * HALO;

#ifdef WITH_MPI
	HALO = pHaloes[ThisTask];
	nHaloes = pSettings[ThisTask].n_threshold;
	TASK_INFO_MSG(ThisTask, "fitting halo densities to NFW profiles");
#else 
	HALO = Haloes;
	nHaloes = Settings.n_threshold;
	INFO_MSG("Fitting halo densities to NFW profiles");
#endif

		for(k=0; k<nHaloes; k++)
			fit_halo_profile(&HALO[k]);			
}



void fit_halo_profile(struct halo *HALO)
{
	double c=0, r=0, rho0, rho0_halo, rs, chi, gof, per; 
	double *x, *y, *e, *R, *y_th, *x_bin, *y_bin, *e_bin, rMin, rMax; 
	int bins, skip, N, j=0;

		r = HALO->Rvir;
		c = HALO->c; 
		bins = HALO->n_bins;
		skip = HALO->neg_r_bins;

		N = bins - skip;

		rho0 = HALO->rho0;
		rs = HALO->r2;

		x = (double*) calloc(N, sizeof(double));
		y = (double*) calloc(N, sizeof(double));
		e = (double*) calloc(N, sizeof(double));
		R = (double*) calloc(N, sizeof(double));
		
		for(j=0; j<N; j++)
		{
			x[j] = HALO->radius[j+skip];
			y[j] = HALO->rho[j+skip];
			e[j] = HALO->err[j+skip];
			R[j] = HALO->radius[j+skip]/r;
		}

			best_fit_nfw(rho0, rs, N, x, y, e);

			HALO->fit_nfw.rho0 = rho0;
			HALO->fit_nfw.rs = rs;
			HALO->fit_nfw.c = HALO->Rvir/rs;

			y_th = (double*) calloc(bins-skip,sizeof(double));

		for(j=skip; j<bins; j++)
		{
			y_th[j-skip] = nfw(HALO->radius[j], HALO->fit_nfw.rs, HALO->fit_nfw.rho0);
			//fprintf(stderr, "%d) R=%e, %e %e  %e\n", j, R[j-skip], rho0, y[j-skip], y_th[j-skip]);
		}

	// Various estimators for the goodness of fit
	chi = chi_square(y_th, y, e, N);
	gof = goodness_of_fit(y_th, y, N);
	per = percentage_error(y_th, y, N);
	
	chi /= (double) (bins-skip);

	HALO->fit_nfw.chi = chi;
	HALO->fit_nfw.gof = gof;
	HALO->fit_nfw.per = per;

		x_bin = (double*) calloc(BIN_PROFILE+1, sizeof(double));
		y_bin = (double*) calloc(BIN_PROFILE, sizeof(double));
		e_bin = (double*) calloc(BIN_PROFILE, sizeof(double));

		rMin = 2 * Rvir_frac_min;
		rMax = F_MAX * 1.01; //HALO->radius[bins-1]/r;
		x_bin = log_stepper(rMin, rMax, BIN_PROFILE+1);

		average_bin(R, y, x_bin, y_bin, e_bin, BIN_PROFILE+1, N);

	for(j=0; j<BIN_PROFILE; j++)
	{
		HALO->nfw.x[j] = 0.5 * (x_bin[j] + x_bin[j+1]);
		HALO->nfw.y[j] = y_bin[j];
		//fprintf(stderr, "%d  %e  %e\n", j, x_bin[j+1], y_bin[j]);
	}

	free(x);
	free(y);
	free(R);
	free(y_th);
	free(e);
//	fprintf(stderr, "ThisTask=%d, skip=%d, bins=%d, rho=%f, rs=%f, ChiSquare=%lf, Red=%lf\n", 
//		ThisTask, skip, bins, rho0, rs, chi_sq, chi_sq/(bins-skip));
}



void average_nfw_profile(void)
{
	int k=0, i=0, m=0; 
	int nHaloes, nHaloesCut;
	int nfw_bin;
	double nfw, nfw_tot;

	struct halo_properties *HALOPROPERTIES;

	INFO_MSG("Sorting halo radial velocities and concentrations");

	nHaloesCut=n_haloes_per_criterion();
	
	if(Settings.use_sub == 1)
	{
		HALOPROPERTIES = SubHaloProperties;	
	} 
		else 
	{
		HALOPROPERTIES = HaloProperties;
	}

#ifdef WITH_MPI
		nHaloes=Settings.n_haloes; 
#else
		nHaloes=Settings.n_threshold; 
#endif

		for(i=0; i<BIN_PROFILE; i++)
		{
			nfw_tot = 0;
			nfw_bin = 0;

			for(k=0; k<nHaloes; k++)
			{
				if(halo_condition(k) == 1)
				{
					nfw = Haloes[k].nfw.y[i];

						if(isnan(nfw) == 0 && nfw>0 && nfw < 1./0.)
						{
							nfw_tot += nfw;
							nfw_bin++;
						}
		
				}
		
			}

			HALOPROPERTIES[HALO_INDEX].nfw.x[i] = Haloes[0].nfw.x[i];
			HALOPROPERTIES[HALO_INDEX].nfw.y[i] = nfw_tot/nfw_bin;
			HALOPROPERTIES[HALO_INDEX].nfw.n[i] = nfw_bin;
		}
}



double nfw(double r, double rs, double rho_0)
{
	return (rho_0*rs)/(r*pow2(1.+ r/rs));
}



double nfw_drs(double r, double rs, double rho_0)
{
	return (rho_0/r)*(1./pow2(1.+r/rs)+3.*pow2(rs)/pow2(rs+r)-2.*pow3(rs)/pow3(rs+r));
}



double nfw_drho0(double r, double rs, double rho_0)
{
	return 1./((r/rs)*pow2(1.+r/rs));
}



int nfw_f(const gsl_vector *x, void *data, gsl_vector *f)
{
	size_t i, n;
	double rs, rho0, t, rrs, Yi, fset, *vx, *vy, *err;

	n = ((struct data *)data)->n;
	vx = ((struct data *)data)->x;
	vy = ((struct data *)data)->y;
	err = ((struct data *)data)->err;

	rs = gsl_vector_get(x,0);
	rho0 = gsl_vector_get(x,1);

	for(i=1; i<n; i++)
	{
		t = vx[i];
		rrs = t/rs;
		Yi = rho0*(1./(rrs*pow2(1.+rrs)));
		fset =  (Yi - vy[i])/err[i];
	
		if(fset!=fset) 
			fset=0;

		gsl_vector_set(f, i, fset);
	}

	return GSL_SUCCESS;
}



int d_nfw_f(const gsl_vector *x, void *data, gsl_matrix *J)
{
	size_t i, n;
	double rs, rho0, s, r, e1, e2, *vx, *err;
		
	n = ((struct data*)data)->n;	
	vx = ((struct data *)data)->x;
	err = ((struct data*)data)->err;

	rs = gsl_vector_get(x,0);
	rho0 = gsl_vector_get(x,1);

	for(i = 1; i < n; i++)
	{
		s = err[i];
		r = vx[i]; 
	
		e1 = nfw_drs(r, rs, rho0);
		e2 = nfw_drho0(r, rs, rho0);

		gsl_matrix_set(J,i,0,e1/s);
		gsl_matrix_set(J,i,1,e2/s);
	}

	return GSL_SUCCESS;
}



int fd_nfw_f(const gsl_vector *x, void *data, gsl_vector * f, gsl_matrix *J)
{
	nfw_f(x,data,f);
	d_nfw_f(x,data,J);

	return GSL_SUCCESS;
}



void best_fit_nfw(double rho0, double rs, int nBins, double *array_data_x, double *array_data_y, double *y_err)
{
	double *params;
	struct data dat;
	struct parameters par;

	dat.n = (size_t) nBins;
	dat.x = array_data_x;
	dat.y = array_data_y;
	dat.err = y_err; 

		/* First guess initialization */
	par.n = 2;
	par.guess_p = (double *) calloc(par.n, sizeof(double));
	par.guess_p[0] = rs;
	par.guess_p[1] = rho0;
	par.fitted_p = gsl_vector_alloc(par.n);

		/* Initialize function */
	gsl_multifit_function_fdf f;
       	f.f = &nfw_f;
       	f.df = &d_nfw_f;
       	f.fdf = &fd_nfw_f;
       	f.n = dat.n;
       	f.p = par.n;
       	f.params = &dat;

	if(dat.n > 3)
	{
		/* Do the fit */
		par.fitted_p = least_square_nl_fit(dat, par, f);
	}
		/* Set the correctly fitted parameters */
		rs = gsl_vector_get(par.fitted_p,0);
		rho0 = gsl_vector_get(par.fitted_p,1);
}
