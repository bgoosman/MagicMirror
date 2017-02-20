#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSeedRandom();

    m_videoFileName = "gargle";

    m_gui.setup("Parameters", "settings.xml");
    m_gui.add(m_delayMilliseconds.setup("delay", m_defaultDelayMilliseconds, 0, m_maxDelayMillseconds));
    m_gui.add(m_loopMilliseconds.setup("loop", m_defaultLoopMilliseconds, 0, m_maxLoopMilliseconds));
    m_gui.add(m_seekMilliseconds.setup("seek backwards", m_defaultSeekMilliseconds, -m_maxSeekMillseconds, m_maxSeekMillseconds));
    m_gui.add(m_recordMilliseconds.setup("record", m_defaultRecordMilliseconds, 0, m_maxRecordMilliseconds * 2));
    m_recordMilliseconds.addListener(this, &ofApp::recordMillisecondsChanged);
    m_gui.add(m_recordIndexFollowsBufferIndex.setup("record/buffer sync", false));
    m_gui.add(m_randomizeButton.setup("randomize"));
    m_randomizeButton.addListener(this, &ofApp::randomizeButtonClicked);
    m_gui.add(m_isRecording.setup("is recording", false));
    m_gui.loadFromFile("settings.xml");

    // setup video
    std::vector<ofVideoDevice> cameraList = m_grabber.listDevices();
    int preferredDeviceId = 0;
    for (int i = 0; i < cameraList.size(); i++)
    {
        auto camera = cameraList[i];
        if (camera.deviceName.find("Kinect") != string::npos)
        {
            preferredDeviceId = i;
            camera.deviceName += '\0';
            printf("Selecting camera device id=%d\n", i);
        }
    }
    m_grabber.setDeviceID(preferredDeviceId);
    m_grabber.setDesiredFrameRate(m_framesPerSecond);
    m_grabber.initGrabber(m_frameWidth, m_frameHeight);
    m_bufferSize = (int)(((double)(m_defaultRecordMilliseconds) / 1000.0f) * m_framesPerSecond);
    m_frames.resize(m_bufferSize);
    printf("Allocated %d frames\n", m_bufferSize);

    // setup audio
    std::vector<ofSoundDevice> soundDeviceList = m_soundStream.listDevices();
    preferredDeviceId = 0;
    for (int i = 0; i < soundDeviceList.size(); i++)
    {
        auto soundDevice = soundDeviceList[i];
        if (soundDevice.name.find("2-") != string::npos)
        {
            preferredDeviceId = i;
            printf("Selecting sound device id=%d\n", i);
        }
    }
    m_soundStream.setDeviceID(preferredDeviceId);
    m_soundStream.setup(this, 0, 1, 44100, 512, 4);
}

//--------------------------------------------------------------
void ofApp::update()
{
    static int m_framesTraveled = 0;
    m_grabber.update();

    // delete the previous frame stored at this location
    if (m_frames[m_recordIndex] != nullptr)
    {
        delete m_frames[m_recordIndex];
    }

    // save the pixels
    ofPixels* pixels = new ofPixels(m_grabber.getPixels()); // make a copy
    m_frames[m_recordIndex] = pixels;

    // pick a frame to render
    size_t renderIndex = computeDelayIndex(m_bufferIndex, m_bufferSize, m_framesPerSecond, m_delayMilliseconds);
    if (m_frames[renderIndex] != nullptr)
    {
        m_renderFrame.loadData(*m_frames[renderIndex]);
    }

    // save frame to recording if we are recording and we are not paused
    if (m_videoRecorder.isRecording() && !m_videoRecorder.isPaused())
    {
        if (!m_videoRecorder.addFrame(*m_frames[renderIndex]))
        {
            ofLogWarning("This frame was not added!");
        }
    }

    // check if the video recorder encountered any error while writing video frame or audio smaples.
    if (m_videoRecorder.hasVideoError())
    {
        ofLogWarning("The video recorder failed to write some frames!");
    }

    if (m_videoRecorder.hasAudioError())
    {
        ofLogWarning("The video recorder failed to write some audio samples!");
    }

    m_bufferIndex += m_playDirection;
    if (m_bufferIndex == m_bufferSize)
    {
        m_bufferIndex = 0;
    }
    else if (m_bufferIndex < 0)
    {
        m_bufferIndex = m_bufferSize - 1;
    }

    m_recordIndex += 1;
    if (m_recordIndex == m_bufferSize)
    {
        m_recordIndex = 0;
    }
    else if (m_recordIndex < 0)
    {
        m_recordIndex = m_bufferSize - 1;
    }

    if (++m_realTimeFrameIndex == m_bufferSize)
    {
        m_realTimeFrameIndex = 0;
    }

    if (m_loudSoundDetected)
    {
        //m_playDirection *= -1;
        //m_bufferIndex = m_realTimeFrameIndex;
        //m_recordIndex = m_realTimeFrameIndex;
        //m_framesTraveled = 0;
    }

    // jump back in time one delay interval when we travel past one delay interval
    int const loopInFrames = millisecondsToFrames(m_loopMilliseconds, m_framesPerSecond);
    int const seekInFrames = millisecondsToFrames(m_seekMilliseconds, m_framesPerSecond);
    if (++m_framesTraveled >= loopInFrames)
    {
        m_loudSoundDetected = false;
        ofLogNotice("MagicMirror", "Traveled %d frames\n", m_framesTraveled);
        m_framesTraveled = 0;
        int const seekIndex = findNextNumberInRange(m_bufferIndex, seekInFrames, 0, m_bufferSize - 1);
        printf("Seeking from frame %d to %d (%d total frames)\n", m_bufferIndex, seekIndex, seekInFrames);
        m_bufferIndex = seekIndex;
        if (m_recordIndexFollowsBufferIndex)
        {
            m_recordIndex = m_bufferIndex;
        }
    }
}

