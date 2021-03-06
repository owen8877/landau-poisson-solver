/* This is the source file which contains the subroutines necessary for calculating the electric potential and the
 * corresponding field, as well as any integrals which involve the field for the sake of the collisionless advection
 * problem.
 *
 * Functions included: rho_x, rho, computePhi_x_0, computePhi_x_0, computePhi, PrintFieldLoc, PrintFieldData, computeC_rho, Int_Int_rho
 * Int_Int_rho1st, Int_fE, Int_E, Int_E1st, Int_E2nd,
 *
 */

#include "FieldCalculations.h"																					// FieldCalculations.h is where the prototypes for the functions contained in this file are declared

// COMMON FUNCTIONS FOR BOTH UNIFORM AND NON-UNIFORM DOPING PROFILES (When Doping = False or True):

double rho_x(double x, double *U, int i) // for x in I_i
{
  int j, k;
  double tmp=0.;
  //#pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+1]*(x-Gridx((double)i))/dx + U[k*6+5]/4.;
  }
  
  return dv*dv*dv*tmp;
}

double rho(double *U, int i) //  \int f(x,v) dxdv in I_i * K_j
{
  int j, k;
  double tmp=0.;
 // #pragma omp parallel for shared(U) reduction(+:tmp)
  for(j=0;j<size_v;j++){
	k=i*size_v + j;
	tmp += U[k*6+0] + U[k*6+5]/4.;
  }
  
  return dx*dv*dv*dv*tmp;
}

void PrintFieldLoc(FILE *phifile, FILE *Efile)							// function to print the values of x & v1 which the marginal will be evaluated at in the first two rows of the file with tag margfile (subsequent rows will be the values of the marginal at given timesteps)
{
	double x_0, x_val, ddx;												// declare x_0 (the x value at the left edge of a given cell), x_val (the x value to be evaluated at) & ddx (the space between x values)
	int np = 4;															// declare np (the number of points to evaluate in a given space cell) and set its value

	ddx = dx/np;														// set ddx to the space cell width divided by np
	for(int i=0; i<Nx; i++)
	{
		x_0 = Gridx((double)i - 0.5);									// set x_0 to the value of x at the left edge of the i-th space cell
		for(int nx=0; nx<np; nx++)
		{
			x_val = x_0 + nx*ddx;										// set x_val to x_0 plus nx increments of width ddx
			fprintf(phifile, "%11.8g  ", x_val);						// in the file tagged as phifile, print the x coordinate
			fprintf(Efile, "%11.8g  ", x_val);							// in the file tagged as Efile, print the x coordinate
		}
	}
	fprintf(phifile, "\n");												// print a new line in the file tagged as phifile
	fprintf(Efile, "\n");												// print a new line in the file tagged as Efile
}

double computeC_rho(double *U, int i) // sum_m=0..i-1 int_{I_m} rho(z)dz   (Calculate integral of rho_h(z,t) from 0 to x as in eq. 53)
{
	double retn=0.;
	int j, k, m;
	for (m=0;m<i;m++){ //BUG: was "m < i-1"
		for(j=0;j<size_v;j++){
			k = m*size_v + j;
			retn += U[k*6+0] + U[k*6+5]/4.;
		}
	}

	retn *= dx*scalev;
	return retn;
}

double Int_Int_rho(double *U, int i) // \int_{I_i} [ \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
  int j, k;
  double retn=0.;
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
  }

  return retn*dx*dx*scalev;
}

double Int_Int_rho1st(double *U, int i)// \int_{I_i} [(x-x_i)/delta_x * \int^{x}_{x_i-0.5} rho(z)dz ] dx
{
 int j, k;
 double retn=0.;
 for(j=0;j<size_v;j++){
    k=i*size_v + j;
    retn += (U[k*6+0] + U[k*6+5]/4.)/12.;
  }
  return retn*dx*dx*scalev;
}

