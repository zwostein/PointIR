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

//#define _AUTOUNPROJECTORCV__LIVEDEBUG_


#include "AutoUnprojectorCV.hpp"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>


#ifdef _AUTOUNPROJECTORCV__LIVEDEBUG_
	#include <opencv2/highgui/highgui.hpp>
#endif


static const unsigned int chessboardNumFieldsX = 10;
static const unsigned int chessboardNumFieldsY = 7;
static const unsigned int chessboardNumCornersX = chessboardNumFieldsX-1;
static const unsigned int chessboardNumCornersY = chessboardNumFieldsY-1;
static const float chessboardBorder = 0.01f;


class AutoUnprojectorCV::Impl
{
public:
	unsigned int width = 0;
	unsigned int height = 0;
	cv::Mat perspective;
	cv::Mat normalizedPerspective;

	void setCalibrationData( cv::Mat & perspective, unsigned int width, unsigned int height )
	{
		this->perspective = perspective;
		this->width = width;
		this->height = height;
		double normalizeData[] = { 1.0/(double)width, 0,                  0,
		                           0,                 1.0/(double)height, 0,
		                           0,                 0,                  1 };
		cv::Mat normalize( 3, 3, CV_64FC1, normalizeData );
		this->normalizedPerspective = normalize * this->perspective;
	}
};


static void clear( uint8_t * greyImage, unsigned int imageWidth, unsigned int imageHeight, unsigned int tone = 0xff )
{
	for( unsigned int wh = 0; wh < imageHeight * imageWidth; wh++ )
	{
		greyImage[ wh ] = tone;
	}
}


static void drawChessboard( uint8_t * greyImage, unsigned int imageWidth, unsigned int imageHeight,
                            unsigned int x, unsigned int y, unsigned int width, unsigned int height,
                            unsigned int fieldsX, unsigned int fieldsY )
{
	float pixelsPerFieldX = (float)width / (float)(fieldsX);
	float pixelsPerFieldY = (float)height / (float)(fieldsY);
	for( unsigned int h = 0; h < height; h++ )
	{
		for( unsigned int w = 0; w < width; w++ )
		{
			unsigned int fieldX = (float)w / (float)pixelsPerFieldX;
			unsigned int fieldY = (float)h / (float)pixelsPerFieldY;
			bool isWhite = ( fieldX + fieldY ) & 1;
			greyImage[ (x+w) + (y+h) * imageWidth ] = isWhite ? 0xff : 0x00;
		}
	}
}


AutoUnprojectorCV::AutoUnprojectorCV() : pImpl( new Impl )
{
	this->pImpl->perspective = cv::Mat::eye( 3, 3, CV_64F );
	this->pImpl->normalizedPerspective = cv::Mat::eye( 3, 3, CV_64F );
}


AutoUnprojectorCV::~AutoUnprojectorCV()
{
}


struct Data
{
	unsigned int width;
	unsigned int height;
	double perspective[9];
};


std::vector< uint8_t > AutoUnprojectorCV::getRawCalibrationData() const
{
	std::vector< uint8_t > rawData( sizeof(Data) );
	Data * data = reinterpret_cast< Data * >( rawData.data() );
	data->width = this->pImpl->width;
	data->height = this->pImpl->height;
	assert( sizeof(Data::perspective) == this->pImpl->perspective.total() * this->pImpl->perspective.elemSize() );
	assert( this->pImpl->perspective.isContinuous() );
	memcpy( data->perspective, this->pImpl->perspective.data, sizeof(Data::perspective) );
	return rawData;
}


bool AutoUnprojectorCV::setRawCalibrationData( const std::vector< uint8_t > & rawData )
{
	if( rawData.size() != sizeof(Data) )
		return false;
	const Data * data = reinterpret_cast< const Data * >( rawData.data() );
	cv::Mat perspective( 3, 3, CV_64FC1 );
	assert( sizeof(Data::perspective) == perspective.total() * perspective.elemSize() );
	assert( perspective.isContinuous() );
	memcpy( perspective.data, data->perspective, sizeof(Data::perspective) );
	this->pImpl->setCalibrationData( perspective, data->width, data->height );
	return true;
}


void AutoUnprojectorCV::generateCalibrationImage( uint8_t * greyImage, unsigned int width, unsigned int height ) const
{
	clear( greyImage, width, height, 0xff );
	drawChessboard(
		greyImage, width, height,
		width * chessboardBorder, height * chessboardBorder, // position of top left field
		width * ( 1.0f - 2.0f * chessboardBorder ), height * ( 1.0f - 2.0f * chessboardBorder ), // width and height of board
		chessboardNumFieldsX, chessboardNumFieldsY
	);
}


bool AutoUnprojectorCV::calibrate( const uint8_t * greyImage, unsigned int width, unsigned int height )
{
	cv::Mat image( cv::Size( width, height ), CV_8UC1 );
	assert( image.isContinuous() );
	memcpy( image.data, greyImage, width * height );

	// points in object coordinates
	float offsetX = (float)width * chessboardBorder;
	float offsetY = (float)height * chessboardBorder;
	float boardWidth = (float)width * ( 1.0f - 2.0f * chessboardBorder );
	float boardHeight = (float)height * ( 1.0f - 2.0f * chessboardBorder );
	std::vector< cv::Point2f > objectPoints;
	for( unsigned int h = 1; h <= chessboardNumCornersY; h++ )
	{
		for( unsigned int w = 1; w <= chessboardNumCornersX; w++ )
		{
			objectPoints.push_back( cv::Point2f(
				offsetX + boardWidth * (float)w / (float)chessboardNumFieldsX,
				offsetY + boardHeight * (float)h / (float)chessboardNumFieldsY
			) );
		}
	}

	// find points in image
	std::vector< cv::Point2f > imagePoints;
	bool found = cv::findChessboardCorners( image, cv::Size( chessboardNumCornersX, chessboardNumCornersY ),
	                                        imagePoints, CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_FILTER_QUADS );

#ifdef _AUTOUNPROJECTORCV__LIVEDEBUG_
	cv::drawChessboardCorners( image, cv::Size( chessboardNumCornersX, chessboardNumCornersY ), imagePoints, found );
	cv::imshow( "AutoUnprojectorCV", image );
#endif

	if( !found )
		return false;

	cv::Mat perspective = cv::findHomography( imagePoints, objectPoints );

	this->pImpl->setCalibrationData( perspective, width, height );
	return true;
}


void AutoUnprojectorCV::unproject( uint8_t * image, unsigned int width, unsigned int height ) const
{
	cv::Mat img( cv::Size( width, height ), CV_8UC1, (void*)image );
	cv::Mat tmp( cv::Size( width, height ), CV_8UC1 );
	cv::warpPerspective( img, tmp, this->pImpl->perspective, cv::Size( width, height ) );
	memcpy( image, tmp.data, width * height );
}


void AutoUnprojectorCV::unproject( std::vector< PointIR_Point > & points ) const
{
	const double * m = (const double*)this->pImpl->normalizedPerspective.data;
	for( PointIR_Point & point : points )
	{
		double x = point.x;
		double y = point.y;
		double w = x * m[6] + y * m[7] + m[8];

		if( fabs( w ) > std::numeric_limits< double >::epsilon() )
		{
			w = 1.0 / w;
			point.x = ( x*m[0] + y*m[1] + m[2] ) * w;
			point.y = ( x*m[3] + y*m[4] + m[5] ) * w;
		}
		else
			point.x = point.y = 0.0;
	}
}
