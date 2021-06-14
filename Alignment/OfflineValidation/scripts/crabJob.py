#!/usr/bin/env python

import sys
import os
import yaml
import time
import argparse
import subprocess
from subprocess import Popen, PIPE, STDOUT

try:
    from CRABAPI.RawCommand import crabCommand
    from CRABClient.UserUtilities import config
    from CRABClient.ClientExceptions import TaskNotFoundException, CachefileNotFoundException, ConfigException

except ImportError:
    print("Crab is not sourced! Use 'source /cvmfs/cms.cern.ch/crab3/crab.sh' to source crab properly")
    sys.exit(-1)

def parser():
    parser = argparse.ArgumentParser(description = "AllInOneTool for the crab submission of validations jobs", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("dir", metavar='dir', type=str, action="store", help="Directory which will be used to submit crab")
    parser.add_argument("-n", "--name", action = "store", help ="Job name")

    return parser.parse_args()

def nicePrint(func):
    def doPrint(*args, **kwargs):
        sys.stdout = open(os.devnull, 'w')
        out = func(*args, **kwargs)
        sys.stdout = sys.__stdout__

        if not out.startswith("["):
            out = "[{}] ".format(time.asctime()) + out

        print(out)

        return out

    return doPrint

def prepareCrab(jobName, jobDir):
    ##Get local validation config
    with open("{}/validation.yaml".format(jobDir), "r") as configFile:
        valiConf = yaml.load(configFile, Loader=yaml.Loader)

    ##Crab config with set up for a shell script
    crabConf = config()

    crabConf.General.requestName = jobName
    crabConf.General.workArea = jobDir
    crabConf.General.transferOutputs = True

    crabConf.JobType.pluginName = "Analysis"
    crabConf.JobType.psetName = "{}/validation_cfg.py".format(jobDir) ##Dummy file, otherwise crab will complain
    crabConf.JobType.inputFiles = ["{}/validation.py".format(jobDir)] ##Real cms python config which will be used
    crabConf.JobType.maxJobRuntimeMin = 1440
    crabConf.JobType.maxMemoryMB = 2500
    crabConf.JobType.scriptExe = "{}/runCrab.sh".format(jobDir)

    relDir = valiConf["output"][valiConf["output"].find("/store"):]
    
    if not "/store/group/alca_trackeralign/" in relDir:
        raise Exception("Crab out dir needs to be '.../store/group/alca_trackeralign/...'. Given dir: '{}'".format(outDir))

    crabConf.Data.userInputFiles = ["/store/data/Run2018A/HLTPhysics/ALCARECO/TkAlMinBias-06Jun2018-v1/40000/92C90299-6D9C-E811-B797-00259029ED0E.root"] ##Dummy file, otherwise crab will complain
    crabConf.Data.outLFNDirBase = relDir 
    crabConf.Data.unitsPerJob = 1 ##Dummy configuration, otherwise crab will complain
    crabConf.Data.splitting = "FileBased" ##Dummy configuration, otherwise crab will complain
    crabConf.Data.outputPrimaryDataset = "Validation"
    
    crabConf.Site.whitelist = ["T2_CH_CERN"]
    crabConf.Site.storageSite = "T2_CH_CERN"

    return crabConf

@nicePrint
def submit(name, crabJob):
    crabDir = "{}/crab_{}".format(crabJob.General.workArea, crabJob.General.requestName)
    workDir = crabJob.General.workArea
    out = ""

    ##Remove old dir if existing
    if os.path.exists(crabDir):
        try:
            status = crabCommand("status", dir=crabDir)["jobs"].values()[0]["State"]

            out += "Job '{}' is submitted already with status '{}'".format(name, status)

            return out

        except:
            subprocess.check_output("command rm -rf {}".format(crabDir), shell = True)

    ##Write exe for crab
    with open("{}/runCrab.sh".format(crabJob.General.workArea), "w") as runCrab:
        runContent = [
            "#!/bin/bash\n",
            "cmsRun -j FrameworkJobReport.xml -p validation.py"
        ]

        for line in runContent:
            runCrab.write(line)

    subprocess.call("chmod a+rx {}/runCrab.sh".format(crabJob.General.workArea), shell = True)

    ##Write cms config extended in one python file
    with open("{}/validation.py".format(workDir), "w") as vali:
        p = Popen("python -i {w}/validation_cfg.py config={w}/validation.json isCrab=1".format(w=workDir), shell = True, stdout=PIPE, stdin=PIPE)
        vali.write(p.communicate("print(process.dumpPython())")[0])

    ##Try to submit
    try:
        crabCommand("submit", config=crabJob)

        out += "Job '{}' sucessfully submitted".format(name)
        subprocess.check_output("command rm -rf {}/crabRun.sh {}/validation.py".format(workDir, workDir), shell = True)

    except Exception as e:
        subprocess.check_output("command rm -rf {}/crabRun.sh {}/validation.py".format(workDir, workDir), shell = True)
        raise Exception("Crab job failed for submitting '{}' because '{}'".format(name, str(e)))

    return out

@nicePrint
def monitor(name, crabJob):
    ##Crab dir
    crabDir = "{}/crab_{}".format(crabJob.General.workArea, crabJob.General.requestName)

    out = ""

    ##Get status, if not status, submit the job
    try:
        crabStatus = crabCommand("status", dir=crabDir)
        
    except Exception as e:
        return submit(name, crabJob)

    ##Only one job, because it is configured this way for the validation
    job = crabStatus["jobs"].values()[0] if len(crabStatus["jobs"].values()) != 0 else {"State": "pending"}

    if job["State"] == "failed":
        out += "Job '{}' failed with error code '{}' and will be resubmitted".format(name, job["Error"][0] if "Error" in job else -1)

        ##Try to resubmit, in case of failing to just next iteration
        try:
            crabCommand("resubmit", dir=crabDir)

        except Exception as e:
            pass

        return out

    return "Job '{}' is '{}'".format(name, job["State"])

@nicePrint
def getOutput(name, crabJob):
    ##Crab dir
    crabDir = "{}/crab_{}".format(crabJob.General.workArea, crabJob.General.requestName)

    outPut = crabCommand("getoutput", dir = crabDir, dump = True)

    while not "lfn" in outPut:
        outPut = crabCommand("getoutput", dir = crabDir, dump = True)

    crabOutFile = "/eos/cms/" + outPut["lfn"][0]
    wishedOutFile = "/eos/cms/{}/{}".format(crabJob.Data.outLFNDirBase, crabOutFile.split("/")[-1].replace("_1.root", ".root"))
    
    subprocess.check_output(["cp", crabOutFile, wishedOutFile])

    out = "Output file transfered: {}".format(wishedOutFile)

    return out

def main():
    ##Read parser arguments
    args = parser()

    ##List of crab jobs
    crabConfigs = {}

    ##Read out job dirs
    crabJob = prepareCrab(args.name, args.dir)

    ##Submit job
    submit(args.name, crabJob)

    ##Monitor jobs
    while(True):
        out = monitor(args.name, crabJob)
        
        if "finished" in out:
            getOutput(args.name, crabJob)
            break

        time.sleep(30)

if __name__ == "__main__":
    main()
