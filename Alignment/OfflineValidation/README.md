# The new AllInOne tool

For the validation of the produced alignments, several validation methods (Primary vertex, Muon Track Split, etc.) are implemented for the user. The configuration of the validation jobs is encoded in the `AllInOne` config file, which is provided to the master python executable `validateAlignments.py`. A clear directory hierachy is set up, for each a job a slimmed config is configured and all jobs are submitted with HTCondor.

## Validation logic and a little terminology  

Each validation typically is a several step process. The first step is the production of flat NTuples or/and histograms while looping over the given input datasets. In the `AllInOne` config this jobs are referred as `single` jobs and utilize custom CMSSW EDM modules which are run with `cmsRun`. In the next step the flat NTuples/histograms are used to produce the wished plots, this kind of jobs are referred as `merge` jobs. In some cases, the alignments are produced in several periods/time intervalls (IOVs), for this purpose `trends` jobs are used to produce plots displaying the alignment performance for the different periods.

## AllInOne config

All wished validation are configured in one config file written in either `json`/`yaml` format. Both config formats allow hierarchical/tree configuration structures and are equivalent in its information encoding and usage. The `json` format is typically used in JavaScript and has a very strict syntax with a lot symbols, while `yaml` follows the lax syntax of python and has overall better readibility. All python executables can handle both `json`/`yaml` format, while C++ executable only can deal with `json` with the usage of property tree class of boost.

### Basic structure of the config

The basic structure of the config is always the same. The two main arguments which appear in the configs are `validations` and `alignments`. In the `alignments` the wished alignment condition and the plotting style is configured, while in the `validations` section the type of validation are configured, the job type and custom arguments needed for each validation job. Hence the general structure will look like this (in `yaml` format):

```
alignments:
    Alignment1:
        color: ...
        style: ...
        globalTag: ...
        title: ...
        
    Alignment2:
        color: ...
        style: ...
        conditions: ...
        title: ...
    
    ...

validations:
     ValidationType1:
         single:
             ValidationName: 
                 IOV: [...]
                 max-events: ...
                 dataset: ...
          
          merge: 
             ValidationName: 
                 legend-title: ...    
                 
     ValidationType2:
         single: ...
         
         merge: ...       
```

### Old config style

In the old implementation of the tool the config format is `ini`. This format allows only configuration of sections without deeper substructure. The section names are connected by the old tool to get a pseudo substructure, but this leads for complicated configuration to unreadable/non-understandable files. Generally the ini config follows a logic like this:

```
[alignment: Alingment1]
color = ...
...

[alignment: Alingment2]
color = ...
...

[validationType1: validationName]
IOV: [...]
max-events: ...

[plots: validationType1]
legend-title: ...    

[validation]
validationType1 validationName = Alingment1
validationType1 validationName = Alingment2
``` 

Following the naming convention used in the example of old and new configutaion, an `ini` config should be translatable into a `json`/`yaml` in most cases. In a completed configuration the better approach is probably to write the `json`/`yaml` from scratch using the documentation below.

## General usage of validationsAlignment.py

The main script is `validateAlignments.py`. One can check the options with:
```
validateAlignments.py -h
usage: validateAlignments.py [-h] [-d] [-v] [-e] [-f]
                             [-j {espresso,microcentury,longlunch,workday,tomorrow,testmatch,nextweek}]
                             config

AllInOneTool for validation of the tracker alignment

positional arguments:
  config                Global AllInOneTool config (json/yaml format)

optional arguments:
  -h, --help            show this help message and exit
  -d, --dry             Set up everything, but don't run anything
  -v, --verbose         Enable standard output stream
  -e, --example         Print example of config in JSON format
  -f, --force           Force creation of enviroment, possible overwritten old configuration
  -j {espresso,microcentury,longlunch,workday,tomorrow,testmatch,nextweek}, --job-flavour {espresso,microcentury,longlunch,workday,tomorrow,testmatch,nextweek}
                        Job flavours for HTCondor at CERN, default is 'longlunch'
```

As input the AllInOneTool config in `yaml` or `json` file format has to be provided. One proper example can be find here: `Alignment/OfflineValidation/test/test.yaml`. To create the set up and submit everything to the HTCondor batch system, one can call

```
validateAlignments.py $CMSSW_BASE/src/Alignment/OfflineValidation/test/test.yaml 

-----------------------------------------------------------------------
File for submitting this DAG to HTCondor           : /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile.condor.sub
Log of DAGMan debugging messages                 : /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile.dagman.out
Log of HTCondor library output                     : /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile.lib.out
Log of HTCondor library error messages             : /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile.lib.err
Log of the life of condor_dagman itself          : /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile.dagman.log

Submitting job(s).
1 job(s) submitted to cluster 5140155.
-----------------------------------------------------------------------
```

