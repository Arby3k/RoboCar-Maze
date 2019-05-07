#pragma once
#include "opencv.hpp"
#include <iostream>
#include <cstdlib>
#include <string>
#include <cmath>
#include <vector>
#include <pigpio.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <raspicam/raspicam_cv.h>
#include "raspicam_cv.h"
#include "opencv.hpp"
// OpenCV Include
#include <opencv2/opencv.hpp>

class CCar
{
    private:
        raspicam::RaspiCam_Cv vid;
        cv::Mat rcam, crop, hsv, pink, purple, green, orange; //Green and Purple are right
        enum STATES{GET_IMAGE = 0, PROCESS_IMAGE, MOVE_CAR};
        STATES state;
        int moveRLN;

    public:
        CCar();
        ~CCar();
        void update();
        void processImage();
        int kbhit(void);
        void delay(double);
        void camera();
        int manual(char);
};
