#ifndef MTS_H
#define MTS_H

#include <vector>
#include <iostream>
#include <memory>
#include <filesystem>
#include <numeric>
#include <string>

#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TProfile.h>
#include <TParameter.h>

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

        std::string GetUnit(const std::string& label){
            std::string unit = "";
    
            if(label.find_last_of("(") != std::string::npos and label.find("#eta") == std::string::npos and label.find("#delta") == std::string::npos){
                unit = label.substr(label.find_last_of("(") + 1, 
                       label.find_last_of(")") - label.find_last_of("(") - 1);
            }

            return unit;
        }

        template <typename T, typename = typename std::enable_if<std::is_base_of<TH1, T>::value, bool>::type>
        std::shared_ptr<T> BuildHist(const bool& isPixel, const std::string& xVar, const std::string& yVar = ""){
            //Build either TH1D or TH2D by templating
            std::shared_ptr<T> hist = std::make_shared<T>();

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

                std::string binWidth = std::to_string(hist->GetBinWidth(1));
                binWidth.erase(binWidth.find_last_not_of('0') + 1, std::string::npos); //Remove trailing zeros
                if(binWidth.size() - 1 == binWidth.find(".")) binWidth.erase(binWidth.size() - 1, std::string::npos);

                //Extract unit from x label (wanted to avoid another map just for units)
                hist->GetYaxis()->SetTitle(("Number of tracks / " + binWidth + " " + GetUnit(axisLabels[xVar])).c_str());
            }
        
            return hist;
        }

    public:
        MTSPlotter();
        std::string ProduceHists(const std::string& inputFile, const std::string& outDir, const std::string& alignName, const float& outlier = -1);
        void Plot(const std::vector<std::string>& histFiles, const std::string& outDir, const std::vector<int>& colors, const std::vector<int>& styles, const std::vector<std::string>& titles, const std::string& rightTitle);
};

#endif
