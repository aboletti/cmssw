#include "RecoTauTag/RecoTau/interface/AntiElectronIDMVA6.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#include "DataFormats/MuonReco/interface/Muon.h"

#include "CondFormats/DataRecord/interface/GBRWrapperRcd.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include <TFile.h>
#include <array>

using namespace antiElecIDMVA6_blocks;

template <class TauType, class ElectronType>
AntiElectronIDMVA6<TauType, ElectronType>::AntiElectronIDMVA6(const edm::ParameterSet& cfg, edm::ConsumesCollector&& cc)
    : isInitialized_(false),
      mva_NoEleMatch_woGwoGSF_BL_(nullptr),
      mva_NoEleMatch_wGwoGSF_BL_(nullptr),
      mva_woGwGSF_BL_(nullptr),
      mva_wGwGSF_BL_(nullptr),
      mva_NoEleMatch_woGwoGSF_EC_(nullptr),
      mva_NoEleMatch_wGwoGSF_EC_(nullptr),
      mva_woGwGSF_EC_(nullptr),
      mva_wGwGSF_EC_(nullptr),
      mva_NoEleMatch_woGwoGSF_VFEC_(nullptr),
      mva_NoEleMatch_wGwoGSF_VFEC_(nullptr),
      mva_woGwGSF_VFEC_(nullptr),
      mva_wGwGSF_VFEC_(nullptr),
      isPhase2_(cfg.getParameter<bool>("isPhase2")),
      verbosity_(cfg.getParameter<int>("verbosity")) {
  loadMVAfromDB_ = cfg.exists("loadMVAfromDB") ? cfg.getParameter<bool>("loadMVAfromDB") : false;
  if (!loadMVAfromDB_) {
    if (cfg.exists("inputFileName")) {
      inputFileName_ = cfg.getParameter<edm::FileInPath>("inputFileName");
    } else
      throw cms::Exception("MVA input not defined")
          << "Requested to load tau MVA input from ROOT file but no file provided in cfg file";
  }

  mvaName_NoEleMatch_woGwoGSF_BL_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_woGwoGSF_BL");
  mvaName_NoEleMatch_wGwoGSF_BL_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_wGwoGSF_BL");
  mvaName_woGwGSF_BL_ = cfg.getParameter<std::string>("mvaName_woGwGSF_BL");
  mvaName_wGwGSF_BL_ = cfg.getParameter<std::string>("mvaName_wGwGSF_BL");
  mvaName_NoEleMatch_woGwoGSF_EC_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_woGwoGSF_EC");
  mvaName_NoEleMatch_wGwoGSF_EC_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_wGwoGSF_EC");
  mvaName_woGwGSF_EC_ = cfg.getParameter<std::string>("mvaName_woGwGSF_EC");
  mvaName_wGwGSF_EC_ = cfg.getParameter<std::string>("mvaName_wGwGSF_EC");
  mvaName_NoEleMatch_woGwoGSF_VFEC_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_woGwoGSF_VFEC");
  mvaName_NoEleMatch_wGwoGSF_VFEC_ = cfg.getParameter<std::string>("mvaName_NoEleMatch_wGwoGSF_VFEC");
  mvaName_woGwGSF_VFEC_ = cfg.getParameter<std::string>("mvaName_woGwGSF_VFEC");
  mvaName_wGwGSF_VFEC_ = cfg.getParameter<std::string>("mvaName_wGwGSF_VFEC");

  usePhiAtEcalEntranceExtrapolation_ = cfg.getParameter<bool>("usePhiAtEcalEntranceExtrapolation");

  if (!isPhase2_) {
    Var_NoEleMatch_woGwoGSF_Barrel_ = new float[10];
    Var_NoEleMatch_wGwoGSF_Barrel_ = new float[18];
    Var_woGwGSF_Barrel_ = new float[24];
    Var_wGwGSF_Barrel_ = new float[32];
    Var_NoEleMatch_woGwoGSF_Endcap_ = new float[9];
    Var_NoEleMatch_wGwoGSF_Endcap_ = new float[17];
    Var_woGwGSF_Endcap_ = new float[23];
    Var_wGwGSF_Endcap_ = new float[31];
    Var_NoEleMatch_woGwoGSF_VFEndcap_ = nullptr;
    Var_NoEleMatch_wGwoGSF_VFEndcap_ = nullptr;
    Var_woGwGSF_VFEndcap_ = nullptr;
    Var_wGwGSF_VFEndcap_ = nullptr;
  } else {
    Var_NoEleMatch_woGwoGSF_Barrel_ = new float[9];
    Var_NoEleMatch_wGwoGSF_Barrel_ = new float[17];
    Var_woGwGSF_Barrel_ = new float[27];
    Var_wGwGSF_Barrel_ = new float[36];
    Var_NoEleMatch_woGwoGSF_Endcap_ = new float[6];
    Var_NoEleMatch_wGwoGSF_Endcap_ = new float[14];
    Var_woGwGSF_Endcap_ = new float[31];
    Var_wGwGSF_Endcap_ = new float[38];
    Var_NoEleMatch_woGwoGSF_VFEndcap_ = new float[6];
    Var_NoEleMatch_wGwoGSF_VFEndcap_ = new float[14];
    Var_woGwGSF_VFEndcap_ = new float[32];
    Var_wGwGSF_VFEndcap_ = new float[40];

    //MB: Tokens for additional inputs (HGCal EleID variables) only for Phase2 and reco::GsfElectrons
    if (std::is_same<ElectronType, reco::GsfElectron>::value) {
      for (const auto& inputTag : cfg.getParameter<std::vector<edm::InputTag>>("hgcalElectronIDs")) {
        std::string elecIdLabel = "hgcElectronID:" + inputTag.instance();
        electronIds_tokens_[elecIdLabel] = cc.consumes<edm::ValueMap<float>>(
            inputTag);  //MB: It assumes that instances are not empty and meaningful (as for userData in patElectrons)
        electronIds_[elecIdLabel] = edm::Handle<edm::ValueMap<float>>();
      }
    }
  }
}

template <class TauType, class ElectronType>
AntiElectronIDMVA6<TauType, ElectronType>::~AntiElectronIDMVA6() {
  delete[] Var_NoEleMatch_woGwoGSF_Barrel_;
  delete[] Var_NoEleMatch_wGwoGSF_Barrel_;
  delete[] Var_woGwGSF_Barrel_;
  delete[] Var_wGwGSF_Barrel_;
  delete[] Var_NoEleMatch_woGwoGSF_Endcap_;
  delete[] Var_NoEleMatch_wGwoGSF_Endcap_;
  delete[] Var_woGwGSF_Endcap_;
  delete[] Var_wGwGSF_Endcap_;
  if (isPhase2_) {
    delete[] Var_NoEleMatch_woGwoGSF_VFEndcap_;
    delete[] Var_NoEleMatch_wGwoGSF_VFEndcap_;
    delete[] Var_woGwGSF_VFEndcap_;
    delete[] Var_wGwGSF_VFEndcap_;
  }

  if (!loadMVAfromDB_) {
    delete mva_NoEleMatch_woGwoGSF_BL_;
    delete mva_NoEleMatch_wGwoGSF_BL_;
    delete mva_woGwGSF_BL_;
    delete mva_wGwGSF_BL_;
    delete mva_NoEleMatch_woGwoGSF_EC_;
    delete mva_NoEleMatch_wGwoGSF_EC_;
    delete mva_woGwGSF_EC_;
    delete mva_wGwGSF_EC_;
    if (isPhase2_) {
      delete mva_NoEleMatch_woGwoGSF_VFEC_;
      delete mva_NoEleMatch_wGwoGSF_VFEC_;
      delete mva_woGwGSF_VFEC_;
      delete mva_wGwGSF_VFEC_;
    }
  }

  for (std::vector<TFile*>::iterator it = inputFilesToDelete_.begin(); it != inputFilesToDelete_.end(); ++it) {
    delete (*it);
  }
}

namespace {
  const GBRForest* loadMVAfromFile(TFile* inputFile, const std::string& mvaName) {
    const GBRForest* mva = (GBRForest*)inputFile->Get(mvaName.data());
    if (mva == nullptr)
      throw cms::Exception("PFRecoTauDiscriminationAgainstElectronMVA6::loadMVA")
          << " Failed to load MVA = " << mvaName.data() << " from file "
          << " !!\n";

    return mva;
  }

  const GBRForest* loadMVAfromDB(const edm::EventSetup& es, const std::string& mvaName) {
    edm::ESHandle<GBRForest> mva;
    es.get<GBRWrapperRcd>().get(mvaName, mva);
    return mva.product();
  }
}  // namespace

