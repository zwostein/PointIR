/*
 * Copyright (C) 2014 Tobias Himmer <provisorisch@online.de>
 *
 * This file is part of PointIR.
 *
 * PointIR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PointIR is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PointIR.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define _POINTDETECTORCV__LIVEDEBUG_


#include "PointDetectorCV.hpp"

#include <iostream>
#include <limits>

#include <assert.h>

#include <opencv2/imgproc/imgproc.hpp>


#ifdef _POINTDETECTORCV__LIVEDEBUG_
	#include <opencv2/highgui/highgui.hpp>
#endif


struct BoundingBox
{
	float minX = std::numeric_limits< float >::max();
	float minY = std::numeric_limits< float >::max();
	float maxX = std::numeric_limits< float >::min();
	float maxY = std::numeric_limits< float >::min();
};


#ifdef _POINTDETECTORCV__LIVEDEBUG_
static cv::Mat imageDebug;
#endif


static void pointsFromContours( std::vector<PointIR_Point> & points, const std::vector< std::vector<cv::Point> > & contours )
{
	points.reserve( contours.size() );
	for( size_t i = 0 ; i < contours.size() ; i++ )
	{
		PointIR_Point point = {};
		assert( !contours[i].empty() );
		for( const cv::Point & contourPoint : contours[i] )
		{
			point.x += contourPoint.x;
			point.y += contourPoint.y;
		}
		point.x /= contours[i].size();
		point.y /= contours[i].size();
		points.push_back( point );
#ifdef _POINTDETECTORCV__LIVEDEBUG_
		cv::circle( imageDebug, cv::Point2f( point.x, point.y ), 3.0f, cv::Scalar( 0, 255, 0 ) );
#endif
	}
}


static void pointsFromContours_BoundFiltered( std::vector<PointIR_Point> & points, const std::vector< std::vector<cv::Point> > & contours,
                                              const float & minSize, const float & maxSize )
{
	points.reserve( contours.size() );
	for( size_t i = 0 ; i < contours.size() ; i++ )
	{
		BoundingBox box;
		PointIR_Point point = {};
		assert( !contours[i].empty() );
		for( const cv::Point & contourPoint : contours[i] )
		{
			point.x += contourPoint.x;
			point.y += contourPoint.y;
			if( contourPoint.x > box.maxX )
				box.maxX = contourPoint.x;
			if( contourPoint.y > box.maxY )
				box.maxY = contourPoint.y;
			if( contourPoint.x < box.minX )
				box.minX = contourPoint.x;
			if( contourPoint.y < box.minY )
				box.minY = contourPoint.y;
		}
#ifdef _POINTDETECTORCV__LIVEDEBUG_
		cv::circle( imageDebug, cv::Point2f( point.x/contours[i].size(), point.y/contours[i].size() ), 3.0f, cv::Scalar( 0, 64, 0 ) );
		cv::rectangle( imageDebug, cv::Point2f( box.minX, box.minY ), cv::Point2f( box.maxX, box.maxY ), cv::Scalar( 0, 64, 64 ) );
#endif
		float boxSizeX = box.maxX - box.minX + 1.0f;
		float boxSizeY = box.maxY - box.minY + 1.0f;
		if( boxSizeX > maxSize || boxSizeY > maxSize || boxSizeX < minSize || boxSizeY < minSize )
			continue;
		point.x /= contours[i].size();
		point.y /= contours[i].size();
		points.push_back( point );
#ifdef _POINTDETECTORCV__LIVEDEBUG_
		cv::circle( imageDebug, cv::Point2f( point.x, point.y ), 3.0f, cv::Scalar( 0, 255, 0 ) );
		cv::rectangle( imageDebug, cv::Point2f( box.minX, box.minY ), cv::Point2f( box.maxX, box.maxY ), cv::Scalar( 0, 255, 255 ) );
#endif
	}
}


std::vector< PointIR_Point > PointDetectorCV::detect( const uint8_t * image, unsigned int width, unsigned int height )
{
	// create a thresholded copy of input image
	cv::Mat imageThresholded( cv::Size( width, height), CV_8UC1 );
	assert( imageThresholded.isContinuous() );
	unsigned int imageSize = width * height;
	for( unsigned int wh = 0 ; wh < imageSize ; wh++ )
	{
		if( image[wh] >= this->intensityThreshold )
			imageThresholded.data[wh] = 0xff;
		else
			imageThresholded.data[wh] = 0x00;
	}

//	cv::morphologyEx( imageThresholded, imageThresholded, cv::MORPH_OPEN, cv::Mat(), cv::Point(-1,-1), 5 );

#ifdef _POINTDETECTORCV__LIVEDEBUG_
	// need to copy image here for debug view because findContours will modify the thresholded image
	cv::cvtColor( imageThresholded, imageDebug, CV_GRAY2RGB );
#endif

	// find contours will also modify thresholded image
	std::vector< std::vector<cv::Point> > contours;
	cv::findContours( imageThresholded, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE );

#ifdef _POINTDETECTORCV__LIVEDEBUG_
	cv::drawContours( imageDebug, contours, -1, cv::Scalar( 0, 0, 255 ), 1, 1 );
#endif

	// approximate the middle of each contour - this is our point
	std::vector< PointIR_Point > points;
	points.reserve( contours.size() );
	if( boundingFilterEnabled )
	{
		float averageImageSize = (width+height)/2;
		// minimum of one pixel for absolute point sizes
		float minSize = std::max( 1.0f, this->minBoundingSize * averageImageSize );
		float maxSize = std::max( 1.0f, this->maxBoundingSize * averageImageSize );
		pointsFromContours_BoundFiltered( points, contours, minSize, maxSize );
	}
	else
	{
		pointsFromContours( points, contours );
	}

#ifdef _POINTDETECTORCV__LIVEDEBUG_
	cv::imshow( "PointDetectorCV", imageDebug );
#endif

	return points;
}