To create the set up without submitting jobs to HTCondor one can use the dry run option:

```
validateAlignments.py $CMSSW_BASE/src/Alignment/OfflineValidation/test/test.yaml -d
Enviroment is set up. If you want to submit everything, call 'condor_submit_dag /afs/cern.ch/user/d/dbrunner/ToolDev/CMSSW_10_6_0/src/MyTest/DAG/dagFile'
```

## Implementation of a validation in the tool

To implement a new/or porting an existing validation to the new frame work, two things needs to be provided: executables and a python file providing the information for each job.

#### Executables

In the new frame work standalone executables do the job of the validations. They are designed to run indenpendently from the set up of validateAlignments.py, the executables only need a configuration file with information needed for the validation/plotting. One can implement a C++ or a python executable. 

If a C++ executable is implemented, the source file of the executable needs to be placed in the `Alignment/OfflineValidation/bin` directory and the BuildFile.xml in this directory needs to be modified. For the readout of the configuration file, which is in JSON format, the property tree class from the boost library is used. See `bin/DMRmerge.cc` as an example of a proper C++ implementation.

If a python executable is implemented, the source file needs to be placed in the `Alignment/OfflineValidation/scripts` directory. In the first line of the python script a shebang like `#!/usr/bin/env python` must be written and the script itself must be changed to be executable. In the case of python the configuration file can be both in JSON/YAML, because in python both after read in are just python dictionaries. See `scripts/GCPpyPlots.py` as an example of a proper python implementation.

For the special case of a cmsRun job, one needs to provide only the CMS python configuration. Because it is python again, both JSON/YAML for the configuration file are fine to use. Also for this case the execution via cmsRun is independent from the set up provided by validateAligments.py and only need the proper configuration file. See `python/TkAlAllInOneTool/DMR_cfg.py` as an example of a proper implementation.

#### Python file for configuration

For each validation several jobs can be executed, because there are several steps like nTupling, fitting, plotting or there is categorization like alignments, IOVs. The information will be encoded in a global config provided by the aligner, see `Alignment/OfflineValidation/test/test.yaml` as an example. To figure out from the global config which/how many jobs should be prepared, a python file needs to be implemented which reads the global config, extract the relevant information of the global config and yields smaller config designed to be read from the respective executable. As an example see `python/TkAlAllInOneTool/DMR.py`.

There is a logic which needed to be followed. Each job needs to be directionary with a structure like this:

```
job = {
       "name": Job name ##Needs to be unique!
       "dir": workingDirectory  ##Also needs to be unique!
       "exe": Name of executable/or cmsRun
       "run-mode": Batch cluster (Either "Condor" or possibly "Crab" for cmsRun execution)
       "cms-config": path to CMS config if exe = cmsRun, else leave this out
       "dependencies": [name of jobs this jobs needs to wait for] ##Empty list [] if no depedencies
       "config": Slimmed config from global config only with information needed for this job
}
```

The python file returns a list of jobs to the `validateAligments.py` which finally creates the directory structure/configuration files/DAG file. To let `validateAligments.py` know one validation implementation exist, import the respective python file and extend the if statements which starts at line 69. This is the only time one needs to touch `validateAligments.py`!
 

# Validation overview

## General and alignment configuration

For all validations the general and alignment section are essential identical to configure. Generally the config general/alignment will look like this (in `yaml`):

```
general1: ...
general2: ...

alignments:
    Alignment1:
    ...
    
    Alignment2:
    ...
    
    ...
```

The table below show the possible options for both general and alignment section.

Variable | Default value | Explanation
-------- | ------------- | -----------
LFS      |       -       |  Base directory created for the output (EOS/LFS group dir)
name     |       -       |  Name of working directory

Variable | Default value | Explanation
-------- | ------------- | -----------
color    |       -       |  Integer value of color from ROOT color wheel
style    |       -       |  Integer value of marker style for ROOT plot
title    |       -       |  Title as shown in the plot legend
globalTag |      -       |  Global tag used by the CMSSW config
conditions |     -       |  Custom conditions loaded from the database

## Muon track split validation

The Muon track split (`MTS`) validation is processed in two steps. In the first step a flat NTuple is produced by the EDM module `plugins/CosmicSplitterValidation.cc` executed by `cmsRun` and configured by `python/TkAlAllInOne/MTS_cfg.py`. In the second step the plots are produced by the `bin/MTSmerge.cc` executable. In general a config for the `MTS` will look like this:

