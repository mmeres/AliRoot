#!/bin/bash
SOURCE=/cvmfs/alice-ocdb.cern.ch/calibration/data/2017/OCDB
TARGET=/opt/HLT/data/HCDB_new_tmp#
FUTURE_RUN=500000
SOURCE_RUN=260014
UPDATE_MAGNETIC_FIELD=0

if [ "0$1" != "0" ]; then
    echo Using Source $1
    SOURCE=$1
fi
if [ "0$2" != "0" ]; then
    echo Using Target $2
    TARGET=$2
fi
if [ "0$3" != "0" ]; then
    echo Using run for default objects: $3
    FUTURE_RUN=$3
fi
if [ "0$4" != "0" ]; then
    echo Reference run for extending objects: $4
    SOURCE_RUN=$4
fi
if [ "0$5" == "01" ]; then
    echo Setting update of magnetic field
    UPDATE_MAGNETIC_FIELD=1
fi

if [ "0$TARGET" == "0" ]; then
    echo No target set
    exit 1
fi

rm -Rf $TARGET/*
mkdir -p $TARGET

#Download default CDB entries for future run
aliroot -l -b -q $ALICE_SOURCE/HLT/programs/downloadCDB.C"($FUTURE_RUN,\"local://$SOURCE\",\"local://$TARGET/tmp\",\"*/*/*\")"

#Extend run ranges of all these objects to infinity
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$TARGET/tmp ocdbTarget=$TARGET cdbEntries=*/*/* sourceRun=$FUTURE_RUN

#Clearn up temporary copy of default OCDB
rm -Rf $TARGET/tmp

#Remove objects not wanted in HCDB
rm -Rf $TARGET/TPC/Calib/CorrectionMaps*

#Create HLT HCDB configuration objects that differ from OCDB
aliroot -l -q -b $ALICE_SOURCE/HLT/exa/makeComponentConfigurationObject.C"(\"HLT/ConfigHLT/OnlineMode\", \"HLT Online Mode\", \"local://$TARGET\")"
aliroot -l -q -b $ALICE_SOURCE/HLT/exa/makeComponentConfigurationObject.C"(\"HLT/ConfigTPC/TPCHWClusterDecoder\", \"\", \"local://$TARGET\")"
aliroot -l -q -b $ALICE_SOURCE/HLT/exa/makeComponentConfigurationObject.C"(\"HLT/ConfigTPC/TPCHWClusterFinder\", \
    \"-debug-level 0 -do-mc 0 -deconvolute-time 1 -deconvolute-pad 1 -flow-control 0 -single-pad-suppression 0 -bypass-merger 0 -cluster-lower-limit 10 -single-sequence-limit 0 -use-timebin-window 1 -merger-distance 4 -charge-fluctuation 0 -rcu2-data 1\", \
    \"local://$TARGET\")"
#ATTENTION: the -rcu2-data flag is NOT set for the HCDB!!!!!! This is set by the chain configuration not by the HCDB!!!!!! Thus, the setting is not applied running aliroot!!!!!!

#Create empty string dummy HLT configuration objects to suppress CDB Manager warnings
aliroot -l -q -b $ALICE_SOURCE/HLT/exa/makeComponentConfigurationObject.C"(\"HLT/ConfigT0/T0Reconstruction\", \"\", \"local://$TARGET\")"

#Create Global Trigger Configuration
aliroot -l -q -b $ALICE_SOURCE/HLT/programs/create-globaltrigger-HM-TPC-comp.C"(\"local://$TARGET\")"

#Update TPC CalibDB
aliroot -b << EOF
  AliCDBManager* man = AliCDBManager::Instance();
  man->SetDefaultStorage("local://$TARGET");
  man->SetRun($SOURCE_RUN)
  AliCDBEntry* entry = man->Get("TPC/Calib/RecoParam");
  TObjArray* array = dynamic_cast<TObjArray*>(entry->GetObject());
  AliTPCRecoParam* param;
  TIter iter(array);
  while (param = dynamic_cast<AliTPCRecoParam*>(iter())) \
  { \
    param->SetUseCorrectionMap(kFALSE); \
    param->SetAccountDistortions(kFALSE); \
    param->SetUseTOFCorrection(kFALSE); \
    param->SetUseAlignmentTime(kTRUE); \
    param->SetUseComposedCorrection(kTRUE); \
  }
  man->Put(entry);
EOF

#Fetch some special objects from one reference run and extend them, because default object does not exist
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="GRP/GRP/Data" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="GRP/CTP/Config" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="GRP/CTP/CTPtiming" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="GRP/CTP/Scalers" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="TPC/Calib/HighVoltage" sourceRun=$SOURCE_RUN

#Fetch some special objects from one reference run and extend them, because we want the latest object not the default object
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="TPC/Calib/AltroConfig" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="TPC/Calib/Temperature" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="TPC/Calib/GasComposition" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="ITS/Calib/BadChannelsSSD" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="ITS/Calib/SPDDead" sourceRun=$SOURCE_RUN

#Fetch some more special objects needed such that offline reco does not crash (used to compare offline reco runs with hlt recraw local)
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="MUON/Calib/HV" sourceRun=$SOURCE_RUN
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="MUON/Calib/OccupancyMap" sourceRun=$SOURCE_RUN

#Fetch CTP Aliases to make offline reco work with trigger class filter
$ALICE_SOURCE/HLT/programs/extendHLTOCDB.sh ocdbSource=$SOURCE ocdbTarget=$TARGET cdbEntries="GRP/CTP/Aliases" sourceRun=$SOURCE_RUN

if [ $UPDATE_MAGNETIC_FIELD == 1 ]; then
    let START_SHIFT=`date +%s`-300
    END_SHIFT=`date +%s`
    DIM_DNS_HOST=aldaqecs.cern.ch DIM_DNS_NODE=alidcsdimdns.cern.ch ALIHLT_T_HCDBDIR=$TARGET aliroot -l -q -b "$ALICE_ROOT/HLT/createGRP/AliHLTCreateGRP.C($SOURCE_RUN, \"TPC\", \"pp\", \"PHYSICS\", $START_SHIFT, $END_SHIFT, 0)"
    retVal=$?
    echo $ALICE_ROOT/HLT/createGRP/AliHLTCreateGRP.C returns with $retVal
    if [ ! $retVal -eq 2 ]; then
        echo ERROR creating GRP with up to date magnet state
        exit 1
    fi
fi

#Create TPC Fast transform object
rm -f $ALICE_ROOT/OCDB/HLT/ConfigTPC/TPCFastTransform/*
aliroot -l -q -b $ALICE_SOURCE/HLT/TPCLib/macros/makeTPCFastTransformOCDBObject.C"(\"local://$TARGET\", $SOURCE_RUN, $SOURCE_RUN)"
SRCFILE=`ls $ALICE_ROOT/OCDB/HLT/ConfigTPC/TPCFastTransform | tail -n 1`
aliroot -l -q -b $ALICE_SOURCE/HLT/programs/adjustOCDBObject.C"(\"$ALICE_ROOT/OCDB/HLT/ConfigTPC/TPCFastTransform/$SRCFILE\", \"local://$TARGET\", 0)"
rm -f $ALICE_ROOT/OCDB/HLT/ConfigTPC/TPCFastTransform/*

if [ $UPDATE_MAGNETIC_FIELD == 1 ]; then
    rm -f $TARGET/GRP/GRP/Data/Run${SOURCE_RUN}_${SOURCE_RUN}*.root
fi

#Use all TPC Compression objects from cvmfs, to have correct behavior for all runs
rm -f $TARGET/HLT/ConfigTPC/TPCDataCompressor/*
rm -f $TARGET/HLT/ConfigTPC/TPCDataCompressorHuffmanTables/*
mkdir -p $TARGET/HLT/ConfigTPC/TPCDataCompressor
mkdir -p $TARGET/HLT/ConfigTPC/TPCDataCompressorHuffmanTables

#Create a default huffman table with diffential compression to match HLT settings in data replay, will have low version number, so it is overwritten by new tables for new runs
#aliroot -l -q -b $ALICE_SOURCE/HLT/programs/adjustOCDBObject.C"(\""`echo $SOURCE | sed "s,data/[0-9]*/OCDB,data/2017/OCDB,g"`"/HLT/ConfigTPC/TPCDataCompressorHuffmanTables/Run276551_999999999_v2_s0.root\", \"local://$TARGET\", 0, -1, 1)"
#aliroot -l -q -b $ALICE_SOURCE/HLT/programs/adjustOCDBObject.C"(\""`echo $SOURCE | sed "s,data/[0-9]*/OCDB,data/2017/OCDB,g"`"/HLT/ConfigTPC/TPCDataCompressor/Run276551_999999999_v2_s0.root\", \"local://$TARGET\", 0, -1, 1)"
#NOT USED ANYMORE, INSTALL ALL OBJECTS FOR ALL RUNS, SO YOU CAN PROCESS OLD RAW FILES WITH THIS HCDB. CURRENT OBJECT FOR NEW RUNS IS AUTOMATICALLY INSTALLED

