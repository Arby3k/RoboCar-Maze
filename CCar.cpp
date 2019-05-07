#include "CCar.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <thread>
#include <mutex>
#include <iomanip>
#include <cmath>
#include <vector>
#include <pigpio.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <raspicam/raspicam_cv.h>
#include "raspicam_cv.h"
#include "server.h"
#include "opencv.hpp"
// OpenCV Include
#include <opencv2/opencv.hpp>

#define RANGE 50
#define BLK 25
#define HB_LOW 90
#define SB_LOW 90
#define VB_LOW 110
#define HB_HIGH 135
#define SB_HIGH 255
#define VB_HIGH 255
#define HO_LOW 0
#define SO_LOW 0
#define VO_LOW 90
#define HO_HIGH 179
#define SO_HIGH 245
#define VO_HIGH 255

int iLowH = 0;
int iHighH = 179;

int iLowS = 0;
int iHighS = 255;

int iLowV = 0;
int iHighV = 255;

using namespace std;
using namespace cv;

std::vector<cv::Point> contour;
std::vector<std::vector<cv::Point>> contoursPink;
std::vector<std::vector<cv::Point>> contoursPurple;
std::vector<std::vector<cv::Point>> contoursGreen;
std::vector<std::vector<cv::Point>> contoursOrange;
std::vector<cv::Vec4i> hierarchyPink;
std::vector<cv::Vec4i> hierarchyPurple;
std::vector<cv::Vec4i> hierarchyGreen;
std::vector<cv::Vec4i> hierarchyOrange;
std::vector<string> commands;
//string keyPress;
char keyPress;
mutex cam;
mutex pro;
mutex man;
//mutex image;
Server serv;

CCar::CCar()
{
    state = GET_IMAGE;
    moveRLN = 2;

    vid.open();
    //cv::namedWindow("Control", CV_WINDOW_AUTOSIZE); //create a window called "Control"

    if (gpioInitialise() < 0)
	{
		return ;
	}

    // Motor A Forward
    gpioSetMode(27, PI_OUTPUT);

    // Motor A Reverse
    gpioSetMode(17, PI_OUTPUT);

    // Motor B Forward
    gpioSetMode(4, PI_OUTPUT);

    // Motor B Reverse
    gpioSetMode(25, PI_OUTPUT);

    // Motor A PWM
    gpioSetMode(12, PI_OUTPUT);

    // Motor B PWM
    gpioSetMode(13, PI_OUTPUT);

    // System LED
    gpioSetMode(26, PI_OUTPUT);

    // Optical Sensor
    gpioSetMode(10, PI_INPUT);

    gpioWrite(12, 1);
    gpioWrite(13, 1);

    //serv.start(4618);
    thread t1(&CCar::camera,this);
    t1.detach();
    thread t2(&CCar::manual,this,keyPress);
    t2.detach();
    thread t3(&CCar::kbhit,this);
    t3.detach();
    thread t4(&CCar::processImage,this);
    t4.detach();

	/*//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &iLowH, 179); //Hue (0 - 179)
	cvCreateTrackbar("HighH", "Control", &iHighH, 179);

	cvCreateTrackbar("LowS", "Control", &iLowS, 255); //Saturation (0 - 255)
	cvCreateTrackbar("HighS", "Control", &iHighS, 255);

	cvCreateTrackbar("LowV", "Control", &iLowV, 255); //Value (0 - 255)
	cvCreateTrackbar("HighV", "Control", &iHighV, 255);*/
}
CCar::~CCar()
{
    gpioTerminate();
}

