#include "stdafx.h"
#include "PlumeHeightCalculator.h"
#include <vector>

using namespace DualBeamMeasurement;

CPlumeHeightCalculator::CPlumeHeightCalculator(void)
{
}

CPlumeHeightCalculator::~CPlumeHeightCalculator(void)
{
}

/** Calculates the plume-height from the difference in centre-of mass
		position. */
double CPlumeHeightCalculator::GetPlumeHeight_CentreOfMass(const CMeasurementSeries *forwardLookingSerie,const CMeasurementSeries *backwardLookingSerie,double sourceLat, double sourceLon, double angleSeparation, double *windDir){
	double windDirection		= 1e19;
	double oldWindDirection = 0;
	double plumeDirection;
	int nIterations = 0;
	int k; // iterator
	int massIndexB, massIndexF;
	Common common;

	// 1. Make local copies of the data
	long length = forwardLookingSerie->length;
	std::vector<double> lat(length);
	std::vector<double> lon(length);
	std::vector<double> col1(length);
	std::vector<double> col2(length);

	memcpy(lat.data(),  forwardLookingSerie->lat, length*sizeof(double));
	memcpy(lon.data(),  forwardLookingSerie->lon, length*sizeof(double));
	memcpy(col1.data(), forwardLookingSerie->column, length*sizeof(double));
	memcpy(col2.data(), backwardLookingSerie->column,length*sizeof(double));

	// 2. Remove the offset from the traverses
	RemoveOffset(col1.data(), length);
	RemoveOffset(col2.data(), length);

	// 3. Calculate the wind-direction from the measured data 

	// calculate the distances between each pair of measurement points
	std::vector<double> distances(length);
	for(k = 0; k < length-1; ++k){
		distances[k] = GPSDistance(lat[k], lon[k], lat[k+1], lon[k+1]);
	}

	// Iterate until we find a suitable wind-direction
	while(fabs(windDirection - oldWindDirection) > 5){

    // get the mass center of the plume
	massIndexF = GetCentreOfMass(col1.data(), distances.data(), length);
   
    oldWindDirection = windDirection;

		// the wind direction
		windDirection		= GPSBearing(lat[massIndexF], lon[massIndexF], sourceLat, sourceLon);
		plumeDirection	= 180 + windDirection;

		// re-calculate the distances
		for(k = 0; k < length-1; ++k){
			double bearing	= GPSBearing(lat[k], lon[k], lat[k+1], lon[k+1]);
			distances[k]		= GPSDistance(lat[k], lon[k], lat[k+1], lon[k+1]);
			distances[k]	 *= cos(HALF_PI + DEGREETORAD * (bearing - plumeDirection));
		}

		// Keep track of how may iterations we've done
		++nIterations;
		if(nIterations > 10)
			break;
	}

	// 4. Calculate the distance between the two centre of mass positions
	//		Geometrically corrected for the wind-direction
	massIndexB = GetCentreOfMass(col2.data(), distances.data(), backwardLookingSerie->length);

	// 4a. The distances
	double centreDistance		= GPSDistance(lat[massIndexF], lon[massIndexF], lat[massIndexB], lon[massIndexB]);

	// 4b. The direction
	double centreDirection	= GPSBearing(lat[massIndexF], lon[massIndexF], lat[massIndexB], lon[massIndexB]);

	// 4c. The Geometrically corrected distance
	double centreDistance_corrected	= fabs(centreDistance * cos(HALF_PI + DEGREETORAD*(centreDirection - plumeDirection)));

	// 4d. The Plume Height
	double plumeHeight			= centreDistance_corrected / tan(DEGREETORAD * angleSeparation);

	if(windDir != nullptr)
		*windDir = windDirection;

	return plumeHeight;
}

/** Calculates the centre of mass distance.
		@param columns - the measured columns in the traverse 
		@param distances - the distance between each two measurement points 
		@return the index of the centre of mass point in the given arrays*/
int CPlumeHeightCalculator::GetCentreOfMass(const double *columns, const double *distances, long length){
	if(length <= 0 || columns == nullptr || distances == nullptr)
		return -1;
	
	// 1. Calculate the total mass of the plume
	double totalMass = 0.0;
	for(int k = 0; k < length - 1; ++k){
		totalMass += columns[k] * distances[k];
		if(fabs(columns[k]) > 300 || fabs(distances[k]) > 100){
			puts("StrangeData");
		}
	}

	// 2. Half of the total mass
	double midMass = totalMass * 0.5;

	// 3. Find the index of the half centre of mass
	double totalMassSoFar = 0.0;
	for(int k = 0; k < length - 1; ++k){
		totalMassSoFar += columns[k] * distances[k];
		if(totalMassSoFar > midMass){
			return (k-1);
		}
	}

	// 4. Error: could not find centre of mass
	return -1;
}

/** Removes the offset from the given set of columns */
void CPlumeHeightCalculator::RemoveOffset(double *columns, long length){
	static const int BUFFER = 30;
	double x[2*BUFFER], y[2*BUFFER];
	int index = 0;
	// Copy the first 30 and the last 30 datapoints to a buffer
	for(int i = 0; i < BUFFER; ++i){
		x[index] = i;
		y[index] = columns[i];
		++index;
	}
	for(int i = length - BUFFER; i < length; ++i){
		x[index] = i;
		y[index] = columns[i];
		++index;
	}		

	// Adapt a 1:st order polynomial to this dataset
	double k, m;
	Common::AdaptStraightLine(x, y, 2*BUFFER, &k, &m);

	// Remove this line from the dataset
	for(int i = 0; i < length; ++i){
		columns[i] -= (k*i + m);
	}
}
