//------------------------------------------------------------------------------
//
// Copyright 2025 University of Aveiro, Portugal, All Rights Reserved.
//
// These programs are supplied free of charge for research purposes only,
// and may not be sold or incorporated into any commercial product. There is
// ABSOLUTELY NO WARRANTY of any sort, nor any undertaking that they are
// fit for ANY PURPOSE WHATSOEVER. Use them at your own risk. If you do
// happen to find a bug, or have modifications to suggest, please report
// the same to Armando J. Pinho, ap@ua.pt. The copyright notice above
// and this statement of conditions must remain an integral part of each
// and every copy made of these files.
//
// Armando J. Pinho (ap@ua.pt)
// IEETA / DETI / University of Aveiro
//
#include <iostream>
#include <vector>
#include <sndfile.hh>
#include "wav_hist.h"

using namespace std;

constexpr size_t FRAMES_BUFFER_SIZE = 65536; // Buffer for reading frames

int main(int argc, char *argv[]) {

    if(argc < 3 || argc > 4) {
        cerr << "Usage: " << argv[0] << " <input file> <channel> [k]\n";
        return 1;
    }

    const char* inputPath = argv[1];
    int channel = stoi(argv[2]);
    unsigned binShift = 0;
    if (argc == 4) {
        int k = stoi(argv[3]);
        if (k < 0) k = 0;
        if (k > 15) k = 15;
        binShift = static_cast<unsigned>(k);
    }


	SndfileHandle sndFile { inputPath };
	if(sndFile.error()) {
		cerr << "Error: invalid input file\n";
		return 1;
    }

	if((sndFile.format() & SF_FORMAT_TYPEMASK) != SF_FORMAT_WAV) {
		cerr << "Error: file is not in WAV format\n";
		return 1;
	}

	if((sndFile.format() & SF_FORMAT_SUBMASK) != SF_FORMAT_PCM_16) {
		cerr << "Error: file is not in PCM_16 format\n";
		return 1;
	}

	int channel { stoi(argv[argc-1]) };
	WAVHist hist { sndFile, binShift };
    
    // Validate channel number
    if(channel >= sndFile.channels()) {
        if(sndFile.channels() == 2) {
            if(channel == 2) {
                // MID channel for stereo
            } else if(channel == 3) {
                // SIDE channel for stereo
            } else {
                cerr << "Error: invalid channel requested (0-" << sndFile.channels()-1 << " for individual channels";
                if(sndFile.channels() == 2) {
                    cerr << ", 2 for MID, 3 for SIDE";
                }
                cerr << ")\n";
                return 1;
            }
        } else {
            cerr << "Error: invalid channel requested\n";
            return 1;
        }
    }

    size_t nFrames;
    std::vector<short> samples(FRAMES_BUFFER_SIZE * sndFile.channels());

    while((nFrames = sndFile.readf(samples.data(), FRAMES_BUFFER_SIZE))) {
        const size_t nSamples = nFrames * sndFile.channels();
        hist.update(samples.data(), nSamples); 
    }

    hist.dump(channel);
    return 0;
}

