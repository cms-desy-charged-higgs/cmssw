#include <Alignment/OfflineValidation/interface/MTSplotter.h>

MTSPlotter::MTSPlotter(){
    //Hardcoded variables, binning, axis labels
    variables = {"pt",  "eta", "phi", "dz",  "dxy", "theta", "qoverpt"};
    rel = {1., 1., 1e3, 1e4, 1e4, 1e3, 1e3};

    pixelBinFactor = {
        {"dxy", 10},
        {"dz", 10},
        {"Delta_dxy", 10},
        {"Delta_dz", 10},
        {"Delta_theta", 2},
        {"Delta_eta", 2},
    };

    binning = {
        {"pt", {38, 5, 100}},
        {"qoverpt", {35, -0.35, 0.35}},
        {"dxy", {20, -100, 100}},
        {"dz", {20, -250, 250}},
        {"theta", {40, 0, 2.5}},
        {"eta", {40, -1.2, 1.2}},
        {"phi", {30, -3, 0.}},
        {"Delta_pt", {40, -0.8, 0.8}},
        {"Delta_qoverpt", {50, -2.5, 2.5}},
        {"Delta_dxy", {50, -1250, 1250}},
        {"Delta_dz", {40, -2000, 2000}},
        {"Delta_theta", {50, -10, 10}},
        {"Delta_eta", {30, -0.006, 0.006}},
        {"Delta_phi", {40, -2, 2}},
    };

    axisLabels = {
        {"pt", "p_{T} (GeV)"},
        {"qoverpt", "q / p_{T} (e/GeV)"},
        {"dxy", "d_{xy} (cm)"},
        {"dz", "d_{z} (cm)"},
        {"theta", "#theta (rad)"},
        {"eta", "#eta"},
        {"phi", "#phi (rad)"},
        {"Delta_pt", "#Deltap_{T} / #sqrt{2} (GeV)"},
        {"Delta_qoverpt", "#Deltaq / p_{T} (x10^{-3}e/GeV)"},
        {"Delta_dxy", "#Deltad_{xy} / #sqrt{2} (#mum)"},
        {"Delta_dz", "#Deltad_{z} / #sqrt{2} (#mum)"},
        {"Delta_theta", "#Delta#theta / #sqrt{2} (mrad)"},
        {"Delta_eta", "#Delta#eta / #sqrt{2}"},
        {"Delta_phi", "#Delta#phi / #sqrt{2} (mrad)"},
        {"Delta_pt_pull", "#Deltap_{T} / #delta(#Deltap_{T})"},
        {"Delta_qoverpt_pull", "#Deltaq/p_{T} / #delta(#Deltaq/p_{T})"},
        {"Delta_dxy_pull", "#Deltad_{xy} / #delta(#Deltad_{xy})"},
        {"Delta_dz_pull", "#Deltad_{z} / #delta(#Deltad_{z})"},
        {"Delta_theta_pull", "#Delta#theta / #delta(#Delta#theta)"},
        {"Delta_eta_pull", "#Delta#eta / #delta(#Delta#eta)"},
        {"Delta_phi_pull", "#Delta#phi / #delta(#Delta#phi))"},
    };
}

