#include <memory>
#include <vector>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <Alignment/OfflineValidation/interface/MTSplotter.h>

namespace pt = boost::property_tree;

int plotMTS(int argc, char *argv[]) {
    //parse the command line
    AllInOneConfig::Options options;
    options.helper(argc, argv);
    options.parser(argc, argv);

    //Read in AllInOne json config
    pt::ptree main_tree;
    pt::read_json(options.config, main_tree);

    //Need variables read from config
    std::string outDir = main_tree.get<std::string>("output");
    std::string legendTitle = main_tree.get<std::string>("validation.customrighttitle");

    std::vector<std::string> files, titles, alignments;
    std::vector<int> colors, styles;

    for(const std::pair<std::string, pt::ptree>& alignment : main_tree.get_child("alignment")){
        colors.push_back(alignment.second.get<int>("color"));
        styles.push_back(alignment.second.get<int>("style"));
        titles.push_back(alignment.second.get<std::string>("title"));
        files.push_back(alignment.second.get<std::string>("input"));
        alignments.push_back(alignment.first);
    }

    std::vector<std::string> outFiles(files.size(), "");

    //Use MTSplotter class to do the magic
    MTSPlotter plotter;

    for(std::size_t i = 0; i < files.size(); ++i){
        outFiles[i] = plotter.ProduceHists(files.at(i), outDir, alignments.at(i), main_tree.get<float>("outliercut", 1.));
    }

    plotter.Plot(outFiles, outDir, colors, styles, titles, legendTitle);

    return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char* argv[])
{
    return AllInOneConfig::exceptions<plotMTS>(argc, argv);
}
#endif
