/**************************************************************************
 * Copyright(c) 1998-2007, ALICE Experiment at CERN, All rights reserved. *
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

// Generator for particles according generic functions
// For high pt performance studies realistic distribution of the particles - local density in jets to be approximated
//
//
//  TF1 *   fF1Momentum;          // 1/momentum distribution function inGeV
//  TF1 *   fFPhi;                // phi distribution function in rad     - if not set flat 0-2pi used
//  TF1 *   fFTheta;              // theta distribution function in rad   - if not set flat pi/4-3pi/4 used
//  TF3 *   fFPosition;           // position distribution function in cm - TO FIX if not set 0 used ()
//  TF1 *   fFPdg;                // pdg distribution function            - if not set flat jet pdg used
//  We assume that the moment, postion and PDG code of particles are independent
//  Only tracks/particle crossing the reference radius at given z range
//
// Origin: marian.ivanov@cern.ch


/*
  To test generator for particular setting run
  AliGenPerformance::TestAliGenPerformance(Int_t nEvents, TF1 *f1pt, TF1 *fpdg){
  For distribution of
    fF1Momentum=new TF1("f1pt","1-10*x",0,0.1);
    AliGenPerformance::TestAliGenPerformance(1000, fF1Momentum,0)
    pt distribution of charged primary particles described by powerlaw with slope -1.7

  gSystem->Load("libpythia6");
  gSystem->Load("libEGPythia6");
  gSystem->Load("liblhapdf");
  gSystem->Load("libAliPythia6");
  //
  AliGenPerformance::TestAliGenPerformance(5000,0,0);
  TFile * f = TFile::Open("testAliGenPerformance.root");
  testGener.Draw("pt>>his(100,10,100)","charge!=0&&abs(fKF)<2000","");

*/




#include <TParticle.h>
#include <TF1.h>
#include <TF3.h>
#include <TDatabasePDG.h>
#include <TParticlePDG.h>
#include "TPDGCode.h"

#include "AliRun.h"
#include "AliLog.h"
#include "AliESDtrack.h"
#include "AliESDVertex.h"
#include "AliGenPerformance.h"
#include "AliGenEventHeader.h"
#include "TMCParticle.h"
#include <AliPythia.h>
#include <TTreeStream.h>
#include <TChain.h>
#include "AliSysInfo.h"


ClassImp(AliGenPerformance)

//-----------------------------------------------------------------------------
AliGenPerformance::AliGenPerformance():
  AliGenerator(),
  fNJets(1),                // mean number of jets per event
  fF1Momentum(0),           // momentum distribution function
  fFPhi(0),                 // phi distribution function
  fFTheta(0),               // theta distribution function
  fFPosition(0),            // position distribution function
  fFPdg(0),                 // pdg distribution function
  fStreamer(0),           // test stream - used for tuning of parameters of generator
  fVerboseLevel(0)            // verbose level
{
  //
  // Default constructor
  //
  SetNumberParticles(1);
}
//-----------------------------------------------------------------------------
AliGenPerformance::AliGenPerformance(const char* generName, Int_t verboseLevel):
        AliGenerator(),
        fNJets(1),                // mean number of jets per event
        fF1Momentum(0),           // momentum distribution function
        fFPhi(0),                 // phi distribution function
        fFTheta(0),               // theta distribution function
        fFPosition(0),            // position distribution function
        fFPdg(0),                 // pdg distribution function
        fStreamer(0),           // test stream - used for tuning of parameters of generator
        fVerboseLevel(0)            // verbose level
{
    //
    // Default constructor
    //
    SetName(generName);
    fVerboseLevel=verboseLevel;
    if (fVerboseLevel >0) fStreamer = new TTreeSRedirector(TString::Format("%s.root",generName).Data(),"recreate");
}




AliGenPerformance::AliGenPerformance(const AliGenPerformance& func):
        AliGenerator(),
        fNJets(func.fNJets),                   //
        fF1Momentum(func.fF1Momentum),           // momentum distribution function
        fFPhi(func.fFPhi),                     // phi distribution function
        fFTheta(func.fFTheta),                 // theta distribution function
        fFPosition(func.fFPosition),           // position distribution function
        fFPdg(func.fFPdg)                     // pdg distribution function
{
    // Copy constructor
    SetNumberParticles(1);
}

AliGenPerformance & AliGenPerformance::operator=(const AliGenPerformance& func)
{
    // Assigment operator
    if(&func == this) return *this;
    fNJets      = func.fNJets;
    fF1Momentum = func.fF1Momentum;
    fFPhi       = func.fFPhi;
    fFTheta     = func.fFTheta;
    fFPosition  = func.fFPosition;
    fFPdg       = func.fFPdg;
    return *this;
}

