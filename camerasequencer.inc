/*==============================================================================


	Southclaw's Camera Sequencer
		v3.0


==============================================================================*/


#include <sscanf2>
#include <YSI\y_iterate>

#if !defined CAMSEQ_DEBUG
	#define CAMSEQ_DEBUG (false)
#endif

#define MAX_CAMERAS					(128)	// Max cameras that can be loaded at once
#define MAX_CAMNODE					(32)	// Max camera nodes that can be loaded for one camera
#define MAX_CAMERA_FILENAME			(64)	// Length for file names in loading and saving
#define INVALID_CAMERA_SEQUENCER_ID	(-1)

#define DIR_CAMERAS					"camseq/"
#define CAMERA_FILE					DIR_CAMERAS"%s.cam"

#define DEFAULT_MOVETIME			(3000)
#define DEFAULT_WAITTIME			(0)
#define DEFAULT_MOVETYPE			(CAMERA_MOVE)

#if CAMSEQ_DEBUG == true
	#define camseq_debug(%0); printf(%0);
#else
	#define camseq_debug(%0);
#endif


/*==============================================================================

	Setup

==============================================================================*/


enum E_CAM_DATA
{
			cam_totalNodes,
			cam_isPaused,
			cam_currentNode,
			cam_isMoving // TODO: make this work
}

enum E_CAM_NODE_DATA
{
Float:		cam_posX,
Float:		cam_posY,
Float:		cam_posZ,
Float:		cam_tarX,
Float:		cam_tarY,
Float:		cam_tarZ,
			cam_moveTime,
			cam_waitTime,
			cam_moveType
}


static
			cam_Data[MAX_CAMERAS][E_CAM_DATA],
			cam_NodeData[MAX_CAMERAS][MAX_CAMNODE][E_CAM_NODE_DATA],
			cam_NodeTimer[MAX_CAMERAS],
Float:		cam_PausePos[MAX_CAMERAS][6],
			cam_PlayerCurrentCamera[MAX_PLAYERS];

new
Iterator:	cam_Index<MAX_CAMERAS>;


forward MoveCameraToNextNode(playerid, bool:tp, bool:freeze, bool:loop);
forward internal_OnCamMove(playerid, bool:tp, bool:freeze, bool:loop);
forward UpdateFreeCam(playerid);

forward OnCameraLeaveNode(playerid, cameraid, currnode);
forward OnCameraLeftNode(playerid, cameraid, currnode);
forward OnCameraReachNode(playerid, cameraid, node);
forward OnCameraReachedNode(playerid, cameraid, node);
forward OnCameraSequenceFinish(playerid, cameraid);


/*==============================================================================

	Core Functions

==============================================================================*/


