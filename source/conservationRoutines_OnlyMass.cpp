/* This is the source file which contains the subroutines necessary for enforcing conservation on the
 * collision operator
 *
 * Functions included: sinc, solveWithCCt, conserveAllMoments, createCCtAndPivot
 *
 */

#include "conservationRoutines.h"																		// conservationRoutines.h is where the prototypes for the functions contained in this file are declared

#ifdef MassConsOnly																						// only compile this file if MassConsOnly is defined and conserving mass only

extern fftw_plan p_forward; 
extern fftw_plan p_backward; 
extern fftw_complex *temp;

double sinc(double x)
{
  double result;

  if(x==0.0)
    {
      result = 1.0;
    }
  else
    {
      result = sin(x)/x;
    }

  return result;
}

void solveWithCCt(int nElem, double *b)
{
	double tmp, sum = 0.0, btmp[nElem];
	int i, j, k;
	
	for(i=0;i<nElem;i++)btmp[i]=b[i];
	
	if(nElem==2){ //specially when only have conservation of mass and energy
	  for(i=0;i<nElem;i++){
	    tmp=0.;
	    for(j=0;j<nElem;j++){
		    tmp += CCt_linear[j+i*nElem]*btmp[j];
	    }
	    b[i] = tmp;		
	  }	    
	}
	else
	{
	  for(i=0;i<nElem;i++){
	    tmp=0.;
	    for(j=0;j<nElem;j++){
		    tmp += CCt[j+i*nElem]*btmp[j];
	    }
	    b[i] = tmp;		
	  }
	}
}	

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

//MAIN CONSERVATION ROUTINE

#ifdef FullandLinear
void conserveAllMoments(fftw_complex *qHat, fftw_complex *qHat_linear) // Q^ -->Q--->conserve-->new Q^
{
	double tmp0=0., tmp1=0., tmp2=0., tmp3=0., tmp4=0., tp0=0., tp1=0.;
	int i,k_eta;	
	
	//lamb = (double *)malloc(M*sizeof(double));
	//for(i=0;i<M;i++) lamb[i]=0.;
	
	#pragma omp parallel for private(k_eta) shared(qHat, qHat_linear) reduction(+:tmp0,tmp1,tmp2,tmp3,tmp4, tp0, tp1)
	for(k_eta=0;k_eta<size_ft;k_eta++){	  
	  tmp0 += qHat[k_eta][0]*C1[0][k_eta];
	  tmp1 += qHat[k_eta][1]*C2[1][k_eta];
	  tmp2 += qHat[k_eta][1]*C2[2][k_eta];
	  tmp3 += qHat[k_eta][1]*C2[3][k_eta];
	  tmp4 += qHat[k_eta][0]*C1[4][k_eta]; 

      tp0 += qHat_linear[k_eta][0]*C1[0][k_eta];	tp1 += qHat_linear[k_eta][0]*C1[4][k_eta];	  
	}
	lamb[0]=tmp0; lamb[1]=tmp1; lamb[2]=tmp2; lamb[3]=tmp3;  lamb[4]=tmp4; 	
	solveWithCCt(M, lamb); 
	
	lamb_linear[0]=tp0; lamb_linear[1]=tp1;
	solveWithCCt(2, lamb_linear); 
	//tmp0=0.; tmp1=0.;
	#pragma omp parallel for private(k_eta) shared(lamb, qHat, qHat_linear) //reduction(+:tmp0,tmp1)
	for(k_eta=0;k_eta<size_ft;k_eta++){	  
	  qHat[k_eta][0] -= (C1[0][k_eta]*lamb[0] + C1[4][k_eta]*lamb[4]);
	  qHat[k_eta][1] -= (C2[1][k_eta]*lamb[1] + C2[2][k_eta]*lamb[2] + C2[3][k_eta]*lamb[3]);	

	  qHat_linear[k_eta][0] -= (C1[0][k_eta]*lamb_linear[0] + C1[4][k_eta]*lamb_linear[1]);
	}	

	//free(lamb); 	
}
#else
void conserveAllMoments(fftw_complex *qHat) // Q^ -->Q--->conserve-->new Q^
{
	double tmp0=0., tmp1=0., tmp2=0., tmp3=0., tmp4=0., tp0=0., tp1=0.;
	int i,k_eta;	
	
	#pragma omp parallel for private(k_eta) shared(qHat) reduction(+:tmp0,tmp1,tmp2,tmp3,tmp4, tp0, tp1)
	for(k_eta=0;k_eta<size_ft;k_eta++)							// calculate C*Q^
	{
	  tmp0 += qHat[k_eta][0]*C1[k_eta];
	}
	lamb[0]=tmp0;
	solveWithCCt(M, lamb); 										// calculate (C*C^T)^(-1)*lamb = (C*C^T)^(-1)*C*Q^ (i.e. solve (C*C^T)*X=lamb for X)
	
	#pragma omp parallel for private(k_eta) shared(lamb, qHat) //reduction(+:tmp0,tmp1)
	for(k_eta=0;k_eta<size_ft;k_eta++)							// calculate Q^ - (C^T)*(C*C^T)^(-1)*C*Q^ (the conserved Q^)
	{
	  qHat[k_eta][0] -= (C1[k_eta]*lamb[0]);
	}	
}
#endif
/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

