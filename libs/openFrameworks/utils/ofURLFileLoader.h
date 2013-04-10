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

	ofHttpRequest(string setUrl,string setName,bool setSaveTo=false)
	:url(setUrl)
	,name(setName)
	,saveTo(setSaveTo)
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

	ofHttpResponse(ofHttpRequest setRequest,const ofBuffer & setData,int setStatus, string setError)
	:request(setRequest)
	,data(setData)
	,status(setStatus)
	,error(setError){}

	ofHttpResponse(ofHttpRequest setRequest,int setStatus,string setError)
	:request(setRequest)
	,status(setStatus)
	,error(setError){}

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

ofEvent<ofHttpResponse> & ofURLResponseEvent();

template<class T>
void ofRegisterURLNotification(T * obj){
	ofAddListener(ofURLResponseEvent(),obj,&T::urlResponse);
}

template<class T>
void ofUnregisterURLNotification(T * obj){
	ofRemoveListener(ofURLResponseEvent(),obj,&T::urlResponse);
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
