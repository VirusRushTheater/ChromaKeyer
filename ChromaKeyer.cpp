/*
 
Copyright (c) 2017, Virus Rush Theater

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the
following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or
promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "ChromaKeyer.hpp"

#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#define _CK_CHECKINPUTEMPTY					if(_input.empty())	{_stage = CK_ZERO; return false;} \
											else				{_stage = CK_IMG_LOADED; return true;}

#define _CK_COLORWHITE                      Scalar(255,255,255)
#define _CK_COLORCYAN                       Scalar(255,255,0)
#define _CK_COLORYELLOW                      Scalar(0,255,255)

#define _CK_STAGELOWER(LV)					((int)_stage < (LV))
#define _CK_STAGEHIGHER(LV)					((int)_stage >= (LV))
#define _CK_ASSERTSTAGE(LV,RETVAL)			if(_CK_STAGELOWER(LV)){return(RETVAL);}
#define _CK_DRAWCROSS(MAT, PX, PY, WIDTH, COLOR)	cv::line(retval, Point(PX+WIDTH, PY+WIDTH), Point(PX-WIDTH, PY-WIDTH), COLOR); \
                                                    cv::line(retval, Point(PX+WIDTH, PY-WIDTH), Point(PX-WIDTH, PY+WIDTH), COLOR);

using namespace cv;

ChromaKeyer::ChromaKeyer() :
	_stage(CK_ZERO)
{
}

ChromaKeyer::ChromaKeyer(ChromaKeyerParams& params) :
	_stage(CK_ZERO)
{
	setParams(params);
}

/**
 * Utility to convert a pixel to CbCr values.
*/
Vec2f ChromaKeyer::colorRGB2CbCr(Vec3f color_rgb)
{
	Mat in_pixel = Mat(1, 1, CV_32FC3);
    Mat out_pixel = Mat(1, 1, CV_32FC3);

    Vec3f outcolor;

    float  	newR = color_rgb[0],
            newG = color_rgb[1],
            newB = color_rgb[2];

    in_pixel.at<Vec3f>(0,0) = Vec3f(newB, newG, newR);
    cvtColor(in_pixel, out_pixel, CV_RGB2YCrCb);
    outcolor = out_pixel.at<Vec3f>(0,0);

    return Vec2f(outcolor[1], outcolor[2]);
}

Vec2f ChromaKeyer::colorRGB2CbCr(uint8_t R, uint8_t G, uint8_t B)
{
	return ChromaKeyer::colorRGB2CbCr(Vec3f((float)B/256.0f, (float)G/256.0f, (float)R/256.0f));
}

cv::Mat ChromaKeyer::histomask()
{
    Mat retval = Mat(_pars.histogram_size, _pars.histogram_size, CV_8U);
    retval = Scalar(0);
    circle(retval, Point((float)_pars.histogram_size*0.25, (float)_pars.histogram_size*0.25), (float)_pars.histogram_size*0.3, Scalar(255), -1);

    return retval;
}

void ChromaKeyer::view(Mat& m)
{
    imshow("View", m);
    waitKey(0);
}

void ChromaKeyer::viewAsJet(Mat& view)
{
    Mat retval;

    view.copyTo(retval);
    normalize(retval, retval, 0, 255, NORM_MINMAX, CV_8UC1);
    applyColorMap(retval, retval, COLORMAP_JET);

    imshow("Jet", retval);
    waitKey(0);
}

bool ChromaKeyer::loadFromFile(std::string path)
{
	_input = imread(path);
	_CK_CHECKINPUTEMPTY
}

bool ChromaKeyer::loadFromMat(Mat& input)
{
	input.copyTo(_input);
	_CK_CHECKINPUTEMPTY
}

