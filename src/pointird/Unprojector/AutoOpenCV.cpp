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

//#define _UNPROJECTOR_AUTOOPENCV__LIVEDEBUG_


#include "AutoOpenCV.hpp"

#include <PointIR/Frame.h>
#include <PointIR/Point.h>
#include <PointIR/PointArray.h>

#include <vector>
#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>


#ifdef _UNPROJECTOR_AUTOOPENCV__LIVEDEBUG_
	#include <opencv2/highgui/highgui.hpp>
#endif


using namespace Unprojector;


static const unsigned int chessboardNumFieldsX = 10;
static const unsigned int chessboardNumFieldsY = 7;
static const unsigned int chessboardNumCornersX = chessboardNumFieldsX-1;
static const unsigned int chessboardNumCornersY = chessboardNumFieldsY-1;
static const float chessboardBorder = 0.01f;
static const float mirrorMarkBorder = 0.03f;


class AutoOpenCV::Impl
{
public:
	unsigned int width = 0;
	unsigned int height = 0;

	double perspective[9] =
	{
		1, 0, 0,
		0, 1, 0,
		0, 0, 1
	};
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


static void drawQuad( uint8_t * greyImage, unsigned int imageWidth, unsigned int imageHeight,
                      unsigned int x, unsigned int y, unsigned int width, unsigned int height,
		      uint8_t tone )
{
	for( unsigned int h = 0; h < height; h++ )
	{
		for( unsigned int w = 0; w < width; w++ )
		{
			greyImage[ (x+w) + (y+h) * imageWidth ] = tone;
		}
	}
}


static PointIR::Point unprojected( const double matrix[9], const PointIR::Point & point )
{
	PointIR::Point ret;
	double w = point.x * matrix[6] + point.y * matrix[7] + matrix[8];

	if( fabs( w ) > std::numeric_limits< double >::epsilon() )
	{
		w = 1.0 / w;
		ret.x = ( point.x*matrix[0] + point.y*matrix[1] + matrix[2] ) * w;
		ret.y = ( point.x*matrix[3] + point.y*matrix[4] + matrix[5] ) * w;
	}
	else
		ret.x = ret.y = 0.0;

	return ret;
}


AutoOpenCV::AutoOpenCV() : pImpl( new Impl )
{
}


AutoOpenCV::~AutoOpenCV()
{
}


std::vector< uint8_t > AutoOpenCV::getRawCalibrationData() const
{
	std::vector< uint8_t > rawData( sizeof(AutoOpenCV::Impl) );
	memcpy( rawData.data(), this->pImpl.get(), sizeof(AutoOpenCV::Impl) );
	return rawData;
}


bool AutoOpenCV::setRawCalibrationData( const std::vector< uint8_t > & rawData )
{
	if( rawData.size() != sizeof(AutoOpenCV::Impl) )
		return false;
	memcpy( this->pImpl.get(), rawData.data(), sizeof(AutoOpenCV::Impl) );
	return true;
}


void AutoOpenCV::generateCalibrationImage( PointIR::Frame & frame, unsigned int width, unsigned int height ) const
{
	frame.resize( width, height );
	clear( frame.getData(), width, height, 0xff );
	unsigned int chessBoardX = width * chessboardBorder;
	unsigned int chessBoardY = height * chessboardBorder;
	unsigned int chessBoardWidth = width * ( 1.0f - 2.0f * chessboardBorder );
	unsigned int chessBoardHeight = height * ( 1.0f - 2.0f * chessboardBorder );
	drawChessboard(
		frame.getData(), width, height,
		chessBoardX, chessBoardY, // position of top left field
		chessBoardWidth, chessBoardHeight, // width and height of board
		chessboardNumFieldsX, chessboardNumFieldsY
	);
	// A mark to check if the calibration image is mirrored
	unsigned int mirrorMarkWidth = ((float)chessBoardWidth/(float)chessboardNumFieldsX) - (mirrorMarkBorder*(float)width);
	unsigned int mirrorMarkHeight = ((float)chessBoardHeight/(float)chessboardNumFieldsY) - (mirrorMarkBorder*(float)height);
	unsigned int mirrorMarkX = chessBoardX + chessBoardWidth - mirrorMarkWidth;
	unsigned int mirrorMarkY = chessBoardY + chessBoardHeight - mirrorMarkHeight;
	drawQuad(
		frame.getData(), width, height,
		mirrorMarkX, mirrorMarkY,
		mirrorMarkWidth, mirrorMarkHeight,
		0x00
	);
}


bool AutoOpenCV::calibrate( const PointIR::Frame & frame )
{
	cv::Mat image( cv::Size( frame.getWidth(), frame.getHeight() ), CV_8UC1 );
	assert( image.isContinuous() );
	memcpy( image.data, frame.getData(), frame.getWidth() * frame.getHeight() );

	// points in object coordinates
	float offsetX = (float)frame.getWidth() * chessboardBorder;
	float offsetY = (float)frame.getHeight() * chessboardBorder;
	float boardWidth = (float)frame.getWidth() * ( 1.0f - 2.0f * chessboardBorder );
	float boardHeight = (float)frame.getHeight() * ( 1.0f - 2.0f * chessboardBorder );
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

	if( !found )
		return false;

	cv::Mat perspective = cv::findHomography( imagePoints, objectPoints );
	cv::Mat perspectiveInv = perspective.inv();

	// mirror check
	PointIR::Point mirrorMarkObjectPoint( // sampling point for mirrorcheck
		offsetX + boardWidth - ((float)boardWidth/(float)chessboardNumFieldsX)/3.0f,
		offsetY + boardHeight - ((float)boardHeight/(float)chessboardNumFieldsY)/3.0f
	);
	assert( perspectiveInv.total()*perspectiveInv.elemSize() == sizeof(double)*9 );
	assert( perspectiveInv.isContinuous() );
	PointIR::Point mirrorMarkImagePoint = unprojected( (const double *)perspectiveInv.data, mirrorMarkObjectPoint );
	bool mirrored = false;
	if( (int)mirrorMarkImagePoint.x < 0 || (int)mirrorMarkImagePoint.x >= (int)frame.getWidth()
	 || (int)mirrorMarkImagePoint.y < 0 || (int)mirrorMarkImagePoint.y >= (int)frame.getHeight() )
		std::cerr << std::string(__PRETTY_FUNCTION__) << ": Mirror marker outside of captured image - assuming no mirror.\n";
	else
		mirrored = frame.getAt( (int)mirrorMarkImagePoint.x, (int)mirrorMarkImagePoint.y ) > 0x3f;
	if( mirrored )
	{
		std::cout << std::string(__PRETTY_FUNCTION__) << ": Mirror detected.\n";
		double mirror[] = {
			1, 0, 0,
			0,-1, (double)frame.getHeight(),
			0, 0, 1
		};
		perspective = cv::Mat( 3, 3, CV_64FC1, mirror ) * perspective;
	}

#ifdef _UNPROJECTOR_AUTOOPENCV__LIVEDEBUG_
	cv::Mat debugImage( image.clone() );
	cv::cvtColor( debugImage, debugImage, CV_GRAY2RGB );
	cv::drawChessboardCorners( debugImage, cv::Size( chessboardNumCornersX, chessboardNumCornersY ), imagePoints, found );

	std::vector< PointIR::Point > screenObjectPoints;
	screenObjectPoints.push_back( PointIR::Point( 0.0f, 0.0f ) );
	screenObjectPoints.push_back( PointIR::Point( (float)frame.getWidth(), 0.0f ) );
	screenObjectPoints.push_back( PointIR::Point( (float)frame.getWidth(), (float)frame.getHeight() ) );
	screenObjectPoints.push_back( PointIR::Point( 0.0f, (float)frame.getHeight() ) );

	std::vector< PointIR::Point > screenImagePoints;
	for( PointIR::Point & point : screenObjectPoints )
		screenImagePoints.push_back( unprojected( (const double *)perspectiveInv.data, point ) );

	for( unsigned int i = 0; i < screenImagePoints.size(); i++ )
		cv::line( debugImage,
			reinterpret_cast<cv::Point2f &>(screenImagePoints[i]),
			reinterpret_cast<cv::Point2f &>(screenImagePoints[(i+1)%screenImagePoints.size()]),
			cv::Scalar(127,0,255) );

	cv::circle( debugImage, reinterpret_cast<cv::Point2f &>(mirrorMarkImagePoint), 4, cv::Scalar(127,0,mirrored?255:127) );

	cv::imshow( "Unprojector::AutoOpenCV", debugImage );
	cv::waitKey(1); // need this for event processing - window wouldn't be visible
#endif

	double normalize[] = {
		1.0/(double)frame.getWidth(), 0.0,                           0.0,
		0.0,                          1.0/(double)frame.getHeight(), 0.0,
		0.0,                          0.0,                           1.0
	};
	perspective = cv::Mat( 3, 3, CV_64FC1, normalize ) * perspective;

	assert( perspective.total()*perspective.elemSize() == sizeof(AutoOpenCV::Impl::perspective) );
	assert( perspective.isContinuous() );
	memcpy( this->pImpl->perspective, perspective.data, perspective.total()*perspective.elemSize() );
	return true;
}


void AutoOpenCV::unproject( uint8_t * image, unsigned int width, unsigned int height ) const
{
	cv::Mat img( cv::Size( width, height ), CV_8UC1, (void*)image );
	cv::Mat tmp( cv::Size( width, height ), CV_8UC1 );
	double denormalize[] = {
		(double)width, 0.0,            0.0,
		0.0,           (double)height, 0.0,
		0.0,           0.0,            1.0
	};
	cv::Mat perspective = cv::Mat( 3, 3, CV_64FC1, denormalize ) * cv::Mat( 3, 3, CV_64FC1, this->pImpl->perspective );
	cv::warpPerspective( img, tmp, perspective, cv::Size( width, height ) );
	memcpy( image, tmp.data, width * height );
}


void AutoOpenCV::unproject( PointIR::Point & point ) const
{
	point = unprojected( this->pImpl->perspective, point );
}
