#pragma once

#include "ofMain.h"
#include "ofxJSON.h"
#include "ofxDatGui.h"
#include "ofxTextSuite.h"

#include <vector>

#include "AEAudio.h"
#include <set>

class ofApp : public ofBaseApp{

	struct Datapoint
	{
		double
			latitude,
			longitude;
		std::string narrative;
		std::vector<std::string> victims;
		std::string info;
		time_t time;
		int year, month, day;
		std::string date;
		
		/*Datapoint(void) :
			latitude(0),
			longitude(0),
			name(""),
			info(""),
		{
			
		}*/
	};
	
	struct Country
	{
		struct Point
		{
			double
				latitude,
				longitude;
		};
		std::vector<Point> points;
	};
	
	std::vector<Country>  countries;
	
	struct SoundEvent
	{
		double
			startTime,
			latitude,
			longitude;
	};
	
	ofxDatGui gui;
	
	ofTrueTypeFont font;
	ofxDatGuiTextInput input;
	//ofxDatGuiLabel label;
	
	ofxTextBlock textBlock;
	
	AEAudioContext* audioContext = NULL;
	AEAudioHandle explosionSoundHandle = 0;
	std::vector<AEAudioHandle> explosionSourceHandles;
	std::vector<SoundEvent> soundEvents;
	size_t nextSoundEventIndex = 0;
	const double soundEventTimespanSeconds = 5.0 * 60.0;
	double currentSoundEventTime = 0;
	
	std::set<std::string> revealedWords;
	
	std::string displayedString;
	
	ofImage earthImage;
	
	struct
	{
		double
			latitude = 0,
			longitude = 0;
	}viewerPosition;
	
	class Theme : public ofxDatGuiThemeSmoke
	{
	};
	
	Theme theme;
	
	std::vector<Datapoint> datapoints;
	size_t currentDatapointIndex;
	double
		timeUntilNextDatapoint,
		secondsPerDatapoint = 2;
	
	void onTextInputEvent(ofxDatGuiTextInputEvent e);
	
	bool plotIsVisible = false;


	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
	ofApp(void) :
		input("TEXT INPUT", "Type Something Here")
	{}
};
