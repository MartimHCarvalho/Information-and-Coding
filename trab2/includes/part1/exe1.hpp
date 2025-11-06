#ifndef EXE1_HPP
#define EXE1_HPP

#include <opencv2/opencv.hpp>
#include <string>

/**
 * Extracts a single channel from a color image
 * @param inputFile Path to the input image file
 * @param outputFile Path to save the extracted channel
 * @param channel Channel to extract (0=Blue, 1=Green, 2=Red)
 * @return 0 on success, -1 on error
 */
int extractChannel(const std::string& inputFile, const std::string& outputFile, int channel);

#endif // EXE1_HPP
