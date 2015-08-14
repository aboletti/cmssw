import FWCore.ParameterSet.Config as cms

from RecoLocalCalo.CaloTowersCreator.calotowermaker_cfi import calotowermaker
from RecoHI.HiJetAlgos.HiRecoJets_cff import akPu4CaloJets
from JetMETCorrections.Configuration.DefaultJEC_cff import *

hiCaloTowerForTrk = calotowermaker.clone(hbheInput=cms.InputTag('hbhereco'))
akPu4CaloJetsForTrk = akPu4CaloJets.clone( srcPVs = cms.InputTag('hiSelectedVertex'), src= cms.InputTag('hiCaloTowerForTrk'))


akPu4CaloJetsCorrected  = ak4CaloJetsL2L3.clone(
    src = cms.InputTag("akPu4CaloJetsForTrk")
)


hiCaloJetsForTrk = cms.Sequence(hiCaloTowerForTrk*akPu4CaloJetsForTrk*akPu4CaloJetsCorrected)
