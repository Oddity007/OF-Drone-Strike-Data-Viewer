#include "ofApp.h"
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <time.h>

//--------------------------------------------------------------
void ofApp::setup(){
	ofEnableAlphaBlending();
	ofEnableArbTex();
	earthImage.load("worldmap_equirectangular.png");

	audioContext = AEAudioContextNew();
	explosionSoundHandle = AEAudioContextBufferLoad(audioContext, ofToDataPath("108640__juskiddink__distant-explosion.ogg").c_str());
	std::cout << explosionSoundHandle << std::endl;
	
	timeUntilNextDatapoint = secondsPerDatapoint;
	currentDatapointIndex = 0;
	textBlock.init("PT_Serif-Web-Regular.ttf", 12);
	theme.layout.textInput.forceUpperCase = false;
	theme.font.file = "PT_Serif-Web-Regular.ttf";
	theme.font.size = 12;
	//label.setPosition(0, 400);
	//label.setWidth(ofGetWidth(), 1);
	gui.setTheme(& theme);
	input.setTheme(& theme);
	input.onTextInputEvent(this, & ofApp::onTextInputEvent);
	input.setPosition(0, 0);
	input.setWidth(ofGetWidth(), .2);
	font.load("PT_Serif-Web-Regular.ttf", 24);
	ofxJSONElement root;
	if (not root.open("drone-strikes-data.json")) abort();
	if (not root.isObject()) abort();
	const ofxJSONElement& strikes = root["strike"];
	if (not strikes.isArray()) abort();
	if (strikes.empty()) abort();
	//for (Json::ValueConstIterator strikeIterator = strikes.begin(); strikeIterator not_eq strikes.end(); strikeIterator++)
	assert(strikes.size() > 0);
	viewerPosition.latitude = 0;
	viewerPosition.longitude = 0;
	double minStartTime = 0;
	double maxStartTime = 0;
	for (Json::ArrayIndex i = 0; i < strikes.size(); i++)
	{
		//100% robust.  I assure you.
		//const ofxJSONElement& strike = *strikeIterator;
		const ofxJSONElement& strike = strikes[i];
		if (not strike.isObject()) abort();
		Datapoint datapoint;
		const ofxJSONElement& latitude = strike["lat"];
		if (not latitude.isString()) abort();
		std::cout << latitude.getRawString() << std::endl;
		datapoint.latitude = atof(latitude.getRawString().c_str() + 1);
		assert(latitude.getRawString().length() > 0);
		const ofxJSONElement& longitude = strike["lon"];
		if (not longitude.isString()) abort();
		assert(longitude.getRawString().length() > 0);
		datapoint.longitude = atof(longitude.getRawString().c_str() + 1);
		const ofxJSONElement& narrative = strike["narrative"];
		if (not narrative.isString()) abort();
		datapoint.narrative = narrative.getRawString();
		const ofxJSONElement& date = strike["date"];
		if (not date.isString()) abort();
		{
			//std::stringstream stream;
			//stream << date;
			struct tm t;
			t.tm_sec = 0;
			t.tm_min = 0;
			t.tm_hour = 0;
			t.tm_mday = atoi(date.getRawString().c_str() + 9);
			datapoint.day = t.tm_mday;
			t.tm_mon = atoi(date.getRawString().c_str() + 6);
			datapoint.month = t.tm_mon;
			t.tm_year = atoi(date.getRawString().c_str() + 1);
			datapoint.year = t.tm_year;
			t.tm_wday = 0;
			t.tm_yday = 0;
			//stream >> "\"" >> t.tm_year >> "-" >> t.tm_mon >> "-" >> t.tm_mday;
			
			std::cout << date.getRawString() << std::endl;
			std::cout << "Year " << t.tm_year << " Month " << t.tm_mon << " Day " << t.tm_mday << std::endl;
			t.tm_year -= 1900;
			datapoint.time = timegm(& t);
			
			SoundEvent soundEvent;
			soundEvent.latitude = datapoint.latitude;
			soundEvent.longitude = datapoint.longitude;
			soundEvent.startTime = (datapoint.year - 2000) * (30.0 * 12.0) + datapoint.month * 30.0 + datapoint.day;
			std::cout << soundEvent.startTime << std::endl;
			if (soundEvents.size() > 0)
			{
				minStartTime = std::min(minStartTime, soundEvent.startTime);
				maxStartTime = std::max(maxStartTime, soundEvent.startTime);
			}
			else
			{
				minStartTime = soundEvent.startTime;
				maxStartTime = soundEvent.startTime;
			}
			soundEvents.push_back(soundEvent);
			datapoint.date = date.getRawString();
		}
		datapoints.push_back(datapoint);
		std::cout << "Lat: " << datapoint.latitude << ", Lon: " << datapoint.longitude << ", Narrative: " << datapoint.narrative << std::endl;
		
		viewerPosition.latitude += datapoint.latitude * (1.0 / strikes.size());
		viewerPosition.longitude += datapoint.longitude * (1.0 / strikes.size());
	}
	assert (datapoints.size() > 0);
	std::cout << viewerPosition.latitude << ", " << viewerPosition.longitude << std::endl;
	
	
	std::sort(soundEvents.begin(), soundEvents.end(), [](const SoundEvent& e1, const SoundEvent& e2){return e1.startTime < e2.startTime;});
	
	assert(maxStartTime > minStartTime);
	for (size_t i = 0; i < soundEvents.size(); i++)
		soundEvents[i].startTime = ((soundEvents[i].startTime - minStartTime) / (maxStartTime - minStartTime)) * soundEventTimespanSeconds;
	
	AEAudioContextSetPosition(audioContext, viewerPosition.latitude, 0, viewerPosition.longitude);
	
	
	ofxJSONElement countryRoot;
	if (not countryRoot.open("world-countries.json")) abort();
	if (not countryRoot.isArray()) abort();
	for (Json::ArrayIndex i = 0; i < countryRoot.size(); i++)
	{
		Country country;
		const ofxJSONElement& features = countryRoot[i]["features"];
		for (Json::ArrayIndex h = 0; h < features.size(); h++)
		{
			const ofxJSONElement& coordinates = features[h]["geometry"]["coordinates"];
			for (Json::ArrayIndex j = 0; j < coordinates.size(); j++)
			{
				const ofxJSONElement& as = coordinates[i];
				for (Json::ArrayIndex k = 0; k < as.size(); k++)
				{
					const ofxJSONElement& bs = as[i];
					for (Json::ArrayIndex l = 0; l < bs.size(); l++)
					{
						const ofxJSONElement& cs = bs[l];
						Country::Point point;
						point.longitude = cs[0].asDouble();
						point.latitude = cs[1].asDouble();
						country.points.push_back(point);
					}
					
				}
			}
		}
		countries.push_back(country);
	}
	
	/*ofxJSONElement country2Root;
	if (not country2Root.open("world-countries2.json")) abort();
	if (not country2Root.isArray()) abort();
	for (Json::ArrayIndex i = 0; i < country2Root.size(); i++)
	{
		Country country;
		const ofxJSONElement& features = country2Root[i]["features"];
		for (Json::ArrayIndex h = 0; h < features.size(); h++)
		{
			const ofxJSONElement& coordinates = features[h]["geometry"]["coordinates"];
			for (Json::ArrayIndex j = 0; j < coordinates.size(); j++)
			{
				const ofxJSONElement& as = coordinates[i];
				for (Json::ArrayIndex k = 0; k < as.size(); k++)
				{
					const ofxJSONElement& bs = as[i];
					for (Json::ArrayIndex l = 0; l < bs.size(); l++)
					{
						const ofxJSONElement& cs = bs[l];
						Country::Point point;
						point.latitude = cs[0].asDouble();
						point.longitude = cs[1].asDouble();
						country.points.push_back(point);
					}
					
				}
			}
		}
		countries.push_back(country);
	}*/
}

