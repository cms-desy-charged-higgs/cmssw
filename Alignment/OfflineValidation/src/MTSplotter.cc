#include <Alignment/OfflineValidation/interface/MTSplotter.h>

std::string MTSPlotter::ProduceHists(const std::string& inputFile, const std::string& outDir, const std::string& alignName){
    //Vector for storing the readout of the TTree for the several variables
    std::vector<double> splitVar1(variables.size(), 0.), splitVar1Err(variables.size(), 0.),
                        splitVar2(variables.size(), 0.), splitVar2Err(variables.size(), 0.),
                        orgVar(variables.size(), 0.), orgVarErr(variables.size(), 0.),
                        DeltaVar(variables.size(), 0.);

    //Vector with histograms to be filled
    std::vector<TH1D*> hists(variables.size(), nullptr), orgHists(variables.size(), nullptr), 
                       deltaHists(variables.size(), nullptr), pullHists(variables.size(), nullptr);
    std::vector<TH2D*> orgVsOrgHist(variables.size()*variables.size(), nullptr),
                       orgVsDeltaHist(variables.size()*variables.size(), nullptr),
                       DeltaVsDeltaHist(variables.size()*variables.size(), nullptr),
                       orgVsPullHist(variables.size()*variables.size(), nullptr),
                       DeltaVsPullHist(variables.size()*variables.size(), nullptr),
                       PullVsPullHist(variables.size()*variables.size(), nullptr);

    //Get TTree
    TFile* file = TFile::Open(inputFile.c_str(), "READ");
    TTree* tree = file->Get<TTree>("cosmicValidation/splitterTree");
    if (tree == nullptr) tree = file->Get<TTree>("splitterTree");

    //Open outfile for histograms
    std::filesystem::create_directories(outDir);
    TFile* out = TFile::Open((outDir + "/" + alignName + ".root").c_str(), "RECREATE");

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
//        pullHists[j] = BuildHist<TH1D>(true, "Delta_" + variables[j] + "_pull");

        //Instiate 2D histograms
        for(std::size_t k = 0; k < j; ++k){
            orgVsOrgHist[k + j*variables.size()] = BuildHist<TH2D>(true, variables[j], variables[k]);
        }
    }

    //Loop and fill histograms
    for(int i = 0; i < tree->GetEntries(); ++i){
        if(i % 10000 == 0 and i != 0) std::cout << "Processed " << i << " events!" << std::endl;
        tree->GetEntry(i);

        for(std::size_t j = 0; j < variables.size(); ++j){
            double error = std::sqrt(splitVar1Err[j] * splitVar1Err[j] + splitVar2Err[j] * splitVar2Err[j]);

            orgHists[j]->Fill(orgVar[j]);
            deltaHists[j]->Fill(DeltaVar[j]/(std::sqrt(2.)*rel[j]));
  //          pullHists[j]->Fill(DeltaVar[j]/(error*rel[j]));

            for(std::size_t k = 0; k < j; ++k){
                orgVsOrgHist[k + j*variables.size()]->Fill(orgVar[j], orgVar[k]);
            }
        }
    }

    out->cd();

    //Save histograms for variables in another root file which will be read out in MTSPlotter::Plot function
    for(std::size_t j = 0; j < variables.size(); ++j){
        orgHists[j]->Write();
        deltaHists[j]->Write();
   //     pullHists[j]->Write();

        for(std::size_t k = 0; k < j; ++k){
            orgVsOrgHist[k + j*variables.size()]->Write();
        }
    }

    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "Closed input file: " << inputFile << std::endl;
    std::cout << "Closed output file: " << out->GetName() << std::endl << std::endl;

    out->Close();
    file->Close();

    return out->GetName();
}