template <class TauType, class ElectronType>
void AntiElectronIDMVA6<TauType, ElectronType>::beginEvent(const edm::Event& evt, const edm::EventSetup& es) {
  if (!isInitialized_) {
    if (loadMVAfromDB_) {
      mva_NoEleMatch_woGwoGSF_BL_ = loadMVAfromDB(es, mvaName_NoEleMatch_woGwoGSF_BL_);
      mva_NoEleMatch_wGwoGSF_BL_ = loadMVAfromDB(es, mvaName_NoEleMatch_wGwoGSF_BL_);
      mva_woGwGSF_BL_ = loadMVAfromDB(es, mvaName_woGwGSF_BL_);
      mva_wGwGSF_BL_ = loadMVAfromDB(es, mvaName_wGwGSF_BL_);
      mva_NoEleMatch_woGwoGSF_EC_ = loadMVAfromDB(es, mvaName_NoEleMatch_woGwoGSF_EC_);
      mva_NoEleMatch_wGwoGSF_EC_ = loadMVAfromDB(es, mvaName_NoEleMatch_wGwoGSF_EC_);
      mva_woGwGSF_EC_ = loadMVAfromDB(es, mvaName_woGwGSF_EC_);
      mva_wGwGSF_EC_ = loadMVAfromDB(es, mvaName_wGwGSF_EC_);
      if (isPhase2_) {
        mva_NoEleMatch_woGwoGSF_VFEC_ = loadMVAfromDB(es, mvaName_NoEleMatch_woGwoGSF_VFEC_);
        mva_NoEleMatch_wGwoGSF_VFEC_ = loadMVAfromDB(es, mvaName_NoEleMatch_wGwoGSF_VFEC_);
        mva_woGwGSF_VFEC_ = loadMVAfromDB(es, mvaName_woGwGSF_VFEC_);
        mva_wGwGSF_VFEC_ = loadMVAfromDB(es, mvaName_wGwGSF_VFEC_);
      }
    } else {
      if (inputFileName_.location() == edm::FileInPath::Unknown)
        throw cms::Exception("PFRecoTauDiscriminationAgainstElectronMVA6::loadMVA")
            << " Failed to find File = " << inputFileName_ << " !!\n";
      TFile* inputFile = new TFile(inputFileName_.fullPath().data());

      mva_NoEleMatch_woGwoGSF_BL_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_woGwoGSF_BL_);
      mva_NoEleMatch_wGwoGSF_BL_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_wGwoGSF_BL_);
      mva_woGwGSF_BL_ = loadMVAfromFile(inputFile, mvaName_woGwGSF_BL_);
      mva_wGwGSF_BL_ = loadMVAfromFile(inputFile, mvaName_wGwGSF_BL_);
      mva_NoEleMatch_woGwoGSF_EC_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_woGwoGSF_EC_);
      mva_NoEleMatch_wGwoGSF_EC_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_wGwoGSF_EC_);
      mva_woGwGSF_EC_ = loadMVAfromFile(inputFile, mvaName_woGwGSF_EC_);
      mva_wGwGSF_EC_ = loadMVAfromFile(inputFile, mvaName_wGwGSF_EC_);
      if (isPhase2_) {
        mva_NoEleMatch_woGwoGSF_VFEC_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_woGwoGSF_VFEC_);
        mva_NoEleMatch_wGwoGSF_VFEC_ = loadMVAfromFile(inputFile, mvaName_NoEleMatch_wGwoGSF_VFEC_);
        mva_woGwGSF_VFEC_ = loadMVAfromFile(inputFile, mvaName_woGwGSF_VFEC_);
        mva_wGwGSF_VFEC_ = loadMVAfromFile(inputFile, mvaName_wGwGSF_VFEC_);
      }
      inputFilesToDelete_.push_back(inputFile);
    }
    isInitialized_ = true;
  }
  positionAtECalEntrance_.beginEvent(es);
  //MB: Handle additional inputs (HGCal EleID variables) only for Phase2 and reco::GsfElectrons
  if (isPhase2_ && std::is_same<ElectronType, reco::GsfElectron>::value) {
    for (const auto& eId_token : electronIds_tokens_) {
      edm::Handle<edm::ValueMap<float>> electronId;
      evt.getByToken(eId_token.second, electronId);
      electronIds_[eId_token.first] = electronId;
    }
  }
}

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::MVAValue(const TauVars& tauVars,
                                                           const TauGammaVecs& tauGammaVecs,
                                                           const ElecVars& elecVars) {
  TauGammaMoms tauGammaMoms;
  double sumPt = 0.;
  double dEta2 = 0.;
  double dPhi2 = 0.;
  tauGammaMoms.signalPFGammaCandsIn = tauGammaVecs.gammasPtInSigCone.size();
  for (size_t i = 0; i < tauGammaVecs.gammasPtInSigCone.size(); ++i) {
    double pt_i = tauGammaVecs.gammasPtInSigCone[i];
    double phi_i = tauGammaVecs.gammasdPhiInSigCone[i];
    if (tauGammaVecs.gammasdPhiInSigCone[i] > M_PI)
      phi_i = tauGammaVecs.gammasdPhiInSigCone[i] - 2 * M_PI;
    else if (tauGammaVecs.gammasdPhiInSigCone[i] < -M_PI)
      phi_i = tauGammaVecs.gammasdPhiInSigCone[i] + 2 * M_PI;
    double eta_i = tauGammaVecs.gammasdEtaInSigCone[i];
    sumPt += pt_i;
    dEta2 += (pt_i * eta_i * eta_i);
    dPhi2 += (pt_i * phi_i * phi_i);
  }

  tauGammaMoms.gammaEnFracIn = -99.;
  if (tauVars.pt > 0.) {
    tauGammaMoms.gammaEnFracIn = sumPt / tauVars.pt;
  }
  if (sumPt > 0.) {
    dEta2 /= sumPt;
    dPhi2 /= sumPt;
  }
  tauGammaMoms.gammaEtaMomIn = std::sqrt(dEta2 * tauGammaMoms.gammaEnFracIn) * tauVars.pt;
  tauGammaMoms.gammaPhiMomIn = std::sqrt(dPhi2 * tauGammaMoms.gammaEnFracIn) * tauVars.pt;

  sumPt = 0.;
  dEta2 = 0.;
  dPhi2 = 0.;
  tauGammaMoms.signalPFGammaCandsOut = tauGammaVecs.gammasPtOutSigCone.size();
  for (size_t i = 0; i < tauGammaVecs.gammasPtOutSigCone.size(); ++i) {
    double pt_i = tauGammaVecs.gammasPtOutSigCone[i];
    double phi_i = tauGammaVecs.gammasdPhiOutSigCone[i];
    if (tauGammaVecs.gammasdPhiOutSigCone[i] > M_PI)
      phi_i = tauGammaVecs.gammasdPhiOutSigCone[i] - 2 * M_PI;
    else if (tauGammaVecs.gammasdPhiOutSigCone[i] < -M_PI)
      phi_i = tauGammaVecs.gammasdPhiOutSigCone[i] + 2 * M_PI;
    double eta_i = tauGammaVecs.gammasdEtaOutSigCone[i];
    sumPt += pt_i;
    dEta2 += (pt_i * eta_i * eta_i);
    dPhi2 += (pt_i * phi_i * phi_i);
  }

  tauGammaMoms.gammaEnFracOut = -99.;
  if (tauVars.pt > 0.) {
    tauGammaMoms.gammaEnFracOut = sumPt / tauVars.pt;
  }
  if (sumPt > 0.) {
    dEta2 /= sumPt;
    dPhi2 /= sumPt;
  }
  tauGammaMoms.gammaEtaMomOut = std::sqrt(dEta2 * tauGammaMoms.gammaEnFracOut) * tauVars.pt;
  tauGammaMoms.gammaPhiMomOut = std::sqrt(dPhi2 * tauGammaMoms.gammaEnFracOut) * tauVars.pt;

  if (!isPhase2_) {
    return MVAValue(tauVars, tauGammaMoms, elecVars);
  } else {
    return MVAValuePhase2(tauVars, tauGammaMoms, elecVars);
  }
}

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::MVAValue(const TauVars& tauVars,
                                                           const TauGammaMoms& tauGammaMoms,
                                                           const ElecVars& elecVars) {
  if (!isInitialized_) {
    throw cms::Exception("ClassNotInitialized") << " AntiElectronMVA6 not properly initialized !!\n";
  }

  double mvaValue = -99.;

  float elecDeltaPinPoutOverPin = (elecVars.pIn > 0.0) ? (std::abs(elecVars.pIn - elecVars.pOut) / elecVars.pIn) : 1.0;
  float elecEecalOverPout = (elecVars.pOut > 0.0) ? (elecVars.eEcal / elecVars.pOut) : 20.0;
  float elecNumHitsDiffOverSum =
      ((elecVars.gsfNumHits + elecVars.kfNumHits) > 0.0)
          ? ((elecVars.gsfNumHits - elecVars.kfNumHits) / (elecVars.gsfNumHits + elecVars.kfNumHits))
          : 1.0;

  if (deltaR(tauVars.etaAtEcalEntrance, tauVars.phi, elecVars.eta, elecVars.phi) > 0.3 &&
      tauGammaMoms.signalPFGammaCandsIn == 0 && tauVars.hasGsf < 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_NoEleMatch_woGwoGSF_Barrel_[0] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Barrel_[1] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Barrel_[2] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Barrel_[3] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Barrel_[4] = tauVars.emFraction;
      Var_NoEleMatch_woGwoGSF_Barrel_[5] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_woGwoGSF_Barrel_[6] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_woGwoGSF_Barrel_[7] = tauVars.visMassIn;
      Var_NoEleMatch_woGwoGSF_Barrel_[8] = tauVars.dCrackEta;
      Var_NoEleMatch_woGwoGSF_Barrel_[9] = tauVars.dCrackPhi;
      mvaValue = mva_NoEleMatch_woGwoGSF_BL_->GetClassifier(Var_NoEleMatch_woGwoGSF_Barrel_);
    } else {
      Var_NoEleMatch_woGwoGSF_Endcap_[0] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Endcap_[1] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Endcap_[2] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Endcap_[3] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Endcap_[4] = tauVars.emFraction;
      Var_NoEleMatch_woGwoGSF_Endcap_[5] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_woGwoGSF_Endcap_[6] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_woGwoGSF_Endcap_[7] = tauVars.visMassIn;
      Var_NoEleMatch_woGwoGSF_Endcap_[8] = tauVars.dCrackEta;
      mvaValue = mva_NoEleMatch_woGwoGSF_EC_->GetClassifier(Var_NoEleMatch_woGwoGSF_Endcap_);
    }
  } else if (deltaR(tauVars.etaAtEcalEntrance, tauVars.phi, elecVars.eta, elecVars.phi) > 0.3 &&
             tauGammaMoms.signalPFGammaCandsIn > 0 && tauVars.hasGsf < 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_NoEleMatch_wGwoGSF_Barrel_[0] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Barrel_[1] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Barrel_[2] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Barrel_[3] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Barrel_[4] = tauVars.emFraction;
      Var_NoEleMatch_wGwoGSF_Barrel_[5] = tauGammaMoms.signalPFGammaCandsIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[6] = tauGammaMoms.signalPFGammaCandsOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[7] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_wGwoGSF_Barrel_[8] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_wGwoGSF_Barrel_[9] = tauVars.visMassIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[10] = tauGammaMoms.gammaEtaMomIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[11] = tauGammaMoms.gammaEtaMomOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[12] = tauGammaMoms.gammaPhiMomIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[13] = tauGammaMoms.gammaPhiMomOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[14] = tauGammaMoms.gammaEnFracIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[15] = tauGammaMoms.gammaEnFracOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[16] = tauVars.dCrackEta;
      Var_NoEleMatch_wGwoGSF_Barrel_[17] = tauVars.dCrackPhi;
      mvaValue = mva_NoEleMatch_wGwoGSF_BL_->GetClassifier(Var_NoEleMatch_wGwoGSF_Barrel_);
    } else {
      Var_NoEleMatch_wGwoGSF_Endcap_[0] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Endcap_[1] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Endcap_[2] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Endcap_[3] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Endcap_[4] = tauVars.emFraction;
      Var_NoEleMatch_wGwoGSF_Endcap_[5] = tauGammaMoms.signalPFGammaCandsIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[6] = tauGammaMoms.signalPFGammaCandsOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[7] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_wGwoGSF_Endcap_[8] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_wGwoGSF_Endcap_[9] = tauVars.visMassIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[10] = tauGammaMoms.gammaEtaMomIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[11] = tauGammaMoms.gammaEtaMomOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[12] = tauGammaMoms.gammaPhiMomIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[13] = tauGammaMoms.gammaPhiMomOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[14] = tauGammaMoms.gammaEnFracIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[15] = tauGammaMoms.gammaEnFracOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[16] = tauVars.dCrackEta;
      mvaValue = mva_NoEleMatch_wGwoGSF_EC_->GetClassifier(Var_NoEleMatch_wGwoGSF_Endcap_);
    }
  } else if (tauGammaMoms.signalPFGammaCandsIn == 0 && tauVars.hasGsf > 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_woGwGSF_Barrel_[0] = std::max(-0.1f, elecVars.eTotOverPin);
      Var_woGwGSF_Barrel_[1] = std::log(std::max(0.01f, elecVars.chi2NormGSF));
      Var_woGwGSF_Barrel_[2] = elecVars.gsfNumHits;
      Var_woGwGSF_Barrel_[3] = std::log(std::max(0.01f, elecVars.gsfTrackResol));
      Var_woGwGSF_Barrel_[4] = elecVars.gsfTracklnPt;
      Var_woGwGSF_Barrel_[5] = elecNumHitsDiffOverSum;
      Var_woGwGSF_Barrel_[6] = std::log(std::max(0.01f, elecVars.chi2NormKF));
      Var_woGwGSF_Barrel_[7] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_woGwGSF_Barrel_[8] = std::min(elecEecalOverPout, 20.f);
      Var_woGwGSF_Barrel_[9] = elecVars.deltaEta;
      Var_woGwGSF_Barrel_[10] = elecVars.deltaPhi;
      Var_woGwGSF_Barrel_[11] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_woGwGSF_Barrel_[12] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_woGwGSF_Barrel_[13] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_woGwGSF_Barrel_[14] = tauVars.etaAtEcalEntrance;
      Var_woGwGSF_Barrel_[15] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_woGwGSF_Barrel_[16] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_woGwGSF_Barrel_[17] = std::log(std::max(1.f, tauVars.pt));
      Var_woGwGSF_Barrel_[18] = tauVars.emFraction;
      Var_woGwGSF_Barrel_[19] = tauVars.leadPFChargedHadrHoP;
      Var_woGwGSF_Barrel_[20] = tauVars.leadPFChargedHadrEoP;
      Var_woGwGSF_Barrel_[21] = tauVars.visMassIn;
      Var_woGwGSF_Barrel_[22] = tauVars.dCrackEta;
      Var_woGwGSF_Barrel_[23] = tauVars.dCrackPhi;
      mvaValue = mva_woGwGSF_BL_->GetClassifier(Var_woGwGSF_Barrel_);
    } else {
      Var_woGwGSF_Endcap_[0] = std::max(-0.1f, elecVars.eTotOverPin);
      Var_woGwGSF_Endcap_[1] = std::log(std::max(0.01f, elecVars.chi2NormGSF));
      Var_woGwGSF_Endcap_[2] = elecVars.gsfNumHits;
      Var_woGwGSF_Endcap_[3] = std::log(std::max(0.01f, elecVars.gsfTrackResol));
      Var_woGwGSF_Endcap_[4] = elecVars.gsfTracklnPt;
      Var_woGwGSF_Endcap_[5] = elecNumHitsDiffOverSum;
      Var_woGwGSF_Endcap_[6] = std::log(std::max(0.01f, elecVars.chi2NormKF));
      Var_woGwGSF_Endcap_[7] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_woGwGSF_Endcap_[8] = std::min(elecEecalOverPout, 20.f);
      Var_woGwGSF_Endcap_[9] = elecVars.deltaEta;
      Var_woGwGSF_Endcap_[10] = elecVars.deltaPhi;
      Var_woGwGSF_Endcap_[11] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_woGwGSF_Endcap_[12] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_woGwGSF_Endcap_[13] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_woGwGSF_Endcap_[14] = tauVars.etaAtEcalEntrance;
      Var_woGwGSF_Endcap_[15] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_woGwGSF_Endcap_[16] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_woGwGSF_Endcap_[17] = std::log(std::max(1.f, tauVars.pt));
      Var_woGwGSF_Endcap_[18] = tauVars.emFraction;
      Var_woGwGSF_Endcap_[19] = tauVars.leadPFChargedHadrHoP;
      Var_woGwGSF_Endcap_[20] = tauVars.leadPFChargedHadrEoP;
      Var_woGwGSF_Endcap_[21] = tauVars.visMassIn;
      Var_woGwGSF_Endcap_[22] = tauVars.dCrackEta;
      mvaValue = mva_woGwGSF_EC_->GetClassifier(Var_woGwGSF_Endcap_);
    }
  } else if (tauGammaMoms.signalPFGammaCandsIn > 0 && tauVars.hasGsf > 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_wGwGSF_Barrel_[0] = std::max(-0.1f, elecVars.eTotOverPin);
      Var_wGwGSF_Barrel_[1] = std::log(std::max(0.01f, elecVars.chi2NormGSF));
      Var_wGwGSF_Barrel_[2] = elecVars.gsfNumHits;
      Var_wGwGSF_Barrel_[3] = std::log(std::max(0.01f, elecVars.gsfTrackResol));
      Var_wGwGSF_Barrel_[4] = elecVars.gsfTracklnPt;
      Var_wGwGSF_Barrel_[5] = elecNumHitsDiffOverSum;
      Var_wGwGSF_Barrel_[6] = std::log(std::max(0.01f, elecVars.chi2NormKF));
      Var_wGwGSF_Barrel_[7] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_wGwGSF_Barrel_[8] = std::min(elecEecalOverPout, 20.f);
      Var_wGwGSF_Barrel_[9] = elecVars.deltaEta;
      Var_wGwGSF_Barrel_[10] = elecVars.deltaPhi;
      Var_wGwGSF_Barrel_[11] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_wGwGSF_Barrel_[12] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_wGwGSF_Barrel_[13] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_wGwGSF_Barrel_[14] = tauVars.etaAtEcalEntrance;
      Var_wGwGSF_Barrel_[15] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_wGwGSF_Barrel_[16] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_wGwGSF_Barrel_[17] = std::log(std::max(1.f, tauVars.pt));
      Var_wGwGSF_Barrel_[18] = tauVars.emFraction;
      Var_wGwGSF_Barrel_[19] = tauGammaMoms.signalPFGammaCandsIn;
      Var_wGwGSF_Barrel_[20] = tauGammaMoms.signalPFGammaCandsOut;
      Var_wGwGSF_Barrel_[21] = tauVars.leadPFChargedHadrHoP;
      Var_wGwGSF_Barrel_[22] = tauVars.leadPFChargedHadrEoP;
      Var_wGwGSF_Barrel_[23] = tauVars.visMassIn;
      Var_wGwGSF_Barrel_[24] = tauGammaMoms.gammaEtaMomIn;
      Var_wGwGSF_Barrel_[25] = tauGammaMoms.gammaEtaMomOut;
      Var_wGwGSF_Barrel_[26] = tauGammaMoms.gammaPhiMomIn;
      Var_wGwGSF_Barrel_[27] = tauGammaMoms.gammaPhiMomOut;
      Var_wGwGSF_Barrel_[28] = tauGammaMoms.gammaEnFracIn;
      Var_wGwGSF_Barrel_[29] = tauGammaMoms.gammaEnFracOut;
      Var_wGwGSF_Barrel_[30] = tauVars.dCrackEta;
      Var_wGwGSF_Barrel_[31] = tauVars.dCrackPhi;
      mvaValue = mva_wGwGSF_BL_->GetClassifier(Var_wGwGSF_Barrel_);
    } else {
      Var_wGwGSF_Endcap_[0] = std::max(-0.1f, elecVars.eTotOverPin);
      Var_wGwGSF_Endcap_[1] = std::log(std::max(0.01f, elecVars.chi2NormGSF));
      Var_wGwGSF_Endcap_[2] = elecVars.gsfNumHits;
      Var_wGwGSF_Endcap_[3] = std::log(std::max(0.01f, elecVars.gsfTrackResol));
      Var_wGwGSF_Endcap_[4] = elecVars.gsfTracklnPt;
      Var_wGwGSF_Endcap_[5] = elecNumHitsDiffOverSum;
      Var_wGwGSF_Endcap_[6] = std::log(std::max(0.01f, elecVars.chi2NormKF));
      Var_wGwGSF_Endcap_[7] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_wGwGSF_Endcap_[8] = std::min(elecEecalOverPout, 20.f);
      Var_wGwGSF_Endcap_[9] = elecVars.deltaEta;
      Var_wGwGSF_Endcap_[10] = elecVars.deltaPhi;
      Var_wGwGSF_Endcap_[11] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_wGwGSF_Endcap_[12] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_wGwGSF_Endcap_[13] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_wGwGSF_Endcap_[14] = tauVars.etaAtEcalEntrance;
      Var_wGwGSF_Endcap_[15] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_wGwGSF_Endcap_[16] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_wGwGSF_Endcap_[17] = std::log(std::max(1.f, tauVars.pt));
      Var_wGwGSF_Endcap_[18] = tauVars.emFraction;
      Var_wGwGSF_Endcap_[19] = tauGammaMoms.signalPFGammaCandsIn;
      Var_wGwGSF_Endcap_[20] = tauGammaMoms.signalPFGammaCandsOut;
      Var_wGwGSF_Endcap_[21] = tauVars.leadPFChargedHadrHoP;
      Var_wGwGSF_Endcap_[22] = tauVars.leadPFChargedHadrEoP;
      Var_wGwGSF_Endcap_[23] = tauVars.visMassIn;
      Var_wGwGSF_Endcap_[24] = tauGammaMoms.gammaEtaMomIn;
      Var_wGwGSF_Endcap_[25] = tauGammaMoms.gammaEtaMomOut;
      Var_wGwGSF_Endcap_[26] = tauGammaMoms.gammaPhiMomIn;
      Var_wGwGSF_Endcap_[27] = tauGammaMoms.gammaPhiMomOut;
      Var_wGwGSF_Endcap_[28] = tauGammaMoms.gammaEnFracIn;
      Var_wGwGSF_Endcap_[29] = tauGammaMoms.gammaEnFracOut;
      Var_wGwGSF_Endcap_[30] = tauVars.dCrackEta;
      mvaValue = mva_wGwGSF_EC_->GetClassifier(Var_wGwGSF_Endcap_);
    }
  }
  return mvaValue;
}
////
template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::MVAValuePhase2(const TauVars& tauVars,
                                                                 const TauGammaMoms& tauGammaMoms,
                                                                 const ElecVars& elecVars) {
  if (!isInitialized_) {
    throw cms::Exception("ClassNotInitialized") << " AntiElectronMVA6 not properly initialized !!\n";
  }

  double mvaValue = -99.;

  //do not consider tau candidates outside the HGCal border at |eta|=3
  if (std::abs(tauVars.etaAtEcalEntrance) > 3.0) {
    return mvaValue;
  }

  float elecDeltaPinPoutOverPin = (elecVars.pIn > 0.0) ? (std::abs(elecVars.pIn - elecVars.pOut) / elecVars.pIn) : 1.0;
  float elecEecalOverPout = (elecVars.pOut > 0.0) ? (elecVars.eEcal / elecVars.pOut) : 20.0;
  float elecNumHitsDiffOverSum =
      ((elecVars.gsfNumHits + elecVars.kfNumHits) > 0.0)
          ? ((elecVars.gsfNumHits - elecVars.kfNumHits) / (elecVars.gsfNumHits + elecVars.kfNumHits))
          : 1.0;

  if (tauGammaMoms.signalPFGammaCandsIn == 0 && tauVars.hasGsf < 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_NoEleMatch_woGwoGSF_Barrel_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Barrel_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Barrel_[2] = tauVars.emFraction;
      Var_NoEleMatch_woGwoGSF_Barrel_[3] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_woGwoGSF_Barrel_[4] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_woGwoGSF_Barrel_[5] = tauVars.visMassIn;
      Var_NoEleMatch_woGwoGSF_Barrel_[6] = tauVars.dCrackEta;
      Var_NoEleMatch_woGwoGSF_Barrel_[7] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Barrel_[8] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_woGwoGSF_BL_->GetClassifier(Var_NoEleMatch_woGwoGSF_Barrel_);
    } else if (std::abs(tauVars.etaAtEcalEntrance) < ecalEndcapVFEndcapEtaBorder_) {
      Var_NoEleMatch_woGwoGSF_Endcap_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Endcap_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_Endcap_[2] = tauVars.visMassIn;
      Var_NoEleMatch_woGwoGSF_Endcap_[3] = tauVars.dCrackEta;
      Var_NoEleMatch_woGwoGSF_Endcap_[4] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_Endcap_[5] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_woGwoGSF_EC_->GetClassifier(Var_NoEleMatch_woGwoGSF_Endcap_);
    } else {
      Var_NoEleMatch_woGwoGSF_VFEndcap_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_VFEndcap_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_woGwoGSF_VFEndcap_[2] = tauVars.visMassIn;
      Var_NoEleMatch_woGwoGSF_VFEndcap_[3] = tauVars.dCrackEta;
      Var_NoEleMatch_woGwoGSF_VFEndcap_[4] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_woGwoGSF_VFEndcap_[5] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_woGwoGSF_VFEC_->GetClassifier(Var_NoEleMatch_woGwoGSF_VFEndcap_);
    }
  } else if (tauGammaMoms.signalPFGammaCandsIn > 0 && tauVars.hasGsf < 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_NoEleMatch_wGwoGSF_Barrel_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Barrel_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Barrel_[2] = tauVars.emFraction;
      Var_NoEleMatch_wGwoGSF_Barrel_[3] = tauGammaMoms.signalPFGammaCandsIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[4] = tauGammaMoms.signalPFGammaCandsOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[5] = tauVars.leadPFChargedHadrHoP;
      Var_NoEleMatch_wGwoGSF_Barrel_[6] = tauVars.leadPFChargedHadrEoP;
      Var_NoEleMatch_wGwoGSF_Barrel_[7] = tauVars.visMassIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[7] = tauGammaMoms.gammaEtaMomIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[9] = tauGammaMoms.gammaEtaMomOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[10] = tauGammaMoms.gammaPhiMomIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[11] = tauGammaMoms.gammaPhiMomOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[12] = tauGammaMoms.gammaEnFracIn;
      Var_NoEleMatch_wGwoGSF_Barrel_[13] = tauGammaMoms.gammaEnFracOut;
      Var_NoEleMatch_wGwoGSF_Barrel_[14] = tauVars.dCrackEta;
      Var_NoEleMatch_wGwoGSF_Barrel_[15] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Barrel_[16] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_wGwoGSF_BL_->GetClassifier(Var_NoEleMatch_wGwoGSF_Barrel_);
    } else if (std::abs(tauVars.etaAtEcalEntrance) < ecalEndcapVFEndcapEtaBorder_) {
      Var_NoEleMatch_wGwoGSF_Endcap_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Endcap_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_Endcap_[2] = tauGammaMoms.signalPFGammaCandsIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[3] = tauGammaMoms.signalPFGammaCandsOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[4] = tauVars.visMassIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[5] = tauGammaMoms.gammaEtaMomIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[6] = tauGammaMoms.gammaEtaMomOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[7] = tauGammaMoms.gammaPhiMomIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[8] = tauGammaMoms.gammaPhiMomOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[9] = tauGammaMoms.gammaEnFracIn;
      Var_NoEleMatch_wGwoGSF_Endcap_[10] = tauGammaMoms.gammaEnFracOut;
      Var_NoEleMatch_wGwoGSF_Endcap_[11] = tauVars.dCrackEta;
      Var_NoEleMatch_wGwoGSF_Endcap_[12] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_Endcap_[13] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_wGwoGSF_EC_->GetClassifier(Var_NoEleMatch_wGwoGSF_Endcap_);
    } else {
      Var_NoEleMatch_wGwoGSF_VFEndcap_[0] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_VFEndcap_[1] = std::log(std::max(1.f, tauVars.pt));
      Var_NoEleMatch_wGwoGSF_VFEndcap_[2] = tauGammaMoms.signalPFGammaCandsIn;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[3] = tauGammaMoms.signalPFGammaCandsOut;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[4] = tauVars.visMassIn;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[5] = tauGammaMoms.gammaEtaMomIn;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[6] = tauGammaMoms.gammaEtaMomOut;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[7] = tauGammaMoms.gammaPhiMomIn;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[8] = tauGammaMoms.gammaPhiMomOut;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[9] = tauGammaMoms.gammaEnFracIn;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[10] = tauGammaMoms.gammaEnFracOut;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[11] = tauVars.dCrackEta;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[12] = tauVars.etaAtEcalEntrance;
      Var_NoEleMatch_wGwoGSF_VFEndcap_[13] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      mvaValue = mva_NoEleMatch_wGwoGSF_VFEC_->GetClassifier(Var_NoEleMatch_wGwoGSF_VFEndcap_);
    }
  } else if (tauGammaMoms.signalPFGammaCandsIn == 0 && tauVars.hasGsf > 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_woGwGSF_Barrel_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_woGwGSF_Barrel_[1] = elecVars.gsfNumHits;
      Var_woGwGSF_Barrel_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_woGwGSF_Barrel_[3] = elecVars.gsfTracklnPt;
      Var_woGwGSF_Barrel_[4] = elecNumHitsDiffOverSum;
      Var_woGwGSF_Barrel_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_woGwGSF_Barrel_[6] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_woGwGSF_Barrel_[7] = std::min(elecEecalOverPout, 20.f);
      Var_woGwGSF_Barrel_[8] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_woGwGSF_Barrel_[9] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_woGwGSF_Barrel_[10] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_woGwGSF_Barrel_[11] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_woGwGSF_Barrel_[12] = std::log(std::max(1.f, tauVars.pt));
      Var_woGwGSF_Barrel_[13] = tauVars.emFraction;
      Var_woGwGSF_Barrel_[14] = tauVars.leadPFChargedHadrHoP;
      Var_woGwGSF_Barrel_[15] = tauVars.leadPFChargedHadrEoP;
      Var_woGwGSF_Barrel_[16] = tauVars.visMassIn;
      Var_woGwGSF_Barrel_[17] = tauVars.dCrackEta;
      Var_woGwGSF_Barrel_[18] = tauVars.etaAtEcalEntrance;
      Var_woGwGSF_Barrel_[19] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_woGwGSF_Barrel_[20] = elecVars.deltaEta;
      Var_woGwGSF_Barrel_[21] = elecVars.deltaPhi;
      Var_woGwGSF_Barrel_[22] = elecVars.sigmaIEtaIEta5x5;
      Var_woGwGSF_Barrel_[23] = elecVars.showerCircularity;
      Var_woGwGSF_Barrel_[24] = elecVars.r9;
      Var_woGwGSF_Barrel_[25] = elecVars.superClusterEtaWidth;
      Var_woGwGSF_Barrel_[26] = elecVars.superClusterPhiWidth;
      mvaValue = mva_woGwGSF_BL_->GetClassifier(Var_woGwGSF_Barrel_);
    } else if (std::abs(tauVars.etaAtEcalEntrance) < ecalEndcapVFEndcapEtaBorder_) {
      Var_woGwGSF_Endcap_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_woGwGSF_Endcap_[1] = elecVars.gsfNumHits;
      Var_woGwGSF_Endcap_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_woGwGSF_Endcap_[3] = elecVars.gsfTracklnPt;
      Var_woGwGSF_Endcap_[4] = elecNumHitsDiffOverSum;
      Var_woGwGSF_Endcap_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_woGwGSF_Endcap_[6] = elecVars.eEcal;
      Var_woGwGSF_Endcap_[7] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_woGwGSF_Endcap_[8] = std::log(std::max(1.f, tauVars.pt));
      Var_woGwGSF_Endcap_[9] = tauVars.visMassIn;
      Var_woGwGSF_Endcap_[10] = tauVars.dCrackEta;
      Var_woGwGSF_Endcap_[11] = tauVars.etaAtEcalEntrance;
      Var_woGwGSF_Endcap_[12] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_woGwGSF_Endcap_[13] = elecVars.hgcalSigmaUU;
      Var_woGwGSF_Endcap_[14] = elecVars.hgcalSigmaVV;
      Var_woGwGSF_Endcap_[15] = elecVars.hgcalSigmaEE;
      Var_woGwGSF_Endcap_[16] = elecVars.hgcalSigmaPP;
      Var_woGwGSF_Endcap_[17] = elecVars.hgcalNLayers;
      Var_woGwGSF_Endcap_[18] = elecVars.hgcalLastLayer;
      Var_woGwGSF_Endcap_[19] = elecVars.hgcalLayerEfrac10;
      Var_woGwGSF_Endcap_[20] = elecVars.hgcalLayerEfrac90;
      Var_woGwGSF_Endcap_[21] = elecVars.hgcalEcEnergyEE;
      Var_woGwGSF_Endcap_[22] = elecVars.hgcalEcEnergyFH;
      Var_woGwGSF_Endcap_[23] = elecVars.hgcalMeasuredDepth;
      Var_woGwGSF_Endcap_[24] = elecVars.hgcalExpectedDepth;
      Var_woGwGSF_Endcap_[25] = elecVars.hgcalDepthCompatibility;
      Var_woGwGSF_Endcap_[26] = elecVars.deltaEta;
      Var_woGwGSF_Endcap_[27] = elecVars.deltaPhi;
      Var_woGwGSF_Endcap_[28] = elecVars.eSeedClusterOverPout;
      Var_woGwGSF_Endcap_[29] = elecVars.superClusterEtaWidth;
      Var_woGwGSF_Endcap_[30] = elecVars.superClusterPhiWidth;
      mvaValue = mva_woGwGSF_EC_->GetClassifier(Var_woGwGSF_Endcap_);
    } else {
      Var_woGwGSF_VFEndcap_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_woGwGSF_VFEndcap_[1] = elecVars.gsfNumHits;
      Var_woGwGSF_VFEndcap_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_woGwGSF_VFEndcap_[3] = elecVars.gsfTracklnPt;
      Var_woGwGSF_VFEndcap_[4] = elecNumHitsDiffOverSum;
      Var_woGwGSF_VFEndcap_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_woGwGSF_VFEndcap_[6] = elecVars.eEcal;
      Var_woGwGSF_VFEndcap_[7] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_woGwGSF_VFEndcap_[8] = std::log(std::max(1.f, tauVars.pt));
      Var_woGwGSF_VFEndcap_[9] = tauVars.visMassIn;
      Var_woGwGSF_VFEndcap_[10] = tauVars.dCrackEta;
      Var_woGwGSF_VFEndcap_[11] = tauVars.etaAtEcalEntrance;
      Var_woGwGSF_VFEndcap_[12] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_woGwGSF_VFEndcap_[13] = elecVars.hgcalSigmaUU;
      Var_woGwGSF_VFEndcap_[14] = elecVars.hgcalSigmaVV;
      Var_woGwGSF_VFEndcap_[15] = elecVars.hgcalSigmaEE;
      Var_woGwGSF_VFEndcap_[16] = elecVars.hgcalSigmaPP;
      Var_woGwGSF_VFEndcap_[17] = elecVars.hgcalNLayers;
      Var_woGwGSF_VFEndcap_[18] = elecVars.hgcalLastLayer;
      Var_woGwGSF_VFEndcap_[19] = elecVars.hgcalLayerEfrac10;
      Var_woGwGSF_VFEndcap_[20] = elecVars.hgcalLayerEfrac90;
      Var_woGwGSF_VFEndcap_[21] = elecVars.hgcalEcEnergyEE;
      Var_woGwGSF_VFEndcap_[22] = elecVars.hgcalEcEnergyFH;
      Var_woGwGSF_VFEndcap_[23] = elecVars.hgcalMeasuredDepth;
      Var_woGwGSF_VFEndcap_[24] = elecVars.hgcalExpectedDepth;
      Var_woGwGSF_VFEndcap_[25] = elecVars.hgcalExpectedSigma;
      Var_woGwGSF_VFEndcap_[26] = elecVars.hgcalDepthCompatibility;
      Var_woGwGSF_VFEndcap_[27] = elecVars.deltaEta;
      Var_woGwGSF_VFEndcap_[28] = elecVars.deltaPhi;
      Var_woGwGSF_VFEndcap_[29] = elecVars.eSeedClusterOverPout;
      Var_woGwGSF_VFEndcap_[30] = elecVars.superClusterEtaWidth;
      Var_woGwGSF_VFEndcap_[31] = elecVars.superClusterPhiWidth;
      mvaValue = mva_woGwGSF_VFEC_->GetClassifier(Var_woGwGSF_VFEndcap_);
    }
  } else if (tauGammaMoms.signalPFGammaCandsIn > 0 && tauVars.hasGsf > 0.5) {
    if (std::abs(tauVars.etaAtEcalEntrance) < ecalBarrelEndcapEtaBorder_) {
      Var_wGwGSF_Barrel_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_wGwGSF_Barrel_[1] = elecVars.gsfNumHits;
      Var_wGwGSF_Barrel_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_wGwGSF_Barrel_[3] = elecVars.gsfTracklnPt;
      Var_wGwGSF_Barrel_[4] = elecNumHitsDiffOverSum;
      Var_wGwGSF_Barrel_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_wGwGSF_Barrel_[6] = std::min(elecDeltaPinPoutOverPin, 1.f);
      Var_wGwGSF_Barrel_[7] = std::min(elecEecalOverPout, 20.f);
      Var_wGwGSF_Barrel_[8] = std::min(elecVars.mvaInSigmaEtaEta, 0.01f);
      Var_wGwGSF_Barrel_[9] = std::min(elecVars.mvaInHadEnergy, 20.f);
      Var_wGwGSF_Barrel_[10] = std::min(elecVars.mvaInDeltaEta, 0.1f);
      Var_wGwGSF_Barrel_[11] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_wGwGSF_Barrel_[12] = std::log(std::max(1.f, tauVars.pt));
      Var_wGwGSF_Barrel_[13] = tauVars.emFraction;
      Var_wGwGSF_Barrel_[14] = tauGammaMoms.signalPFGammaCandsIn;
      Var_wGwGSF_Barrel_[15] = tauGammaMoms.signalPFGammaCandsOut;
      Var_wGwGSF_Barrel_[16] = tauVars.leadPFChargedHadrHoP;
      Var_wGwGSF_Barrel_[17] = tauVars.leadPFChargedHadrEoP;
      Var_wGwGSF_Barrel_[18] = tauVars.visMassIn;
      Var_wGwGSF_Barrel_[19] = tauGammaMoms.gammaEtaMomIn;
      Var_wGwGSF_Barrel_[20] = tauGammaMoms.gammaEtaMomOut;
      Var_wGwGSF_Barrel_[21] = tauGammaMoms.gammaPhiMomIn;
      Var_wGwGSF_Barrel_[22] = tauGammaMoms.gammaPhiMomOut;
      Var_wGwGSF_Barrel_[23] = tauGammaMoms.gammaEnFracIn;
      Var_wGwGSF_Barrel_[24] = tauGammaMoms.gammaEnFracOut;
      Var_wGwGSF_Barrel_[25] = tauVars.dCrackEta;
      Var_wGwGSF_Barrel_[26] = tauVars.etaAtEcalEntrance;
      Var_wGwGSF_Barrel_[27] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_wGwGSF_Barrel_[28] = elecVars.deltaEta;
      Var_wGwGSF_Barrel_[29] = elecVars.deltaPhi;
      Var_wGwGSF_Barrel_[30] = elecVars.sigmaIPhiIPhi5x5;
      Var_wGwGSF_Barrel_[31] = elecVars.sigmaIEtaIEta5x5;
      Var_wGwGSF_Barrel_[32] = elecVars.showerCircularity;
      Var_wGwGSF_Barrel_[33] = elecVars.eSeedClusterOverPout;
      Var_wGwGSF_Barrel_[34] = elecVars.superClusterEtaWidth;
      Var_wGwGSF_Barrel_[35] = elecVars.superClusterPhiWidth;
      mvaValue = mva_wGwGSF_BL_->GetClassifier(Var_wGwGSF_Barrel_);
    } else if (std::abs(tauVars.etaAtEcalEntrance) < ecalEndcapVFEndcapEtaBorder_) {
      Var_wGwGSF_Endcap_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_wGwGSF_Endcap_[1] = elecVars.gsfNumHits;
      Var_wGwGSF_Endcap_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_wGwGSF_Endcap_[3] = elecVars.gsfTracklnPt;
      Var_wGwGSF_Endcap_[4] = elecNumHitsDiffOverSum;
      Var_wGwGSF_Endcap_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_wGwGSF_Endcap_[6] = elecVars.eEcal;
      Var_wGwGSF_Endcap_[7] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_wGwGSF_Endcap_[8] = std::log(std::max(1.f, tauVars.pt));
      Var_wGwGSF_Endcap_[9] = tauGammaMoms.signalPFGammaCandsIn;
      Var_wGwGSF_Endcap_[10] = tauGammaMoms.signalPFGammaCandsOut;
      Var_wGwGSF_Endcap_[11] = tauVars.visMassIn;
      Var_wGwGSF_Endcap_[12] = tauGammaMoms.gammaEtaMomIn;
      Var_wGwGSF_Endcap_[13] = tauGammaMoms.gammaEtaMomOut;
      Var_wGwGSF_Endcap_[14] = tauGammaMoms.gammaPhiMomIn;
      Var_wGwGSF_Endcap_[15] = tauGammaMoms.gammaPhiMomOut;
      Var_wGwGSF_Endcap_[16] = tauGammaMoms.gammaEnFracIn;
      Var_wGwGSF_Endcap_[17] = tauGammaMoms.gammaEnFracOut;
      Var_wGwGSF_Endcap_[18] = tauVars.dCrackEta;
      Var_wGwGSF_Endcap_[19] = tauVars.etaAtEcalEntrance;
      Var_wGwGSF_Endcap_[20] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_wGwGSF_Endcap_[21] = elecVars.hgcalSigmaVV;
      Var_wGwGSF_Endcap_[22] = elecVars.hgcalSigmaEE;
      Var_wGwGSF_Endcap_[23] = elecVars.hgcalSigmaPP;
      Var_wGwGSF_Endcap_[24] = elecVars.hgcalNLayers;
      Var_wGwGSF_Endcap_[25] = elecVars.hgcalFirstLayer;
      Var_wGwGSF_Endcap_[26] = elecVars.hgcalLastLayer;
      Var_wGwGSF_Endcap_[27] = elecVars.hgcalLayerEfrac10;
      Var_wGwGSF_Endcap_[28] = elecVars.hgcalLayerEfrac90;
      Var_wGwGSF_Endcap_[29] = elecVars.hgcalEcEnergyEE;
      Var_wGwGSF_Endcap_[30] = elecVars.hgcalEcEnergyFH;
      Var_wGwGSF_Endcap_[31] = elecVars.hgcalMeasuredDepth;
      Var_wGwGSF_Endcap_[32] = elecVars.hgcalExpectedDepth;
      Var_wGwGSF_Endcap_[33] = elecVars.deltaEta;
      Var_wGwGSF_Endcap_[34] = elecVars.deltaPhi;
      Var_wGwGSF_Endcap_[35] = elecVars.eSeedClusterOverPout;
      Var_wGwGSF_Endcap_[36] = elecVars.superClusterEtaWidth;
      Var_wGwGSF_Endcap_[37] = elecVars.superClusterPhiWidth;
      mvaValue = mva_wGwGSF_EC_->GetClassifier(Var_wGwGSF_Endcap_);
    } else {
      Var_wGwGSF_VFEndcap_[0] = std::log(std::max(0.1f, elecVars.chi2NormGSF));
      Var_wGwGSF_VFEndcap_[1] = elecVars.gsfNumHits;
      Var_wGwGSF_VFEndcap_[2] = std::log(std::max(0.1f, elecVars.gsfTrackResol));
      Var_wGwGSF_VFEndcap_[3] = elecVars.gsfTracklnPt;
      Var_wGwGSF_VFEndcap_[4] = elecNumHitsDiffOverSum;
      Var_wGwGSF_VFEndcap_[5] = std::log(std::max(0.1f, elecVars.chi2NormKF));
      Var_wGwGSF_VFEndcap_[6] = elecVars.eEcal;
      Var_wGwGSF_VFEndcap_[7] = std::min(2.f, tauVars.leadChargedPFCandPt / std::max(1.f, tauVars.pt));
      Var_wGwGSF_VFEndcap_[8] = std::log(std::max(1.f, tauVars.pt));
      Var_wGwGSF_VFEndcap_[9] = tauGammaMoms.signalPFGammaCandsIn;
      Var_wGwGSF_VFEndcap_[10] = tauGammaMoms.signalPFGammaCandsOut;
      Var_wGwGSF_VFEndcap_[11] = tauVars.visMassIn;
      Var_wGwGSF_VFEndcap_[12] = tauGammaMoms.gammaEtaMomIn;
      Var_wGwGSF_VFEndcap_[13] = tauGammaMoms.gammaEtaMomOut;
      Var_wGwGSF_VFEndcap_[14] = tauGammaMoms.gammaPhiMomIn;
      Var_wGwGSF_VFEndcap_[15] = tauGammaMoms.gammaPhiMomOut;
      Var_wGwGSF_VFEndcap_[16] = tauGammaMoms.gammaEnFracIn;
      Var_wGwGSF_VFEndcap_[17] = tauGammaMoms.gammaEnFracOut;
      Var_wGwGSF_VFEndcap_[18] = tauVars.dCrackEta;
      Var_wGwGSF_VFEndcap_[19] = tauVars.etaAtEcalEntrance;
      Var_wGwGSF_VFEndcap_[20] = tauVars.leadChargedPFCandEtaAtEcalEntrance;
      Var_wGwGSF_VFEndcap_[21] = elecVars.hgcalSigmaUU;
      Var_wGwGSF_VFEndcap_[22] = elecVars.hgcalSigmaVV;
      Var_wGwGSF_VFEndcap_[23] = elecVars.hgcalSigmaEE;
      Var_wGwGSF_VFEndcap_[24] = elecVars.hgcalSigmaPP;
      Var_wGwGSF_VFEndcap_[25] = elecVars.hgcalNLayers;
      Var_wGwGSF_VFEndcap_[26] = elecVars.hgcalLastLayer;
      Var_wGwGSF_VFEndcap_[27] = elecVars.hgcalLayerEfrac10;
      Var_wGwGSF_VFEndcap_[28] = elecVars.hgcalLayerEfrac90;
      Var_wGwGSF_VFEndcap_[29] = elecVars.hgcalEcEnergyEE;
      Var_wGwGSF_VFEndcap_[30] = elecVars.hgcalEcEnergyFH;
      Var_wGwGSF_VFEndcap_[31] = elecVars.hgcalMeasuredDepth;
      Var_wGwGSF_VFEndcap_[32] = elecVars.hgcalExpectedDepth;
      Var_wGwGSF_VFEndcap_[33] = elecVars.hgcalExpectedSigma;
      Var_wGwGSF_VFEndcap_[34] = elecVars.hgcalDepthCompatibility;
      Var_wGwGSF_VFEndcap_[35] = elecVars.deltaEta;
      Var_wGwGSF_VFEndcap_[36] = elecVars.deltaPhi;
      Var_wGwGSF_VFEndcap_[37] = elecVars.eSeedClusterOverPout;
      Var_wGwGSF_VFEndcap_[38] = elecVars.superClusterEtaWidth;
      Var_wGwGSF_VFEndcap_[39] = elecVars.superClusterPhiWidth;
      mvaValue = mva_wGwGSF_VFEC_->GetClassifier(Var_wGwGSF_VFEndcap_);
    }
  }
  return mvaValue;
}
////
template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::MVAValue(const TauType& theTau, const ElectronRef& theEleRef)