static std::string FilterText(const std::string& input, const std::set<std::string>& allowedWords)
{
	struct Range
	{
		enum class Type
		{
			InProgress,
			Nonword,
			Word,
		};
		
		const char* start;
		const char* end;
		Type type;
		
		Range() :
			start(0),
			end(0),
			type(Type::InProgress)
		{}
	};
	
	
	const char* s = input.c_str();
	
	std::vector<Range> ranges;
	
	//ranges.push_back(Range());
	
	Range currentRange;
	
	currentRange.start = s;
	
	while (*s)
	{
		Range::Type type = Range::Type::Nonword;
	
		if (isalnum(*s))
			type = Range::Type::Word;
		else
			type = Range::Type::Nonword;
		
		if (type not_eq currentRange.type)
		{
			currentRange.end = s;
			if (currentRange.start not_eq currentRange.end)
				ranges.push_back(currentRange);
			currentRange = Range();
			currentRange.type = type;
			currentRange.start = s;
		}
		
		s++;
	}
	
	currentRange.end = s;
	ranges.push_back(currentRange);
	
	std::stringstream stream;
	
	for (size_t i = 0; i < ranges.size(); i++)
	{
		const Range& range = ranges[i];
		std::string string(range.start, range.end);
		
		if (range.type == Range::Type::Nonword)
		{
			stream << string;
		}
		else if (range.type == Range::Type::Word)
		{
			if (allowedWords.find(string) not_eq allowedWords.end())
				stream << string;
			else for (size_t j = 0; j < string.length(); j++)
			{
				stream << '_';
			}
		}
		else abort();
	}
	
	return stream.str();
}

