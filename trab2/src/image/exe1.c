#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_image> <output_image> <channel_number>\n", argv[0]);
        printf("Channel: 0 = Blue, 1 = Green, 2 = Red\n");
        return -1;
    }

    char *inputFile = argv[1];
    char *outputFile = argv[2];
    int channel = atoi(argv[3]);

    if (channel < 0 || channel > 2) {
        fprintf(stderr, "Error: Channel number must be 0, 1, or 2.\n");
        return -1;
    }

    // Load the color image
    Mat img = imread(inputFile, IMREAD_COLOR);
    if (img.empty()) {
        fprintf(stderr, "Error: Cannot read image file %s\n", inputFile);
        return -1;
    }

    // Create a single-channel image to store the extracted channel
    Mat channelImg(img.rows, img.cols, CV_8UC1);

    // Extract the channel pixel by pixel
    for (int i = 0; i < img.rows; i++) {
        for (int j = 0; j < img.cols; j++) {
            Vec3b pixel = img.at<Vec3b>(i, j);   // BGR pixel
            channelImg.at<uchar>(i, j) = pixel[channel];
        }
    }

    // Write the result
    imwrite(outputFile, channelImg);

    printf("Channel %d extracted and saved to %s\n", channel, outputFile);
    return 0;
}
