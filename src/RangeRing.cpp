#include "RangeRing.hpp"
#include "Feature.hpp"
#include "LineFeature.hpp"
#include "PointFeature.hpp"

#include <memory>

namespace PFB
{
  using namespace std;

  RangeRing::RangeRing(const string& name) : name_(name), pnt_(), ranges_() {}

  RangeRing::RangeRing(const string& name, double lat, double lon, const vector<double>& ranges) :
    name_( name), pnt_(lat, lon), ranges_(ranges) {}

  RangeRing::RangeRing(const string& name, double lat, double lon, vector<double>&& ranges):
    name_(name), pnt_(lat, lon), ranges_(move(ranges)) {}

  vector<FP> RangeRing::getPlaceFileFeatures(int dispThresh, int lineWidth, PlaceFileColor color) const
  {
    vector<FP> toRet{};
    toRet.reserve(1 + ranges_.size());

    // Put the center point on the list.
    toRet.push_back(FP(new PointFeature(name_, color, pnt_.latitude, pnt_.longitude, dispThresh)));

    // Put the rings on the list
    const double earthRadius = 3959.0; // miles
    const double PI = 3.14159;
    for(auto rng: ranges_)
    {
      const size_t NUM_POINTS = 360; // Number of line segments to use to make up ring.
      vector<point> pnts{};
      pnts.reserve(NUM_POINTS + 1);
      double deltaBearing = 360.0 / (NUM_POINTS) / 180.0 * PI; // radians
      for(size_t i = 0; i < NUM_POINTS + 1; i++)
      {
        const double bearing = i * deltaBearing; // radians
        const double delta = rng/earthRadius;

        const double oldLat = pnt_.latitude * PI / 180.0; // radians
        const double oldLon = pnt_.longitude * PI / 180.0; // radians

        //
        // SIMPLIFICATION - assume we'll never be near a pole, cause there is no radar their, so why
        // make a placefile.
        //
        double newLat = asin(sin(oldLat)*cos(delta) + cos(oldLat)*sin(delta)*cos(bearing));
        double newLon = fmod(oldLon - asin(sin(bearing)*sin(delta)/cos(newLat)) + PI, 2*PI) - PI;

        // Convert back to degrees
        newLat *= 180.0 / PI;
        newLon *= 180.0 / PI;

        pnts.push_back(point(newLat, newLon));
      }

      toRet.push_back(FP(new LineFeature(name_, color, pnts, dispThresh, lineWidth)));
    }

    return move(toRet);
  }

  const string& RangeRing::name() const
  {
    return name_;
  }
}