```
validations:
    MTS:
        ValidationName1:
             ...
             ...
```

The table below show the possible options for the `MTS` configuration.

Variable | Default value | Explanation
-------- | ------------- | -----------
run-mode | Condor        | Batch scheduler
alignments      |       -       |  List of alignment name defined in the alignments section
dataset     |       -       |  Txt file with list of input ROOT files
goodLumi | Empty list | JSON format with good lumi sections
trackcollection | ctfWithMaterialTracksP5 | Name of used track collection
tthrbuilder |  WithAngleAndTemplate | 
maxevents | -1 (-> all events) | Maximum number of events to process
magneticfield | 0 (-> zero Tesla) | Magnetic field strength in Tesla
subdetector | - | Name of detector system to apply cuts one
customrighttitle | "" | String shown on top right of the plot
outliercut | 0 (-> nothing is cutted) | Threshold to cut away events in the tails of the distributions

## Distributions of the medians of the residuals

The distributions of the medians of the residuals (`DMR`) is a three step validation. In the first step histograms are produced by the EDM module `plugins/TrackerOfflineValidation.cc` executed by `cmsRun` and configured by `python/TkAlAllInOne/DMR_cfg.py`. In the second step the plots are produced by the `bin/DMRmerge.cc` executable. If wished, a trend of `IOV`s can be plotted in the third step with the `bin/DMRtrends.cc` executable. In general a config for the `DMR` will look like this:

```
validations:
    DMR:
        single:
            ValidationName1:
                 ...
                 ...
                 
         merge:
            ValidationName1:
                 ...
                 ...
                 
         trends:
                ...
```

Possible options for `single` configuration:

Variable | Default value | Explanation
-------- | ------------- | -----------
run-mode | Condor        | Batch scheduler
IOV      | empty list    | List of IOVs to processes
alignments      |       -       |  List of alignment name defined in the alignments section
dataset     |       -       |  Txt file with list of input ROOT files
goodLumi | Empty list | JSON format with good lumi sections
trackcollection | generalTracks | Name of used track collection
tthrbuilder |  WithAngleAndTemplate | 
maxevents | -1 (-> all events) | Maximum number of events to process
maxtracks | 1 | Maximum number of tracks to process
magneticfield | True | Magnetic field on or off
usePixelQualityFlag | True |
vertexcollection | offlinePrimaryVertices | Name of primary vertex collection
moduleLevelHistsTransient | False |
moduleLevelProfiles | False |
stripYResiduals | False |
chargecut | 0 |

Possible options for `merge` configuration:

Variable | Default value | Explanation
-------- | ------------- | -----------
customrighttitle | ""  | Title shown in the top right of the plot
singles | - | List of validation names used in the single section
usefit | True |
minimum | 15 |
bigtext | True |
legendheader | "" |
moduleid | Empty list | 
curves | plain |
methods | [median, rmsNorm] |
legendoptions | [mean, rms] |

## JetHT validation

### Validation analysis - JetHT_cfg.py

The vast majority of the time in the JetHT validation goes to running the actual validation analysis. This is done with cmsRun using the JetHT_cfg.py configuration file. To configure the analysis in the all-in-one framework, the following parameters can be set in the json/yaml configuration file:

```
All-in-one:

validations
    JetHT
        single
            myDataset
```

Variable | Default value | Explanation
-------- | ------------- | -----------
dataset | Single test file | A file list containing all analysis files.
alignments | None | An array of alignment sets for which the validation is run.
trackCollection     | "ALCARECOTkAlMinBias" | Track collection used for the analysis.
maxevents     | 1 | Maximum number of events before cmsRun terminates.
iovListFile  | "nothing" | File containing a list of IOVs one run boundary each line.
iovListList     | [0,500000] | If iovListFile is not defined, the IOV run boundaries are read from this array.
triggerFilter    | "nothing" | HLT trigger filter used to filter the events selected for the analysis.
printTriggers    | false | Print all available triggers to console. Set true only for local tests.
mc | false | Flag for Monte Carlo. Use true for MC and false for data.
profilePtBorders | [3,5,10,20,50,100] | List for pT borders used in wide pT bin profiles. Also the trend plots are calculated using these pT borders. If changed from default, the variable widePtBinBorders for the jetHtPlotter needs to be changed accordingly to get legends for trend plots correctly and avoid segmentation violations. This is done automatically by the all-in-one configuration.
TrackerAlignmentRcdFile | "nothing" | Local database file from which the TrackerAlignmentRcd is read. Notice that usual method to set this is reading from the database for each alignment.
TrackerAlignmentErrorFile | "nothing" | Local database file from which the TrackerAlignmentExtendedErrorRcd is read. Notice that usual method to set this is reading from the database for each alignment.