//--------------------------------------------------------------
void ofApp::update(){
	AEAudioContextUpdateStreams(audioContext);
	
	{
		size_t i = 0;
		std::vector<AEAudioHandle> remainingHandles;
		for (; i < explosionSourceHandles.size();)
		{
			if (AEAudioContextSourceGetStopped(audioContext, explosionSourceHandles[i]))
			{
				AEAudioContextSourceDelete(audioContext, explosionSourceHandles[i]);
				std::swap(explosionSourceHandles[i], explosionSourceHandles[explosionSourceHandles.size() - 1]);
				explosionSourceHandles.pop_back();
				std::cout << "Removed source" << std::endl;
			}
			else
			{
				//std::cout << "There is a source that's still playing" << std::endl;
				i++;
			}
		}
	}
	
	input.update();
	if (revealedWords.find(input.getText()) == revealedWords.cend())
		revealedWords.insert(input.getText());
	
	std::cout << "ofGetFrameRate(): " << ofGetFrameRate() << std::endl;
	
	if (ofGetFrameRate() > 1)
		currentSoundEventTime += 1.0 / ofGetFrameRate();
	std::cout << "currentSoundEventTime: " << currentSoundEventTime << std::endl;
	std::cout << "soundEvents[" << nextSoundEventIndex << "].startTime: " << soundEvents[nextSoundEventIndex].startTime << std::endl;
	
	while (currentSoundEventTime > soundEventTimespanSeconds)
	{
		currentSoundEventTime -= soundEventTimespanSeconds;
		nextSoundEventIndex = 0;
	}
	
	assert(nextSoundEventIndex < soundEvents.size());
	
	while (currentSoundEventTime > soundEvents[nextSoundEventIndex].startTime)
	{
		std::cout << "currentSoundEventTime: " << currentSoundEventTime << std::endl;
		std::cout << "soundEvents[" << nextSoundEventIndex << "].startTime: " << soundEvents[nextSoundEventIndex].startTime << std::endl;
		std::cout << "nextSoundEventIndex: " << nextSoundEventIndex << std::endl;
		AEAudioHandle source = AEAudioContextSourceNew(audioContext, explosionSoundHandle);
		//AEAudioContextSourceSetPosition(audioContext, source, datapoints[currentDatapointIndex].latitude, 0, datapoints[currentDatapointIndex].longitude);
		
		double lat = soundEvents[nextSoundEventIndex].latitude;
		double lon = soundEvents[nextSoundEventIndex].longitude;
		
		//Do a bit of scaling to get a nicer attenuation/spread.
		lat -= viewerPosition.latitude;
		lon -= viewerPosition.longitude;
		
		lat *= 50;
		lon *= 50;
	
		lat += viewerPosition.latitude;
		lon += viewerPosition.longitude;
		
		
		AEAudioContextSourceSetPosition(audioContext, source, lat, 0, lon);
		
		//AEAudioContextSourceSetPosition(audioContext, source, viewerPosition.latitude, 0, viewerPosition.longitude);
		
		AEAudioContextSourceSetVolume(audioContext, source, 1);
		explosionSourceHandles.push_back(source);
		
		nextSoundEventIndex++;
		if (nextSoundEventIndex >= soundEvents.size())
		{
			currentSoundEventTime -= soundEventTimespanSeconds;
			nextSoundEventIndex = 0;
		}
	}
	
	timeUntilNextDatapoint -= 1.0 / ofGetFrameRate();
	if (timeUntilNextDatapoint <= 0)
	{
		timeUntilNextDatapoint = secondsPerDatapoint;
		currentDatapointIndex++;
		currentDatapointIndex %= datapoints.size();
		
		displayedString = FilterText(datapoints[currentDatapointIndex].narrative, revealedWords);
		textBlock.setText(displayedString);
		textBlock.wrapTextX(ofGetWidth()/2);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetBackgroundColor(255, 255, 255);
	
	if (plotIsVisible)
	{
		const double radius = 300;
		const double markerSize = 25;
		const double borderMarkerSize = 1;
		
		//earthImage.draw(0, 0);
		
		assert(earthImage.isUsingTexture());

		ofSetLineWidth(1.0);
		
		ofEnableDepthTest();
		//ofNoFill();
		ofPushMatrix();
		ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
		ofRotate(-15, 1, 0, 0);
		//ofRotate(360 * (0.05 - 0.25), 0, 1, 0);
		ofRotate(360 * (currentSoundEventTime /soundEventTimespanSeconds - 0.25), 0, 1, 0);
		ofSetColor(255, 255, 255, 255);
		//earthImage.bind();
		//earthImage.getTextureReference().bind();
		ofDrawSphere(0, 0, 0, radius);
		//earthImage.unbind();
		//earthImage.getTextureReference().unbind();
		ofFill();
		
		
		ofSetColor(128, 0, 0, 255);
		ofSetLineWidth(1.0);
		for (size_t i = 0; i < datapoints.size(); i++)
		{
			ofQuaternion latitudeRotation, longitudeRotation;
			latitudeRotation.makeRotate(datapoints[i].latitude, 1, 0, 0);
			longitudeRotation.makeRotate(datapoints[i].longitude, 0, 1, 0);
			ofVec3f inside = ofVec3f(0, 0, radius - markerSize);
			ofVec3f outside = ofVec3f(0, 0, markerSize + radius);
			ofDrawLine(latitudeRotation * longitudeRotation * inside, latitudeRotation * longitudeRotation * outside);
		}
		
		ofSetColor(0, 0, 0, 255);
		ofSetLineWidth(1.0);
		for (size_t j = 0; j < countries.size(); j++)
		for (size_t i = 0; i < countries[j].points.size(); i++)
		{
			ofQuaternion latitudeRotation, longitudeRotation;
			latitudeRotation.makeRotate(countries[j].points[i].latitude, 1, 0, 0);
			longitudeRotation.makeRotate(countries[j].points[i].longitude, 0, 1, 0);
			ofVec3f inside = ofVec3f(0, 0, radius - borderMarkerSize);
			ofVec3f outside = ofVec3f(0, 0, borderMarkerSize + radius);
			ofDrawLine(latitudeRotation * longitudeRotation * inside, latitudeRotation * longitudeRotation * outside);
		}
		
		ofPopMatrix();
		ofDisableDepthTest();
	}

	input.draw();
	//assert(currentDatapointIndex < datapoints.size());
	//std::string str = ;//datapoints[currentDatapointIndex].narrative;//"Text Input: " + input.getText();
	
	
	/*std::stringstream stream;
	stream << datapoints[currentDatapointIndex].latitude << ", " << datapoints[currentDatapointIndex].longitude;
	//datapoints[currentDatapointIndex].date;// <<
	ofRectangle bounds = font.getStringBoundingBox(stream.str(), ofGetWidth()/2, ofGetHeight()/3);
	ofSetColor(ofColor::black);
	font.drawString(stream.str(), bounds.x - bounds.width/2, bounds.y - bounds.height/2);*/
	
	ofSetColor(0, 0, 0, 255);
	
	textBlock.drawCenter(ofGetWidth()/2, ofGetHeight() / 2);
	
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == OF_KEY_ALT)
	{
		plotIsVisible = not plotIsVisible;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


void ofApp::onTextInputEvent(ofxDatGuiTextInputEvent e)
{
	
}
