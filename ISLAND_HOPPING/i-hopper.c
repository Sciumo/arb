#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "i-hopper.h"

/*============================================================================*/

#ifndef ARB
int main(void) {

 Error=NULL;

 fprintf(stdout,"\nAligning ...");

 {
/*
  char X[]="CGACGATCCCTAGCTGGTCTGAGAGGATGACCAGCCACACTGGAACTGAGACACGGTCCAGACTCCTACGGGAGGCAGCAGTGGGGAATATTGCACAACTAGCTGGTCTGAGAGGGATGACCAGCCACACTGGTGCACTAGCTGGTCGAGAGGATGACCAGCCACACTGGTGGGCGCAAGCCTGATGCAGCCA";
  int secX[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};

  char Y[]="CGACGATCCCTAGCTGGTCTGAGAGGATGACCAGCCACACTGGAACTGAGACACGGTCCAGACTCCTACGGGAGGCAGCAGTGGGGAATATTGCACAACTAGCTGGTCTGAGAGGGATGACCAGCCACACTGGTGCACTAGCTGGTCGAGAGGATGACCAGCCACACTGGTGGGCGCAAGCCTGATGCAGCCA";
  int secY[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
*/

  char X[]="CTTCGCTTTGGATCCTTACTAGGATCTGCCTAGTACATTCAAATCTTAACAGGCTTATTTCTGTGTGGGTGTGTGTGTGAATACATTACACATCAGACACATCAACTG";
  int secX[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  char Y[]="GGTCTTTATTAGGAATATGCCTAATTATTCAAATTCTCACCGGACTATTCACACCCCAACACACACACACAATACACTACACCTCTGACACATTCACCGCCTT";
  int secY[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  int freqs=FALSE;
  double fT=0.25;
  double fC=0.25;
  double fA=0.25;
  double fG=0.25;

  int rates=FALSE;
  double rTC=4.0;
  double rTA=1.0;
  double rTG=1.0;
  double rCA=1.0;
  double rCG=1.0;
  double rAG=4.0;

  double dist=0.3;
  double supp=0.5;
  double gap=10.;
  double thres=0.005;

  char *XX=NULL;
  char *YY=NULL;

  int nX;
  int nY;

  nX=strlen(X);
  nY=strlen(Y);

  Align(
   nX,X,secX,&XX,nY,Y,secY,&YY,
   freqs,fT,fC,fA,fG,
   rates,rTC,rTA,rTG,rCA,rCG,rAG,
   dist,supp,gap,thres
  );

  if(Error) goto error;

  { FILE *fp; int i,nXX,nYY;
   nXX=strlen(XX);
   nYY=strlen(YY);
   fp=fopen("alignment.txt","w");
   for(i=0;i<nXX;i++) fprintf(fp,"%c",XX[i]); fprintf(fp,"\n");
   for(i=0;i<nYY;i++) fprintf(fp,"%c",YY[i]); fprintf(fp,"\n");
   fclose(fp);
  }

  freeBlock(&XX);
  freeBlock(&YY);

 }

 clearUp();

 return(EXIT_SUCCESS);

 error:

 fprintf(stdout,"\n!!! %s\n",Error);
 fprintf(stdout,"\nPress RETURN to exit\n");
 getc(stdin);

 clearUp();

 return(EXIT_FAILURE);

}
#endif
