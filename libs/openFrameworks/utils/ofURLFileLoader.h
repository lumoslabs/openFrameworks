#pragma once

#include <deque>
#include <queue>

#include "ofThread.h"
#include "ofEvents.h"
#include "ofFileUtils.h"

#include "Poco/Condition.h"


class ofHttpRequest{
public:
	ofHttpRequest(){};

	ofHttpRequest(string seturl,string setname,bool setsaveTo=false)
	:url(seturl)
	,name(setname)
	,saveTo(setsaveTo)
	,id(nextID++){}

	string				url;
	string				name;
	bool				saveTo;

	int getID(){return id;}
private:
	int					id;
	static int			nextID;
};

class ofHttpResponse{
public:
	ofHttpResponse(){}

	ofHttpResponse(ofHttpRequest setrequest,const ofBuffer & setdata,int setstatus, string seterror)
	:request(setrequest)
	,data(setdata)
	,status(setstatus)
	,error(seterror){}

	ofHttpResponse(ofHttpRequest setrequest,int setstatus,string seterror)
	:request(setrequest)
	,status(setstatus)
	,error(seterror){}

	operator ofBuffer&(){
		return data;
	}

	ofHttpRequest	    request;
	ofBuffer		    data;
	int					status;
	string				error;
};

ofHttpResponse ofLoadURL(string url);
int ofLoadURLAsync(string url, string name=""); // returns id
ofHttpResponse ofSaveURLTo(string url, string path);
int ofSaveURLAsync(string url, string path);
void ofRemoveURLRequest(int id);
void ofRemoveAllURLRequests();

extern ofEvent<ofHttpResponse> ofURLResponseEvent;

template<class T>
void ofRegisterURLNotification(T * obj){
	ofAddListener(ofURLResponseEvent,obj,&T::urlResponse);
}

template<class T>
void ofUnregisterURLNotification(T * obj){
	ofRemoveListener(ofURLResponseEvent,obj,&T::urlResponse);
}


class ofURLFileLoader : public ofThread  {

    public:

        ofURLFileLoader();
        ofHttpResponse get(string url);
        int getAsync(string url, string name=""); // returns id
        ofHttpResponse saveTo(string url, string path);
        int saveAsync(string url, string path);
		void remove(int id);
		void clear();

    protected:

		// threading -----------------------------------------------
		void threadedFunction();
        void start();
        void stop();
        void update(ofEventArgs & args);  // notify in update so the notification is thread safe

    private:

		// perform the requests on the thread
        ofHttpResponse handleRequest(ofHttpRequest request);

		deque<ofHttpRequest> requests;
		queue<ofHttpResponse> responses;

		Poco::Condition condition;

};