AliGenPerformance::~AliGenPerformance(){
    //
    //
    //
    if (fStreamer) delete fStreamer;

}


//-----------------------------------------------------------------------------
void AliGenPerformance::Generate() {
  //
  // Performance generator
  //
  const Int_t kStackSize=10000;
  Int_t naccepted = 0;
  Int_t njets = gRandom->Poisson(fNJets);
  TDatabasePDG *databasePDG = TDatabasePDG::Instance();
  const Int_t nGeant = 14;
  Int_t geantPartiles[nGeant] = {kElectron, kPositron, kMuonMinus, kMuonPlus, kKMinus, kKPlus, kPiPlus, kPiMinus,
                                 kProton, kProtonBar, kK0Short, kK0Long, kLambda0, kLambda0Bar};
  Int_t noGeant[1]={91};
  Int_t waterMark=0;
  for (Int_t iparton = 0; iparton < njets; iparton++) {
    //
    //
    //
    Float_t mom[3];
    Float_t posf[3];
    Double_t pos[3];
    Int_t pdg;
    Double_t ptot, pt, phi, theta;
    //
    if (fF1Momentum) {
      ptot = 1. / fF1Momentum->GetRandom();
    } else {
      ptot = 0.001 + gRandom->Rndm() * 0.2;
      ptot /= ptot;
    }
    if (fFPhi) {
      phi = fFPhi->GetRandom();
    } else {
      phi = gRandom->Rndm() * TMath::TwoPi();
    }
    if (fFTheta) {
      theta = fFTheta->GetRandom();
    } else {
      theta = (gRandom->Rndm() - 0.5) * TMath::Pi() * 0.5 + TMath::Pi() / 2;
    }
    pt = ptot * TMath::Sin(theta);
    mom[0] = pt * TMath::Cos(phi);
    mom[1] = pt * TMath::Sin(phi);
    mom[2] = ptot * TMath::Cos(theta);
    //Set vertex
    pos[0] = fOrigin[0];
    pos[1] = fOrigin[1];
    pos[2] = fOrigin[2];
    //
    if (fFPosition) { // use parameterization if specified
      fFPosition->GetRandom3(pos[0], pos[1], pos[2]);
    }else{  /// use external vertex
      Vertex();
      for (Int_t j=0; j < 3; j++) pos[j] = fVertex[j];
    }
    //
    posf[0] = pos[0];
    posf[1] = pos[1];
    posf[2] = pos[2];
    if (fFPdg) {
      pdg = TMath::Nint(fFPdg->GetRandom());
    } else {
      pdg = 1 + TMath::Nint(gRandom->Rndm() * 5.);
    }
    Float_t polarization[3] = {0, 0, 0};
    Int_t nPart;
    //
    AliPythia *py = AliPythia::Instance();
    TParticlePDG *mParticle = databasePDG->GetParticle(pdg);
    if (mParticle == NULL) continue;
    Double_t mass = mParticle->Mass();
    Double_t energy = TMath::Sqrt(mom[0] * mom[0] + mom[1] * mom[1] + mom[2] * mom[2] + mass * mass);
    py->Py1ent(-1, -pdg, energy, theta, phi);
    py->Py1ent(2, pdg, energy, theta + TMath::Pi(), phi);
    py->SetMINT(51, 0);
    Int_t it = 0;
    for (it = 0; it < 2; it++) { // loop used for debuging purposes - later  to be used to enhance particle fraction
      py->Pyexec();
      if (py->GetN() > 2) break;
      py->SetMINT(51, 0);
      py->Py1ent(-1, -pdg, energy, theta, phi);
      py->Py1ent(2, pdg, energy, theta + TMath::Pi(), phi);
      AliSysInfo::AddStamp("test", iparton, it);
    }
    if (fVerboseLevel >= 0) {
      // debug output - Tol solve how to propagate Debug/Verbose level using AliDPG code - use Env varaible
      py->Pylist(0);
      py->Pylist(1);
    }
    TObjArray *array = py->GetPrimaries();
    Int_t nParticles = array->GetEntries();
    //array->Print();
    TArrayI pLabel(nParticles);        //  particle label on stack
    TArrayI isGeantFlag(nParticles);   //  indicator particle track by GEANT

    for (Int_t iParticle = 0; iParticle < nParticles; iParticle++) {
      pLabel[iParticle]=-1;
      isGeantFlag[iParticle]=0;
      TMCParticle *mcParticle = (TMCParticle *) array->At(iParticle);
      Int_t flavour = mcParticle->GetKF();
      mom[0] = mcParticle->GetPx();
      mom[1] = mcParticle->GetPy();
      mom[2] = mcParticle->GetPz();
      posf[0] = pos[0];
      posf[1] = pos[1];
      posf[2] = pos[2];
      posf[0] += mcParticle->GetVx();
      posf[1] += mcParticle->GetVy();
      posf[2] += mcParticle->GetVz();
      TParticlePDG *pdgParticle = databasePDG->GetParticle(flavour);
      Int_t pythiaParent = mcParticle->GetParent() - 1;

      Int_t decayFlag = (iParticle < 2) ? 1 : 11;
      pLabel[iParticle] = -1;
      if (pythiaParent == iParticle) {
        decayFlag = 1;
        pythiaParent = -1;
      }
      Int_t stackParent = -1;
      Bool_t isOK = kTRUE;
      if (pythiaParent >= 0 && pythiaParent < nParticles) {
        stackParent = pLabel[pythiaParent];
        if (isGeantFlag[pythiaParent]) {
          isGeantFlag[iParticle] = kTRUE;
        }
      }
      //
      if (mcParticle->GetEnergy() < mcParticle->GetMass()) {
        if (fVerboseLevel > 0) {
          ::Error("AliGenPerformance::Generate", "Unphysical particle %d", flavour);
        }
        isOK = kFALSE;
      }
      if (pythiaParent == iParticle) {
        if (fVerboseLevel > 0) {
          ::Error("AliGenPerformance::Generate", "Incorrect particle ID parent=this  %d", pythiaParent);
        }
        isOK = kFALSE;
      }
      Int_t done = (mcParticle->GetFirstChild() >= 0);  // indicator particle was decayed using Pythia -sign it done

      if ((fVerboseLevel & kFastOnly) == 0 && pdgParticle != NULL) {
        // Missing info in order to apply reweighting
        // 1.) validate mother/daughter relationship
        // 2.) validate position distribution (like in AliGenCorrHF::LoadTracks)
        // Note : code fr the mother/daughter inspired by the  AliGenCorrHF::LoadTracks
        TMCProcess type = (stackParent >= 0) ? kPDecay : kPPrimary;
        if (isOK) {
          if (isGeantFlag[iParticle] == kFALSE) {
            Float_t isGeant = 0;
            for (Int_t i = 0; i < nGeant; i++) if (flavour == geantPartiles[i]) isGeant = 1;
            isGeantFlag[iParticle] = isGeant;
            if (isGeant) {
              if (pythiaParent>=0 && pLabel[pythiaParent]<0){
                AliError("Inconsistent labels");
                stackParent=-1;
              }
              PushTrack(1, stackParent, flavour, mom, posf, polarization, 0, type, nPart, 1., decayFlag);
              //PushTrack(1, -1, flavour, mom, posf, polarization, 0, type, nPart, 1., decayFlag);
              KeepTrack(nPart);
              waterMark=nPart;
              pLabel[iParticle] = nPart;
            } else {
              if (pythiaParent>=0 &&pLabel[pythiaParent]<0){
                AliError("Inconsistent labels");
                stackParent=-1;
              }
              if (flavour==92) flavour=1;  ///HACK to check bad flavours for AliRoot - not fixed problem
              PushTrack(0, stackParent, flavour, mom, posf, polarization, 0, type, nPart, 1., decayFlag);
              //PushTrack(1, -1, flavour, mom, posf, polarization, 0, type, nPart, 1., decayFlag);
              KeepTrack(nPart);
              waterMark=nPart;
              pLabel[iParticle] = nPart;
              if (stackParent >= 0) {
                if (databasePDG->GetParticle(stackParent) != NULL) KeepTrack(stackParent);
              }
            }
          }
          //fNprimaries++;
        }
      }
      if (fStreamer) {
        if (pdgParticle) {
          Double_t charge = pdgParticle->Charge();
          Double_t mass = pdgParticle->Mass();
          Double_t pt = TMath::Sqrt(
                  mcParticle->GetPx() * mcParticle->GetPx() + mcParticle->GetPy() * mcParticle->GetPy());
          (*fStreamer) << "testGener" << "isOK=" << isOK << "done=" << done << "njets=" << njets << "ptot=" << ptot
                       << "theta=" << theta << "phi=" << phi << "pdg=" << pdg << "charge=" << charge << "mass=" << mass
                       << "pt=" << pt << "nParticles=" << nParticles << "ipart=" << iParticle << "mcParticle.="
                       << mcParticle << "\n";
        }
      }
      naccepted++;
    }
  }
  SetHighWaterMark(waterMark);
  // Passes header either to the container or to gAlice
  AliGenEventHeader *header = new AliGenEventHeader("THn");
  if (fContainer) {
    header->SetName(fName);
    fContainer->AddHeader(header);
  } else {
    gAlice->SetGenEventHeader(header);
  }

  if (fStreamer) {   // in standard simulation destructor of the streamer (and file->Close() is not called - force writing)
    ((*fStreamer) << "testGener").GetTree()->Write();
    fStreamer->GetFile()->Flush();
  }
  if ((fVerboseLevel & kStreamEvent) > 0 && fStreamer) {
    TString name = fStreamer->GetFile()->GetName();
    delete fStreamer;
    fStreamer = new TTreeSRedirector(name.Data(), "update");
  }
  return;
}


