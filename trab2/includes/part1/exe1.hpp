#ifndef EXE1_HPP
#define EXE1_HPP

#include <string>

/**
 * Extracts a single color channel from a BGR image and saves it as a grayscale image.
 * 
 * @param inputFile Path to the input color image
 * @param outputFile Path where the extracted channel will be saved
 * @param channel Channel to extract (0 = Blue, 1 = Green, 2 = Red)
 * @return 0 on success, -1 on error
 */
int extractChannel(const std::string& inputFile, const std::string& outputFile, int channel);

#endif // EXE1_H
