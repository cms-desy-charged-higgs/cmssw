import copy
import os

def MTS(config, validationDir):
    ##List with all jobs
    jobs = []

    for datasetName in config["validations"]["MTS"]:
        for alignment in config["validations"]["MTS"][datasetName]["alignments"]:
            ##Work directory for each IOV
            workDir = "{}/MTS/{}/{}/".format(validationDir, datasetName, alignment)

            ##Write local config
            local = {}
            local["output"] = "{}/{}/{}/{}".format(config["LFS"], config["name"], alignment, datasetName)
            local["alignment"] = copy.deepcopy(config["alignments"][alignment])
            local["validation"] = copy.deepcopy(config["validations"]["MTS"][datasetName])
            local["validation"].pop("alignments")

            ##Write job info
            job = {
                "name": "MTS_{}_{}".format(datasetName, alignment),
                "dir": workDir,
                "exe": "cmsRun",
                "cms-config": "{}/src/Alignment/OfflineValidation/python/TkAlAllInOneTool/MTS_cfg.py".format(os.environ["CMSSW_BASE"]),
                "run-mode": "Condor",
                "dependencies": [],
                "config": local, 
            }

            jobs.append(job)

    return jobs