/*double Int_Cumulativerho(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ] dx
{
  double retn=0., cp, tmp;
  cp = computeC_rho(U,i);
  tmp = Int_Int_rho(U,i);

  retn = dx*cp + tmp;
  return retn;
}

double Int_Cumulativerho_sqr(double **U, int i)// \int_{I_i} [ \int^{x}_{0} rho(z)dz ]^2 dx
{
  double retn=0., cp, tmp1, tmp2, tmp3, c1=0., c2=0.;
  int j, k;
  cp = computeC_rho(U,i);
  tmp1 = cp*cp*dx;
  tmp2 = 2*cp*Int_Int_rho(U,i);
  for(j=0;j<size_v;j++){
    k=i*size_v + j;
    c1 += U[k][0] + U[k][5]/4.;
    c2 += U[k][1];
  }
  c2 *= dx/2.;
  tmp3 = pow(dv, 6)* ( c1*c1*dx*dx*dx/3. + c2*c2*dx/30. + c1*c2*(dx*Gridx((double)i)/6. - dx*dx/4.) );
  retn = tmp1 + tmp2 + tmp3;
  return retn;
}*/

double Int_fE(double *U, int i, int j) // \int f * E(f) dxdv on element I_i * K_j
{
	double retn=0.;
	int k;
	k = i*size_v + j;

	//retn = (U[k][0] + U[k][5]/4.)*Int_E(U,i) + U[k][1]*Int_E1st(U,i);
	retn = (U[k*6+0] + U[k*6+5]/4.)*intE[i] + U[k*6+1]*intE1[i];
	return retn*scalev;
}

double computePhi_x_0(double *U) 																				// wrapper for function to compute the constant coefficient of x in phi, which is actually phi_x(0) (Calculate C_E in the paper -between eq. 52 & 53?)
{
	if(Doping)
	{
		return computePhi_x_0_Doping(U);
	}
	else
	{
		return computePhi_x_0_Normal(U);
	}
}

double computePhi(double *U, double x, int ix)																	// wrapper for function to compute the potential Phi at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	if(Doping)
	{
		return computePhi_Doping(U, x, ix);
	}
	else
	{
		return computePhi_Normal(U, x, ix);
	}
}

double computeE(double *U, double x, int ix)																	// wrapper for function to compute the field E at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	if(Doping)
	{
		return computeE_Doping(U, x, ix);
	}
	else
	{
		//return computeE_Normal(U, x, ix);
	}
}

void PrintFieldData(double* U_vals, FILE *phifile, FILE *Efile)													// wrapper for function to print the values of the potential and the field in the x1 & x2 directions in the file tagged as phifile, Ex1file & Ex2file, respectively, at the given timestep
{
	if(Doping)
	{
		PrintFieldData_Doping(U_vals, phifile, Efile);
	}
	else
	{
		PrintFieldData_Normal(U_vals, phifile, Efile);
	}
}

double Int_E(double *U, int i) 		 						      												// wrapper for function to calculate the integral of E_h w.r.t. x over the interval I_i = [x_(i-1/2), x_(i+1/2))
{
	if(Doping)
	{
		return Int_E_Doping(U, i);
	}
	else
	{
		return Int_E_Normal(U, i);
	}
}

double Int_E1st(double *U, int i) 																				// wrapper for function to calculate \int_i E*(x-x_i)/delta_x dx
{
	if(Doping)
	{
		return Int_E1st_Doping(U, i);
	}
	else
	{
		return Int_E1st_Normal(U, i);
	}
}

double Int_E2nd(double *U, int i) 																				// wrapper for function to calculate \int_i E* [(x-x_i)/delta_x]^2 dx
{
	if(Doping)
	{
		return Int_E2nd_Doping(U, i);
	}
	else
	{
		return Int_E2nd_Normal(U, i);
	}
}

// REGULAR SUBROUTINES WITH UNIFORM DOPING PROFILE (When Doping = False):

double computePhi_x_0_Normal(double *U) // compute the constant coefficient of x in phi, which is actually phi_x(0) (Calculate C_E in the paper -between eq. 52 & 53?)
{
	int i, j, k, m, q;
	double tmp=0.;

	//#pragma omp parallel for private(j,q,m,k) shared(U) reduction(+:tmp) //reduction may change the final result a little bit
	for(j=0;j<size_v;j++){
		//j = j1*Nv*Nv + j2*Nv + j3;
		for(q=0;q<Nx;q++){
			for(m=0;m<q;m++){
				k=m*size_v + j;
				tmp += U[k*6] + U[k*6+5]/4.;
			}
			k=q*size_v + j;
			tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
		}
	}
	tmp = tmp*scalev*dx*dx;

	return 0.5*Lx - tmp/Lx;
}