/*
	Todo:
	CreateCameraSequencer
	CreateEmptyCameraSequencer
	Separate file loaded cameras from hardcoded
*/
stock LoadCameraSequencer(name[])
{
	camseq_debug("[LoadCameraSequencer] Loading '%s'", name);

	new id = Iter_Free(cam_Index);

	if(id == -1)
	{
		printf("[LoadCameraSequencer] ERROR: Camera ID limit reached while loading '%s'.", name);
		return -1;
	}

	new filename[128];

	format(filename, MAX_CAMERA_FILENAME, CAMERA_FILE, name);

	if(!fexist(filename))
	{
		printf("[LoadCameraSequencer] ERROR: Camera data file '%s' not found.", filename);
		return -1;
	}

	new
		File:file,
		line[128],
		idx,
		defaults[3];

	// Default values in case no values are defined in the file
	defaults[0] = DEFAULT_MOVETIME;
	defaults[1] = DEFAULT_WAITTIME;
	defaults[2] = DEFAULT_MOVETYPE;

	file = fopen(filename, io_read);

	while(fread(file, line))
	{
		if(idx == MAX_CAMNODE)
		{
			printf("[LoadCameraSequencer] ERROR: Camera node limit reached in '%s'", name);
			break;
		}

		if (!sscanf(line, "p<,>ffffffD(-1)D(-1)D(-1)",
			cam_NodeData[id][idx][cam_posX], cam_NodeData[id][idx][cam_posY], cam_NodeData[id][idx][cam_posZ],
			cam_NodeData[id][idx][cam_tarX], cam_NodeData[id][idx][cam_tarY], cam_NodeData[id][idx][cam_tarZ],
			cam_NodeData[id][idx][cam_moveTime], cam_NodeData[id][idx][cam_waitTime], cam_NodeData[id][idx][cam_moveType]))
		{
			// If -1, use defaults
			// Also, assign defaults inline
			defaults[0] = cam_NodeData[id][idx][cam_moveTime] = cam_NodeData[id][idx][cam_moveTime] == -1 ? (defaults[0]) : (cam_NodeData[id][idx][cam_moveTime]);
			defaults[1] = cam_NodeData[id][idx][cam_waitTime] = cam_NodeData[id][idx][cam_waitTime] == -1 ? (defaults[1]) : (cam_NodeData[id][idx][cam_waitTime]);
			defaults[2] = cam_NodeData[id][idx][cam_moveType] = cam_NodeData[id][idx][cam_moveType] == -1 ? (defaults[2]) : (cam_NodeData[id][idx][cam_moveType]);

			camseq_debug("[LoadCameraSequencer] Loaded node %d: %.1f, %.1f, %.1f, %.1f, %.1f, %.1f, %d, %d, %d", idx, cam_NodeData[id][idx][cam_posX], cam_NodeData[id][idx][cam_posY], cam_NodeData[id][idx][cam_posZ], cam_NodeData[id][idx][cam_tarX], cam_NodeData[id][idx][cam_tarY], cam_NodeData[id][idx][cam_tarZ], cam_NodeData[id][idx][cam_moveTime], cam_NodeData[id][idx][cam_waitTime], cam_NodeData[id][idx][cam_moveType]);

			idx++;
		}
	}

	if(idx == 0)
	{
		printf("[LoadCameraSequencer] ERROR: Data file for camera '%s' contains no node data.", name);
		return INVALID_CAMERA_SEQUENCER_ID;
	}

	cam_Data[id][cam_totalNodes] = idx;

	camseq_debug("[LoadCameraSequencer] Loaded camera sequencer '%s' with %d nodes.", name, idx);

	fclose(file);

	Iter_Add(cam_Index, id);

	return id;
}

stock DestroyCameraSequencer(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	camseq_debug("[DestroyCameraSequencer] Destroying camera sequencer %d.", cameraid);

	KillTimer(cam_NodeTimer[cameraid]);

	cam_Data[cameraid][cam_totalNodes] = 0;
	cam_Data[cameraid][cam_isPaused] = false;
	cam_Data[cameraid][cam_isMoving] = false;

	cam_PausePos[cameraid][0] = 0.0;
	cam_PausePos[cameraid][1] = 0.0;
	cam_PausePos[cameraid][2] = 0.0;
	cam_PausePos[cameraid][3] = 0.0;
	cam_PausePos[cameraid][4] = 0.0;
	cam_PausePos[cameraid][5] = 0.0;

	for(new i; i <= cam_totalNodes[cameraid]; i++)
	{
		cam_NodeData[cameraid][i][cam_posX] = 0.0;
		cam_NodeData[cameraid][i][cam_posY] = 0.0;
		cam_NodeData[cameraid][i][cam_posZ] = 0.0;
		cam_NodeData[cameraid][i][cam_tarX] = 0.0;
		cam_NodeData[cameraid][i][cam_tarY] = 0.0;
		cam_NodeData[cameraid][i][cam_tarZ] = 0.0;
		cam_NodeData[cameraid][i][cam_moveTime] = 0;
		cam_NodeData[cameraid][i][cam_waitTime] = 0;
		cam_NodeData[cameraid][i][cam_moveType] = 0;
	}

	Iter_Remove(cam_Index, cameraid);

	return 0;
}

stock PlayCameraSequenceForPlayer(playerid, cameraid, startnode = 0, bool:tp = true, bool:freeze = true, bool:loop = false)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	camseq_debug("[PlayCameraSequenceForPlayer] playerid: %d cameraid: %d starting at: %d tp: %d freeze: %d loop: %d", playerid, cameraid, startnode, tp, freeze, loop);

	cam_PlayerCurrentCamera[playerid] = cameraid;
	cam_Data[cameraid][cam_currentNode] = startnode;

	MoveCameraToNextNode(playerid, tp, freeze, loop);
	
	return 1;
}

