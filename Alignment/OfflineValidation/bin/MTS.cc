#include "exceptions.h"
#include "toolbox.h"
#include "Options.h"

#include <memory>
#include <string>


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Alignment/OfflineValidation/interface/MTSplotter.h>


int plotMTS(int argc, char *argv[]) {
  /*// parse the command line
  AllInOneConfig::Options options;
  options.helper(argc, argv);
  options.parser(argc, argv);

  //Read in AllInOne json config
  pt::ptree main_tree;
  pt::read_json(options.config, main_tree);
*/
    MTS::produceCanvas("TrackSplitting_Cosmics_final2017_newT.root");

    return EXIT_SUCCESS;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
int main (int argc, char* argv[])
{
    return AllInOneConfig::exceptions<plotMTS>(argc, argv);
}
#endif