### File merging - addHistograms.sh

The addHistograms.sh script is used to merge the root files for jetHT validation. Merging is fast and can easily be done locally in seconds, but the tool is fully integrated to the all-in-one configuration for automated processing.

The instructions for standalone usage are:

```
addHistograms.sh [doLocal] [inputFolderName] [outputFolderName] [baseName]
doLocal = True: Merge local files. False: Merge files on CERN EOS
inputFolderName = Name of the folder where the root files are
outputFolderName = Name of the folder to which the output is transferred
baseName = Name given for the output file without .root extension
```

In use with all-in-one configuration, the following parameters can be set in the json/yaml configuration file:

```
All-in-one:

validations
    JetHT
        merge
            myDataset
```

Any number of datasets can be defined.

Variable | Default value | Explanation
-------- | ------------- | -----------
singles    | None | An array of single job names that must be finished before plotting can be run.
alignments | None | An array of alignment names for which the files are merged within those alignments. Different alignments are kept separate.


### Plotting - jetHtPlotter

The tool is originally designed to be used standalone, since the plotting the histograms locally does not take more that tens of second at maximum. But the plotter works also together with the all-in-one configuration. The only difference for user is the structure of the configuration file, that changes a bit between standalone and all-in-one usage.

Below are listed all the variables that can be configured in the json file for the jetHtPlotter macro. If the value is not given in the configuration, the default value is used instead.

```
Standalone:              All-in-one:

jethtplot                alignments
    alignments               myAlignment
        myAlignment
```

Up to four different alignments can be added for plotting. If more than four alignments are added, only first four are plotted.

Variable | Default value | Explanation
-------- | ------------- | -----------
inputFile   |  None | File containing the jetHT validation histograms for myAlignment. All-in-one config automatically uses default file name for merge job. Must be given if using plotter standalone.
legendText  | "AlignmentN" | Name with which the alignment is referred to in the legend of the drawn figures. For all-in-one configuration, this variable is called "title" instead of "legendText".
color       | Varies | Marker color used with this alignment
style       | 20 | Marker style used with this alignment

```
Standalone:               All-in-one:

jethtplot                 validations
    drawHistograms            JetHT
                                  plot
                                      myDataset
                                          jethtplot
                                              drawHistograms      
```

Select histograms to be drawn for each alignment.

Variable | Default value | Explanation
-------- | ------------- | -----------
drawDz       | false |  Draw probe track dz distributions
drawDzError  | false |  Draw probe track dz error distributions
drawDxy      | false |  Draw probe track dxy distributions
drawDxyError | false |  Draw probe track dxy error distributions

```
Standalone:               All-in-one:

jethtplot                 validations
    drawProfiles              JetHT
                                  plot
                                      myDataset
                                          jethtplot
                                              drawProfiles      
```

Select profile histograms to be drawn for each alignment.

Variable | Default value | Explanation
-------- | ------------- | -----------
drawDzErrorVsPt      | false | Draw dz error profiles as a function of pT
drawDzErrorVsPhi     | false | Draw dz error profiles as a function of phi
drawDzErrorVsEta     | false | Draw dz error profiles as a function of eta
drawDzErrorVsPtWide  | false | Draw dz error profiles as a function of pT in wide pT bins
drawDxyErrorVsPt     | false | Draw dxy error profiles as a function of pT
drawDxyErrorVsPhi    | false | Draw dxy error profiles as a function of phi
drawDxyErrorVsEta    | false | Draw dxy error profiles as a function of eta
drawDxyErrorVsPtWide | false | Draw dxy error profiles as a function of pT in wide pT bins

```
Standalone:               All-in-one:

jethtplot                 validations
    profileZoom               JetHT
                                  plot
                                      myDataset
                                          jethtplot
                                              profileZoom     
```

Axis zooms for profile histograms.

