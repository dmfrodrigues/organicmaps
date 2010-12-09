#include "classif_routine.hpp"
#include "classificator.hpp"
#include "drawing_rules.hpp"

#include "../indexer/osm2type.hpp"
#include "../indexer/scales.hpp"

#include "../coding/reader.hpp"

#include "../base/logging.hpp"

#include "../std/stdio.hpp"

#include "../base/start_mem_debug.hpp"


namespace classificator
{
  void Read(string const & rules, string const & classificator, string const & visibility)
  {
    drule::ReadRules(rules.c_str());
    if (!classif().ReadClassificator(classificator.c_str()))
      MYTHROW(Reader::OpenException, ("drawing rules or classificator file"));

    (void)classif().ReadVisibility(visibility.c_str());
  }

  void parse_osm_types(int start, int end, string const & path)
  {
    for (int i = start; i <= end; ++i)
    {
      char buf[5] = { 0 };
      sprintf(buf, "%d", i);

      string const inFile = path + buf + ".xml";
      ftype::ParseOSMTypes(inFile.c_str(), i);
    }
  }

  void GenerateAndWrite(string const & path)
  {
    // Experimental - add drawing rules in programm.
    //string const fullName = path + "drawing_rules.bin";
    //drule::ReadRules(fullName.c_str());

    //int const color = 0;
    //double const pixWidth = 1.5;
    //for (int i = 0; i <= scales::GetUpperScale(); ++i)
    //{
    //  size_t const ind = drule::rules().AddLineRule(i, color, pixWidth);
    //  LOG_SHORT(LINFO, ("Scale = ", i, "; Index = ", ind));
    //}

    //drule::WriteRules(fullName.c_str());

    //return;

    // 1. generic types
    parse_osm_types(0, 11, path + "styles/caption-z");
    parse_osm_types(6, 17, path + "styles/osm-map-features-z");

    // 2. POI (not used)
    //parse_osm_types(12, 17, path + "styles/osm-POI-features-z");

    // 3. generate map
    string const inFile = path + "styles/mapswithme.xml";
    for (int i = 0; i <= 17; ++i)
      ftype::ParseOSMTypes(inFile.c_str(), i);

    drule::WriteRules(string(path + "drawing_rules.bin").c_str());
    classif().PrintClassificator(string(path + "classificator.txt").c_str());
  }

  void PrepareForFeatureGeneration()
  {
    classif().SortClassificator();
  }
}