void MTSPlotter::Plot(const std::vector<std::string>& histFiles, const std::string& outDir, const std::vector<int>& colors, const std::vector<int>& styles, const std::vector<std::string>& titles){
    if (TkAlStyle::status() == NO_STATUS) TkAlStyle::set(INTERNAL);
    gStyle->SetMarkerSize(1.5);

    std::vector<TFile*> files;
    std::filesystem::create_directories(outDir);

    for(const std::string& histFile : histFiles){
        files.push_back(TFile::Open(histFile.c_str(), "READ"));
    }

    TList* histNames = files.at(0)->GetListOfKeys();
    int nPlots = 0;

    std::cout << "Start producing MTS plot in directory: " << outDir << std::endl;

    for(int i = 0; i < histNames->GetSize(); ++i){
        std::unique_ptr<TCanvas> canvas = std::make_unique<TCanvas>();
        std::string histName(histNames->At(i)->GetName());

        //TLegend
        TLegend* legend = TkAlStyle::legend(histFiles.size(), 1.);
        legend->SetTextSize(0);

        float max = 1., min = 0.;

        //Histogram which will be the frame of the plot
        TObject* obj = files.at(0)->Get(histName.c_str());

        //Plot configuration for a 1D histogram
        if(obj->InheritsFrom(TH1D::Class())){
            TH1D* frameHist = static_cast<TH1D*>(obj);

            frameHist->SetMarkerColor(colors.at(0));
            frameHist->SetMarkerStyle(styles.at(0)/100.);
            frameHist->Draw("P");

            legend->AddEntry(frameHist, titles.at(0).c_str(), "P");
            max = frameHist->GetMaximum();

            for(std::size_t j = 1; j < files.size(); ++j){
                TH1D* hist = files.at(j)->Get<TH1D>(histName.c_str());
                legend->AddEntry(hist, titles.at(j).c_str(), "P");

                if(max < hist->GetMaximum()) max = hist->GetMaximum();

                hist->SetMarkerColor(colors.at(j));
                hist->SetMarkerStyle(styles.at(j) / 100.);
                hist->Draw("P SAME");
            }

            frameHist->SetMaximum(max*(1 + 0.15*files.size()));

            TkAlStyle::drawStandardTitle();
            legend->Draw();

            canvas->Draw();
            canvas->SaveAs((outDir + "/" + histName + ".pdf").c_str());
            canvas->SaveAs((outDir + "/" + histName + ".png").c_str());
            ++nPlots;
        }

        //Plot configuration for a 2D scatter histogram
        if(obj->InheritsFrom(TH2D::Class())){
            //Profile plots
            TProfile* frameHist = static_cast<TH2D*>(obj)->ProfileX();
            
            frameHist->SetMarkerColor(colors.at(0));
            frameHist->SetLineColor(colors.at(0));
            frameHist->SetMarkerStyle(styles.at(0)/100.);
            frameHist->GetYaxis()->SetTitle(static_cast<TH2D*>(obj)->GetYaxis()->GetTitle());
            frameHist->Draw("EP");

            min = frameHist->GetMaximum();
            max = frameHist->GetMaximum();
            
            legend->AddEntry(frameHist, titles.at(0).c_str(), "P");

            for(std::size_t j = 1; j < files.size(); ++j){
                TProfile* hist = files.at(j)->Get<TH2D>(histName.c_str())->ProfileX();
                legend->AddEntry(hist, titles.at(j).c_str(), "P");

                if(min > hist->GetMinimum()) max = hist->GetMaximum();
                if(max < hist->GetMaximum()) max = hist->GetMaximum();

                hist->SetMarkerColor(colors.at(j));
                hist->SetLineColor(colors.at(j));
                hist->SetMarkerStyle(styles.at(j) / 100.);
                hist->Draw("EP SAME");
            }

            frameHist->SetMaximum(0.9*min);
            frameHist->SetMaximum(max*(1 + 0.15*files.size()));

            TkAlStyle::drawStandardTitle();
            legend->Draw();

            canvas->Draw();
            canvas->SaveAs((outDir + "/" + histName + "_profile.pdf").c_str());
            canvas->SaveAs((outDir + "/" + histName + "_profile.png").c_str());
            ++nPlots;
        }
    }

    std::cout << "Produced " << nPlots << " plots!" << std::endl;
}