//-----------------------------------------------------------------------------
void AliGenPerformance::Init()
{
  //
  // Initialisation, check consistency of selected ranges
  //


  printf("************ AliGenPerformance ****************\n");
  printf("************************************************\n");
  if (!fF1Momentum){
    AliInfo("Momentum distribution function not specified");
  }
  if (!fFPhi){
    AliInfo("phi distribution function not specified");
  }
  if (!fFTheta){
    AliInfo("Theta distribution function not specified");
  }
  if (!fFPosition){
    AliInfo("Position distribution function not specified");
  }
  if (!fFPdg){
    AliInfo("PDG distribution function not specified");
  }

  return;
}

void AliGenPerformance::SetFunctions(TF1 * momentum, TF1 *fphi, TF1 *ftheta,TF3 * position, TF1* pdg){
  //
  // Set the function
  //
  fF1Momentum = momentum;
  fFPhi = fphi;
  fFTheta = ftheta;
  fFPosition = position;
  fFPdg = pdg;
}

void AliGenPerformance::TestAliGenPerformance(Int_t nEvents, TF1 *f1pt, TF1 *fpdg){
  //
  // test the genrator class - write particle  to the tree
  //
  AliGenPerformance *genPerformance= new AliGenPerformance;
  genPerformance->SetNJets(1);
  if (!f1pt) f1pt = new TF1("f1pt","1-10*x",0,0.1);
  if (!fpdg) fpdg = new TF1("f1pt","x",1,6);
  genPerformance->SetFunctions(f1pt,0,0,0,fpdg);
  TTreeSRedirector*pcstream = new TTreeSRedirector("testAliGenPerformance.root","recreate");
  genPerformance->SetStreamer(pcstream);
  for (Int_t i=0; i<nEvents;i++){
    genPerformance->Generate();
  }
  delete pcstream;
}



