/*
 *  ofxiPhoneApp.h
 *  MSA Labs Vol. 1
 *
 *  Created by Mehmet Akten on 14/07/2009.
 *  Copyright 2009 MSA Visuals Ltd.. All rights reserved.
 *
 */

#pragma once

#include "ofMain.h"
#include "ofEvents.h"
#include "ofxiPhoneAlerts.h"
#include "ofxMultiTouch.h"

class ofxiPhoneApp : public ofSimpleApp, public ofxiPhoneAlertsListener, public ofxMultiTouchListener {
	
public:
	virtual void setup() {};
	virtual void update() {};
	virtual void draw() {};
	virtual void exit() {};

	virtual void touchDown(int x, int y, int id) {};
	virtual void touchMoved(int x, int y, int id) {};
	virtual void touchUp(int x, int y, int id) {};
	virtual void touchDoubleTap(int x, int y, int id) {};
	virtual void touchCancelled(int x, int y, int id) {};
	
	virtual void lostFocus() {}
	virtual void gotFocus() {}
	virtual void gotMemoryWarning() {}

	virtual void touchDown(ofTouchEventArgs & touch) {
		touchDown((int)touch.x, (int)touch.y, touch.id);
	};
	virtual void touchMoved(ofTouchEventArgs & touch) {
		touchMoved((int)touch.x, (int)touch.y, touch.id);
	};
	virtual void touchUp(ofTouchEventArgs & touch) {
		touchUp((int)touch.x, (int)touch.y, touch.id);
	};
	virtual void touchDoubleTap(ofTouchEventArgs & touch) {
		touchDoubleTap((int)touch.x, (int)touch.y, touch.id);
	};
	virtual void touchCancelled(ofTouchEventArgs & touch) {
		touchCancelled((int)touch.x, (int)touch.y, touch.id);
	};

};