{
  // === tau variables ===
  TauVars tauVars = AntiElectronIDMVA6<TauType, ElectronType>::getTauVars(theTau);
  TauGammaVecs tauGammaVecs = AntiElectronIDMVA6<TauType, ElectronType>::getTauGammaVecs(theTau);

  // === electron variables ===
  ElecVars elecVars = AntiElectronIDMVA6<TauType, ElectronType>::getElecVars(theEleRef);

  return MVAValue(tauVars, tauGammaVecs, elecVars);
}

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::MVAValue(const TauType& theTau) {
  // === tau variables ===
  TauVars tauVars = AntiElectronIDMVA6<TauType, ElectronType>::getTauVars(theTau);
  TauGammaVecs tauGammaVecs = AntiElectronIDMVA6<TauType, ElectronType>::getTauGammaVecs(theTau);

  // === electron variables ===
  ElecVars elecVars;
  elecVars.eta = 9.9;  // Dummy value used in MVA training

  return MVAValue(tauVars, tauGammaVecs, elecVars);
}

template <class TauType, class ElectronType>
TauVars AntiElectronIDMVA6<TauType, ElectronType>::getTauVars(const TauType& theTau) {
  TauVars tauVars;
  if (std::is_same<TauType, reco::PFTau>::value || std::is_same<TauType, pat::Tau>::value)
    tauVars = getTauVarsTypeSpecific(theTau);
  else
    throw cms::Exception("AntiElectronIDMVA6")
        << "Unsupported TauType used. You must use either reco::PFTau or pat::Tau.";

  tauVars.pt = theTau.pt();

  reco::Candidate::LorentzVector pfGammaSum(0, 0, 0, 0);
  reco::Candidate::LorentzVector pfChargedSum(0, 0, 0, 0);
  float signalrad = std::clamp(3.0 / std::max(1.0, theTau.pt()), 0.05, 0.10);
  for (const auto& gamma : theTau.signalGammaCands()) {
    float dR = deltaR(gamma->p4(), theTau.leadChargedHadrCand()->p4());
    // pfGammas inside the tau signal cone
    if (dR < signalrad) {
      pfGammaSum += gamma->p4();
    }
  }
  for (const auto& charged : theTau.signalChargedHadrCands()) {
    float dR = deltaR(charged->p4(), theTau.leadChargedHadrCand()->p4());
    // charged particles inside the tau signal cone
    if (dR < signalrad) {
      pfChargedSum += charged->p4();
    }
  }
  tauVars.visMassIn = (pfGammaSum + pfChargedSum).mass();

  tauVars.hasGsf = 0;
  if (theTau.leadChargedHadrCand().isNonnull()) {
    const pat::PackedCandidate* packedLeadChCand =
        dynamic_cast<const pat::PackedCandidate*>(theTau.leadChargedHadrCand().get());
    if (packedLeadChCand != nullptr) {
      if (std::abs(packedLeadChCand->pdgId()) == 11)
        tauVars.hasGsf = 1;
    } else {
      const reco::PFCandidate* pfLeadChCand =
          dynamic_cast<const reco::PFCandidate*>(theTau.leadChargedHadrCand().get());
      if (pfLeadChCand != nullptr && pfLeadChCand->gsfTrackRef().isNonnull())
        tauVars.hasGsf = 1;
    }
  }
  tauVars.dCrackPhi = dCrackPhi(tauVars.phi, tauVars.etaAtEcalEntrance);
  tauVars.dCrackEta = dCrackEta(tauVars.etaAtEcalEntrance);

  return tauVars;
}