bool ChromaKeyer::generateHistogram(Mat* output)
{
    std::vector<Mat> _input_cbcr3c;

	// Avoid unnecessary calculation if not needed, or return 
	_CK_ASSERTSTAGE(CK_IMG_LOADED, false);

	// Skip all of this if it has been generated already.
	if(!_CK_STAGEHIGHER(CK_HISTOGRAM_GENERATED))
	{
		// Histogram calculation values
        const int hist_channels[2] =   	{0, 1};
		const int hist_size[2] =		{_pars.histogram_size, _pars.histogram_size};
        const float hist_ranges_cb[2] = {0.0f, 1.0f};
        const float hist_ranges_cr[2] = {0.0f, 1.0f};
	    const float* hist_ranges[2] =   {hist_ranges_cb, hist_ranges_cr};

	    // Adapted Histogram presets
	    const float   	log_floor_preset = 		1.0f;
        int 			kernel_size_preset = 	_pars.histogram_size / 20;
        if(kernel_size_preset % 2 == 0)
            kernel_size_preset++;

		// Convert to YCbCr. We don't need the Y channel.
        cvtColor(_input, _inputcbcr, CV_RGB2YCrCb);
        _inputcbcr.convertTo(_inputcbcr, CV_32F);
        _inputcbcr /= 256.0f;
        split(_inputcbcr, _input_cbcr3c);
        _input_cbcr3c.erase(_input_cbcr3c.begin());
        merge(_input_cbcr3c, _inputcbcr);

	    // Generate a 2D histogram of CbCr values.
        calcHist(&_inputcbcr, 1, hist_channels, Mat(), _realhisto,
	             2, hist_size, hist_ranges, true, false);

	    // Adapts it to a logarithmic version.
        _realhisto.convertTo(_adaptedhisto, CV_64F);
        cv::add(_adaptedhisto, log_floor_preset, _adaptedhisto);
        cv::GaussianBlur(_adaptedhisto, _adaptedhisto, Size(kernel_size_preset, kernel_size_preset), 0, 0);
        cv::log(_adaptedhisto, _adaptedhisto);
        normalize(_adaptedhisto, _adaptedhisto, 0.0, 1.0, NORM_MINMAX, CV_64F);

	    _stage =	CK_HISTOGRAM_GENERATED;
    }

    // Deep copies the Adapted histogram if requested.
    if(output)
    {
        _adaptedhisto.copyTo(*output);
    }

    // Done.
    return true;
}

void ChromaKeyer::setParams(ChromaKeyerParams& pars)
{
	memcpy(&_pars, &pars, sizeof(ChromaKeyerParams));
}

Mat ChromaKeyer::getHistogram() const
{
	Mat retval;
    _adaptedhisto.copyTo(retval);
	return retval;
}

cv::Mat ChromaKeyer::drawHistogram(bool markings)
{
	Mat retval;
	if(!generateHistogram())
	{
		return Mat();
	}
    _adaptedhisto.copyTo(retval);
	normalize(retval, retval, 0, 255, NORM_MINMAX, CV_8UC1);
    applyColorMap(retval, retval, COLORMAP_JET);

    if(markings)
    {
        // Draw crosshair on key color
        const int x_width = 3;

        int bgcolor_position[2] = {(int)((float)_pars.histogram_size * _pars.bgcolor_cbcr[0]),
                                    (int)((float)_pars.histogram_size * _pars.bgcolor_cbcr[1])};
        int fgcolor_position[2] = {(int)((float)_pars.histogram_size * _pars.fgcolor_cbcr[0]),
                                    (int)((float)_pars.histogram_size * _pars.fgcolor_cbcr[1])};

        // Cross on CbCr maximum
        _CK_DRAWCROSS(retval, bgcolor_position[0], bgcolor_position[1], x_width, _CK_COLORWHITE);
        
        // Draw tolerances
        circle(retval, Point(bgcolor_position[0], bgcolor_position[1]),
                (int)(_pars.tolerance_mask * _pars.histogram_size), Scalar(255, 255, 0));
        circle(retval, Point(bgcolor_position[0], bgcolor_position[1]),
                (int)(_pars.tolerance_lo * _pars.histogram_size), Scalar(255, 255, 255));
        circle(retval, Point(bgcolor_position[0], bgcolor_position[1]),
                (int)(_pars.tolerance_hi * _pars.histogram_size), Scalar(255, 255, 255));

        // Cross on CbCr second maximum
        _CK_DRAWCROSS(retval, fgcolor_position[0], fgcolor_position[1], x_width, _CK_COLORYELLOW);
    }

	return retval;
}

bool ChromaKeyer::generateMaskClassic()
{
    return generateMaskClassic(_pars.tolerance_lo, _pars.tolerance_hi);
}

