#pragma once

#include "ofMain.h"
#include "ofxVideoRecorder.h"
#include "ofxGui.h"

class ofApp : public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    int computeDelayIndex(size_t currentIndex, size_t bufferSize, int framesPerSecond, int delayMilliseconds);
    int findNextNumberInRange(int currentNumber, int steps, int rangeStart, int rangeEnd);
    int millisecondsToFrames(double delayMilliseconds, double frameRate);
    void audioReceived(float* input, int bufferSize, int nChannels);
    void recordMillisecondsChanged(int& value);
    void randomizeButtonClicked();

    ofFbo m_fbo;
    ofSoundStream m_soundStream;
    ofTexture m_renderFrame;
    ofVideoGrabber m_grabber;
    vector<ofPixels*> m_frames;
    bool m_loudSoundDetected = false;
    int m_realTimeFrameIndex = 0;
    int m_bufferSize;
    int m_bufferIndex = 0;
    int m_recordIndex = 0;
    int m_delayFrames;
    int m_frameWidth = 1920;
    int m_frameHeight = 1080;
    int m_defaultDelayMilliseconds = 0;
    int m_defaultLoopMilliseconds = 10000;
    int m_defaultSeekMilliseconds = 0;
    int m_defaultRecordMilliseconds = 15000;
    int m_maxRecordMilliseconds = 15000;
    int m_maxDelayMillseconds = m_maxRecordMilliseconds * 2;
    int m_maxLoopMilliseconds = m_maxRecordMilliseconds * 2;
    int m_maxSeekMillseconds = m_maxRecordMilliseconds * 2;
    int m_playDirection = 1;
    double m_framesPerSecond = 30;
    ofxGuiGroup m_time;
    ofxButton m_randomizeButton;
    ofxButton m_backToRealTimeButton;
    ofxIntSlider m_delayMilliseconds;
    ofxIntSlider m_loopMilliseconds;
    ofxIntSlider m_recordMilliseconds;
    ofxIntSlider m_seekMilliseconds;
    ofxToggle m_recordIndexFollowsBufferIndex;
    ofxToggle m_isRecording;
    ofxPanel m_gui;
    ofxVideoRecorder m_videoRecorder;
};
