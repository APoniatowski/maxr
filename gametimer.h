#ifndef SIMSPEEDCTRL_H
#define SIMSPEEDCTRL_H


#include "clist.h"
#include "player.h"

class cNetMessage;

#define GAME_TICK_TIME 10
#define MAX_CLIENT_LAG 30
#define PAUSE_GAME_TIMEOUT 200
#define MAX_SERVER_EVENT_COUNTER 15
#define MAX_WAITING_FOR_SERVER 10

//TODO: comments
class cGameTimer
{
	friend class cDebugOutput;
protected:
	static Uint32 gameTimerCallback (Uint32 interval, void* arg);
	
	cGameTimer ();

	/** SDL_Timer that controls the game time */
	SDL_TimerID timerID;
	cMutex mutex;
	int eventCounter;
	unsigned int lastTimerCall;
	cList<unsigned int> receivedTime;

	void timerCallback ();
	void handleTimer ();
	bool popEvent ();

public:
	~cGameTimer ();

	int maxEventQueueSize;

	unsigned int gameTime;

	bool timer10ms;
	bool timer50ms;
	bool timer100ms;
	bool timer400ms;

	void setReceivedTime (unsigned int time, unsigned int nr = 0);
	unsigned int getReceivedTime (unsigned int nr = 0);

	void start ();
	void stop ();

	static bool syncDebugSingleStep;
};

class cGameTimerServer : public cGameTimer
{
	friend class cDebugOutput;
private:
	int waitingForPlayer;
	
	bool nextTickAllowed ();
public:
	cGameTimerServer ();

	void run ();
	void handleSyncMessage (cNetMessage& message);

};

class cGameTimerClient : public cGameTimer
{
	friend class cDebugOutput;
private:
	unsigned int remoteChecksum;
	unsigned int localChecksum;
	unsigned int waitingForServer;
	unsigned int debugRemoteChecksum;	//saved data for debug view only
	int gameTimeAdjustment;				//saved data for debug view only

	bool nextTickAllowed ();
public:
	bool nextMsgIsNextGameTime;
	cGameTimerClient ();

	void run ();
	void handleSyncMessage (cNetMessage& message);

};

Uint32 calcClientChecksum();
Uint32 calcServerChecksum(cPlayer* player);

void compareGameData();

#endif