std::string MTSPlotter::ProduceHists(const std::string& inputFile, const std::string& outDir, const std::string& alignName, const float& outlier){
    //Get TTree
    std::shared_ptr<TFile> file(TFile::Open(inputFile.c_str(), "READ"));
    std::shared_ptr<TTree> tree(file->Get<TTree>("cosmicValidation/splitterTree"));

    //Vector for storing the readout of the TTree for the several variables
    std::vector<double> splitVar1(variables.size(), 0.), splitVar1Err(variables.size(), 0.),
                        splitVar2(variables.size(), 0.), splitVar2Err(variables.size(), 0.),
                        orgVar(variables.size(), 0.), orgVarErr(variables.size(), 0.),
                        DeltaVar(variables.size(), 0.);

    //Vector with values used for calc mean/stdDev
    std::vector<std::vector<double>> orgVarVal(variables.size(), std::vector<double>()), 
                        pullVarVal(variables.size(), std::vector<double>()), 
                        DeltaVarVal(variables.size(), std::vector<double>());

    //Vector with histograms to be filled
    std::vector<std::shared_ptr<TH1D>> orgHists(variables.size(), nullptr), 
                       deltaHists(variables.size(), nullptr), pullHists(variables.size(), nullptr);
    std::vector<std::shared_ptr<TH2D>> orgVsOrgHist(variables.size()*variables.size(), nullptr),
                       orgVsDeltaHist(variables.size()*variables.size(), nullptr),
                       DeltaVsDeltaHist(variables.size()*variables.size(), nullptr),
                       orgVsPullHist(variables.size()*variables.size(), nullptr),
                       DeltaVsPullHist(variables.size()*variables.size(), nullptr),
                       PullVsPullHist(variables.size()*variables.size(), nullptr);

    //Open outfile for histograms
    std::filesystem::create_directories(outDir);
    std::shared_ptr<TFile> out = std::make_shared<TFile>((outDir + "/" + alignName + ".root").c_str(), "RECREATE");

    std::cout << "Opened input file: " << inputFile << std::endl;
    std::cout << "Opened output file: " << out->GetName() << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    for(std::size_t j = 0; j < variables.size(); ++j){
        //Set Branch status off all variables
        tree->SetBranchAddress((variables[j] + "1_spl").c_str(), &splitVar1[j]);
        tree->SetBranchAddress((variables[j] + "2_spl").c_str(), &splitVar2[j]);
        tree->SetBranchAddress((variables[j] + "1Err_spl").c_str(), &splitVar1Err[j]);
        tree->SetBranchAddress((variables[j] + "2Err_spl").c_str(), &splitVar2Err[j]);
        tree->SetBranchAddress((variables[j] + "_org").c_str(), &orgVar[j]);
        tree->SetBranchAddress(("Delta_" + variables[j]).c_str(), &DeltaVar[j]);

        //Instiate 1D histograms
        orgHists[j] = BuildHist<TH1D>(true, variables[j]);
        deltaHists[j] = BuildHist<TH1D>(true, "Delta_" + variables[j]);
        pullHists[j] = BuildHist<TH1D>(true, "Delta_" + variables[j] + "_pull");

        //Instiate 2D histograms
        for(std::size_t k = 0; k < variables.size(); ++k){
            orgVsOrgHist[k + j*variables.size()] = BuildHist<TH2D>(true, variables[j], variables[k]);
            orgVsDeltaHist[k + j*variables.size()] = BuildHist<TH2D>(true, variables[j], "Delta_" + variables[k]);
        }
    }

    //Loop and fill histograms
    for(int i = 0; i < tree->GetEntries(); ++i){
        if(i % 1000 == 0 and i != 0) std::cout << "Processed " << i << " events!" << std::endl;
        tree->GetEntry(i);

        for(std::size_t j = 0; j < variables.size(); ++j){
            double error = std::sqrt(splitVar1Err[j] * splitVar1Err[j] + splitVar2Err[j] * splitVar2Err[j]);

            int orgBin = orgHists[j]->Fill(orgVar[j]);
            int deltaBin = deltaHists[j]->Fill(DeltaVar[j]*rel[j]/std::sqrt(2.));
            int pullBin = pullHists[j]->Fill(DeltaVar[j]/error);

            for(std::size_t k = 0; k < variables.size(); ++k){
                orgVsOrgHist[k + j*variables.size()]->Fill(orgVar[j], orgVar[k]);
                orgVsDeltaHist[k + j*variables.size()]->Fill(orgVar[j], DeltaVar[k]*rel[k]/std::sqrt(2.));
            }

            if(orgBin > 0) orgVarVal[j].push_back(orgVar[j]);
            if(deltaBin > 0) DeltaVarVal[j].push_back(DeltaVar[j]*rel[j]/std::sqrt(2.));
            if(pullBin > 0) pullVarVal[j].push_back(DeltaVar[j]/error);
        }
    }

    out->cd();

    //Save histograms for variables in another root file which will be read out in MTSPlotter::Plot function
    for(std::size_t j = 0; j < variables.size(); ++j){
        orgHists[j]->Write();
        deltaHists[j]->Write();
        pullHists[j]->Write();

        for(std::size_t k = 0; k < variables.size(); ++k){
            if(j == k) continue; //skip redudant x vs x
    
            orgVsOrgHist[k + j*variables.size()]->Write();
            orgVsDeltaHist[k + j*variables.size()]->Write();
        }

        //Sort values, remove outlier if wished and calc mean/stddev
        std::sort(orgVarVal[j].begin(), orgVarVal[j].end());
        std::sort(DeltaVarVal[j].begin(), DeltaVarVal[j].end());
        std::sort(pullVarVal[j].begin(), pullVarVal[j].end());

        if(outlier > 0.){
            orgVarVal[j] = std::vector<double>(orgVarVal[j].begin() + orgVarVal[j].size()*(1 - outlier)/2., orgVarVal[j].end() - orgVarVal[j].size()*(1 - outlier)/2.);
            DeltaVarVal[j] = std::vector<double>(DeltaVarVal[j].begin() + DeltaVarVal[j].size()*(1 - outlier)/2., DeltaVarVal[j].end() - DeltaVarVal[j].size()*(1 - outlier)/2.);
            pullVarVal[j] = std::vector<double>(pullVarVal[j].begin() + pullVarVal[j].size()*(1 - outlier)/2., pullVarVal[j].end() - pullVarVal[j].size()*(1 - outlier)/2.);
        }

        //Calculate mean/stdDev with/without outlier removal if wished
        double meanOrg, meanDelta, meanPull,
               stdDevOrg, stdDevDelta, stdDevPull;

        meanOrg = 1./orgVarVal[j].size()* std::accumulate(orgVarVal[j].begin(), orgVarVal[j].end(), 0.);
        meanDelta = 1./DeltaVarVal[j].size()* std::accumulate(DeltaVarVal[j].begin(), DeltaVarVal[j].end(), 0.);
        meanPull = 1./pullVarVal[j].size()* std::accumulate(pullVarVal[j].begin(), pullVarVal[j].end(), 0.);

        stdDevOrg = std::sqrt(std::accumulate(orgVarVal[j].begin(), orgVarVal[j].end(), 0., [&](double& v, double& i){return v + 1./(orgVarVal[j].size())* std::pow(i - meanOrg, 2);}));
        stdDevDelta = std::sqrt(std::accumulate(DeltaVarVal[j].begin(), DeltaVarVal[j].end(), 0., [&](double& v, double& i){return v + 1./(DeltaVarVal[j].size())* std::pow(i - meanDelta, 2);}));
        stdDevPull = std::sqrt(std::accumulate(pullVarVal[j].begin(), pullVarVal[j].end(), 0., [&](double& v, double& i){return v + 1./(pullVarVal[j].size())* std::pow(i - meanPull, 2);}));

        TParameter<double>* meanOrgP = new TParameter<double>(("mean_" + std::string(orgHists[j]->GetName())).c_str(), meanOrg);
        TParameter<double>* meanDeltaP = new TParameter<double>(("mean_" + std::string(deltaHists[j]->GetName())).c_str(), meanDelta);
        TParameter<double>* meanPullP = new TParameter<double>(("mean_" + std::string(pullHists[j]->GetName())).c_str(), meanPull);

        TParameter<double>* stdDevOrgP = new TParameter<double>(("stdDev_" + std::string(orgHists[j]->GetName())).c_str(), stdDevOrg);
        TParameter<double>* stdDevDeltaP = new TParameter<double>(("stdDev_" + std::string(deltaHists[j]->GetName())).c_str(), stdDevDelta);
        TParameter<double>* stdDevPullP = new TParameter<double>(("stdDev_" + std::string(pullHists[j]->GetName())).c_str(), stdDevPull);

        meanOrgP->Write();
        meanDeltaP->Write();
        meanPullP->Write();
        stdDevOrgP->Write();
        stdDevDeltaP->Write();
        stdDevPullP->Write();
    }

    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "Closed input file: " << inputFile << std::endl;
    std::cout << "Closed output file: " << out->GetName() << std::endl << std::endl;

    return out->GetName();
}

