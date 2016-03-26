#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <iomanip>
using namespace std;

#include "OGRDataSourceWrapper.h"
#include "OGRFeatureWrapper.h"
#include "PlaceFile.h"
#include "PlaceFileColor.h"
using namespace PFB;

class AppController
{
public:
  explicit AppController();
  ~AppController();

  // Get a list of sources, this is files only
  const vector<string> getSources();

  // Get a list of layers for the given source.
  const vector<string> getLayers(const string& source);
  
  // Summarize the properties of the given layer.
  const string& summarizeLayerProperties(const string& source, 
    const string& layer);

  // Open a file and add the resource to the project. Return the name of the 
  // source as it would be returned in the list returned by getSource().
  string addSource(const string& path);

  // Save a place file
  void savePlaceFile(const string& fileName, unsigned int threshold, 
    unsigned int refreshMinutes, const string& title);

  // Save a KML file
  void saveKMLFile(const string& fileName);

  // Cause a layer to be hidden, if all layers in a source are hidden it is
  // deleted via deleteSource
  void hideLayer(const string& source, const string& layer);

  // Delete a source, called by hideLayer if all the layers of a source are
  // hidden.
  void deleteSource(const string& source);

  // A nested class to keep track of the options associated with a layer.
  class LayerOptions
  {
  public:
    // The name of the field to use as a label for each feature.
    string labelField;

    // The color to make all features in this layer
    PlaceFileColor color;

    // Convert polygons to strings
    bool polyAsLine;

    // Has this been deleted from the tree view?
    bool visible;

    // Display threshold
    int displayThresh;

    // Summary string
    const string summary;

    // Constructor 
    explicit LayerOptions(string lField, PlaceFileColor clr, bool polyAsLine, 
                          bool vsbl, int dispThresh, const string smry);

  };

private:

  // Map a simple file name (no path) to an OGRDataSource
  unordered_map<string,OGRDataSourceWrapper> srcs_;
  using SrcsPair = pair<string, OGRDataSourceWrapper>;

  // Map sources to layers to options.
  unordered_map<string,unordered_map<string,LayerOptions>> layers_;
  using SrcsInfoPair = pair<string,unordered_map<string,LayerOptions>>;
  using LayerInfo = unordered_map<string,LayerOptions>;
  using LayerInfoPair = pair<string,LayerOptions>;

  static const string DO_NOT_USE_LAYER; // = "**Do Not Use Layer**";
  static const string NO_LABEL;         // = "**No Label**";

  // Given a layer, analyze it's properties and create a string summarizing them
  const string summarize(OGRLayer *lyr);

};

