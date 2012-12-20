#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "halo_properties.h"
#include "subhalo_general.h"
#include "../general_variables.h"
#include "../general_functions.h"
#include "../libmath/mathtools.h"
#include "../libmath/log_norm.h"
#include "../libmath/power_law.h"
#include "../libcosmo/cosmological_relations.h"
#include "../libcosmo/mass_function.h"


void initialize_halo_properties_structure()
{
	int rBins, nBins;

	rBins=Settings.r_bins; 
	nBins=Settings.n_bins;

			// Halo axis alignment
		HaloZ.r_bins=rBins-1; rBins--;
		HaloZ.R = (double*) calloc(rBins, sizeof(double));
		HaloZ.Th_p = (double*) calloc(rBins, sizeof(double));
		HaloZ.Th_c = (double*) calloc(rBins, sizeof(double));
		HaloZ.N_pairs = (int*) calloc(rBins, sizeof(double));
			// Other halo properties
		HaloZ.n_bins=nBins-1; nBins--;
		HaloZ.c = (double*) calloc(nBins, sizeof(double));
		HaloZ.p_c = (double*) calloc(nBins, sizeof(double));
		HaloZ.l = (double*) calloc(nBins, sizeof(double));
		HaloZ.p_l = (double*) calloc(nBins, sizeof(double));
		HaloZ.err_p_l = (double*) calloc(nBins, sizeof(double));
		HaloZ.shape = (double*) calloc(nBins, sizeof(double));
		HaloZ.p_shape = (double*) calloc(nBins, sizeof(double));
		HaloZ.n_shape = (int*) calloc(nBins, sizeof(double));
		HaloZ.triax = (double*) calloc(nBins, sizeof(double));
		HaloZ.p_triax = (double*) calloc(nBins, sizeof(double));
		HaloZ.n_triax = (int*) calloc(nBins, sizeof(double));
		HaloZ.mass = (double*) calloc(nBins, sizeof(double));
		HaloZ.radVel = (double*) calloc(nBins, sizeof(double));
		HaloZ.err_radVel = (double*) calloc(nBins, sizeof(double));

#ifdef GAS
	HaloZ.gas_T = (double*) calloc(nBins, sizeof(double));
	HaloZ.gas_u = (double*) calloc(nBins, sizeof(double));
	HaloZ.gas_fraction = (double*) calloc(nBins, sizeof(double));
#endif
}



void free_halo_properties()
{
		free(HaloZ.c);
		free(HaloZ.p_c);
		free(HaloZ.l);
		free(HaloZ.p_l);
		free(HaloZ.err_p_l);
		free(HaloZ.p_shape);
		free(HaloZ.n_shape);
		free(HaloZ.shape);
		free(HaloZ.triax);
		free(HaloZ.p_triax);
		free(HaloZ.n_triax);
		free(HaloZ.mass);
		free(HaloZ.radVel);
		free(HaloZ.err_radVel);
		free(haloes);
#ifdef GAS
		free(HaloZ.gas_T);
		free(HaloZ.gas_u);
		free(HaloZ.gas_fraction);
#endif 
}