template <class TauType, class ElectronType>
TauGammaVecs AntiElectronIDMVA6<TauType, ElectronType>::getTauGammaVecs(const TauType& theTau) {
  TauGammaVecs tauGammaVecs;

  float signalrad = std::clamp(3.0 / std::max(1.0, theTau.pt()), 0.05, 0.10);
  for (const auto& gamma : theTau.signalGammaCands()) {
    float dR = deltaR(gamma->p4(), theTau.leadChargedHadrCand()->p4());
    // pfGammas inside the tau signal cone
    if (dR < signalrad) {
      tauGammaVecs.gammasdEtaInSigCone.push_back(gamma->eta() - theTau.leadChargedHadrCand()->eta());
      tauGammaVecs.gammasdPhiInSigCone.push_back(gamma->phi() - theTau.leadChargedHadrCand()->phi());
      tauGammaVecs.gammasPtInSigCone.push_back(gamma->pt());
    }
    // pfGammas outside the tau signal cone
    else {
      tauGammaVecs.gammasdEtaOutSigCone.push_back(gamma->eta() - theTau.leadChargedHadrCand()->eta());
      tauGammaVecs.gammasdPhiOutSigCone.push_back(gamma->phi() - theTau.leadChargedHadrCand()->phi());
      tauGammaVecs.gammasPtOutSigCone.push_back(gamma->pt());
    }
  }
  return tauGammaVecs;
}