void CCar::update()
{
    //vid.grab();
    //vid.retrieve(rcam);
    //crop = rcam(cv::Rect(340, 360, 600, 300));

    camera();
    keyPress = 'z';
    //serv.get_cmd(commands);
    //if(commands.size() > 0)
    //{
        //keyPress = commands[0];

    if(kbhit())
        keyPress = getchar();

    if(keyPress == 'm')
    {
        gpioWrite(26, 1);
        keyPress = 'z';
        //commands.clear();
        do
        {
            if(kbhit())
                keyPress = getchar();
            //serv.get_cmd(commands);
            //if(commands.size() > 0)
            //{
                //keyPress = commands[0];
                //camera();
                //vid.grab();
                //vid.retrieve(rcam);
                //crop = rcam(cv::Rect(340, 640, 600, 300));

                //int state = processImage();
                if  (state == MOVE_CAR ) {

                switch(moveRLN)
                {
                    case 0:
                        //cout << "PINK/ORANGE" << endl;
                        gpioWrite(27, 0);
                        gpioWrite(17, 0);
                        gpioWrite(4, 0);
                        gpioWrite(25, 0);
                        delay(500);
                        gpioWrite(27,1);
                        gpioWrite(17,0);
                        gpioWrite(4,1);
                        gpioWrite(25,0);
                        break;
                    case 1:
                        //cout << "PURPLE/GREEN" << endl;
                        gpioWrite(27, 0);
                        gpioWrite(17, 0);
                        gpioWrite(4, 0);
                        gpioWrite(25, 0);
                        delay(500);
                        gpioWrite(27,0);
                        gpioWrite(17,1);
                        gpioWrite(4,0);
                        gpioWrite(25,1);
                        break;
                    case 2:
                        //cout << "NOTHING" << endl;
                        gpioWrite(27,0);
                        gpioWrite(17,1);
                        gpioWrite(4,1);
                        gpioWrite(25,0);
                        break;
                }
                state = GET_IMAGE;
                }
            //}
        }while(keyPress != 'm');
        keyPress = 'z';
        gpioWrite(26, 0);
            //commands.clear();
    }
    else
    {
        manual(keyPress);
        /*if(keyPress == 'w')
        {
            cout << "FORWARD" << endl;
            gpioWrite(27, 0);
            gpioWrite(17, 1);
            gpioWrite(4, 1);
            gpioWrite(25, 0);
            keyPress = 'z';
        }
        else if(keyPress == 's')
        {
            cout << "REVERSE" << endl;
            gpioWrite(27, 1);
            gpioWrite(17, 0);
            gpioWrite(4, 0);
            gpioWrite(25, 1);
            keyPress = 'z';
        }
        else if(keyPress == 'd')
        {
            cout << "RIGHT" << endl;
            gpioWrite(27, 0);
            gpioWrite(17, 1);
            gpioWrite(4, 0);
            gpioWrite(25, 1);
            keyPress = 'z';
        }
        else if(keyPress == 'a')
        {
            cout << "LEFT" << endl;
            gpioWrite(27, 1);
            gpioWrite(17, 0);
            gpioWrite(4, 1);
            gpioWrite(25, 0);
            keyPress = 'z';
        }
        else if(keyPress == 'z')
        {
            gpioWrite(27, 0);
            gpioWrite(17, 0);
            gpioWrite(4, 0);
            gpioWrite(25, 0);
        }*/
    }
    //}
    //commands.clear();
    //cv::imshow("REAL WORLD", crop);
    //cv::waitKey(1);
}

