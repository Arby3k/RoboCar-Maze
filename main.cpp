#include <iostream>
#include <cstdlib>
#include <string>
#include <cmath>
#include <vector>
#include <thread>
#include <pigpio.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <raspicam/raspicam_cv.h>
#include "raspicam_cv.h"
#include "opencv.hpp"
#include "CCar.h"
// OpenCV Include
#include <opencv2/opencv.hpp>


// OpenCV Library
#pragma comment(lib,".\\opencv\\lib\\opencv_world310d.lib")
using namespace std;

int main()
{
    CCar car;
    while(1)
    {
        car.update();
    }
    return 0;
}