void sort_axis_alignement()
{
	int *Nbins, j=0, k=0, i=0, max_haloes, nBins, skip;
	double *radius, *Rbins, *Abins, *Bbins; 
	double Rmin, Rmax, R, A, B;

	fprintf(stdout,"Computing halo major axis alignement angles for %d bins.\n", HaloZ.r_bins);
	
		max_haloes = Settings.haloes_over_threshold;
		skip = Settings.halo_skip; nBins = Settings.r_bins; 
		Settings.tick=0;

		radius = (double *) calloc(nBins, sizeof(double));
		Rbins = (double *) calloc(nBins-1, sizeof(double));
		Abins = (double *) calloc(nBins-1, sizeof(double));
		Bbins = (double *) calloc(nBins-1, sizeof(double));
		Nbins = (int *) calloc(nBins-1, sizeof(int));
	
		Rmin = Settings.Rmin; Rmax = Settings.Rmax;
		radius = log_stepper(Rmin,Rmax,nBins);

			for(i=0; i<nBins-1; i++)
				Rbins[i] = 0.5*(radius[i+1]+radius[i]);

			for(j=0; j<max_haloes; j++) 
			{
				for(k=j; k<max_haloes; k++)
				{
					A = 0; B = 0; R = 0;
					R = sqrt(
						pow(haloes[j].Xc - haloes[k].Xc,2) +
						pow(haloes[j].Yc - haloes[k].Yc,2) +
						pow(haloes[j].Zc - haloes[k].Zc,2) );

					print_counter(500000);

				if(R > Rmin && R < Rmax)
				{
					A = 
						haloes[j].Eax*haloes[k].Eax +
						haloes[j].Eay*haloes[k].Eay + 
						haloes[j].Eaz*haloes[k].Eaz;
					B = (
						haloes[j].Eax*(haloes[j].Xc - haloes[k].Xc) +
						haloes[j].Eay*(haloes[j].Yc - haloes[k].Yc) + 
						haloes[j].Eaz*(haloes[j].Zc - haloes[k].Zc) ) / R;

				for(i=0; i<nBins-1; i++) 
				{
					if(R>radius[i] && R<radius[i+1])
					{
						Abins[i] += sqrt(A*A);
						Bbins[i] += sqrt(B*B);
						Nbins[i] ++;
					}

				HaloZ.R[i]=Rbins[i]; 
				HaloZ.Th_c[i]=Abins[i]; 
				HaloZ.Th_p[i]=Bbins[i]; 
				HaloZ.N_pairs[i]=Nbins[i];

					}
				}
			}
		}

	free(radius);
	free(Abins);
	free(Bbins);
	free(Rbins);
	free(Nbins);

	fprintf(stdout, "\n");
}



void sort_shape_and_triaxiality()
{
	int i=0, m=0, nBins=Settings.n_bins,  nHaloes=Settings.haloes_over_threshold;
	int *array_shape_bin_y, *array_triax_bin_y;
	double *array_shape, *array_triax, *array_shape_bin, *array_triax_bin; 
	double half_t, half_s, sMax, sMin, tMax, tMin, p_s, p_t;

	fprintf(stdout, "\nSorting shape and triaxiality."); 

	Settings.tick=0;
	array_shape = (double*) calloc(nHaloes, sizeof(double));	
	array_triax = (double*) calloc(nHaloes, sizeof(double));	
	array_shape_bin = (double*) calloc(nBins, sizeof(double));	
	array_triax_bin = (double*) calloc(nBins, sizeof(double));	
	array_shape_bin_y = (int*) calloc(nBins-1, sizeof(int));	
	array_triax_bin_y = (int*) calloc(nBins-1, sizeof(int));	

		for(i=0; i<nHaloes; i++)
		{
			array_shape[i] = haloes[i].c_a;
			array_triax[i] = haloes[i].triax;
			m++;
		}

			HaloZ.s0 = average(array_shape, nHaloes);
			HaloZ.t0 = average(array_triax, nHaloes);
	
			array_shape = shellsort(array_shape, nHaloes);
			array_triax = shellsort(array_triax, nHaloes);

			sMax = array_shape[nHaloes-1]; sMin = array_shape[0];
			tMax = array_triax[nHaloes-1]; tMin = array_triax[0];

			array_shape_bin = lin_stepper(sMin, sMax, nBins);
			lin_bin(array_shape, array_shape_bin, nBins, nHaloes, array_shape_bin_y);	

			array_triax_bin = lin_stepper(tMin, tMax, nBins);
			lin_bin(array_triax, array_triax_bin, nBins, nHaloes, array_triax_bin_y);	

			half_s = 0.5*(array_shape_bin[1]-array_shape_bin[0]);
			half_t = 0.5*(array_triax_bin[1]-array_triax_bin[0]);

			for(i=0; i<nBins-1; i++)	
			{
				p_s = (double) array_shape_bin_y[i]/nHaloes;
				p_t = (double) array_triax_bin_y[i]/nHaloes;
				HaloZ.shape[i]=array_shape_bin[i]+half_s;
				HaloZ.triax[i]=array_triax_bin[i]+half_t;
				HaloZ.p_shape[i]=p_s;
				HaloZ.p_triax[i]=p_t;
				HaloZ.n_shape[i]=array_shape_bin_y[i];
				HaloZ.n_triax[i]=array_triax_bin_y[i];
			}

	free(array_shape); 
	free(array_triax);
	free(array_shape_bin); 
	free(array_triax_bin);
	free(array_shape_bin_y); 
	free(array_triax_bin_y);
	fprintf(stderr, "\n");
}