void MTSPlotter::Plot(const std::vector<std::string>& histFiles, const std::string& outDir, const std::vector<int>& colors, const std::vector<int>& styles, const std::vector<std::string>& titles, const std::string& rightTitle){
    //Global style stuff
    TkAlStyle::legendheader = "Bye";
    TkAlStyle::legendoptions = "all";
    TkAlStyle::set(INTERNAL, NONE, "Hello", rightTitle);
    gStyle->SetMarkerSize(1.5);

    //Open histogram files
    std::filesystem::create_directories(outDir);
    std::vector<std::shared_ptr<TFile>> files;

    for(const std::string& histFile : histFiles){
        files.push_back(std::shared_ptr<TFile>(TFile::Open(histFile.c_str(), "READ")));
    }

    TList* histNames = files.at(0)->GetListOfKeys();
    int nPlots = 0;

    std::cout << "Start producing MTS plot in directory: " << outDir << std::endl;
    std::cout << "------------------------------------------------------------------" << std::endl;

    for(int i = 0; i < histNames->GetSize(); ++i){
        std::unique_ptr<TCanvas> canvas = std::make_unique<TCanvas>();
        std::string histName(histNames->At(i)->GetName());

        float max = 1., min = 0.;

        //Histogram which will be the frame of the plot
        TObject* obj = files.at(0)->Get(histName.c_str());

        //Plot configuration for a 1D histogram
        if(obj->InheritsFrom(TH1D::Class())){
            //TLegend
            std::shared_ptr<TLegend> legend(TkAlStyle::legend(histFiles.size()*2, 1));
            legend->SetTextSize(0);
            legend->SetNColumns(2);

            //Frame hist
            TH1D* frameHist = static_cast<TH1D*>(obj);

            frameHist->UseCurrentStyle();
            frameHist->SetTitle("");
            frameHist->SetLineColor(colors.at(0));
            frameHist->SetMarkerColor(colors.at(0));
            frameHist->SetMarkerStyle(styles.at(0)/100.);
            frameHist->Draw("EP");

            //Get mean/stdDev
            double mean = static_cast<TParameter<double>*>(files.at(0)->Get(("mean_" + std::string(frameHist->GetName())).c_str()))->GetVal();
            double stdDev = static_cast<TParameter<double>*>(files.at(0)->Get(("stdDev_" + std::string(frameHist->GetName())).c_str()))->GetVal();

            //Hist stats which will be dran in legend
            std::stringstream stats;
            stats.precision(3);

            stats << "#mu = " << mean << " #pm " << stdDev/std::sqrt(frameHist->GetEntries()) << " " << GetUnit(frameHist->GetXaxis()->GetTitle());
            stats << ", rms = " << stdDev << " #pm " << stdDev/std::sqrt(2*frameHist->GetEntries()) << " " << GetUnit(frameHist->GetXaxis()->GetTitle());

            legend->AddEntry(frameHist, titles.at(0).c_str(), "pl");
            legend->AddEntry(new TObject(), stats.str().c_str(), "");

            max = frameHist->GetMaximum();

            //Other histograms
            for(std::size_t j = 1; j < files.size(); ++j){
                TH1D* hist = files.at(j)->Get<TH1D>(histName.c_str());

                double mean = static_cast<TParameter<double>*>(files.at(j)->Get(("mean_" + std::string(frameHist->GetName())).c_str()))->GetVal();
                double stdDev = static_cast<TParameter<double>*>(files.at(j)->Get(("stdDev_" + std::string(frameHist->GetName())).c_str()))->GetVal();

                std::stringstream stats;
                stats.precision(3);
                stats << "#mu = " << mean << " #pm " << stdDev/std::sqrt(hist->GetEntries()) << " " << GetUnit(hist->GetXaxis()->GetTitle());
                stats << ", rms = " << stdDev << " #pm " << stdDev/std::sqrt(2*hist->GetEntries()) << " " << GetUnit(hist->GetXaxis()->GetTitle());

                legend->AddEntry(hist, titles.at(j).c_str(), "pl");
                legend->AddEntry(new TObject(), stats.str().c_str(), "");

                if(max < hist->GetMaximum()) max = hist->GetMaximum();

                hist->SetMarkerColor(colors.at(j));
                hist->SetLineColor(colors.at(j));
                hist->SetMarkerStyle(styles.at(j) / 100.);
                hist->Draw("EP SAME");
            }

            frameHist->SetMaximum(max*(1 + 0.25*files.size()));

            TkAlStyle::drawStandardTitle();
            legend->Draw();

            canvas->Update();
            canvas->Draw();
            canvas->SaveAs((outDir + "/" + histName + ".pdf").c_str());
            canvas->SaveAs((outDir + "/" + histName + ".png").c_str());

            std::cout << "Produced plot: " << histName + ".pdf" << std::endl;

            ++nPlots;
        }

        //Plot configuration for a 2D scatter histogram
        if(obj->InheritsFrom(TH2D::Class())){
            //TLegend
            std::shared_ptr<TLegend> legend(TkAlStyle::legend(histFiles.size()));
            legend->SetTextSize(0);
            legend->SetNColumns(1);

            //Profile frame hist
            TProfile* frameHist = static_cast<TH2D*>(obj)->ProfileX();
            
            frameHist->UseCurrentStyle();
            frameHist->SetTitle("");
            frameHist->SetMarkerSize(1.5);
            frameHist->SetMarkerColor(colors.at(0));
            frameHist->SetLineColor(colors.at(0));
            frameHist->SetMarkerStyle(styles.at(0)/100.);

            std::string yTitle = static_cast<TH2D*>(obj)->GetYaxis()->GetTitle();
            yTitle.insert(0, "#LT");

            if(yTitle.find(")") == yTitle.size() - 1 and yTitle.find("#delta") == std::string::npos){
                yTitle.insert(yTitle.find("(") - 1, "#RT");
            }
            else yTitle += "#RT";

            frameHist->GetYaxis()->SetTitle(yTitle.c_str());

            frameHist->Draw("EP");

            min = frameHist->GetBinContent(frameHist->GetMinimumBin()) - frameHist->GetBinError(frameHist->GetMinimumBin());
            max = frameHist->GetBinContent(frameHist->GetMaximumBin()) + frameHist->GetBinError(frameHist->GetMaximumBin());
            
            legend->AddEntry(frameHist, titles.at(0).c_str(), "pl");

            //Other profiles
            for(std::size_t j = 1; j < files.size(); ++j){
                TProfile* hist = files.at(j)->Get<TH2D>(histName.c_str())->ProfileX();
                legend->AddEntry(hist, titles.at(j).c_str(), "pl");

                if(min > hist->GetBinContent(hist->GetMinimumBin()) - hist->GetBinError(hist->GetMinimumBin())) min = hist->GetBinContent(hist->GetMinimumBin()) - hist->GetBinError(hist->GetMinimumBin());
                if(max < hist->GetBinContent(hist->GetMaximumBin()) + hist->GetBinError(hist->GetMaximumBin())) max = hist->GetBinContent(hist->GetMaximumBin()) + hist->GetBinError(hist->GetMaximumBin());

                hist->SetMarkerColor(colors.at(j));
                hist->SetLineColor(colors.at(j));
                hist->SetMarkerStyle(styles.at(j) / 100.);
                hist->Draw("EP SAME");
            }

            frameHist->SetMinimum(0.8*min);
            frameHist->SetMaximum(max*(1. + 0.25*files.size()));

            legend->Draw();
            TkAlStyle::drawStandardTitle();

            canvas->Update();
            canvas->Draw();
            canvas->SaveAs((outDir + "/" + histName + "_profile.pdf").c_str());
            canvas->SaveAs((outDir + "/" + histName + "_profile.png").c_str());

            std::cout << "Produced plot: " << histName + "_profile.pdf" << std::endl;

            ++nPlots;
        }
    }

    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "Produced " << nPlots << " plots!" << std::endl;
}
