#include "../../../includes/part1/exe1.hpp"
#include <iostream>

using namespace cv;
using namespace std;

int extractChannel(const string& inputFile, const string& outputFile, int channel) {
    // Validate channel number
    if (channel < 0 || channel > 2) {
        cerr << "Error: Channel number must be 0, 1, or 2." << endl;
        return -1;
    }
    
    // Load the color image
    Mat img = imread(inputFile, IMREAD_COLOR);
    if (img.empty()) {
        cerr << "Error: Cannot read image file " << inputFile << endl;
        return -1;
    }
    
    // Create a single-channel image to store the extracted channel
    Mat channelImg(img.rows, img.cols, CV_8UC1);
    
    // Extract the channel pixel by pixel
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            Vec3b pixel = img.at<Vec3b>(i, j); // BGR pixel
            channelImg.at<uchar>(i, j) = pixel[channel];
        }
    }
    
    // Write the result
    imwrite(outputFile, channelImg);
    cout << "Channel " << channel << " extracted and saved to " << outputFile << endl;
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <input_image> <output_image> <channel>" << endl;
        cout << "Channel: 0 = Blue, 1 = Green, 2 = Red" << endl;
        return -1;
    }
    
    string inputFile = argv[1];
    string outputFile = argv[2];
    int channel = atoi(argv[3]);
    
    return extractChannel(inputFile, outputFile, channel);
}