double computePhi_Normal(double *U, double x, int ix)									// function to compute the potential Phi at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	int i_out, i, j1, j2, j3, iNNN, j1NN, j2N, k;										// declare counters i_out (for the outer sum of i values), i, j1, j2, j3 for summing the contribution from cell I_i x K_(j1, j2, j3), iNNN (the value of i*Nv^3), j1NN (the value of j1*Nv^2), j2N (the value of j2*N) & k (the location in U of the cell I_i x K_(j1, j2, j3))
	double retn, sum1, sum3, sum4, x_diff, x_diff_mid, x_diff_sq, x_eval, C_E;			// declare retn (the value of Phi returned at the end), sum1 (the value of the first two sums), sum3 (the value of the third sum), sum4 (the value of the fourth sum), x_diff (the value of x - x_(ix-1/2)), x_diff_mid (the value of x - x_ix), x_diff_sq (the value of x_diff^2), x_eval (the value associated to the integral of (x - x_i)^2) & C_E (the value of the constant in the formula for phi)
	sum1 = 0;
	sum3 = 0;
	sum4 = 0;
	retn = 0;
	x_diff = x - Gridx(ix-0.5);
	x_diff_mid = x - Gridx(ix);
	x_diff_sq = x_diff*x_diff;
	x_eval = x_diff_mid*x_diff_mid*x_diff_mid/(6.*dx) - dx*x_diff_mid/8. - dx*dx/24.;

	for(i_out = 0; i_out < ix; i_out++)
	{
		for(i = 0; i < i_out; i++)
		{
			iNNN = i*Nv*Nv*Nv;
			for(j1 = 0; j1 < Nv; j1++)
			{
				j1NN = j1*Nv*Nv;
				for(j2 = 0; j2 < Nv; j2++)
				{
					j2N = j2*Nv;
					for(j3 = 0; j3 < Nv; j3++)
					{
						k = iNNN + j1NN + j2N + j3;
						sum1 += U[6*k] + U[6*k+5]/4.;
					}
				}
			}
		}
		iNNN = i_out*Nv*Nv*Nv;
		for(j1 = 0; j1 < Nv; j1++)
		{
			j1NN = j1*Nv*Nv;
			for(j2 = 0; j2 < Nv; j2++)
			{
				j2N = j2*Nv;
				for(j3 = 0; j3 < Nv; j3++)
				{
					k = iNNN + j1NN + j2N + j3;
					sum1 += U[6*k]/2 - U[6*k+1]/12. +  U[6*k+5]/8;
				}
			}
		}
	}
	sum1 = sum1*dx*dx;
	for(i = 0; i < i_out; i++)
	{
		iNNN = i*Nv*Nv*Nv;
		for(j1 = 0; j1 < Nv; j1++)
		{
			j1NN = j1*Nv*Nv;
			for(j2 = 0; j2 < Nv; j2++)
			{
				j2N = j2*Nv;
				for(j3 = 0; j3 < Nv; j3++)
				{
					k = iNNN + j1NN + j2N + j3;
					sum3 += (U[6*k] + U[6*k+5]/4.);
				}
			}
		}
	}
	sum3 = sum3*dx*x_diff;
	iNNN = ix*Nv*Nv*Nv;
	for(j1 = 0; j1 < Nv; j1++)
	{
		j1NN = j1*Nv*Nv;
		for(j2 = 0; j2 < Nv; j2++)
		{
			j2N = j2*Nv;
			for(j3 = 0; j3 < Nv; j3++)
			{
				k = iNNN + j1NN + j2N + j3;
				sum4 += U[6*k]*x_diff_sq/2. + U[6*k+1]*x_eval +  U[6*k+5]*x_diff_sq/8.;
			}
		}
	}

	C_E = computePhi_x_0(U);
	retn = (sum1 + sum3 + sum4)*dv*dv*dv - x*x/2 - C_E*x;
	return retn;
}