//--------------------------------------------------------------
void ofApp::draw()
{
    m_renderFrame.draw(0, 0, ofGetWidth(), ofGetHeight());
    m_gui.draw();
}

int ofApp::findNextNumberInRange(int currentNumber, int steps, int rangeStart, int rangeEnd)
{
    int const rangeWidth = rangeEnd - rangeStart + 1;

    // loop around the range steps / rangeWidth times
    steps %= abs(rangeWidth);

    // add steps to current number and wrap around if we exit the range
    int nextNumber = currentNumber + steps;
    if (nextNumber < rangeStart)
    {
        nextNumber = rangeEnd + nextNumber;
    }
    else if (nextNumber > rangeEnd)
    {
        nextNumber = rangeStart + (nextNumber - rangeEnd);
    }

    return nextNumber;
}

int ofApp::computeDelayIndex(size_t currentIndex, size_t bufferSize, int framesPerSecond, int delayMilliseconds)
{
    double const millisecondsPerFrame = 1 / ((double)framesPerSecond / 1000.0);
    size_t delayIndex = currentIndex;
    double remaininingDelayMilliseconds = (double)delayMilliseconds;
    while (remaininingDelayMilliseconds > 0.0)
    {
        if (delayIndex == 0)
        {
            delayIndex = bufferSize - 1;
        }
        else
        {
            delayIndex--;
        }

        remaininingDelayMilliseconds -= millisecondsPerFrame;
    }

    return delayIndex;
}

int ofApp::millisecondsToFrames(double delayMilliseconds, double frameRate)
{
    return (int)floor(delayMilliseconds * (frameRate / 1000.0));
}

void ofApp::audioReceived(float *input, int bufferSize, int nChannels)
{
    float averageInput = 0.0;
    float smallestInput = 1.0;
    float largestInput = 0.0;
    for (int i = 0; i < bufferSize; i++)
    {
        averageInput += input[i];
        if (input[i] > largestInput)
        {
            largestInput = input[i];
        }
        if (input[i] < smallestInput)
        {
            smallestInput = input[i];
        }
    }
    m_averageSound = (largestInput + smallestInput) / 2;
    m_quietestSound = smallestInput;
    m_loudestSound = largestInput;
}

void ofApp::recordMillisecondsChanged(int& value)
{
    m_frames.resize(value);
}

void ofApp::randomizeButtonClicked()
{
    m_loopMilliseconds = (int)ofRandom(0, m_maxLoopMilliseconds);
    m_seekMilliseconds = (int)ofRandom(0, m_maxSeekMillseconds);
    m_delayMilliseconds = (int)ofRandom(0, m_maxDelayMillseconds);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    if (key == ' ')
    {
        // remove window border
        HWND hwnd = WindowFromDC(wglGetCurrentDC());
        LONG lStyle = GetWindowLong(hwnd, GWL_STYLE);
        lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
        SetWindowLong(hwnd, GWL_STYLE, lStyle);
    }
    else if (key == 'r')
    {
        // start recording video or toggle pausing the recording
        if (!m_videoRecorder.isInitialized())
        {
            m_videoRecorder.setup(m_videoFileName + ofGetTimestampString(), m_frameWidth, m_frameHeight, m_framesPerSecond);
            m_videoRecorder.start();
        }
        else
        {
            m_videoRecorder.setPaused(!m_videoRecorder.isPaused());
        }

        m_isRecording = m_videoRecorder.isRecording() && !m_videoRecorder.isPaused();
    }
    else if (key == 'c')
    {
        // stop the recording video
        m_isRecording = false;
        m_videoRecorder.close();
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{

}