template <class TauType, class ElectronType>
ElecVars AntiElectronIDMVA6<TauType, ElectronType>::getElecVars(const ElectronRef& theEleRef) {
  ElecVars elecVars;

  elecVars.eta = theEleRef->eta();
  elecVars.phi = theEleRef->phi();

  // Variables related to the electron Cluster
  float elecEe = 0.;
  float elecEgamma = 0.;
  reco::SuperClusterRef pfSuperCluster = theEleRef->superCluster();
  if (pfSuperCluster.isNonnull() && pfSuperCluster.isAvailable()) {
    if (!isPhase2_) {
      for (reco::CaloCluster_iterator pfCluster = pfSuperCluster->clustersBegin();
           pfCluster != pfSuperCluster->clustersEnd();
           ++pfCluster) {
        double pfClusterEn = (*pfCluster)->energy();
        if (pfCluster == pfSuperCluster->clustersBegin())
          elecEe += pfClusterEn;
        else
          elecEgamma += pfClusterEn;
      }
    }
    elecVars.superClusterEtaWidth = pfSuperCluster->etaWidth();
    elecVars.superClusterPhiWidth = pfSuperCluster->phiWidth();
  }
  elecVars.eSeedClusterOverPout = theEleRef->eSeedClusterOverPout();
  elecVars.showerCircularity = 1. - theEleRef->e1x5() / theEleRef->e5x5();
  elecVars.r9 = theEleRef->r9();
  elecVars.sigmaIEtaIEta5x5 = theEleRef->full5x5_sigmaIetaIeta();
  elecVars.sigmaIPhiIPhi5x5 = theEleRef->full5x5_sigmaIphiIphi();

  elecVars.pIn = std::sqrt(theEleRef->trackMomentumAtVtx().Mag2());
  elecVars.pOut = std::sqrt(theEleRef->trackMomentumOut().Mag2());
  elecVars.eTotOverPin = (elecVars.pIn > 0.0) ? ((elecEe + elecEgamma) / elecVars.pIn) : -0.1;
  elecVars.eEcal = theEleRef->ecalEnergy();
  if (!isPhase2_) {
    elecVars.deltaEta = theEleRef->deltaEtaSeedClusterTrackAtCalo();
    elecVars.deltaPhi = theEleRef->deltaPhiSeedClusterTrackAtCalo();
  } else {
    elecVars.deltaEta = theEleRef->deltaEtaEleClusterTrackAtCalo();
    elecVars.deltaPhi = theEleRef->deltaPhiEleClusterTrackAtCalo();
  }
  elecVars.mvaInSigmaEtaEta = theEleRef->mvaInput().sigmaEtaEta;
  elecVars.mvaInHadEnergy = theEleRef->mvaInput().hadEnergy;
  elecVars.mvaInDeltaEta = theEleRef->mvaInput().deltaEta;

  // Variables related to the GsfTrack
  elecVars.chi2NormGSF = -99.;
  elecVars.gsfNumHits = -99.;
  elecVars.gsfTrackResol = -99.;
  elecVars.gsfTracklnPt = -99.;
  if (theEleRef->gsfTrack().isNonnull()) {
    elecVars.chi2NormGSF = theEleRef->gsfTrack()->normalizedChi2();
    elecVars.gsfNumHits = theEleRef->gsfTrack()->numberOfValidHits();
    if (theEleRef->gsfTrack()->pt() > 0.) {
      elecVars.gsfTrackResol = theEleRef->gsfTrack()->ptError() / theEleRef->gsfTrack()->pt();
      elecVars.gsfTracklnPt = log(theEleRef->gsfTrack()->pt()) * M_LN10;
    }
  }

  // Variables related to the CtfTrack
  elecVars.chi2NormKF = -99.;
  elecVars.kfNumHits = -99.;
  if (theEleRef->closestCtfTrackRef().isNonnull()) {
    elecVars.chi2NormKF = theEleRef->closestCtfTrackRef()->normalizedChi2();
    elecVars.kfNumHits = theEleRef->closestCtfTrackRef()->numberOfValidHits();
  }

  // Variables related to HGCal
  if (isPhase2_ && !theEleRef->isEB()) {
    if (std::is_same<ElectronType, reco::GsfElectron>::value || std::is_same<ElectronType, pat::Electron>::value)
      getElecVarsHGCalTypeSpecific(theEleRef, elecVars);
    else
      throw cms::Exception("AntiElectronIDMVA6")
          << "Unsupported ElectronType used. You must use either reco::GsfElectron or pat::Electron.";
  }

  return elecVars;
}

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::minimum(double a, double b) {
  if (std::abs(b) < std::abs(a))
    return b;
  else
    return a;
}