void CCar::processImage()
{
    pro.lock();
    if ( state == PROCESS_IMAGE ){
    pink = crop;
	purple = crop;
	green = crop;
	orange = crop;
	cv::cvtColor(pink, hsv, CV_BGR2HSV);
	cv::cvtColor(purple, hsv, CV_BGR2HSV);
	cv::cvtColor(green, hsv, CV_BGR2HSV);
	cv::cvtColor(orange, hsv, CV_BGR2HSV);
    //Scalar hsv_avg;
    //hsv_avg = mean(hsv);
    //cout << hsv_avg << endl;

    //////////////////////                      PINK              ///////////////////////
	//cv::inRange(hsv, cv::Scalar(120, 120, 100), cv::Scalar(179, 215, 255), pink);
	//cv::inRange(hsv, Scalar(iLowH, iLowS, iLowV), Scalar(iHighH, iHighS, iHighV), pink);
	cv::inRange(hsv, cv::Scalar(137, 87, 188), cv::Scalar(180, 166, 255), pink);

    cv::erode(pink, pink, cv::Mat(), cv::Point(-1, -1), 3);
    cv::dilate(pink, pink, cv::Mat(), cv::Point(-1, -1), 3);

	cv::findContours(pink, contoursPink, hierarchyPink, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);



    //////////////////////                      Purple            ///////////////////////
    //cv::inRange(hsv, cv::Scalar(103, 0, 0), cv::Scalar(120, 255, 255), purple);
    //cv::inRange(hsv, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), purple);
    cv::inRange(hsv, cv::Scalar(0, 237, 172), cv::Scalar(34, 255, 255), purple);

    cv::erode(purple, purple, cv::Mat(), cv::Point(-1, -1), 3);
    cv::dilate(purple, purple, cv::Mat(), cv::Point(-1, -1), 3);

    cv::findContours(purple, contoursPurple, hierarchyPurple, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);



    //////////////////////                      Green             ///////////////////////
    //cv::inRange(hsv, cv::Scalar(43, 70, 0), cv::Scalar(73, 255, 255), green);
    //cv::inRange(hsv, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), green);
    cv::inRange(hsv, cv::Scalar(45, 140, 70), cv::Scalar(77, 255, 255), green);

    cv::erode(green, green, cv::Mat(), cv::Point(-1, -1), 3);
    cv::dilate(green, green, cv::Mat(), cv::Point(-1, -1), 3);

    cv::findContours(green, contoursGreen, hierarchyGreen, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);



    //////////////////////                      Orange            ///////////////////////
    cv::inRange(hsv, cv::Scalar(0, 0, 148), cv::Scalar(22, 255, 255), orange);
    //cv::inRange(hsv, cv::Scalar(iLowH, iLowS, iLowV), cv::Scalar(iHighH, iHighS, iHighV), orange);
    //cv::inRange(hsv, cv::Scalar(105, 96, 212), cv::Scalar(119, 245, 255), orange);

    cv::erode(orange, orange, cv::Mat(), cv::Point(-1, -1), 3);
    cv::dilate(orange, orange, cv::Mat(), cv::Point(-1, -1), 3);

    cv::findContours(orange, contoursOrange, hierarchyOrange, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);



    for (int i = 0; i < contoursPink.size(); i++)
    {
        cv::Rect rpink = boundingRect(contoursPink.at(i));

        if (rpink.area() > 30000)
        {
            moveRLN = 0;
            pro.unlock();
            return;
            //return 0;
        }
		//cv::drawContours(pink, contoursPink, i, cv::Scalar(255, 255, 255), CV_FILLED, 8, hierarchyPink);
    }


    for (int i = 0; i < contoursPurple.size(); i++)
    {
        cv::Rect rpurple = boundingRect(contoursPurple.at(i));

        if (rpurple.area() > 30000)
        {
            moveRLN = 1;
            state = MOVE_CAR;
            pro.unlock();
            return;
           // return 1;
        }
		//drawContours(purple, contoursPurple, i, cv::Scalar(255, 255, 255), CV_FILLED, 8, hierarchyPurple);
    }


    for (int i = 0; i < contoursGreen.size(); i++)
    {
        cv::Rect rgreen = boundingRect(contoursGreen.at(i));

        if (rgreen.area() > 30000)
        {
            moveRLN = 1;
            state = MOVE_CAR;
            pro.unlock();
            return;
            //return 1;
        }
		//drawContours(green, contoursGreen, i, cv::Scalar(255, 255, 255), CV_FILLED, 8, hierarchyGreen);
    }


    for (int i = 0; i < contoursOrange.size(); i++)
    {
        cv::Rect rorange = boundingRect(contoursOrange.at(i));

        if (rorange.area() > 30000)
        {
            moveRLN = 0;
            state = MOVE_CAR;
            pro.unlock();
            //return 0;
            return;
        }
		//drawContours(orange, contoursOrange, i, cv::Scalar(255, 255, 255), CV_FILLED, 8, hierarchyOrange);
    }

    if((contoursPink.size() && contoursPurple.size() && contoursGreen.size() && contoursOrange.size()) == 0)
    {
        //return 2;
        moveRLN = 2;
        state = MOVE_CAR;
        pro.unlock();
        return;
    }
    //state++;
    }
}

int CCar::kbhit(void)
{
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if(ch != EOF)
  {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

void CCar::delay(double msec) // got from stackoverflow
{
	double elapsed_time;
	double freq = cv::getTickFrequency(); // Get tick frequency

	double start_tic = cv::getTickCount(); // Get number of ticks since event (such as computer on)
	double curr_tic;

	do
	{
        curr_tic = cv::getTickCount();
        elapsed_time = (curr_tic - start_tic) / freq * 1000;
	} while ((int)elapsed_time < msec);
}

void CCar::camera()
{
    cam.lock();
    if ( state == GET_IMAGE)
    {
    vid.grab();
    vid.retrieve(rcam);
    crop = rcam(cv::Rect(340, 640, 600, 300));
    cv::imshow("REAL WORLD", crop);
    cv::waitKey(1);
    state = PROCESS_IMAGE;
    }
    cam.unlock();
}

int CCar::manual(char keyPress)
{
    switch(keyPress)
    {
        man.lock();
        case 'w':
            //cout << "FORWARD" << endl;
            gpioWrite(27,0);
            gpioWrite(17,1); //left motor
            gpioWrite(4,1);
            gpioWrite(25,0);
            keyPress = 'z';
            break;
        case 's':
            //cout << "REVERSE" << endl;
            gpioWrite(27,1);
            gpioWrite(17,0);
            gpioWrite(4,0);
            gpioWrite(25,1);
            keyPress = 'z';
        case 'd':
            //cout << "RIGHT" << endl;
            gpioWrite(27,0);
            gpioWrite(17,1);
            gpioWrite(4,0);
            gpioWrite(25,1);
            keyPress = 'z';
            break;
        case 'a':
            //cout << "LEFT" << endl;
            gpioWrite(27,1);
            gpioWrite(17,0);
            gpioWrite(4,1);
            gpioWrite(25,0);
            keyPress = 'z';
            break;
        case 'z':
            gpioWrite(27,0);
            gpioWrite(17,0);
            gpioWrite(4,0);
            gpioWrite(25,0);
            break;
    }
    man.unlock();
}