#First object used in 2010, replaced by updated objects below
cp `echo $SOURCE | sed "s,data/[0-9]*/OCDB,data/2010/OCDB,g"`/HLT/ConfigTPC/TPCDataCompressor/Run0*.root $TARGET/HLT/ConfigTPC/TPCDataCompressor
cp `echo $SOURCE | sed "s,data/[0-9]*/OCDB,data/2010/OCDB,g"`/HLT/ConfigTPC/TPCDataCompressorHuffmanTables/Run0*.root $TARGET/HLT/ConfigTPC/TPCDataCompressorHuffmanTables

#Year from which to take the OCDB objects. First objects would be from 2010, but we overwrite all these objects with a default object from 2016 that has differential compression enabled.
STARTYEAR=2010

CURYEAR=`date +%Y`
VERCONF=1
VERTABLE=1
for i in `seq $STARTYEAR $CURYEAR`; do
    TMPSOURCE=`echo $SOURCE | sed "s,data/[0-9]*/OCDB,data/$i/OCDB,g"`
    for j in `ls $TMPSOURCE/HLT/ConfigTPC/TPCDataCompressor | grep -v Run0`; do
        aliroot -l -q -b $ALICE_SOURCE/HLT/programs/adjustOCDBObject.C"(\"$TMPSOURCE/HLT/ConfigTPC/TPCDataCompressor/$j\", \"local://$TARGET\", -1, -1, $VERCONF)"
        let VERCONF=$VERCONF+1
    done
    for j in `ls $TMPSOURCE/HLT/ConfigTPC/TPCDataCompressorHuffmanTables | grep -v Run0`; do
        aliroot -l -q -b $ALICE_SOURCE/HLT/programs/adjustOCDBObject.C"(\"$TMPSOURCE/HLT/ConfigTPC/TPCDataCompressorHuffmanTables/$j\", \"local://$TARGET\", -1, -1, $VERTABLE)"
        let VERTABLE=$VERTABLE+1
    done
done

#copy old GRP entries
cp -n $ALIHLT_HCDBDIR/GRP/GRP/Data/* $TARGET/GRP/GRP/Data

exit 0
