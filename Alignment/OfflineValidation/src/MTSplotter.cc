#include <Alignment/OfflineValidation/interface/MTSplotter.h>

void MTS::produceCanvas(const std::string& fileName){
    std::vector<std::string> variables = {"pt",  "eta", "phi", "dz",  "dxy", "theta", "qoverpt"};

    //Vector for storing the readout of the TTree for the several variables
    std::vector<double> splitVar1(variables.size(), 0.), splitVar1Err(variables.size(), 0.);
    std::vector<double> splitVar2(variables.size(), 0.), splitVar2Err(variables.size(), 0.);
    std::vector<double> orgVar(variables.size(), 0.), DeltaVar(variables.size(), 0.);

    TFile* out = TFile::Open("out.root", "RECREATE");

    //Vector with histograms to be filled
    std::vector<TH1D*> hists, orgHists, resHists;

    //Get TTree
    TFile* file = TFile::Open(fileName.c_str(), "READ");
    TTree* tree = file->Get<TTree>("cosmicValidation/splitterTree");
    if (tree == nullptr) tree = file->Get<TTree>("splitterTree");

    //Set branch address and declare histograms
    for(std::size_t j = 0; j < variables.size(); ++j){
        std::string xVar1Spl = variables[j] + "1_spl";
        std::string xVar2Spl = variables[j] + "2_spl";
        std::string xVar1ErrSpl = variables[j] + "1Err_spl";
        std::string xVar2ErrSpl = variables[j] + "2Err_spl";
        std::string xVarOrg = variables[j] + "_org";
        std::string xVarDelta = "Delta_" + variables[j];

        tree->SetBranchAddress(xVar1Spl.c_str(), &splitVar1[j]);
        tree->SetBranchAddress(xVar2Spl.c_str(), &splitVar2[j]);
        tree->SetBranchAddress(xVar1ErrSpl.c_str(), &splitVar1Err[j]);
        tree->SetBranchAddress(xVar2ErrSpl.c_str(), &splitVar2Err[j]);
        tree->SetBranchAddress(xVarOrg.c_str(), &orgVar[j]);
        tree->SetBranchAddress(xVarDelta.c_str(), &splitVar2Err[j]);

        orgHists.push_back(new TH1D(xVarOrg.c_str(), xVarOrg.c_str(), 300, 0, 20));
    }

    //Loop and fill histograms
    for(int i = 0; i < tree->GetEntries(); ++i){
        tree->GetEntry(i);

        for(std::size_t j = 0; j < variables.size(); ++j){
            orgHists[j]->Fill(orgVar[j]);
        }
    }

    out->cd();
    
    for(TH1D* hist : hists){
        hist->Write();
    }

    out->Close();
}