void sort_radial_velocity()
{
	int nBins=Settings.n_bins, nHaloes=Settings.haloes_over_threshold, i=0; 
	double mMax = haloes[0].Mvir, mMin = haloes[nHaloes-1].Mvir;
	double *radial_velocity_bin, *radial_velocity_error, *mass, *radial_velocity, *mass_bin; 

	fprintf(stdout, "\nSorting halo radial velocities.\n");
	
	Settings.tick=0;
	mass = (double*) calloc(nHaloes, sizeof(double));	
	radial_velocity = (double*) calloc(nHaloes, sizeof(double));	

	mass_bin = (double*) calloc(nBins, sizeof(double));	
	radial_velocity_bin = (double*) calloc(nBins-1, sizeof(double));	
	radial_velocity_error = (double*) calloc(nBins-1, sizeof(double));	// TODO error estimation

		for(i=0; i<nHaloes; i++)
		{
			radial_velocity[i] = haloes[i].Vmax;
			mass[i] = haloes[i].Mvir; 	
		}

			mass_bin = log_stepper(mMin, mMax, nBins);
			average_bin(mass, radial_velocity, mass_bin, radial_velocity_bin, radial_velocity_error,
				nBins, nHaloes);

			for(i=0; i<nBins; i++)	
			{
				HaloZ.radVel[i]=radial_velocity_bin[i];
				HaloZ.err_radVel[i]=radial_velocity_error[i];
			}
	
	free(mass);
	free(mass_bin);
	free(radial_velocity); 
	free(radial_velocity_bin); 
	free(radial_velocity_error); 
	fprintf(stdout, "\n");
}



void sort_lambda()
{
	int nBins=Settings.n_bins, nHaloes, i=0, m=0, *lambda_int_y; 
	double *bin_x, *params, *lambda, *lambda_bin_x, *lambda_err_y, *lambda_double_y;
	double l_0, sig, halfstep, lMax, lMin, delta_l, norm, value;

	fprintf(stdout, "\nSorting spin parameter.\n"); 

	Settings.tick=0;
	nHaloes=Settings.spin_criterion;

	bin_x = (double*) calloc(nBins, sizeof(double));	
	lambda = (double*) calloc(nHaloes, sizeof(double));	
	params = (double*) calloc(2, sizeof(double));
	lambda_int_y = (int*) calloc(nBins-1, sizeof(int));	
	lambda_bin_x = (double*) calloc(nBins-1, sizeof(double));	
	lambda_double_y = (double*) calloc(nBins-1, sizeof(double));	
	lambda_err_y = (double*) calloc(nBins-1, sizeof(double));	

		for(i=0; i<nHaloes; i++)
		{
			if(haloes[i].spin==1) 
			{
				lambda[m] = haloes[i].lambda;
				m++;
					}
						}	
	
				lambda = shellsort(lambda, nHaloes);

				lMax = lambda[nHaloes-1];  
				lMin = lambda[0];
				delta_l = (lMax-lMin)/nBins; 
				norm = 1./(delta_l*nHaloes);
	
				bin_x = lin_stepper(lMin, lMax, nBins);
				lin_bin(lambda, bin_x, nBins, nHaloes, lambda_int_y);	

			halfstep=(bin_x[1]-bin_x[0])*0.5;

			for(i=0; i<nBins-1; i++)
			{	
				value = lambda_int_y[i];
				lambda_bin_x[i]=bin_x[i]+halfstep;
				lambda_err_y[i]=sqrt(value*norm); 
				lambda_double_y[i]=norm*value; 
			}

			params = best_fit_lognorm(lambda, nHaloes, nBins-1, lambda_bin_x, lambda_double_y, lambda_err_y);

			l_0 = params[0]; 
			sig = params[1];
			HaloZ.l_0=l_0;
			HaloZ.l_sig=sig;

		for(i=0; i<nBins-1; i++)
		{		
			HaloZ.l[i]=lambda_bin_x[i];
			HaloZ.p_l[i]=lambda_double_y[i];
			HaloZ.err_p_l[i]=lambda_err_y[i];
		}	

	free(bin_x);
	free(lambda);
	free(lambda_int_y);
	free(lambda_double_y);
	free(lambda_err_y);
	free(lambda_bin_x);
	free(params);
	fprintf(stdout, "\n");
}



