#include <memory>
#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <exceptions.h>
#include <toolbox.h>
#include <Options.h>

#include <Alignment/OfflineValidation/interface/MTSplotter.h>


int plotMTS(int argc, char *argv[]) {
    // parse the command line
    /*
    AllInOneConfig::Options options;
    options.helper(argc, argv);
    options.parser(argc, argv);

    //Read in AllInOne json config
    pt::ptree main_tree;
    pt::read_json(options.config, main_tree);
    */

    //Need variables read from config
    std::string outDir = "KappaMTS";
    std::vector<std::string> files = {"TrackSplitting_Cosmics_final2017_newT.root", "TrackSplitting_Cosmics_UL17_fineAPEs_SD.root"};
    std::vector<int> colors = {600, 632};
    std::vector<int> styles = {2101, 2101};
    std::vector<std::string> titles = {"Kappa1", "Kappa2"};
    std::vector<std::string> alignments = {"Kappa1", "Kappa2"};

    std::vector<std::string> outFiles(3, "");

    //Use MTSplotter class to do the magic
    MTSPlotter plotter;

    for(std::size_t i < 0; i < files.size(); ++i){
        outFiles[i] = plotter.ProduceHists(files.at(i), outDir, alignments.at(i));
    }

    plotter.Plot(outFiles, outDir, colors, styles, alignments);

    return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char* argv[])
{
    return AllInOneConfig::exceptions<plotMTS>(argc, argv);
}
#endif