bool ChromaKeyer::generateMaskClassic(double tolerance_lo, double tolerance_hi)
{
    int bgindex[2];
    float distance;
    Vec2f distance_ref;

    if(tolerance_lo > tolerance_hi ||
            tolerance_lo < 0.0 || tolerance_hi < 0.0 ||
            tolerance_lo > 1.0 || tolerance_hi > 1.0)
        return false;

    // Generates histogram if it hasn't been generated yet.
    if(_CK_STAGELOWER(CK_HISTOGRAM_GENERATED))
    {
        if(!generateHistogram())
            return false;
    }

    // Finds BG color by selecting the most frequent color.
    cv::minMaxIdx(_adaptedhisto, NULL, NULL, NULL, bgindex, histomask());
    _pars.bgcolor_cbcr[0] = (float)bgindex[1]/(float)_pars.histogram_size;
    _pars.bgcolor_cbcr[1] = (float)bgindex[0]/(float)_pars.histogram_size;

    _mask = Mat(_input.rows, _input.cols, CV_32F);

    distance_ref =  _pars.bgcolor_cbcr;

    // Map pixels to alpha values.
    for(int i = 0; i < _mask.rows; i++)
    {
        for(int j = 0; j < _mask.cols; j++)
        {
            distance = cv::norm(_inputcbcr.at<Vec2f>(i,j), distance_ref);
            if(distance <= tolerance_lo)
            {
                _mask.at<float>(i,j) = 0.0f;
            }
            else if(distance >= tolerance_hi)
            {
                _mask.at<float>(i,j) = 255.0f;
            }
            else
            {
                _mask.at<float>(i,j) = 255.0f*(distance - tolerance_lo)/(tolerance_hi - tolerance_lo);
            }
        }
    }
    _mask.convertTo(_mask, CV_8UC1);

    _stage =	CK_MASK_GENERATED;
    return true;
}

bool ChromaKeyer::generateMaskAuto()
{
    int bgindex[2];
    Mat fgmask, fgblobs, dil_kernel;
    int cclabels, label_key;
    Mat _maskmap;
    Vec2f cbcrvalues;
    float pixel;

    int   bg_expansion = _pars.auto_color_expansion * (float)_pars.histogram_size;
    if(bg_expansion % 2 == 0)
        bg_expansion++;

    // Generates histogram if it hasn't been generated yet.
    if(_CK_STAGELOWER(CK_HISTOGRAM_GENERATED))
    {
        if(!generateHistogram())
            return false;
    }

    // Finds BG color by selecting the most frequent color.
    cv::minMaxIdx(_adaptedhisto, NULL, NULL, NULL, bgindex, histomask());
    _pars.bgcolor_cbcr[0] = (float)bgindex[1]/(float)_pars.histogram_size;
    _pars.bgcolor_cbcr[1] = (float)bgindex[0]/(float)_pars.histogram_size;

    // Prepares a removal mask
    fgmask = _adaptedhisto > _pars.auto_color_threshold;
    cclabels = cv::connectedComponents(fgmask, fgblobs, 8, CV_32S);

    label_key =     fgblobs.at<int>(bgindex[0], bgindex[1]);
    _maskmap =      fgblobs != label_key;

    dil_kernel =    Mat::ones(bg_expansion, bg_expansion, CV_8U);
    cv::erode(_maskmap, _maskmap, dil_kernel);
    cv::GaussianBlur(_maskmap, _maskmap, Size(bg_expansion, bg_expansion), 0, 0);

    // Map pixels into alpha values
    _mask = Mat(_input.rows, _input.cols, CV_32F);

    for(int i = 0; i < _mask.rows; i++)
    {
        for(int j = 0; j < _mask.cols; j++)
        {
            cbcrvalues = _inputcbcr.at<Vec2f>(i,j) * _pars.histogram_size;
            pixel = _maskmap.at<uint8_t>(cbcrvalues[0], cbcrvalues[1]);
            _mask.at<float>(i,j) = pixel;
        }
    }
    _mask.convertTo(_mask, CV_8UC1);

    _stage =	CK_MASK_GENERATED;
    return true;
}

Mat ChromaKeyer::applyChromaKey(ChromaKeyerMethod method)
{
    Mat retval;

    std::vector<Mat> inputlayers;
    bool result;

    switch(method)
    {
        case CK_METHOD_CLASSIC:
            result = generateMaskClassic();
        break;
        case CK_METHOD_AUTOMAGIC:
            result = generateMaskAuto();
        break;
    }

    if(!result)
        return retval;

    // Mask should be generated as for now.
    split(_input, inputlayers);
    inputlayers.push_back(_mask);
    merge(inputlayers, retval);

    return retval;
}

ChromaKeyerParams ChromaKeyer::getParams() const
{
	return _pars;
}

Mat ChromaKeyer::getMask() const
{
    return _mask;
}