void sort_concentration()
{
	int nBins=Settings.n_bins, nHaloes=Settings.haloes_over_threshold, nHaloes_vir, i=0, m=0, *int_c_bin_y;
	double *params, *params2, *conc, *bin_x, *c_bin_x, *c_bin_y, *c_err_y;
	double c_0, sig, max, halfstep, c_02, sig2, cMax, cMin, norm, thr_vir;

	fprintf(stdout, "\nSorting concentrations.\n");

	Settings.tick=0;
	thr_vir=Cosmo.virial;
	nHaloes_vir=Settings.virialized_concentration;

	conc = (double*) calloc(nHaloes_vir, sizeof(double));	
	bin_x = (double*) calloc(nBins, sizeof(double));	
	c_bin_x = (double*) calloc(nBins-1, sizeof(double));	
	c_bin_y = (double*) calloc(nBins-1, sizeof(double));	
	int_c_bin_y = (int*) calloc(nBins-1, sizeof(int));	
	c_err_y = (double*) calloc(nBins-1, sizeof(double));	
	params = (double*) calloc(2, sizeof(double));
	params2 = (double*) calloc(2, sizeof(double));

		for(i=0; i<nHaloes; i++) 
		{
			if(haloes[i].virial==1 && haloes[i].conc==1)
			{ 
#ifdef AHF_v1
				conc[m] = haloes[i].c_nfw;
				if(conc[m] == -1) conc[m] = haloes[i].c;
#else
				conc[m] = haloes[i].c;
#endif
				m++;
				}
					}

					conc = shellsort(conc, nHaloes_vir);
					cMax = conc[nHaloes_vir-1]; cMin = conc[0]; 	
					bin_x=lin_stepper(cMin,cMax,nBins);
					lin_bin(conc,bin_x,nBins,nHaloes_vir,int_c_bin_y);	

				norm=(nBins)/((cMax-cMin)*nHaloes_vir);
				halfstep=0.5*(bin_x[1]-bin_x[0]);

			for(i=0; i<nBins-1; i++) 
			{
				c_bin_x[i] = bin_x[i] + halfstep;
				c_bin_y[i] = norm*int_c_bin_y[i];
				c_err_y[i] = sqrt(norm*int_c_bin_y[i]);
			}	

				params = best_fit_lognorm(conc, nHaloes_vir, nBins-1, c_bin_x, c_bin_y, c_err_y);

				c_0 = params[0]; 
				sig = params[1]; 
				max=maximum(c_bin_y, nBins-1);

				HaloZ.c_0=c_0;
				HaloZ.c_sig=sig;

			for(i=0; i<nBins-1; i++)
			{	
				HaloZ.c[i]=c_bin_x[i];
				HaloZ.p_c[i]=c_bin_y[i];
			}

			for(i=0; i<nHaloes_vir; i++) 
				conc[i]/=c_0;

		for(i=0; i<nBins-1; i++)
		{	
			c_bin_x[i]/=c_0;
		}

		params2 = best_fit_lognorm(conc, nHaloes_vir, nBins-1, c_bin_x, c_bin_y, c_err_y);
		c_02 = params2[0]; 
		sig2 = params2[1];

	free(conc); 
	free(int_c_bin_y);
	free(bin_x); 
	free(c_bin_x); 
	free(c_bin_y); 
	free(c_err_y);
	free(params);
	free(params2);
	fprintf(stdout, "\n");
}