void PrintFieldData_Normal(double* U_vals, FILE *phifile, FILE *Efile)							// function to print the values of the potential and the field in the x1 & x2 directions in the file tagged as phifile, Ex1file & Ex2file, respectively, at the given timestep
{
	double x_0, x_val, phi_val, E_val, ddx;								// declare x_0 (the x value at the left edge of a given cell), x_val (the x value to be evaluated at), phi_val (the value of phi evaluated at x_val), E_val (the value of E evaluated at x_val) & ddx (the space between x values)
	int np = 4;															// declare np (the number of points to evaluate in a given space cell) and set its value

	ddx = dx/np;														// set ddx to the space cell width divided by np
	for(int i=0; i<Nx; i++)
	{
		x_0 = Gridx((double)i - 0.5);									// set x_0 to the value of x at the left edge of the i-th space cell
		for (int nx=0; nx<np; nx++)
		{
			x_val = x_0 + nx*ddx;										// set x_val to x_0 plus nx increments of width ddx

			phi_val = computePhi(U_vals, x_val, i);							// calculate the value of phi, evaluated at x_val by using the function in the space cell i
//			rho_val = rho_x(x_val, U, i);														// calculate the value of rho, evaluated at x_val by using the function in the space cell
//			M_0 = rho_val/(sqrt(1.8*PI));
//			E_val = computeE(U_vals, x_val, i);								// calculate the value of E, evaluated at x_val by using the function in the space cell i
			fprintf(phifile, "%11.8g ", phi_val);						// in the file tagged as phifile, print the value of the potential phi(t, x_val)
//			fprintf(Efile, "%11.8g ", E_val);							// in the file tagged as Efile, print the value of the field E(t, x_val)
		}
	}
	fprintf(phifile, "\n");												// in the file tagged as phifile, print a new line
//	fprintf(Efile, "\n");												// in the file tagged as Efile, print a new line
}

double Int_E_Normal(double *U, int i) // \int_i E dx      // Function to calculate the integral of E_h w.r.t. x over the interval I_i = [x_(i-1/2), x_(i+1/2))
{
	int m, j, k;
	double tmp=0., result;
	//#pragma omp parallel for shared(U) reduction(+:tmp)
	for(j=0;j<size_v;j++){
		for(m=0;m<i;m++){	
			k=m*size_v + j;
			tmp += U[k*6+0] + U[k*6+5]/4.;
		}
		k=i*size_v + j;
		tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;		
	}

	//ce = computePhi_x_0(U);
	result = -ce*dx - tmp*dx*dx*scalev + Gridx((double)i)*dx;

	return result;
}

double Int_E1st_Normal(double *U, int i) // \int_i E*(x-x_i)/delta_x dx
{
	int j, k;
	double tmp=0., result;
	//#pragma omp parallel for reduction(+:tmp)
	for(j=0;j<size_v;j++){
		k=i*size_v + j;
		tmp += U[k*6+0] + U[k*6+5]/4.;
	}
	tmp = tmp*scalev;
	
	result = (1-tmp)*dx*dx/12.;

	return result;
}

double Int_E2nd_Normal(double *U, int i) // \int_i E* [(x-x_i)/delta_x]^2 dx
{
    int m, j, j1, j2, j3, k;
    double c1=0., c2=0., result;
  
    //cp = computeC_rho(U,i); ce = computePhi_x_0(U);

    for(j=0;j<size_v;j++){
	    k=i*size_v + j;
	    c1 += U[k*6+0] + U[k*6+5]/4.;
	    c2 += U[k*6+1];
    }
    c2 *= dx/2.;				
    
    result = (-cp[i] - ce+scalev*(c1*Gridx(i-0.5) + 0.25*c2))*dx/12. + (1-scalev*c1)*dx*Gridx((double)i)/12. - scalev*c2*dx/80.; //BUG: missed -cp

    return result;
}

// SUBROUTINES WITH NON-UNIFORM DOPING PROFILE (When Doping = True):

double DopingProfile(int i)																		// function to return a step function doping profile, based on what cell a given x is in
{
	double DP;																					// declare DP (the value of the doping profile in the current space cell to be returned)
	if(i <= a_i || i > b_i)																// if the space cell i is either in the lower or upper third of all cells then set the value of DP to 1
	{
		DP = NH;
	}
	else																						// if the space cell i is either in the middle third of all cells then set the value of DP to 0.1
	{
		DP = NL;
	}
	return DP;																					// return the value of DP
}

