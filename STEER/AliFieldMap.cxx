/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/*
$Log$
Revision 1.3  2002/02/26 16:13:07  morsch
Correction in streamer.

Revision 1.2  2002/02/22 14:00:20  morsch
Protection against replication of fieldmap data in gAlice.

Revision 1.1  2002/02/14 11:41:28  morsch
Magnetic field map for ALICE for L3+muon spectrometer stored in 3 seperate
root files.

*/

//
// Author: Andreas Morsch <andreas.morsch@cern.ch>
//

#include <TVector.h>
#include "AliFieldMap.h"
#include "TSystem.h"

ClassImp(AliFieldMap)

//________________________________________
AliFieldMap::AliFieldMap()
{
  //
  // Standard constructor
  //
  fB = 0;
  SetWriteEnable();
}

AliFieldMap::AliFieldMap(const char *name, const char *title)
    : TNamed(name,title)
{
  //
  // Standard constructor
  //
  fB = 0;
  ReadField();
  SetWriteEnable();
}

AliFieldMap::~AliFieldMap()
{
//
// Destructor
//  
  delete fB;
}

//________________________________________
AliFieldMap::AliFieldMap(const AliFieldMap &map)
{
  //
  // Copy constructor
  //
  map.Copy(*this);
}

//________________________________________
void AliFieldMap::ReadField()
{
  // 
  // Method to read the magnetic field map from file
  //
  FILE* magfile;
//  FILE* endf = fopen("end.table", "r");
//  FILE* out  = fopen("out", "w");
  
  Int_t   ix, iy, iz, ipx, ipy, ipz;
  Float_t bx, by, bz;
  char *fname = 0;
  printf("%s: Reading Magnetic Field Map %s from file %s\n",
	 ClassName(),fName.Data(),fTitle.Data());

  fname   = gSystem->ExpandPathName(fTitle.Data());
  magfile = fopen(fname,"r");
  delete [] fname;

  fscanf(magfile,"%d %d %d %f %f %f %f %f %f", 
	 &fXn, &fYn, &fZn, &fXbeg, &fYbeg, &fZbeg, &fXdel, &fYdel, &fZdel);
 
  fXdeli = 1./fXdel;
  fYdeli = 1./fYdel;	
  fZdeli = 1./fZdel;	
  fXend  = fXbeg + (fXn-1)*fXdel;
  fYend  = fYbeg + (fYn-1)*fYdel;
  fZend  = fZbeg + (fZn-1)*fZdel;
  
  Int_t nDim   = fXn*fYn*fZn;

//  Float_t x,y,z,b;

  fB = new TVector(3*nDim);
  if (magfile) {
      for (ix = 0; ix < fXn; ix++) {
	  ipx=ix*3*(fZn*fYn);
	  for (iy = 0; iy < fYn; iy++) {
	      ipy=ipx+iy*3*fZn;
	      for (iz = 0; iz < fZn; iz++) {
		  ipz=ipy+iz*3;

		  if (iz == -1) {
//		      fscanf(endf,"%f %f %f", &bx,&by,&bz);
		  } else if (iz > -1) {
		      fscanf(magfile," %f %f %f", &bx, &by, &bz);
		  } else {
		      continue;
		  }
		  
//		  fscanf(magfile,"%f %f %f %f %f %f %f ",
//			 &x, &y, &z, &bx,&by,&bz, &b);
//		  fprintf(out, "%15.8e %15.8e %15.8e \n", bx, by, bz);

		  (*fB)(ipz+2) = 10.*bz;
		  (*fB)(ipz+1) = 10.*by;
		  (*fB)(ipz  ) = 10.*bx;
	      } //iz
	  } // iy
      } // ix
/*
//
// this part for interpolation between z = 700 and 720 cm to get
// z = 710 cm
//      
      for (ix = 0; ix < fXn; ix++) {
	  ipx=ix*3*(fZn*fYn);
	  for (iy = 0; iy < fYn; iy++) {
	      ipy=ipx+iy*3*fZn;
	      Float_t bxx = (Bx(ix,iy,0) + Bx(ix,iy,2))/2.;
	      Float_t byy = (By(ix,iy,0) + By(ix,iy,2))/2.;
	      Float_t bzz = (Bz(ix,iy,0) + Bz(ix,iy,2))/2.;	      
	      ipz=ipy+3;
	      (*fB)(ipz+2) = bzz;
	      (*fB)(ipz+1) = byy;
	      (*fB)(ipz  ) = bxx;
	  } // iy
      } // ix
*/      
  } else { 
      printf("%s: File %s not found !\n",ClassName(),fTitle.Data());
      exit(1);
  } // if mafile
}

