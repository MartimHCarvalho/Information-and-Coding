#include "../../../includes/part1/exe1.hpp"
#include <iostream>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

int extractChannel(const string &inputFile, const string &outputFile, int channel)
{
    // Validate channel number
    if (channel < 0 || channel > 2)
    {
        cerr << "Error: Channel number must be 0, 1, or 2." << endl;
        return -1;
    }

    cout << "DEBUG: Extracting channel " << channel << endl;

    // Load the color image
    Mat img = imread(inputFile, IMREAD_COLOR);
    if (img.empty())
    {
        cerr << "Error: Cannot read image file " << inputFile << endl;
        return -1;
    }

    cout << "DEBUG: Image size: " << img.rows << "x" << img.cols << endl;

    // Create a single-channel image to store the extracted channel
    Mat channelImg(img.rows, img.cols, CV_8UC1);

    // Extract the channel pixel by pixel
    for (int i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            Vec3b pixel = img.at<Vec3b>(i, j); // BGR pixel
            channelImg.at<uchar>(i, j) = pixel[channel];
        }
    }

    // Add sample pixel debug info
    Vec3b testPixel = img.at<Vec3b>(0, 0);
    cout << "DEBUG: First pixel BGR = [" << (int)testPixel[0] << ", "
         << (int)testPixel[1] << ", " << (int)testPixel[2] << "]" << endl;
    cout << "DEBUG: Extracted value = " << (int)channelImg.at<uchar>(0, 0) << endl;

    // Check output file extension
    string extension = outputFile.substr(outputFile.find_last_of(".") + 1);
    
    // If output is PPM, convert single channel to 3-channel BGR image
    if (extension == "ppm" || extension == "PPM")
    {
        Mat outputImg(img.rows, img.cols, CV_8UC3);
        
        // Create a grayscale image in BGR format (all channels have the same value)
        for (int i = 0; i < img.rows; i++)
        {
            for (int j = 0; j < img.cols; j++)
            {
                uchar value = channelImg.at<uchar>(i, j);
                outputImg.at<Vec3b>(i, j) = Vec3b(value, value, value);
            }
        }
        
        // Write the 3-channel result
        if (!imwrite(outputFile, outputImg))
        {
            cerr << "Error: Could not write to " << outputFile << endl;
            return -1;
        }
        cout << "Channel " << channel << " extracted and saved as grayscale PPM to " << outputFile << endl;
    }
    else
    {
        // For PGM or other formats, save the single-channel image directly
        if (!imwrite(outputFile, channelImg))
        {
            cerr << "Error: Could not write to " << outputFile << endl;
            return -1;
        }
        cout << "Channel " << channel << " extracted and saved to " << outputFile << endl;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cout << "Usage: " << argv[0] << " <input_image> <output_image> <channel>" << endl;
        cout << "Channel: 0 = Blue, 1 = Green, 2 = Red" << endl;
        cout << "Note: Use .pgm extension for grayscale output, .ppm for color output" << endl;
        return -1;
    }

    string inputFile = argv[1];
    string outputFile = argv[2];
    int channel = atoi(argv[3]);

    return extractChannel(inputFile, outputFile, channel);
}