#ifdef GAS
void sort_gas_fraction()
{
	int nBins=Settings.n_bins, nHaloes=Settings.haloes_over_threshold, i=0, m=0;
	double *gas_fraction, *gas_fraction_bin, *gas_fraction_error, *mass_bin, *mass;
	double mMax = haloes[0].Mvir, mMin = haloes[nHaloes-1].Mvir;
	
	fprintf(stdout, "\nSorting halo gas fraction.\n");

	Settings.tick=0;
	mass = (double*) calloc(nHaloes, sizeof(double));	
	mass_bin = (double*) calloc(nBins, sizeof(double));	
	gas_fraction = (double*) calloc(nHaloes, sizeof(double));	
	gas_fraction_bin = (double*) calloc(nBins-1, sizeof(double));	
	gas_fraction_error = (double*) calloc(nBins-1, sizeof(double));	

		for(i=0; i<nHaloes; i++)
		{
			mass[i] = haloes[i].Mvir;
			gas_fraction[i] = haloes[i].b_fraction;
		}

			mass_bin = log_stepper(mMin, mMax, nBins);
			average_bin(mass, gas_fraction, gas_fraction_error, mass_bin, 
					gas_fraction_bin, nBins, nHaloes);

		for(i=0; i<nBins-1; i++)
		{
			HaloZ.gas_fraction[i]=gas_fraction_bin[i];
			HaloZ.mass[i]=0.5*(mass_bin[i]+mass_bin[i+1]);
		}

	free(mass);
	free(mass_bin);
	free(gas_fraction); 
	free(gas_fraction_bin); 
	free(gas_fraction_error); 
	fprintf(stdout, "\n");
}



void sort_and_fit_mass_temperature_relation()
{
 	int nBins=Settings.n_bins, nHaloes=Settings.haloes_over_threshold, i=0, m=0, *n_per_mass_bin;
	double *a, *temperature, *temperature_bin, *temperature_error, *mass_bin, *mass;
	double M_0, mMax = haloes[0].Mvir, mMin = haloes[nHaloes-1].Mvir;
	
	fprintf(stdout, "\nSorting and fitting mass temperature relation.\n");
	
	Settings.tick=0;
	a = (double*) calloc(2,sizeof(double));
	mass = (double*) calloc(nHaloes, sizeof(double));	
	mass_bin = (double*) calloc(nBins, sizeof(double));	
	temperature = (double*) calloc(nHaloes, sizeof(double));	
	temperature_bin = (double*) calloc(nBins-1, sizeof(double));	
	temperature_error = (double*) calloc(nBins-1, sizeof(double));	

		for(i=0; i<nHaloes; i++)
		{
			mass[i] = haloes[i].Mvir;
			temperature[i] = haloes[i].T_gas;
		}

			mass_bin = log_stepper(mMin, mMax, nBins);
			average_bin(mass, temperature, mass_bin, temperature_bin
					temperature_error, nBins, nHaloes);


			for(i=0; i<nBins-1; i++)
			{
				HaloZ.gas_T[i]=temperature_bin[i];
				HaloZ.mass[i]=0.5*(mass_bin[i]+mass_bin[i+1]);
				temperature_error[i] = HaloZ.gas_T[i]/sqrt(MF.n_bin[i]);
			}
	
			a[0] = 1.5; 
			a[1] = pow(10.e-14, a[0]);
	
			a=best_fit_power_law(mass_bin, temperature_bin, temperature_error, nBins-1, a);

		M_0 = -log(a[1])/a[0];
		fprintf(stdout, "M-Tx    a:%lf   M_0 10e+%f SM\n",a[0],M_0/log(10));

	free(a);
	free(mass);
	free(mass_bin);
	free(temperature); 
	free(temperature_bin); 
	free(temperature_error); 
	fprintf(stdout, "\n");
}
#endif // Gas


void compute_halo_properties()
{
		initialize_halo_properties_structure();

#ifdef VIRIAL_THEOREM
		virial_theorem();
#endif
		avg_subhalo();

		compute_numerical_mass_function();

			sort_axis_alignement();
			sort_shape_and_triaxiality();
			sort_radial_velocity();
			sort_lambda();
			sort_concentration();

#ifdef GAS
		sort_gas_fraction();
		sort_and_fit_mass_temperature_relation();
#endif
}