namespace {

  // IN: define locations of the 18 phi-cracks
  std::array<double, 18> fill_cPhi() {
    constexpr double pi = M_PI;  // 3.14159265358979323846;
    std::array<double, 18> cPhi;
    // IN: define locations of the 18 phi-cracks
    cPhi[0] = 2.97025;
    for (unsigned iCrack = 1; iCrack <= 17; ++iCrack) {
      cPhi[iCrack] = cPhi[0] - 2. * iCrack * pi / 18;
    }
    return cPhi;
  }

  const std::array<double, 18> cPhi = fill_cPhi();

}  // namespace

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::dCrackPhi(double phi, double eta) {
  //--- compute the (unsigned) distance to the closest phi-crack in the ECAL barrel

  constexpr double pi = M_PI;  // 3.14159265358979323846;

  // IN: shift of this location if eta < 0
  constexpr double delta_cPhi = 0.00638;

  double retVal = 99.;

  if (eta >= -1.47464 && eta <= 1.47464) {
    // the location is shifted
    if (eta < 0.)
      phi += delta_cPhi;

    // CV: need to bring-back phi into interval [-pi,+pi]
    if (phi > pi)
      phi -= 2. * pi;
    if (phi < -pi)
      phi += 2. * pi;

    if (phi >= -pi && phi <= pi) {
      // the problem of the extrema:
      if (phi < cPhi[17] || phi >= cPhi[0]) {
        if (phi < 0.)
          phi += 2. * pi;
        retVal = minimum(phi - cPhi[0], phi - cPhi[17] - 2. * pi);
      } else {
        // between these extrema...
        bool OK = false;
        unsigned iCrack = 16;
        while (!OK) {
          if (phi < cPhi[iCrack]) {
            retVal = minimum(phi - cPhi[iCrack + 1], phi - cPhi[iCrack]);
            OK = true;
          } else {
            iCrack -= 1;
          }
        }
      }
    } else {
      retVal = 0.;  // IN: if there is a problem, we assume that we are in a crack
    }
  } else {
    return -99.;
  }

  return std::abs(retVal);
}

