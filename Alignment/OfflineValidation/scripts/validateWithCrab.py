#!/usr/bin/env python

import argparse
import sys
import os
import subprocess
import yaml
import time
from termcolor import colored
from multiprocessing import Process, Pool, cpu_count

try:
    from CRABAPI.RawCommand import crabCommand
    from CRABClient.UserUtilities import config
    from CRABClient.ClientExceptions import TaskNotFoundException, CachefileNotFoundException, ConfigException

except ImportError:
    print("Crab is not sourced! Use 'source /cvmfs/cms.cern.ch/crab3/crab.sh' to source crab properly")
    sys.exit(0)

def parser():
    parser = argparse.ArgumentParser(description = "AllInOneTool for the crab submission of validations jobs", formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("info", metavar='info', type=str, action="store", help="Csv file with list of directories set up for validation jobs configured by 'validateAlignments.py'")
    parser.add_argument("-s", "--submit", action = "store_true", help ="Submit all the crab jobs")
    parser.add_argument("-m", "--monitor", action = "store_true", help ="Monitor all the crab jobs")

    return parser.parse_args()

def crabConfig(jobName, jobDir):
    ##Get local validation config
    with open("{}/validation.yaml".format(jobDir), "r") as configFile:
        valiConf = yaml.load(configFile, Loader=yaml.Loader)

    ##Crab config
    crabConf = config()

    crabConf.General.requestName = jobName
    crabConf.General.workArea = jobDir
    crabConf.General.transferOutputs = True

    crabConf.JobType.pluginName = "Analysis"
    crabConf.JobType.psetName = "{}/validation_cfg.py".format(jobDir)
    crabConf.JobType.pyCfgParams = ["config={}/validation.json".format(jobDir), "isCrab=1"]
    crabConf.JobType.maxJobRuntimeMin = 1440
    crabConf.JobType.maxMemoryMB = 2500

    relDir = valiConf["output"][valiConf["output"].find("/store"):]
    
    if not "/store/group/alca_trackeralign/" in relDir:
        raise Exception("Crab out dir needs to be '.../store/group/alca_trackeralign/...'. Given dir: '{}'".format(outDir))

    crabConf.Data.userInputFiles = [f.replace("\n", "") for f in open(valiConf["validation"]["dataset"]).readlines()]
    crabConf.Data.outLFNDirBase = relDir
    crabConf.Data.unitsPerJob = len(crabConf.Data.userInputFiles)
    crabConf.Data.splitting = "FileBased"
    crabConf.Data.outputPrimaryDataset = "Validation"
    
    crabConf.Site.whitelist = ["T2_CH_CERN"]
    crabConf.Site.storageSite = "T2_CH_CERN"

    return crabConf

def submit(name, config):
    crabDir = "{}/crab_{}".format(config.General.workArea, config.General.requestName)

    ##Try to submit
    try:
        sys.stdout = open(os.devnull, 'w')
        crabCommand("submit", config=config)
        sys.stdout = sys.__stdout__

        print("{}: Succesfully submitted".format(name))

    ##Handle exception
    except ConfigException as e:
        ##Check if this is already crab directory, if not delete and retry submitting
        try:
            crabCommand("status", dir=crabDir)

        except CachefileNotFoundException as e2:
            subprocess.check_output("command rm -rfv {}".format(crabDir), shell = True)
            crabCommand("submit", config=config)

            sys.stdout = sys.__stdout__

            print("{}: Succesfully submitted".format(name))
            return
      
        ##Already submitted crab, dont do anything
        if "already exists" in str(e):
            sys.stdout = sys.__stdout__
            print("{}: Already submitted".format(name))

def monitor(name, crabJob):
    out = "{}: ".format(name)
    colorStatus = {
        "running": colored("RUNNING", "blue"),
        "finished": colored("FINISHED", "green"),
        "failed": colored("FAILED", "red"),
    }

    ##Crab dir
    crabDir = "{}/crab_{}".format(crabJob.General.workArea, crabJob.General.requestName)

    ##Get status, if not status, submit the job
    try:
        sys.stdout = open(os.devnull, 'w')
        crabStatus = crabCommand("status", dir=crabDir)
        sys.stdout = sys.__stdout__

    except:
        sys.stdout = sys.__stdout__
        submit(name, config)

        return ""

    ##Only one job, because it is configured this way for the validation
    job = crabStatus["jobs"].values()[0] if len(crabStatus["jobs"].values()) != 0 else {"State": ""}

    out += colorStatus[job["State"]] if job["State"] in colorStatus else "PENDING"

    if job["State"] == "failed":
        out += " " + "[Error code = {}] (Resubmitting...)".format(job["Error"][0] if "Error" in job else -1)

        ##Try to resubmit, in case of failing to just next iteration
        try:
            sys.stdout = open(os.devnull, 'w')
            crabCommand("resubmit", dir=crabDir)
            sys.stdout = sys.__stdout__

        except:
            pass

    if job["State"] == "finished":
        sys.stdout = open(os.devnull, 'w')
        crabOutFile = "/eos/cms/" + crabCommand("getoutput", dir = crabDir, dump = True)["lfn"][0]
        sys.stdout = sys.__stdout__

        wishedOutFile = "/eos/cms/{}/{}".format(crabJob.Data.outLFNDirBase, crabOutFile.split("/")[-1].replace("_1.root", ".root"))
    
        subprocess.check_output(["cp", crabOutFile, wishedOutFile])

    return out

def main():
    ##Read parser arguments
    args = parser()

    ##List of crab jobs
    crabConfigs = {}

    ##Read out job dirs
    with open(args.info, "r") as crabInfo:
        for line in crabInfo:
            jobName, jobDir = line.replace("\n", "").split("\t")
            crabConfigs[jobName] = crabConfig(jobName, jobDir)

    ##Submit job if wished
    if args.submit:
        print("\nStart submission of crab jobs\n")

        jobs = [Process(target=submit, args = (name, config)) for name, config in crabConfigs.items()]

        for job in jobs:
            job.start()
            job.join()

    ##Monitor jobs
    if args.monitor:
        p = Pool(cpu_count())

        while crabConfigs:
            print("\nStatus of crab jobs ({})\n".format(time.asctime()))

            jobs = {name: p.apply_async(monitor, (name, config)) for name, config in crabConfigs.items()}

            for name, job in jobs.items():
                out = job.get()
                if out != "":
                    print(out)

                if "FINISHED" in out:
                    crabConfigs.pop(name)

            time.sleep(60)

	print("\nAll crab jobs has finished, proceed the submission chain using 'condor_submit_dag {}/DAG/dagFile'".format("/".join(args.info.split("/")[:-1])))

if __name__ == "__main__":
    main()