void AliFieldMap::Field(Float_t *x, Float_t *b)
{
//
// Use simple interpolation to obtain field at point x
//
    Double_t ratx, raty, ratz, hix, hiy, hiz, ratx1, raty1, ratz1, 
	bhyhz, bhylz, blyhz, blylz, bhz, blz, xl[3];
    const Double_t kone=1;
    Int_t ix, iy, iz;
    b[0]=b[1]=b[2]=0;
//
    
    xl[0]=TMath::Abs(x[0])-fXbeg;
    xl[1]=TMath::Abs(x[1])-fYbeg;
    xl[2]=x[2]-fZbeg;
    
    hix=xl[0]*fXdeli;
    ratx=hix-int(hix);
    ix=int(hix);
    
    hiy=xl[1]*fYdeli;
    raty=hiy-int(hiy);
    iy=int(hiy);
    
    hiz=xl[2]*fZdeli;
    ratz=hiz-int(hiz);
    iz=int(hiz);

    ratx1=kone-ratx;
    raty1=kone-raty;
    ratz1=kone-ratz;

    bhyhz = Bx(ix  ,iy+1,iz+1)*ratx1+Bx(ix+1,iy+1,iz+1)*ratx;
    bhylz = Bx(ix  ,iy+1,iz  )*ratx1+Bx(ix+1,iy+1,iz  )*ratx;
    blyhz = Bx(ix  ,iy  ,iz+1)*ratx1+Bx(ix+1,iy  ,iz+1)*ratx;
    blylz = Bx(ix  ,iy  ,iz  )*ratx1+Bx(ix+1,iy  ,iz  )*ratx;
    bhz   = blyhz             *raty1+bhyhz             *raty;
    blz   = blylz             *raty1+bhylz             *raty;
    b[0]  = blz               *ratz1+bhz               *ratz;
    //
    bhyhz = By(ix  ,iy+1,iz+1)*ratx1+By(ix+1,iy+1,iz+1)*ratx;
    bhylz = By(ix  ,iy+1,iz  )*ratx1+By(ix+1,iy+1,iz  )*ratx;
    blyhz = By(ix  ,iy  ,iz+1)*ratx1+By(ix+1,iy  ,iz+1)*ratx;
    blylz = By(ix  ,iy  ,iz  )*ratx1+By(ix+1,iy  ,iz  )*ratx;
    bhz   = blyhz             *raty1+bhyhz             *raty;
    blz   = blylz             *raty1+bhylz             *raty;
    b[1]  = blz               *ratz1+bhz               *ratz;
    //
    bhyhz = Bz(ix  ,iy+1,iz+1)*ratx1+Bz(ix+1,iy+1,iz+1)*ratx;
    bhylz = Bz(ix  ,iy+1,iz  )*ratx1+Bz(ix+1,iy+1,iz  )*ratx;
    blyhz = Bz(ix  ,iy  ,iz+1)*ratx1+Bz(ix+1,iy  ,iz+1)*ratx;
    blylz = Bz(ix  ,iy  ,iz  )*ratx1+Bz(ix+1,iy  ,iz  )*ratx;
    bhz   = blyhz             *raty1+bhyhz             *raty;
    blz   = blylz             *raty1+bhylz             *raty;
    b[2]  = blz               *ratz1+bhz               *ratz;
}

//________________________________________
void AliFieldMap::Copy(AliFieldMap & /* magf */) const
{
  //
  // Copy *this onto magf -- Not implemented
  //
  Fatal("Copy","Not implemented!\n");
}

//________________________________________
AliFieldMap & AliFieldMap::operator =(const AliFieldMap &magf)
{
  magf.Copy(*this);
  return *this;
}

void AliFieldMap::Streamer(TBuffer &R__b)
{
   // Stream an object of class AliFieldMap.
    TVector* save = 0;
    
    if (R__b.IsReading()) {
	AliFieldMap::Class()->ReadBuffer(R__b, this);
    } else {
	if (!fWriteEnable) {
	    save = fB;
	    fB = 0;
	}
	AliFieldMap::Class()->WriteBuffer(R__b, this);
	if (!fWriteEnable) fB = save;
    }
}
