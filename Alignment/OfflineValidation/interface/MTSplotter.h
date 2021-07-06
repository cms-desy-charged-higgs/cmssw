#ifndef MTS_H
#define MTS_H

#include <vector>
#include <iostream>
#include <memory>
#include <filesystem>

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TProfile.h>

#include <Alignment/OfflineValidation/interface/TkAlStyle.h>

struct Binning{
    int nBins;
    double min;
    double max;
};

class MTSPlotter {
    private:
        std::vector<std::string> variables;
        std::vector<float> rel;
        std::map<std::string, int> pixelBinFactor;
        std::map<std::string, Binning> binning;    
        std::map<std::string, std::string> axisLabels;

        template <typename T, typename = typename std::enable_if<std::is_base_of<TH1, T>::value, bool>::type>
        T* BuildHist(const bool& isPixel, const std::string& xVar, const std::string& yVar = ""){
            //Build either TH1D or TH2D by templating
            T* hist = new T();

            std::string histName = yVar != "" ? xVar + "_VS_" + yVar : xVar;

            hist->SetName(histName.c_str());
            hist->SetTitle(histName.c_str());

            //Check if variable is pull and set binning
            float nBinsX, nBinsY, minX, minY, maxX, maxY;

            bool isPullX = xVar.find("pull") != std::string::npos;
            bool isPullY = yVar.find("pull") != std::string::npos;

            if(xVar != ""){
                nBinsX = binning[xVar].nBins; minX = binning[xVar].min; maxX = binning[xVar].max;

                if(isPixel and pixelBinFactor.count(xVar)){
                    minX /= pixelBinFactor[xVar];
                    maxX /= pixelBinFactor[xVar];
                }

                if(isPullX){
                    nBinsX = 40; minX = -5; maxX = 5;
                }
            }

            if(yVar != ""){
                nBinsY = binning[yVar].nBins; minY = binning[yVar].min; maxY = binning[yVar].max;

                if(isPixel and pixelBinFactor.count(yVar)){
                    minY /= pixelBinFactor[yVar];
                    maxY /= pixelBinFactor[yVar];
                }

                if(isPullY){
                    nBinsY = 40; minY = -5; maxY = 5;
                }
            }

            //2D histograms axis labels
            if(xVar != "" and yVar != ""){
                hist->SetBins(nBinsX, minX, maxX, nBinsY, minY, maxY);
                hist->GetXaxis()->SetTitle(axisLabels[xVar].c_str());
                hist->GetYaxis()->SetTitle(axisLabels[yVar].c_str());
            }

            //1D histograms axis labels
            else{
                hist->SetBins(nBinsX, minX, maxX);
                hist->GetXaxis()->SetTitle(axisLabels[xVar].c_str());
                hist->GetYaxis()->SetTitle("Number of tracks");
            }
        
            return hist;
        }

    public:
        MTSPlotter(){
            //Hardcoded variables, binning, axis labels
            variables = {"pt",  "eta", "phi", "dz",  "dxy", "theta", "qoverpt"};
            rel = {1., 1., 1e-3, 1e-4, 1e-4, 1e-3, 1e-3};

            pixelBinFactor = {
                {"dxy", 10},
                {"dz", 10},
                {"Delta_dxy", 10},
                {"Delta_dz", 10},
                {"Delta_theta", 10},
                {"Delta_eta", 2},
            };

            binning = {
                {"pt", {38, 5, 100}},
                {"qoverpt", {35, -0.35, 0.35}},
                {"dxy", {20, -100, 100}},
                {"dz", {20, -250, 250}},
                {"theta", {40, 0, 2.5}},
                {"eta", {40, -1.2, 1.2}},
                {"phi", {30, -3.14, 0.}},
                {"Delta_pt", {30, -0.8, 0.8}},
                {"Delta_qoverpt", {50, -2.5, 2.5}},
                {"Delta_dxy", {50, -1250, 1250}},
                {"Delta_dz", {40, -2000, 2000}},
                {"Delta_theta", {50, -10, 10}},
                {"Delta_eta", {30, -0.007, 0.007}},
                {"Delta_phi", {40, -2, 2}},
            };

            axisLabels = {
                {"pt", "p_{T} (GeV)"},
                {"qoverpt", "q/p_{T} (e/GeV)"},
                {"dxy", "d_{xy} (cm)"},
                {"dz", "d_{z} (cm)"},
                {"theta", "#theta [rad]"},
                {"eta", "#eta [rad]"},
                {"phi", "#phi [rad]"},
                {"Delta_pt", "#Delta p_{T} (GeV)"},
                {"Delta_qoverpt", "#Delta q/p_{T} (e/GeV)"},
                {"Delta_dxy", "#Delta d_{xy} (cm)"},
                {"Delta_dz", "#Delta d_{z} (cm)"},
                {"Delta_theta", "#Delta#theta [rad]"},
                {"Delta_eta", "#Delta#eta [rad]"},
                {"Delta_phi", "#Delta#phi [rad]"},
                {"Delta_pt_pull", "#Delta p_{T}/#delta(Delta p_{T}) (GeV)"},
                {"Delta_qoverpt_pull", "#Delta q/p_{T}/#delta(#Delta q/p_{T}) (e/GeV)"},
                {"Delta_dxy_pull", "#Delta d_{xy}/#delta(#Delta d_{xy}) (cm)"},
                {"Delta_dz_pull", "#Delta d_{z}/#delta(#Delta d_{z}) (cm)"},
                {"Delta_theta_pull", "#Delta#theta/#delta(#Delta#theta) [rad]"},
                {"Delta_eta_pull", "#Delta#eta/#delta(#Delta#eta) [rad]"},
                {"Delta_phi_pull", "#Delta#phi/#delta(#Delta#phi) [rad]"},
            };
        }

        std::string ProduceHists(const std::string& inputFile, const std::string& outDir, const std::string& alignName);
        void Plot(const std::vector<std::string>& histFiles, const std::string& outDir, const std::vector<int>& colors, const std::vector<int>& styles, const std::vector<std::string>& titles);
};

#endif