Variable | Default value | Explanation
-------- | ------------- | -----------
minZoomPtProfileDz       | 28 | Minimum y-axis zoom value for dz error profiles as a function of pT
maxZoomPtProfileDz       | 60 | Maximum y-axis zoom value for dz error profiles as a function of pT
minZoomPhiProfileDz      | 45 | Minimum y-axis zoom value for dz error profiles as a function of phi
maxZoomPhiProfileDz      | 80 | Maximum y-axis zoom value for dz error profiles as a function of phi
minZoomEtaProfileDz      | 30 | Minimum y-axis zoom value for dz error profiles as a function of eta
maxZoomEtaProfileDz      | 95 | Maximum y-axis zoom value for dz error profiles as a function of eta
minZoomPtWideProfileDz   | 25 | Minimum y-axis zoom value for dz error profiles as a function of pT in wide pT bins
maxZoomPtWideProfileDz   | 90 | Maximum y-axis zoom value for dz error profiles as a function of pT in wide oT bins
minZoomPtProfileDxy      | 7 | Minimum y-axis zoom value for dxy error profiles as a function of pT
maxZoomPtProfileDxy      | 40 | Maximum y-axis zoom value for dxy error profiles as a function of pT
minZoomPhiProfileDxy     | 40 | Minimum y-axis zoom value for dxy error profiles as a function of phi
maxZoomPhiProfileDxy     | 70 | Maximum y-axis zoom value for dxy error profiles as a function of phi
minZoomEtaProfileDxy     | 20 | Minimum y-axis zoom value for dxy error profiles as a function of eta
maxZoomEtaProfileDxy     | 90 | Maximum y-axis zoom value for dxy error profiles as a function of eta
minZoomPtWideProfileDxy  | 20 | Minimum y-axis zoom value for dxy error profiles as a function of pT in wide pT bins
maxZoomPtWideProfileDxy  | 80 | Maximum y-axis zoom value for dxy error profiles as a function of pT in wide pT bins

```
Standalone:               All-in-one:

jethtplot                 validations
    drawTrends                JetHT
                                  plot
                                      myDataset
                                          jethtplot
                                              drawTrends     
```

Select trend histograms to be drawn for each alignment.

Variable | Default value | Explanation
-------- | ------------- | -----------
drawDzError  | false | Draw the trend plots for dz errors
drawDxyError | false | Draw the trend plots for dxy errors

```
Standalone:               All-in-one:

jethtplot                 validations
    trendZoom                 JetHT
                                  plot
                                      myDataset
                                          jethtplot
                                              trendZoom     
```

Axis zooms for trend histograms.

Variable | Default value | Explanation
-------- | ------------- | -----------
minZoomDzTrend   | 20 | Minimum y-axis zoom value for dz error trends
maxZoomDzTrend   | 95 | Maximum y-axis zoom value for dz error trends
minZoomDxyTrend  | 10 | Minimum y-axis zoom value for dxy error trends
maxZoomDxyTrend  | 90 | Maximum y-axis zoom value for dxy error trends

```
Standalone:               All-in-one:

jethtplot                 validations
                              JetHT
                                  plot
                                      myDataset
                                          jethtplot    
```

All the remaining parameters

Variable | Default value | Explanation
-------- | ------------- | -----------
drawTrackQA            | false | Draw the track QA plots (number of vertices, tracks per vertex, track pT, track phi, and track eta).
drawYearLines          | false | Draw vertical lines to trend plot marking for example different years of data taking.
runsForLines           | [0] | List of run numbers to which the vertical draws in the trend plots are drawn.
lumiPerIovFile         | "lumiPerRun_Run2.txt" | File containing integrated luminosity for each IOV.
drawPlotsForEachIOV    | false | true: For profile plots, draw the profiles separately for each IOV defined in the lumiPerIovFile. false: Only draw plots integrated over all IOVs.
nIovInOnePlot          | 1 | Number of successive IOV:s drawn in a single plot is profile plots are drawn for each IOV.
useLuminosityForTrends | true | true: For trend plots, make the width of the x-axis bin for each IOV proportional to the integrated luminosity within that IOV. false: Each IOV has the same bin width in x-axis.
skipRunsWithNoData     | false | true: If an IOV defined in lumiPerIovFile does not have any data, do not draw empty space to the x-axis for this IOV. false: Draw empty space for IOV:s with no data.
widePtBinBorders       | [3,5,10,20,50,100] | List for pT borders used in wide pT bin profiles. Also the trend plots are calculated using these pT borders. This needs to be set to same value as the profilePtBorders variable used in the corresponding validation analysis. The all-in-one config does this automatically for you.
normalizeQAplots       | true | true: For track QA plots, normalize each distribution with its integral. false: No normalization for QA plots, show directly the counts.
makeIovlistForSlides   | false | true: Create a text file to be used as input for prepareSlides.sh script for making latex presentation template with profile plots from each IOV. false: Do not do this.
iovListForSlides       | "iovListForSlides.txt" | Name given to the above list.
saveComment            | "" | String of text added to all saved figures.

```
All-in-one:

validations
    JetHT
        plot
            myDataset
```

Parameters only used in all-in-one tool. Any number of datasets can be defined.

Variable | Default value | Explanation
-------- | ------------- | -----------
merges     | None | An array of merge job names that must be finished before plotting can be run.
alignments | None | An array of alignment names that will be plotted.

