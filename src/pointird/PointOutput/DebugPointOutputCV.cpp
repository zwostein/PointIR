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

#include "DebugPointOutputCV.hpp"
#include "../Processor.hpp"
#include "../Unprojector/AUnprojector.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <iostream>


DebugPointOutputCV::DebugPointOutputCV( const Processor & processor ) : processor(processor)
{
}


DebugPointOutputCV::~DebugPointOutputCV()
{
}


void DebugPointOutputCV::outputPoints( const std::vector< PointIR_Point > & points )
{
	cv::Mat image;
	const PointIR_Frame * frame = this->processor.getProcessedFrame();
	if( (!frame->width) || (!frame->height) )
	{
		image = cv::Mat( cv::Size( 256, 256 ), CV_8UC1 );
		assert( image.isContinuous() );
	}
	else
	{
		image = cv::Mat( cv::Size( frame->width, frame->height ), CV_8UC1 );
		assert( image.isContinuous() );
		memcpy( image.data, frame->data, frame->width * frame->height );
		processor.getUnprojector().unproject( image.data, frame->width, frame->height );
	}
	cv::cvtColor( image, image, CV_GRAY2RGB );
	for( const PointIR_Point & point : points )
		cv::circle( image, cv::Point2f( point.x * image.cols, point.y * image.rows ), 10.0f, cv::Scalar( 0, 255, 0 ) );
	cv::imshow( "DebugPointOutputCV", image );
	cv::waitKey(1); // need this for event processing - window wouldn't be visible
}