stock ExitPlayerCameraSequencer(playerid, returncamera = true, unfreeze = true)
{
	if(!Iter_Contains(cam_Index, cam_PlayerCurrentCamera[playerid]))
		return 0;

	camseq_debug("[ExitPlayerCameraSequencer] playerid: %d returncamera: %d unfreeze: %d", playerid, returncamera, unfreeze);

	KillTimer(cam_NodeTimer[cam_PlayerCurrentCamera[playerid]);

	if(returncamera)
		SetCameraBehindPlayer(playerid);

	if(unfreeze)
		TogglePlayerControllable(playerid, true);

	cam_PlayerCurrentCamera[playerid] = INVALID_CAMERA_SEQUENCER_ID;
	
	return 1;
}

stock PausePlayerCameraSequence(playerid)
{
	if(!Iter_Contains(cam_Index, cam_PlayerCurrentCamera[playerid]))
		return 0;

	new
		cameraid = cam_PlayerCurrentCamera[playerid],
		Float:vecx,
		Float:vecy,
		Float:vecz;

	camseq_debug("[PausePlayerCameraSequence] playerid: %d cameraid: %d current node: %d", playerid, cameraid, cam_Data[cameraid][cam_currentNode]);

	GetPlayerCameraPos(playerid, cam_PausePos[cameraid][0], cam_PausePos[cameraid][1], cam_PausePos[cameraid][2]);
	GetPlayerCameraFrontVector(playerid, vecx, vecy, vecz);

	cam_PausePos[cameraid][3] = cam_PausePos[cameraid][0] + vecx;
	cam_PausePos[cameraid][4] = cam_PausePos[cameraid][1] + vecy;
	cam_PausePos[cameraid][5] = cam_PausePos[cameraid][2] + vecz;

	SetPlayerCameraPos(playerid, cam_PausePos[cameraid][0], cam_PausePos[cameraid][1], cam_PausePos[cameraid][2]);
	SetPlayerCameraLookAt(playerid, cam_PausePos[cameraid][3], cam_PausePos[cameraid][4], cam_PausePos[cameraid][5]);

	KillTimer(cam_NodeTimer[cameraid]);
	cam_Data[cameraid][cam_isPaused] = true;

	return 1;
}


/*==============================================================================

	Internal Functions and Hooks

==============================================================================*/


public MoveCameraToNextNode(playerid, bool:tp, bool:freeze, bool:loop)
{
	new
		cameraid,
		currnode,
		nextnode;

	cameraid = cam_PlayerCurrentCamera[playerid],
	currnode = cam_Data[cameraid][cam_currentNode],
	nextnode = currnode + 1;

	if(nextnode >= cam_Data[cameraid][cam_totalNodes])
	{
		camseq_debug("[MoveCameraToNextNode] Last node reached (%d), looping: %d", nextnode, loop);

		if(loop)
			nextnode = 0;

		else
			return 1;
	}

	camseq_debug("[MoveCameraToNextNode] playerid: %d cameraid: %d current node: %d next node: %d", playerid, cameraid, currnode, nextnode);

	if(CallLocalFunction("OnCameraLeaveNode", "ddd", playerid, cameraid, currnode))
		return 1;

	if(cam_Data[cameraid][cam_isPaused])
	{
		camseq_debug("[MoveCameraToNextNode] Camera was paused, moving from paused position %.1f, %.1f, %.1f", cam_PausePos[cameraid][3], cam_PausePos[cameraid][4], cam_PausePos[cameraid][5]);

		InterpolateCameraPos(playerid,
			cam_PausePos[cameraid][0], cam_PausePos[cameraid][1], cam_PausePos[cameraid][2],
			cam_NodeData[cameraid][nextnode][cam_posX], cam_NodeData[cameraid][nextnode][cam_posY], cam_NodeData[cameraid][nextnode][cam_posZ],
			cam_NodeData[cameraid][currnode][cam_moveTime], cam_NodeData[cameraid][currnode][cam_moveType]);

		InterpolateCameraLookAt(playerid,
			cam_PausePos[cameraid][3], cam_PausePos[cameraid][4], cam_PausePos[cameraid][5],
			cam_NodeData[cameraid][nextnode][cam_tarX], cam_NodeData[cameraid][nextnode][cam_tarY], cam_NodeData[cameraid][nextnode][cam_tarZ],
			cam_NodeData[cameraid][currnode][cam_moveTime], cam_NodeData[cameraid][currnode][cam_moveType]);

		cam_Data[cameraid][cam_isPaused] = false;
	}
	else
	{
		InterpolateCameraPos(playerid,
			cam_NodeData[cameraid][currnode][cam_posX], cam_NodeData[cameraid][currnode][cam_posY], cam_NodeData[cameraid][currnode][cam_posZ],
			cam_NodeData[cameraid][nextnode][cam_posX], cam_NodeData[cameraid][nextnode][cam_posY], cam_NodeData[cameraid][nextnode][cam_posZ],
			cam_NodeData[cameraid][currnode][cam_moveTime], cam_NodeData[cameraid][currnode][cam_moveType]);

		InterpolateCameraLookAt(playerid,
			cam_NodeData[cameraid][currnode][cam_tarX], cam_NodeData[cameraid][currnode][cam_tarY], cam_NodeData[cameraid][currnode][cam_tarZ],
			cam_NodeData[cameraid][nextnode][cam_tarX], cam_NodeData[cameraid][nextnode][cam_tarY], cam_NodeData[cameraid][nextnode][cam_tarZ],
			cam_NodeData[cameraid][currnode][cam_moveTime], cam_NodeData[cameraid][currnode][cam_moveType]);
	}

	if(tp)
		SetPlayerPos(playerid, cam_NodeData[cameraid][nextnode][cam_tarX], cam_NodeData[cameraid][nextnode][cam_tarY], cam_NodeData[cameraid][nextnode][cam_tarZ] - 50.0);

	if(freeze)
		TogglePlayerControllable(playerid, false);

	cam_NodeTimer[cameraid] = SetTimerEx("internal_OnCamMove", cam_NodeData[cameraid][currnode][cam_moveTime], false, "ddddd", playerid, tp, freeze, loop);

	if(CallLocalFunction("OnCameraLeftNode", "ddd", playerid, cameraid, currnode))
		return 1;

	return 1;
}

public internal_OnCamMove(playerid, bool:tp, bool:freeze, bool:loop)
{
	new cameraid = cam_PlayerCurrentCamera[playerid];

	cam_Data[cameraid][cam_currentNode]++;

	if(cam_Data[cameraid][cam_currentNode] >= cam_Data[cameraid][cam_totalNodes])
		cam_Data[cameraid][cam_currentNode] = 0;

	if(CallLocalFunction("OnCameraReachNode", "ddd", playerid, cameraid, cam_Data[cameraid][cam_currentNode]))
		return;

	camseq_debug("[internal_OnCamMove] Camera reached node %d, waiting here for %dms", cam_Data[cameraid][cam_currentNode], cam_NodeData[cameraid][cam_Data[cameraid][cam_currentNode]][cam_waitTime]);

	if(cam_Data[cameraid][cam_currentNode] < cam_Data[cameraid][cam_totalNodes] && !loop)
	{
		if(cam_NodeData[cameraid][cam_Data[cameraid][cam_currentNode]][cam_waitTime] > 0)
			cam_NodeTimer[cameraid] = SetTimerEx("MoveCameraToNextNode", cam_NodeData[cameraid][cam_Data[cameraid][cam_currentNode]][cam_waitTime], false, "dddd", playerid, tp, freeze, loop);

		else
			MoveCameraToNextNode(playerid, tp, freeze, loop);

		CallLocalFunction("OnCameraReachedNode", "ddd", playerid, cameraid, cam_Data[cameraid][cam_currentNode]);
		
		return;
	}

	CallLocalFunction("OnCameraReachedNode", "ddd", playerid, cameraid, cam_Data[cameraid][cam_currentNode]);
	CallLocalFunction("OnCameraSequenceFinish", "ddd", playerid, cameraid);

	return;
}


/*==============================================================================

	Interface Functions

==============================================================================*/


stock IsValidCameraSequencer(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	return 1;
}

// cam_totalNodes
stock GetCameraTotalNodes(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	return cam_Data[cameraid][cam_totalNodes];
}

// cam_isPaused
stock IsCameraPaused(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	return cam_Data[cameraid][cam_isPaused];
}

// cam_currentNode
stock GetCameraCurrentNode(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	return cam_Data[cameraid][cam_currentNode];
}

// cam_isMoving
stock IsCameraMoving(cameraid)
{
	if(!Iter_Contains(cam_Index, cameraid))
		return 0;

	return cam_Data[cameraid][cam_isMoving];
}


// cam_posX
// cam_posY
// cam_posZ
// cam_tarX
// cam_tarY
// cam_tarZ
// cam_moveTime
// cam_waitTime
// cam_moveType

// cam_PausePos
// cam_PlayerCurrentCamera
stock GetPlayerCameraSequencer(playerid)
{
	if(!IsPlayerConnected(playerid))
		return INVALID_CAMERA_SEQUENCER_ID;

	return cam_PlayerCurrentCamera[playerid];
}