double computePhi_x_0_Doping(double *U) /* DIFFERENT FOR withND */																// compute the constant coefficient of x in phi, which is actually phi_x(0) (Calculate C_E in the paper -between eq. 52 & 53?), with doping profile given by ND above
{
	int i, j, k, m, q;
	double tmp=0.;
	double a_val = (a_i+1)*dx;
	double b_val = (b_i+1)*dx;
	double Phi_Lx = 1;																									// declare Phi_Lx (the Dirichlet BC, Phi(t, L_x) = Phi_Lx) and set its value

	//#pragma omp parallel for private(j,q,m,k) shared(U) reduction(+:tmp) //reduction may change the final result a little bit
	for(j=0;j<size_v;j++){
		//j = j1*Nv*Nv + j2*Nv + j3;
		for(q=0;q<Nx;q++){
			for(m=0;m<q;m++){
				k=m*size_v + j;
				tmp += U[k*6] + U[k*6+5]/4.;
			}
			k=q*size_v + j;
			tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
		}
	}
	tmp = tmp*scalev*dx*dx;

	return Phi_Lx/Lx + 0.5*NH*Lx/eps + (NL-NH)*(b_val-a_val)/eps - (0.5*(NL-NH)*(b_val*b_val - a_val*a_val) + tmp)/(Lx*eps);
}

double computePhi_Doping(double *U, double x, int ix)	/* DIFFERENT FOR withND */											// function to compute the potential Phi at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	int i_out, i, j1, j2, j3, iNNN, j1NN, j2N, k;										// declare counters i_out (for the outer sum of i values), i, j1, j2, j3 for summing the contribution from cell I_i x K_(j1, j2, j3), iNNN (the value of i*Nv^3), j1NN (the value of j1*Nv^2), j2N (the value of j2*N) & k (the location in U of the cell I_i x K_(j1, j2, j3))
	double retn, sum1, sum3, sum4, x_diff, x_diff_mid, x_diff_sq, x_eval, C_E;			// declare retn (the value of Phi returned at the end), sum1 (the value of the first two sums), sum3 (the value of the third sum), sum4 (the value of the fourth sum), x_diff (the value of x - x_(ix-1/2)), x_diff_mid (the value of x - x_ix), x_diff_sq (the value of x_diff^2), x_eval (the value associated to the integral of (x - x_i)^2) & C_E (the value of the constant in the formula for phi)
	double ND;																			// declare ND (the value of the doping profile at the given x)
	sum1 = 0;
	sum3 = 0;
	sum4 = 0;
	retn = 0;
	ND = DopingProfile(ix);																// set ND to the value of the doping profile at ix
	x_diff = x - Gridx(ix-0.5);
	x_diff_mid = x - Gridx(ix);
	x_diff_sq = x_diff*x_diff;
	x_eval = x_diff_mid*x_diff_mid*x_diff_mid/(6.*dx) - dx*x_diff_mid/8. - dx*dx/24.;

	for(i_out = 0; i_out < ix; i_out++)
	{
		sum1 += computeC_rho(U, i_out);

		iNNN = i_out*Nv*Nv*Nv;
		for(j1 = 0; j1 < Nv; j1++)
		{
			j1NN = j1*Nv*Nv;
			for(j2 = 0; j2 < Nv; j2++)
			{
				j2N = j2*Nv;
				for(j3 = 0; j3 < Nv; j3++)
				{
					k = iNNN + j1NN + j2N + j3;
					sum1 += (U[6*k]/2 - U[6*k+1]/12. +  U[6*k+5]/8)*dx*scalev;
				}
			}
		}
	}
	sum1 = sum1*dx;//*dx;

	sum3 = computeC_rho(U, ix);

	sum3 = sum3*x_diff;
	iNNN = ix*Nv*Nv*Nv;
	for(j1 = 0; j1 < Nv; j1++)
	{
		j1NN = j1*Nv*Nv;
		for(j2 = 0; j2 < Nv; j2++)
		{
			j2N = j2*Nv;
			for(j3 = 0; j3 < Nv; j3++)
			{
				k = iNNN + j1NN + j2N + j3;
				sum4 += U[6*k]*x_diff_sq/2. + U[6*k+1]*x_eval +  U[6*k+5]*x_diff_sq/8.;
			}
		}
	}
	sum4 = sum4*scalev;

	C_E = computePhi_x_0(U);
	retn = sum1 + sum3 + sum4 - ND*x*x/2.;// + C_E*x;
	if(ix > a_i)																						// if x > a then there is an extra term to add
	{
		double a_val = (a_i+1)*dx;																		// declare a_val and set it to the value of x at the edge of the ai-th space cell
		retn -= (NH-NL)*a_val*(x - 0.5*a_val);															// add (NH-NL)a(x-a/2) to retn
	}
	if(ix > b_i)																						// if x > b then there is an extra term to add
	{
		double b_val = (b_i+1)*dx;																		// declare b_val and set it to the value of x at the edge of the bi-th space cell
		retn -= (NL-NH)*b_val*(x - 0.5*b_val);															// add (NL-NH)b(x-b/2) to retn
	}

	retn = retn/eps + C_E*x;
	return retn;																						// return the value of phi at x
}

