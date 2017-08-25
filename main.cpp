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

#include <cstdint>
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ChromaKeyer.hpp"
#include "Flags.hh"

using namespace cv;

int main(int argc, char** argv)
{
    bool f_automagic;
    Flags flags;

    std::string filein, fileout;

    ChromaKeyer ck;
    Mat matout;

	ChromaKeyerParams ckp = {
		.bgcolor_rgb =		Vec3f(0.0f, 1.0f, 0.0f),
		.bgcolor_cbcr =		ChromaKeyer::colorRGB2CbCr(Vec3f(0.0f, 1.0f, 0.0f)),
		.fgcolor_cbcr =		Vec2f(0.0f, 0.0f),

        .tolerance_hi =		0.30f,
        .tolerance_lo =		0.25f,
		.tolerance_mask =	0.05f,

        .histogram_size = 	512,

        .auto_color_threshold = 0.25f,
        .auto_color_expansion = 0.08f
	};

    flags.Var(filein, 'i', "input", std::string(), "Input image (required)");
    flags.Var(fileout, 'o', "output", std::string(), "Output image (required)");

    flags.Var(ckp.tolerance_lo, 'l', "thresholdlow", 0.25f, "Low threshold - 0.0 to 1.0", "Classic method");
    flags.Var(ckp.tolerance_hi, 'h', "thresholdhigh", 0.30f, "High threshold - 0.0 to 1.0", "Classic method");

    flags.Bool(f_automagic, 'a', "automagic", "Use automagic method instead of the classic one.", "Automagic method");
    flags.Var(ckp.auto_color_threshold, 'e', "expansion", 0.08f, "Color expansion factor - 0.0 to 1.0", "Automagic method");
    flags.Var(ckp.auto_color_threshold, 't', "autothreshold", 0.25f, "Color separation threshold - 0.0 to 1.0", "Automagic method");

    if(!flags.Parse(argc, argv) || argc == 1 || filein == std::string() || fileout == std::string())
    {
        flags.PrintHelp(argv[0]);
        return 1;
    }

    ck.setParams(ckp);

    // Input file
    if(!ck.loadFromFile(filein))
    {
        std::cerr << "Could not open the file \"" << filein << "\"" << std::endl;
        return 2;
    }

    if(f_automagic)
        matout = ck.applyChromaKey(CK_METHOD_AUTOMAGIC);
    else
        matout = ck.applyChromaKey(CK_METHOD_CLASSIC);

    imwrite(fileout, matout);

    std::cerr << "File \"" << fileout << "\" saved successfully!" << std::endl;

    return 0;
}
