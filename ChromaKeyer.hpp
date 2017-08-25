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

#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>

#pragma once

/**
 * @brief Used as initializing arguments.
*/
struct ChromaKeyerParams
{
	// Major params
	cv::Vec3f	bgcolor_rgb;
	cv::Vec2f	bgcolor_cbcr, fgcolor_cbcr;

	float	tolerance_hi, tolerance_lo, tolerance_mask;

	int 	histogram_size;

    float   auto_color_threshold, auto_color_expansion;
};

/**
* @brief Used internally to avoid redundant work.
*/
enum _ChromaKeyerStage
{
	CK_ZERO,
	CK_IMG_LOADED,
	CK_HISTOGRAM_GENERATED,
	CK_MASK_GENERATED
};

enum ChromaKeyerMethod
{
    CK_METHOD_CLASSIC,
    CK_METHOD_AUTOMAGIC
};

class ChromaKeyer
{
private:
	ChromaKeyerParams _pars;

    cv::Mat 	_input, _inputcbcr;
    cv::Mat 	_realhisto, _adaptedhisto;
    cv::Mat 	_mask;

	_ChromaKeyerStage _stage;

public:
    cv::Mat     histomask();

	ChromaKeyer();
	ChromaKeyer(ChromaKeyerParams& params);

	static cv::Vec2f colorRGB2CbCr(cv::Vec3f color_rgb);
	static cv::Vec2f colorRGB2CbCr(uint8_t R, uint8_t G, uint8_t B);
    static void view(cv::Mat& m);
    static void viewAsJet(cv::Mat& view);

	// Stage 1: Loading image
	bool loadFromFile(std::string path);
	bool loadFromMat(cv::Mat& input);

	// Stage 2: Generate histogram
	bool generateHistogram(cv::Mat* output = NULL);

	// Setters
	void setParams(ChromaKeyerParams& pars);

    // Mask generators
    bool generateMaskClassic();
    bool generateMaskClassic(double tolerancelo, double tolerancehi);
    bool generateMaskAuto();

    // Merger
    cv::Mat applyChromaKey(ChromaKeyerMethod method = CK_METHOD_CLASSIC);

	// Getters
	cv::Mat getHistogram() const;
	cv::Mat drawHistogram(bool markings = true);
    cv::Mat getMask() const;
	ChromaKeyerParams getParams() const;
};