void createCCtAndPivot()
{
	double det = 1, tmp, tmp1, tmp2;
	double a1, a2, a3;
	int i, j, k, k_eta, m1,m2,m3, iError = 0;
	int i1, i2, i3, j1, j2, j3;
	
	int pivotArray[M];
	double lapackWorkspace[M*M];
	int errinfo,nele=M,lwork=M*M;
	
	int pivotArray1[2];
	double lapackWorkspace1[4];
	int nele1=2,lwork1=4;

	#pragma omp parallel for private(a1,a2,a3,k_eta,i,j,k) shared(C1)
	for(k_eta=0;k_eta<size_ft;k_eta++){	
	  k = k_eta%N;  j = ((k_eta-k)%(N*N))/N;  i = (k_eta-k-j*N)/(N*N);
	  C1[k_eta] = sinc(L_v*eta[i])*sinc(L_v*eta[j])*sinc(L_v*eta[k]);
	  //C2[k_eta] = 0.;
	} 
	
	for(i=0;i<M;i++){
	for(j=0;j<M;j++){
		tmp = 0.;
		for(k=0;k<size_ft;k++)
		{
			tmp += C1[k]*C1[k];	// + C2[k]*C2[k];
		}
		CCt[i*M+j] = tmp;
	}
	}

	dgetrf(&nele,&nele,CCt,&nele,pivotArray,&errinfo);
	dgetri(&nele,CCt,&nele,pivotArray,lapackWorkspace,&lwork,&errinfo);
	if(myrank_mpi==0)
	{
		printf("CCt = %g \n", CCt[0]);
	}
	
	#ifdef FullandLinear
	tmp = 0.; tmp1=0.; tmp2=0.; 
	for(k=0;k<size_ft;k++) {
	   tmp += C1[0][k]*C1[0][k] + C2[0][k]*C2[0][k]; 
	   tmp1 += C1[0][k]*C1[4][k] + C2[0][k]*C2[4][k];
	   tmp2 += C1[4][k]*C1[4][k] + C2[4][k]*C2[4][k];
	}
	CCt_linear[0] = tmp; CCt_linear[1] = tmp1; CCt_linear[2] = tmp1; CCt_linear[3] = tmp2;
	dgetrf(&nele1,&nele1,CCt_linear,&nele1,pivotArray1,&errinfo); 
	dgetri(&nele1,CCt_linear,&nele1,pivotArray1,lapackWorkspace1,&lwork1,&errinfo);
	#endif
	
	/*printf("\n");
	for(i=0;i<M;i++){
	for(j=0;j<M;j++){		
		printf("%g ", CCt[i*M+j]);
	}
	printf("\n");
	}*/

}

/*$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$*/

#endif	/* MassConsOnly */