template <class TauType, class ElectronType>
double AntiElectronIDMVA6<TauType, ElectronType>::dCrackEta(double eta) {
  //--- compute the (unsigned) distance to the closest eta-crack in the ECAL barrel

  // IN: define locations of the eta-cracks
  double cracks[5] = {0., 4.44747e-01, 7.92824e-01, 1.14090e+00, 1.47464e+00};

  double retVal = 99.;

  for (int iCrack = 0; iCrack < 5; ++iCrack) {
    double d = minimum(eta - cracks[iCrack], eta + cracks[iCrack]);
    if (std::abs(d) < std::abs(retVal)) {
      retVal = d;
    }
  }

  return std::abs(retVal);
}

// pat::Tau
template <class TauType, class ElectronType>
TauVars AntiElectronIDMVA6<TauType, ElectronType>::getTauVarsTypeSpecific(const pat::Tau& theTau) {
  TauVars tauVars;
  tauVars.etaAtEcalEntrance = theTau.etaAtEcalEntrance();
  tauVars.leadChargedPFCandEtaAtEcalEntrance = theTau.etaAtEcalEntranceLeadChargedCand();
  tauVars.leadChargedPFCandPt = theTau.ptLeadChargedCand();
  tauVars.phi = theTau.phi();
  if (!isPhase2_) {
    if (!usePhiAtEcalEntranceExtrapolation_) {
      tauVars.phi = theTau.phiAtEcalEntrance();
    } else {
      float sumPhiTimesEnergy = 0.;
      float sumEnergy = 0.;
      for (const auto& candidate : theTau.signalCands()) {
        float phiAtECalEntrance = candidate->phi();
        bool success = false;
        reco::Candidate::Point posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
        if (success) {
          phiAtECalEntrance = posAtECal.phi();
        }
        sumPhiTimesEnergy += phiAtECalEntrance * candidate->energy();
        sumEnergy += candidate->energy();
      }
      if (sumEnergy > 0.) {
        tauVars.phi = sumPhiTimesEnergy / sumEnergy;
      }
    }
    tauVars.emFraction = std::max(theTau.emFraction_MVA(), 0.f);
  } else {
    if (std::abs(theTau.eta()) >= ecalBarrelEndcapEtaBorder_) {  //HGCal
      tauVars.etaAtEcalEntrance = -99.;
      tauVars.leadChargedPFCandEtaAtEcalEntrance = -99.;
      bool success = false;
      reco::Candidate::Point posAtECal =
          positionAtECalEntrance_(theTau.leadChargedHadrCand().get(), success, isPhase2_);
      if (success) {
        tauVars.leadChargedPFCandEtaAtEcalEntrance = posAtECal.eta();
      }
      float sumEtaTimesEnergy = 0.;
      float sumEnergy = 0.;
      for (const auto& candidate : theTau.signalCands()) {
        float etaAtECalEntrance = candidate->eta();
        success = false;
        posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
        if (success) {
          etaAtECalEntrance = posAtECal.eta();
        }
        sumEtaTimesEnergy += etaAtECalEntrance * candidate->energy();
        sumEnergy += candidate->energy();
      }
      if (sumEnergy > 0.) {
        tauVars.etaAtEcalEntrance = sumEtaTimesEnergy / sumEnergy;
      }
    }
    tauVars.emFraction = std::max(theTau.ecalEnergyLeadChargedHadrCand() /
                                      (theTau.ecalEnergyLeadChargedHadrCand() + theTau.hcalEnergyLeadChargedHadrCand()),
                                  0.f);
  }
  tauVars.leadPFChargedHadrHoP = 0.;
  tauVars.leadPFChargedHadrEoP = 0.;
  if (theTau.leadChargedHadrCand()->p() > 0.) {
    tauVars.leadPFChargedHadrHoP = theTau.hcalEnergyLeadChargedHadrCand() / theTau.leadChargedHadrCand()->p();
    tauVars.leadPFChargedHadrEoP = theTau.ecalEnergyLeadChargedHadrCand() / theTau.leadChargedHadrCand()->p();
  }

  return tauVars;
}