TChain *  AliGenPerformance::MakeKineChain(){
  //
  // Make chain of Kinematic trees
  // input list should be provided as a text files kinematics.list
  //
  TChain *chain = new TChain("xxx","xxx");
  TObjArray *kineArray = gSystem->GetFromPipe("cat kinematics.list").Tokenize("\n");
  Int_t nFiles=kineArray->GetEntries();
  for (Int_t iFile=0; iFile<nFiles; iFile++){
    TFile * fkine = TFile::Open(kineArray->At(iFile)->GetName());
    TList * kineList = fkine->GetListOfKeys();
    for (Int_t iKey=0; iKey<kineList->GetEntries();iKey++){
      chain->AddFile(kineArray->At(iFile)->GetName(), TChain::kBigNumber, TString::Format("%s/TreeK",kineList->At(iKey)->GetName()).Data());
    }
  }
  return chain;
  /*
    // using following lines properties of mother particles and daughter particles can be correlated
    // Unfortuantelly to do it fix in the TChain needed.
    //    1.) problem with aliases (fix prepared)
    //    2.)
    TChain * chainMother = AliGenPerformance::MakeKineChain();
    TChain * chainDaughter = AliGenPerformance::MakeKineChain();
    chainMother->SetAlias("index0","(Particles.GetDaughter(0)+1000000*This->GetTreeNumber()+0)");
    chainDaughter->SetAlias("index0","(This.GetTree().GetReadEntry()+1000000*This->GetTreeNumber()+0)");
    //
    chainMother->BuildIndex("index0");
    chainDaughter->BuildIndex("index0");
    chainMother->AddFriend(chainDaughter,"D");
  */
}
