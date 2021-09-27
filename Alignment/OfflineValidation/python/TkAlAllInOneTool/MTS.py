import copy
import os

def MTS(config, validationDir):
    ##List with all jobs
    jobs = []

    for datasetName in config["validations"]["MTS"]:
        singleOutput = {}
        mergeDependencies = []

        for alignment in config["validations"]["MTS"][datasetName]["alignments"]:
            ##Work directory for each IOV
            workDir = "{}/MTS/single/{}/{}/".format(validationDir, datasetName, alignment)

            ##Write local config
            local = {}
            local["output"] = "{}/{}/single/{}/{}".format(config["LFS"], config["name"], alignment, datasetName)
            local["alignment"] = copy.deepcopy(config["alignments"][alignment])
            local["validation"] = copy.deepcopy(config["validations"]["MTS"][datasetName])
            local["validation"].pop("alignments")

            singleOutput[alignment] = "{}/MTS.root".format(local["output"])

            ##Write job info
            job = {
                "name": "MTS_single_{}_{}".format(datasetName, alignment),
                "dir": workDir,
                "exe": "cmsRun",
                "cms-config": "{}/src/Alignment/OfflineValidation/python/TkAlAllInOneTool/MTS_cfg.py".format(os.environ["CMSSW_BASE"]),
                "run-mode": config["validations"]["MTS"][datasetName].get("run-mode", "Condor"),
                "dependencies": [],
                "config": local, 
            }

            mergeDependencies.append(job["name"])
            jobs.append(job)

        ##Work directory for each IOV
        workDir = "{}/MTS/merge/{}/".format(validationDir, datasetName)

        ##Write local config
        local = {}
        local["output"] = "{}/{}/merge/{}".format(config["LFS"], config["name"], datasetName)
        local["alignment"] = copy.deepcopy(config["alignments"])

        for a in config["alignments"].keys():
            local["alignment"][a]["input"] = singleOutput[a]

        local["validation"] = copy.deepcopy(config["validations"]["MTS"][datasetName])
        local["validation"].pop("alignments")

        ##Write job info
        job = {
            "name": "MTS_merge_{}".format(datasetName),
            "dir": workDir,
            "exe": "MTSmerge",
            "run-mode": "Condor",
            "dependencies": mergeDependencies,
            "config": local, 
        }

        jobs.append(job)
        
    return jobs