// reco::PFTau
template <class TauType, class ElectronType>
TauVars AntiElectronIDMVA6<TauType, ElectronType>::getTauVarsTypeSpecific(const reco::PFTau& theTau) {
  TauVars tauVars;
  tauVars.etaAtEcalEntrance = -99.;
  tauVars.leadChargedPFCandEtaAtEcalEntrance = -99.;
  tauVars.leadChargedPFCandPt = -99.;
  float sumEtaTimesEnergy = 0.;
  float sumPhiTimesEnergy = 0.;
  float sumEnergy = 0.;
  tauVars.phi = theTau.phi();
  // Check type of candidates building tau to avoid dynamic casts further
  bool isFromPFCands =
      (theTau.leadCand().isNonnull() && dynamic_cast<const reco::PFCandidate*>(theTau.leadCand().get()) != nullptr);
  if (!isPhase2_) {
    for (const auto& candidate : theTau.signalCands()) {
      float etaAtECalEntrance = candidate->eta();
      float phiAtECalEntrance = candidate->phi();
      const reco::Track* track = nullptr;
      if (isFromPFCands) {
        const reco::PFCandidate* pfCandidate = static_cast<const reco::PFCandidate*>(candidate.get());
        etaAtECalEntrance = pfCandidate->positionAtECALEntrance().eta();
        if (!usePhiAtEcalEntranceExtrapolation_) {
          phiAtECalEntrance = pfCandidate->positionAtECALEntrance().phi();
        } else {
          bool success = false;
          reco::Candidate::Point posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
          if (success) {
            phiAtECalEntrance = posAtECal.phi();
          }
        }
        if (pfCandidate->trackRef().isNonnull())
          track = pfCandidate->trackRef().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->innerTrack().isNonnull())
          track = pfCandidate->muonRef()->innerTrack().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->globalTrack().isNonnull())
          track = pfCandidate->muonRef()->globalTrack().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->outerTrack().isNonnull())
          track = pfCandidate->muonRef()->outerTrack().get();
        else if (pfCandidate->gsfTrackRef().isNonnull())
          track = pfCandidate->gsfTrackRef().get();
      } else {
        bool success = false;
        reco::Candidate::Point posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
        if (success) {
          etaAtECalEntrance = posAtECal.eta();
          phiAtECalEntrance = posAtECal.phi();
        }
        track = candidate->bestTrack();
      }
      if (track != nullptr) {
        if (track->pt() > tauVars.leadChargedPFCandPt) {
          tauVars.leadChargedPFCandEtaAtEcalEntrance = etaAtECalEntrance;
          tauVars.leadChargedPFCandPt = track->pt();
        }
      }
      sumEtaTimesEnergy += etaAtECalEntrance * candidate->energy();
      sumPhiTimesEnergy += phiAtECalEntrance * candidate->energy();
      sumEnergy += candidate->energy();
    }
    if (sumEnergy > 0.) {
      tauVars.etaAtEcalEntrance = sumEtaTimesEnergy / sumEnergy;
      tauVars.phi = sumPhiTimesEnergy / sumEnergy;
    }
    tauVars.emFraction = std::max(theTau.emFraction(), 0.f);
  } else {  // Phase2
    for (const auto& candidate : theTau.signalCands()) {
      float etaAtECalEntrance = candidate->eta();
      const reco::Track* track = nullptr;
      if (isFromPFCands) {
        const reco::PFCandidate* pfCandidate = static_cast<const reco::PFCandidate*>(candidate.get());
        etaAtECalEntrance = pfCandidate->positionAtECALEntrance().eta();
        if (std::abs(theTau.eta()) >= ecalBarrelEndcapEtaBorder_) {  //HGCal
          bool success = false;
          reco::Candidate::Point posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
          if (success) {
            etaAtECalEntrance = posAtECal.eta();
          }
        }
        if (pfCandidate->trackRef().isNonnull())
          track = pfCandidate->trackRef().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->innerTrack().isNonnull())
          track = pfCandidate->muonRef()->innerTrack().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->globalTrack().isNonnull())
          track = pfCandidate->muonRef()->globalTrack().get();
        else if (pfCandidate->muonRef().isNonnull() && pfCandidate->muonRef()->outerTrack().isNonnull())
          track = pfCandidate->muonRef()->outerTrack().get();
        else if (pfCandidate->gsfTrackRef().isNonnull())
          track = pfCandidate->gsfTrackRef().get();
      } else {
        bool success = false;
        reco::Candidate::Point posAtECal = positionAtECalEntrance_(candidate.get(), success, isPhase2_);
        if (success) {
          etaAtECalEntrance = posAtECal.eta();
        }
        track = candidate->bestTrack();
      }
      if (track != nullptr) {
        if (track->pt() > tauVars.leadChargedPFCandPt) {
          tauVars.leadChargedPFCandEtaAtEcalEntrance = etaAtECalEntrance;
          tauVars.leadChargedPFCandPt = track->pt();
        }
      }
      sumEtaTimesEnergy += etaAtECalEntrance * candidate->energy();
      sumEnergy += candidate->energy();
    }
    if (sumEnergy > 0.) {
      tauVars.etaAtEcalEntrance = sumEtaTimesEnergy / sumEnergy;
    }
    if (isFromPFCands) {
      const reco::PFCandidate* pfLeadCandidte =
          static_cast<const reco::PFCandidate*>(theTau.leadChargedHadrCand().get());
      tauVars.emFraction =
          std::max(pfLeadCandidte->ecalEnergy() / (pfLeadCandidte->ecalEnergy() + pfLeadCandidte->hcalEnergy()), 0.);
    } else {
      const pat::PackedCandidate* patLeadCandiate =
          dynamic_cast<const pat::PackedCandidate*>(theTau.leadChargedHadrCand().get());
      if (patLeadCandiate != nullptr) {
        tauVars.emFraction = std::max(1. - patLeadCandiate->hcalFraction(), 0.);
      }
    }
  }
  tauVars.leadPFChargedHadrHoP = 0.;
  tauVars.leadPFChargedHadrEoP = 0.;
  if (theTau.leadChargedHadrCand()->p() > 0.) {
    if (isFromPFCands) {
      const reco::PFCandidate* pfLeadCandiate =
          static_cast<const reco::PFCandidate*>(theTau.leadChargedHadrCand().get());
      tauVars.leadPFChargedHadrHoP = pfLeadCandiate->hcalEnergy() / pfLeadCandiate->p();
      tauVars.leadPFChargedHadrEoP = pfLeadCandiate->ecalEnergy() / pfLeadCandiate->p();
    } else {
      const pat::PackedCandidate* patLeadCandiate =
          dynamic_cast<const pat::PackedCandidate*>(theTau.leadChargedHadrCand().get());
      if (patLeadCandiate != nullptr) {
        tauVars.leadPFChargedHadrHoP = patLeadCandiate->caloFraction() * patLeadCandiate->energy() *
                                       patLeadCandiate->hcalFraction() / patLeadCandiate->p();
        tauVars.leadPFChargedHadrEoP = patLeadCandiate->caloFraction() * patLeadCandiate->energy() *
                                       (1. - patLeadCandiate->hcalFraction()) / patLeadCandiate->p();
      }
    }
  }

  return tauVars;
}

// reco::GsfElectron
template <class TauType, class ElectronType>
void AntiElectronIDMVA6<TauType, ElectronType>::getElecVarsHGCalTypeSpecific(
    const reco::GsfElectronRef& theEleRef, antiElecIDMVA6_blocks::ElecVars& elecVars) {
  //MB: Assumed that presence of one of the HGCal EleID variables guarantee presence of all
  if (!(electronIds_.find("hgcElectronID:sigmaUU") != electronIds_.end() &&
        electronIds_.at("hgcElectronID:sigmaUU").isValid()))
    return;

  elecVars.hgcalSigmaUU = (*electronIds_.at("hgcElectronID:sigmaUU"))[theEleRef];
  elecVars.hgcalSigmaVV = (*electronIds_.at("hgcElectronID:sigmaVV"))[theEleRef];
  elecVars.hgcalSigmaEE = (*electronIds_.at("hgcElectronID:sigmaEE"))[theEleRef];
  elecVars.hgcalSigmaPP = (*electronIds_.at("hgcElectronID:sigmaPP"))[theEleRef];
  elecVars.hgcalNLayers = (*electronIds_.at("hgcElectronID:nLayers"))[theEleRef];
  elecVars.hgcalFirstLayer = (*electronIds_.at("hgcElectronID:firstLayer"))[theEleRef];
  elecVars.hgcalLastLayer = (*electronIds_.at("hgcElectronID:lastLayer"))[theEleRef];
  elecVars.hgcalLayerEfrac10 = (*electronIds_.at("hgcElectronID:layerEfrac10"))[theEleRef];
  elecVars.hgcalLayerEfrac90 = (*electronIds_.at("hgcElectronID:layerEfrac90"))[theEleRef];
  elecVars.hgcalEcEnergyEE = (*electronIds_.at("hgcElectronID:ecEnergyEE"))[theEleRef];
  elecVars.hgcalEcEnergyFH = (*electronIds_.at("hgcElectronID:ecEnergyFH"))[theEleRef];
  elecVars.hgcalMeasuredDepth = (*electronIds_.at("hgcElectronID:measuredDepth"))[theEleRef];
  elecVars.hgcalExpectedDepth = (*electronIds_.at("hgcElectronID:expectedDepth"))[theEleRef];
  elecVars.hgcalExpectedSigma = (*electronIds_.at("hgcElectronID:expectedSigma"))[theEleRef];
  elecVars.hgcalDepthCompatibility = (*electronIds_.at("hgcElectronID:depthCompatibility"))[theEleRef];

  return;
}

// pat::Electron
template <class TauType, class ElectronType>
void AntiElectronIDMVA6<TauType, ElectronType>::getElecVarsHGCalTypeSpecific(
    const pat::ElectronRef& theEleRef, antiElecIDMVA6_blocks::ElecVars& elecVars) {
  //MB: Assumed that presence of one of the HGCal EleID variables guarantee presence of all
  if (!theEleRef->hasUserFloat("hgcElectronID:sigmaUU"))
    return;

  elecVars.hgcalSigmaUU = theEleRef->userFloat("hgcElectronID:sigmaUU");
  elecVars.hgcalSigmaVV = theEleRef->userFloat("hgcElectronID:sigmaVV");
  elecVars.hgcalSigmaEE = theEleRef->userFloat("hgcElectronID:sigmaEE");
  elecVars.hgcalSigmaPP = theEleRef->userFloat("hgcElectronID:sigmaPP");
  elecVars.hgcalNLayers = theEleRef->userFloat("hgcElectronID:nLayers");
  elecVars.hgcalFirstLayer = theEleRef->userFloat("hgcElectronID:firstLayer");
  elecVars.hgcalLastLayer = theEleRef->userFloat("hgcElectronID:lastLayer");
  elecVars.hgcalLayerEfrac10 = theEleRef->userFloat("hgcElectronID:layerEfrac10");
  elecVars.hgcalLayerEfrac90 = theEleRef->userFloat("hgcElectronID:layerEfrac90");
  elecVars.hgcalEcEnergyEE = theEleRef->userFloat("hgcElectronID:ecEnergyEE");
  elecVars.hgcalEcEnergyFH = theEleRef->userFloat("hgcElectronID:ecEnergyFH");
  elecVars.hgcalMeasuredDepth = theEleRef->userFloat("hgcElectronID:measuredDepth");
  elecVars.hgcalExpectedDepth = theEleRef->userFloat("hgcElectronID:expectedDepth");
  elecVars.hgcalExpectedSigma = theEleRef->userFloat("hgcElectronID:expectedSigma");
  elecVars.hgcalDepthCompatibility = theEleRef->userFloat("hgcElectronID:depthCompatibility");

  return;
}

// compile desired types and make available to linker
template class AntiElectronIDMVA6<reco::PFTau, reco::GsfElectron>;
template class AntiElectronIDMVA6<pat::Tau, pat::Electron>;