double computeE_Doping(double *U, double x, int ix)	/* DIFFERENT FOR withND */								// function to compute the field E at a position x, contained in [x_(ix-1/2), x_(ix+1/2)]
{
	int iNNN, j, k;																						// declare j (a counter for summing the contribution from the velocity cells), iNNN (the value of i*Nv^3) & k (the location in U of the cell I_ix x K_(j1, j2, j3))
	double retn, sum, x_diff, x_diff_mid, x_eval, C_E;													// declare retn (the value of E returned at the end), sum (the value of the sum to calculate the integral of rho), x_diff (the value of x - x_(ix-1/2)), x_diff_mid (the value of x - x_ix), x_eval (the value associated to the integral of (x - x_i)^2) & C_E (the value of the constant in the formula for E)
	double ND;																							// declare ND (the value of the doping profile at the given x)
	sum = 0;																							// initialise the sum at 0
	ND = DopingProfile(ix);																				// set ND to the value of the doping profile at ix
	C_E = computePhi_x_0(U);
	x_diff = x - Gridx(ix-0.5);
	x_diff_mid = x - Gridx(ix);
	x_eval = x_diff_mid*x_diff_mid/(2.*dx) - dx/8.;

	iNNN = ix*size_v;
	for(j=0;j<size_v;j++)
	{
		k = iNNN + j;
		sum += U[6*k+0]*x_diff + U[6*k+1]*x_eval + U[6*k+5]*x_diff/4.;
	}
	sum = sum*scalev;
	sum += computeC_rho(U, ix);

	retn = ND*x - sum;//- C_E;
	if(ix > a_i)																						// if x > a then there is an extra term to add
	{
		double a_val = (a_i+1)*dx;																		// declare a_val and set it to the value of x at the edge of the ai-th space cell
		retn += (NH-NL)*a_val;																			// add (NH-NL)a to retn
	}
	if(ix > b_i)																						// if x > b then there is an extra term to add
	{
		double b_val = (b_i+1)*dx;																		// declare b_val and set it to the value of x at the edge of the bi-th space cell
		retn += (NL-NH)*b_val;																			// add (NL-NH)b to retn
	}

	retn = retn/eps - C_E;
	return retn;																						// return the value of phi at x

}

void PrintFieldData_Doping(double* U_vals, FILE *phifile, FILE *Efile)							// function to print the values of the potential and the field in the x1 & x2 directions in the file tagged as phifile, Ex1file & Ex2file, respectively, at the given timestep
{
	double x_0, x_val, phi_val, E_val, ddx;								// declare x_0 (the x value at the left edge of a given cell), x_val (the x value to be evaluated at), phi_val (the value of phi evaluated at x_val), E_val (the value of E evaluated at x_val) & ddx (the space between x values)
	int np = 4;															// declare np (the number of points to evaluate in a given space cell) and set its value

	ddx = dx/np;														// set ddx to the space cell width divided by np
	for(int i=0; i<Nx; i++)
	{
		x_0 = Gridx((double)i - 0.5);									// set x_0 to the value of x at the left edge of the i-th space cell
		for (int nx=0; nx<np; nx++)
		{
			x_val = x_0 + nx*ddx;										// set x_val to x_0 plus nx increments of width ddx

			phi_val = computePhi(U_vals, x_val, i);							// calculate the value of phi, evaluated at x_val by using the function in the space cell i
			E_val = computeE(U_vals, x_val, i);								// calculate the value of E, evaluated at x_val by using the function in the space cell i
			fprintf(phifile, "%11.8g ", phi_val);						// in the file tagged as phifile, print the value of the potential phi(t, x_val)
			fprintf(Efile, "%11.8g ", E_val);							// in the file tagged as Efile, print the value of the field E(t, x_val)
		}
	}
	fprintf(phifile, "\n");												// in the file tagged as phifile, print a new line
	fprintf(Efile, "\n");												// in the file tagged as Efile, print a new line
}

