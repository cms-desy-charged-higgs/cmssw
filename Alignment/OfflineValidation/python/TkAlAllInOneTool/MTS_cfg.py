import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

from Alignment.OfflineValidation.TkAlAllInOneTool.utils import _byteify

import json
import os

##Define process
process = cms.Process("MuonTrackSplit")

##Argument parsing
options = VarParsing()
options.register("config", "", VarParsing.multiplicity.singleton, VarParsing.varType.string , "AllInOne config")

options.parseArguments()

##Read in AllInOne config in JSON format
with open(options.config, "r") as configFile:
    config = json.load(configFile)

##Load common configuration
##Define process
process = cms.Process("OfflineValidator")

##Argument parsing
options = VarParsing()
options.register("config", "", VarParsing.multiplicity.singleton, VarParsing.varType.string , "AllInOne config")
options.register("isCrab", False, VarParsing.multiplicity.singleton, VarParsing.varType.bool , "Bool to check if this is a crab job")

options.parseArguments()

##Read in AllInOne config in JSON format
if options.config == "":
    config = {"validation": {},
              "alignment": {}}
else:
    with open(options.config, "r") as configFile:
        config = _byteify(json.load(configFile, object_hook=_byteify),ignore_dicts=True)

##Read filenames from given TXT file and define input source
readFiles = []

if "dataset" in config["validation"]:
    with open(config["validation"]["dataset"], "r") as datafiles:
        for fileName in datafiles.readlines():
            readFiles.append(fileName.replace("\n", ""))

    ##Define input source
    process.source = cms.Source("PoolSource",
                                fileNames = cms.untracked.vstring(readFiles),
                                skipEvents = cms.untracked.uint32(0)
                            )

##Get good lumi section
if "goodlumi" in config["validation"]:
    if os.path.isfile(config["validation"]["goodlumi"]):
        goodLumiSecs = cms.untracked.VLuminosityBlockRange(LumiList.LumiList(filename = config["validation"]["goodlumi"]).getCMSSWString().split(','))

    else:
        print("Does not exist: {}. Continue without good lumi section file.".format(config["validation"]["goodlumi"]))
        goodLumiSecs = cms.untracked.VLuminosityBlockRange()

else:
    goodLumiSecs = cms.untracked.VLuminosityBlockRange()

process.source.lumisToProcess = goodLumiSecs

##default set to 1 for unit tests
process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(config["validation"].get("maxevents", 1))
)

##Bookeeping
process.options = cms.untracked.PSet(
    wantSummary = cms.untracked.bool(False),
    Rethrow = cms.untracked.vstring("ProductNotFound"),
    fileMode  =  cms.untracked.string('NOMERGE'),
)

process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.cerr.FwkReport.reportEvery = 100

##Basic modules
process.load("RecoVertex.BeamSpotProducer.BeamSpot_cff")
process.load("Configuration.Geometry.GeometryDB_cff")
process.load('Configuration.StandardSequences.Services_cff')
process.load("Configuration.StandardSequences.MagneticField_cff")

##Track fitting
import Alignment.CommonAlignment.tools.trackselectionRefitting as trackselRefit
process.seqTrackselRefit = trackselRefit.getSequence(process,
                                                     config["validation"].get("trackcollection", "generalTracks"),
                                                     isPVValidation = False, 
                                                     TTRHBuilder = config["validation"].get("tthrbuilder", "WithAngleAndTemplate"),
                                                     usePixelQualityFlag=config["validation"].get("usePixelQualityFlag", True),
                                                     openMassWindow = False,
                                                     cosmicsDecoMode = config["validation"].get("cosmicsDecoMode", True),
                                                     cosmicsZeroTesla= config["validation"].get("cosmicsZeroTesla", False),
                                                     momentumConstraint = None,
                                                     cosmicTrackSplitting = True,
                                                     use_d0cut = False,
)

#Global tag
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, config["alignment"].get("globaltag", "auto:run2_data"))

##Load conditions if wished
if "mp" in config["alignment"]:
    pass ##FIXME Add proper function here to start

if "conditions" in config["alignment"]:
    from CalibTracker.Configuration.Common.PoolDBESSource_cfi import poolDBESSource

    for condition in config["alignment"]["conditions"]:
        setattr(process, "conditionsIn{}".format(condition), poolDBESSource.clone(
                connect = cms.string(str(config["alignment"]["conditions"][condition]["connect"])),
                toGet = cms.VPSet(
                        cms.PSet(
                        record = cms.string(str(condition)),
                        tag = cms.string(str(config["alignment"]["conditions"][condition]["tag"]))
                    )
                )
            )
        )

        setattr(process, "prefer_conditionsIn{}".format(condition), cms.ESPrefer("PoolDBESSource", "conditionsIn{}".format(condition)))


## Adding this ~doubles the efficiency of selection
process.FittingSmootherRKP5.EstimateCut = -1

##Cuts
if "subdetector" in config["validation"]:
    setattr(process.AlignmentTrackSelector.minHitsPerSubDet, "in{}".format(config["validation"]["subdetector"]), 2)

## Configure EDAnalyzer
process.cosmicValidation = cms.EDAnalyzer("CosmicSplitterValidation",
    ifSplitMuons = cms.bool(False),
    ifTrackMCTruth = cms.bool(False),	
    checkIfGolden = cms.bool(False),	
    splitTracks = cms.InputTag("FinalTrackRefitter","","splitter"),
    splitGlobalMuons = cms.InputTag("muons","","splitter"),
    originalTracks = cms.InputTag("FirstTrackRefitter","","splitter"),
    originalGlobalMuons = cms.InputTag("muons","","Rec")
)

##Output file
process.TFileService = cms.Service("TFileService",
    fileName = cms.string("{}/MTS.root".format(config["output"])),
    closeFileFast = cms.untracked.bool(True),
)

##Let all sequences run
process.p = cms.Path(process.seqTrackselRefit*process.cosmicValidation)