double Int_E_Doping(double *U, int i) 		/* DIFFERENT FOR withND */ 						      // Function to calculate the integral of E_h w.r.t. x over the interval I_i = [x_(i-1/2), x_(i+1/2))
{
	int m, j, k;
	double tmp=0., result;
	double ND;																			// declare ND (the value of the doping profile at the given x)
	ND = DopingProfile(i);																// set ND to the value of the doping profile at ix

	//#pragma omp parallel for shared(U) reduction(+:tmp)
	for(j=0;j<size_v;j++){
		for(m=0;m<i;m++){
			k=m*size_v + j;
			tmp += U[k*6+0] + U[k*6+5]/4.;
		}
		k=i*size_v + j;
		tmp += 0.5*(U[k*6+0] + U[k*6+5]/4.) - U[k*6+1]/12.;
	}

	//ce = computePhi_x_0(U);
	result = - tmp*dx*dx*scalev + ND*Gridx((double)i)*dx;// -ce*dx;
	if(i > a_i)																							// if x > a then there is an extra term to add
	{
		double a_val = (a_i+1)*dx;																		// declare a_val and set it to the value of x at the edge of the ai-th space cell
		result += (NH-NL)*a_val*dx;																		// add (NH-NL)a*dx to result
	}
	if(i > b_i)																							// if x > b then there is an extra term to add
	{
		double b_val = (b_i+1)*dx;																		// declare b_val and set it to the value of x at the edge of the bi-th space cell
		result += (NL-NH)*b_val*dx;																		// add (NL-NH)b*dx to result
	}

	result = result/eps - ce*dx;
	return result;
}

double Int_E1st_Doping(double *U, int i) 	/* DIFFERENT FOR withND */					// \int_i E*(x-x_i)/delta_x dx
{
	int j, k;
	double tmp=0., result;
	double ND;																			// declare ND (the value of the doping profile at the given x)
	ND = DopingProfile(i);																// set ND to the value of the doping profile at ix
	//#pragma omp parallel for reduction(+:tmp)
	for(j=0;j<size_v;j++){
		k=i*size_v + j;
		tmp += U[k*6+0] + U[k*6+5]/4.;
	}
	tmp = tmp*scalev;

	result = (ND-tmp)*dx*dx/(12.*eps);

	return result;
}

double Int_E2nd_Doping(double *U, int i) 	/* DIFFERENT FOR withND */							// \int_i E* [(x-x_i)/delta_x]^2 dx
{
    int m, j, j1, j2, j3, k;
    double c1=0., c2=0., result;
	double ND;																			// declare ND (the value of the doping profile at the given x)
	ND = DopingProfile(i);																// set ND to the value of the doping profile at ix

    //cp = computeC_rho(U,i); ce = computePhi_x_0(U);

    for(j=0;j<size_v;j++){
	    k=i*size_v + j;
	    c1 += U[k*6+0] + U[k*6+5]/4.;
	    c2 += U[k*6+1];
    }
    c2 *= dx/2.;

    result = (-cp[i] +scalev*(c1*Gridx(i-0.5) + 0.25*c2))*dx/12. + (ND-scalev*c1)*dx*Gridx((double)i)/12. - scalev*c2*dx/80.;// - ce*dx/12. //BUG: missed -cp

    if(i > a_i)																							// if x > a then there is an extra term to add
	{
		double a_val = (a_i+1)*dx;																		// declare a_val and set it to the value of x at the edge of the ai-th space cell
		result += (NH-NL)*a_val*dx/12.;																	// add (NH-NL)a*dx/12 to result
	}
	if(i > b_i)																							// if x > b then there is an extra term to add
	{
		double b_val = (b_i+1)*dx;																		// declare b_val and set it to the value of x at the edge of the bi-th space cell
		result += (NL-NH)*b_val*dx/12.;																	// add (NL-NH)b*dx/12 to result
	}

	result = result/eps - ce*dx/12.;

    return result;
}
