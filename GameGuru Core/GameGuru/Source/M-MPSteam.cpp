//----------------------------------------------------
//--- GAMEGURU - M-Multiplayer
//----------------------------------------------------

#include "gameguru.h"

#ifdef FREETRIALVERSION

// No multiplayer in free trial version
void mp_init ( void ) {}
void mp_loop ( void ) {}
void mp_free ( void ) {}
void mp_checkVoiceChat ( void ) {}
void mp_spawn_objects ( void ) {}
void mp_lua ( void ) {}
void mp_delete_entities ( void ) {}
void mp_pre_game_file_sync ( void ) {}
void mp_pre_game_file_sync_server ( void ) {}
void mp_pre_game_file_sync_client ( void ) {}
void mp_sendAvatarInfo ( void ) {}
void mp_animation ( void ) {}
void mp_update_player ( void ) {}
void mp_updatePlayerPositions ( void ) {}
void mp_server_message ( void ) {}
void mp_updatePlayerNamePlates ( void ) {}
void mp_updatePlayerAnimations ( void ) {}
void mp_switchAnim ( void ) {}
void mp_update_waist_rotation ( void ) {}
void mp_showdeath ( void ) {}
void mp_respawn ( void ) {}
void mp_getPlaceToSpawn ( void ) {}
void mp_getInitialPlayerCount ( void ) {}
void mp_nukeTestmap ( void ) {}
void mp_respawnEntities ( void ) {}
void mp_addDestroyedObject ( void ) {}
void mp_add_respawn_timed ( void ) {}
void mp_setLuaPlayerNames ( void ) {}
void mp_setLuaResetStats ( void ) {}
void mp_updatePlayerInput ( void ) {}
void mp_load_guns ( void ) {}
void mp_check_for_attachments ( void ) {}
void mp_addJetpackParticles ( void ) {}
void mp_NearOtherPlayers ( void ) {}
void mp_check_respawn_objects ( void ) {}
void mp_checkForEveryoneLeft ( void ) {}
void mp_lostConnection ( void ) {}
void mp_gameLoop ( void ) {}
void mp_dontShowOtherPlayers ( void ) {}
void mp_ending_game ( void ) {}
void mp_free_game ( void ) {}
void mp_subbedToItem ( void ) {}
void mp_checkItemSubbed ( void ) {}
void mp_resetGameStats ( void ) {}
void mp_update_all_projectiles ( void ) {}
void mp_destroyentity ( void ) {}
void mp_refresh ( void ) {}
void mp_setMessage ( void ) {}
void mp_setMessageDots ( void ) {}
void mp_message ( void ) {}
void mp_messageDots ( void ) {}
void mp_update_projectile ( void ) {}
void mp_serverSetLuaGameMode ( void ) {}
void mp_setServerTimer ( void ) {}
void mp_serverRespawnAll ( void ) {}
void mp_restoreEntities ( void ) {}
void mp_serverEndPlay ( void ) {}
void mp_setServerKillsToWin ( void ) {}
void mp_networkkill ( void ) {}
void mp_lobbyListBox ( void ) {}
void mp_createLobby ( void ) {}
void mp_searchForLobbies ( void ) {}
void mp_searchForFpms ( void ) {}
void mp_launchGame ( void ) {}
void mp_backToStart ( void ) {}
void mp_selectedALevel ( void ) {}
void mp_checkIfLevelHasCustomContent ( void ) {}
void mp_buildWorkShopItem ( void ) {}
void mp_buildingWorkshopItemFailed ( void ) {}
void mp_joinALobby ( void ) {}
void mp_canIJoinThisLobby ( void ) {}
void mp_leaveALobby ( void ) {}
void mp_SubscribeToWorkShopItem ( void ) {}
void mp_save_workshop_files_needed ( void ) {}
void mp_grabWorkshopChangedFlagAndVersion ( void ) {}
int mp_check_if_entity_is_from_install ( char* name_s ) { return 0; }
void mp_resetSteam ( void ) {}
void mp_shoot ( void ) {}
void mp_chat ( void ) {}
void mp_chatNew ( void ) {}

void mp_quitGame ( void ) 
{
	t.tstartoffade = Timer();
	t.tfadestealpha_f = 0.0;
	t.tspritetouse = 0;
	for ( t.tloop = 2000 ; t.tloop<=  3000; t.tloop++ )
	{
		if (  SpriteExist(t.tloop)  ==  0 ) { t.tspritetouse  =  t.tloop  ; break; }
	}
	while (  Timer() - t.tstartoffade < 500 ) 
	{
		t.tfadestealpha_f = (Timer() - t.tstartoffade)*2;
		if (  t.tfadestealpha_f < 0  )  t.tfadestealpha_f  =  0.0;
		if (  t.tfadestealpha_f > 255.0  )  t.tfadestealpha_f  =  255.0;
		if (  t.tspritetouse > 0 && ImageExist(g.panelimageoffset+1)  ==  1 ) 
		{
			Sprite (  t.tspritetouse,0,0,g.panelimageoffset+1 );
			SizeSprite (  t.tspritetouse,GetDisplayWidth()*10, GetDisplayHeight()*10 );
			SetSpriteDiffuse (  t.tspritetouse,0,0,0 );
			SetSpriteAlpha (  t.tspritetouse,t.tfadestealpha_f );
		}
		//SteamLoop (  );
		Sync (  );
	}
	if (  g.mp.isGameHost  ==  1 ) 
	{
		//SteamEndGame (  );
	}
	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
}

void mp_freefadesprite ( void ) {}
void mp_backToEditor ( void ) {}
void mp_cleanupGame ( void ) {}
void mp_sendSteamIDToEditor ( void ) {}
void mp_checkIfLobbiesAvailable ( void ) {}
void mp_flashLightOff ( void ) {}
void mp_setupCoopTeam ( void ) {}
void mp_COOP_aiMoveTo ( void ) {}
void mp_entity_lua_lookatplayer ( void ) {}
void mp_entity_lua_fireweaponEffectOnly ( void ) {}
void mp_updateAIForCOOP ( void ) {}
void mp_coop_rotatetoplayer ( void ) {}
void mp_storeOldEntityPositions ( void ) {}
void mp_howManyEnemiesLeftToKill ( void ) {}
void mp_IKilledAnAI ( void ) {}
void mp_text ( int x, int y, int size, char* txt_s ) {}
void mp_textDots ( int x, int y, int size, char* txt_s ) {}
void mp_textColor ( int x, int y, int size, char* txt_s, int r, int g, int b ) {}
void mp_panel ( int x, int y, int x2, int y2 ) {}
void mp_livelog ( char* t_s ) {}
void mp_deleteFile ( char* tempFileToDelete_s ) {}
int mp_check_if_lua_entity_exists ( int tentitytocheck ) { return 0; }
void mp_sendlua ( int code, int e, int v ) {}

void SteamApplyPlayerDamage ( int index, int damage, int x, int y, int z, int force, int limb ) {}
int SteamGetPlayerAlive ( int a ) { return 0; }
char* SteamGetWorkshopItemPath ( void ) { return ""; }
void SteamDestroy ( int a ) { }
void SteamShowAgreement ( void ) { }
int SteamInit ( void ) { return 0; }
void SteamGetWorkshopItemPathDLL ( char* ) {}
int SteamIsWorkshopLoadingOnDLL ( void ) { return 0; }
void SteamSendLua ( int a, int b, int c ) {}



#else

// flag to switch workshop handling from workshop to game managed, by default set to false, set to true for multiplayer mode
extern bool OnlineMultiplayerModeForSharingFiles;

//  Startup Steam
void mp_init ( void )
{
	timestampactivity(0,"_mp_init:");
	t.mp_build = 1121;
	g.mp.isRunning = SteamInit();
	g.mp.mode = MP_MODE_NONE;
	g.mp.dontDrawTitles = 0;
	g.mp.message = "";
	g.mp.messageTime = 0;

	// If a custom character head is used but the image no longer exists, we need to get rid of the avatar file
	characterkit_checkAvatarExists();
}

bool OccluderCheckingForMultiplayer ( void )
{
	if ( t.game.runasmultiplayer == 0 ) return false;

	return true;
}

void mp_loop ( void )
{

	SteamLoop (  );

	//  store old positions of entities if in coop mode
	/*      
	if (  g.mp.coop  ==  1 ) 
	{
		if (  g.mp.madeArrays   ==  0 ) 
		{
			g.mp.madeArrays = 1;
			mp_storeOldEntityPositions ( );
		}
	}
	*/    

	//  debug stuff for loading in custom cc avatars
	/*      
	SetCursor (  0,0 );
	Print (  "My avatar string = " + g.mp.myAvatar_s );
	for ( t.c = 0 ; t.c<=  7; t.c++ )
	{
		Print (  t.mp_playerAvatarOwners_s[t.c] );
		Print (  t.mp_playerAvatars_s[t.c] );
	}
	Print (  "have sent mine " + Str(g.mp.haveSentMyAvatar) );
	*/    

	if (  g.mp.mode  ==  MP_MODE_NONE || t.game.runasmultiplayer  ==  0  )
	{
		// usual workshop mode
		OnlineMultiplayerModeForSharingFiles = false;
		return;
	}

	// game managed mode for sharing files
	OnlineMultiplayerModeForSharingFiles = true;

	if (  g.mp.mode  !=  MP_IN_GAME_CLIENT && g.mp.mode  !=  MP_IN_GAME_SERVER ) 
	{

		g.mp.finishedLoadingMap = 0;

		//  200315 - 021 - flashlight of when starting a game
		mp_flashLightOff ( );
		g.mp.originalEntitycount = 0;
		if (  SpriteExist(g.steamchatpanelsprite)  )  DeleteSprite (  g.steamchatpanelsprite );
		g.mp.dontDrawTitles = 0;
		//  If not connected to steam, retry
		if (  g.mp.isRunning  ==  0 || g.mp.needToResetOnStartup  ==  1 ) 
		{
			g.mp.goBackToEditor = 0;
			mp_resetSteam ( );
			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP001)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
		}
		else
		{
			g.mp.backtoeditorforyou = 0;
		}

		//  Debug Info
		t.steamDoDropShadow = 1;
		t.ttstring_s = cstr("Multiplayer Build ") + Str(t.mp_build);
		mp_text(-1,98,2,t.ttstring_s.Get());


	}
	else
	{
		//  030315 - 013 - Lobby chat
		t.tchatLobbyMode = 0;
		mp_chat ( );
	}
	if (  g.mp.mode  ==  MP_MODE_MAIN_MENU ) 
	{
		//  show avatar name if there is one
		if (  g.mp.myAvatarName_s != "" ) 
		{
		if (  ImageExist(g.charactercreatorEditorImageoffset)  ==  0 ) 
		{
			t.tShowAvatarSprite = 1;
			characterkit_loadMyAvatarInfo ( );
		}
			t.tYPos_f = 95;
			if (  GetDisplayHeight() > 900  )  t.tYPos_f  =  92;
			mp_text(-1,t.tYPos_f,2,g.mp.myAvatarName_s.Get());
			if (  g.charactercreatorEditorImageoffset > 0 ) 
			{
				if (  ImageExist(g.charactercreatorEditorImageoffset)  ==  1 ) 
				{
					t.tYPos_f = GetChildWindowHeight();
					t.tYPos_f = t.tYPos_f * 0.85;
					PasteImage (  g.charactercreatorEditorImageoffset, (GetChildWindowWidth()/2)-32, t.tYPos_f,g.charactercreatorEditorImageoffset );
				}
			}
		}
		g.mp.dontDrawTitles = 0;
		if (  g.mp.originalpath  ==  "" ) 
		{
			g.mp.originalpath = GetDir();
			SteamSetRoot(cstr(g.fpscrootdir_s+"\\Files\\").Get());
		}
		//  110315 - 019 - remove fadeoutsprite if it exists
		if (  t.tspritetouse > 0 ) 
		{
			if (  SpriteExist(t.tspritetouse)  ==  1  )  DeleteSprite (  t.tspritetouse );
			t.tspritetouse = 0;
		}
		/*      
		if (  SpaceKey() && oldspacekey  ==  0 ) 
		{
			SteamSetRoot(g.fpscrootdir_s+"\\Files\\");
			SteamCreateWorkshopItem (  "Awesome Custom Level" );
		}
		if (  LeftKey() && oldleftkey  ==  0 ) 
		{
//mp_save_workshop_files_needed ( );
			SteamDownloadWorkshopItem (  "378579107" );
			Print (  "HELLO" );
		}
		if (  SteamIsWorkshopItemDownloaded()  ==  1 ) 
		{
			mp_text(-1,5,3,"WORKSHOP ITEM DOWNLOADED");
//    `if ImageExist(999) = 0

//     `load image "F:\\TGCSHARED\\fpsc-reloaded\\FPS Creator Files\\Files\\entitybank\\ravey\\fizco\\fizzie.jpg",999

//     `sprite 999,0,0,999

//    `endif

		}
		else
		{
			mp_text(-1,5,3,"WORKSHOP ITEM NOT DOWNLOADED");
		}
		oldleftkey = LeftKey();
		oldspacekey = SpaceKey();
		*/    

//   `print "======================================"

//   `print "(C) Create Lobby"

//   `print "(S) Search for Lobbies"

//   `print "======================================"


		g.mp.lobbyscrollbarOn = 0;
		g.mp.selectedLobby = 0;
		t.tjoinedLobby = 0;
		g.mp.lobbyoffset = 0;
		g.mp.lobbycount = 0;
		mp_resetGameStats ( );

		t.game.jumplevel_s="__multiplayerlevel__";
	}
	else
	{
		if (  g.charactercreatorEditorImageoffset > 0 ) 
		{
			if (  ImageExist(g.charactercreatorEditorImageoffset)  )  DeleteImage (  g.charactercreatorEditorImageoffset );
		}
	}

	if (  g.mp.mode  ==  MP_WAITING_FOR_LOBBY_CREATION ) 
	{
//   `print "======================================"

//   `print "Creating Lobby"

//   `print "======================================"

		if (  SteamIsLobbyCreated()  ==  1 ) 
		{
			g.mp.isLobbyCreated = 1;
			SteamGetLobbyList (  );
			g.mp.mode = MP_MODE_LOBBY;
		}
		else
		{
			g.mp.haveToldAboutSolo = 0;
			if (  Timer() - t.tempsteamlobbycreationtimeout > 5000 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Could not create lobby (Error MP002)";
				mp_lostConnection ( );
				return;
			}
			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP003)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
		}
	}

	if (  g.mp.mode  ==  MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM ) 
	{
		mp_text(-1,45,3,"You do not currently have the workshop item required to");
		mp_text(-1,50,3,"join this game. Do you wish to subscribe to the workshop");
		mp_text(-1,55,3,"item so you can join a game with this level at a later time?");
		mp_text(-1,65,3,"Note: Once you have subscribed the Lobby will remain yellow until");
		mp_text(-1,70,3,"you have downloaded the whole workshop item.");
		t.tempsteamhaveaskedtosubscribeflag = 0;
	}

	if (  g.mp.mode  ==  MP_TELLING_THEY_NEED_TO_RESTART ) 
	{
		mp_text(-1,45,3,"Your version of this workshop item is outdated.");
		mp_text(-1,50,3,"To enable Steam to download the update you will need to:");
		mp_text(-1,55,3,"Exit multiplayer, then exit Game Guru completely.");
		mp_text(-1,60,3,"Then restart Game Guru and Steam will update all");
		mp_text(-1,65,3,"your subscriptions.");
	}

	if (  g.mp.mode  ==  MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM_WAITING_FOR_RESULTS ) 
	{
		if (  t.tempsteamhaveaskedtosubscribeflag  ==  0 ) 
		{
			t.tempsteamhaveaskedtosubscribeflag = 1;
			SteamDownloadWorkshopItem (  g.mp.workshopidtojoin.Get() );
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  0 ) 
		{
			if (  Timer() - g.mp.oldtime > 150 ) 
			{
				g.mp.oldtime = Timer();
				t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
				if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
			}
			mp_text(-1,50,3,cstr( cstr("Subscribing you") + t.tSteamBuildingWorkshopItem_s).Get() );
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  1 ) 
		{
			mp_text(-1,50,3,"You are now subscribed to:");
			mp_text(-1,55,3,g.mp.levelnametojoin.Get());
			mp_text(-1,65,3,"Press back and wait for this level to install");
			mp_text(-1,70,3,"(the lobby will turn from yellow to white)");
			mp_text(-1,75,3,"then re-join the lobby.");
		}
		if (  SteamHasSubscriptionWorkshopItemFinished()  ==  -1 ) 
		{
			mp_text(-1,50,3,"Subscription failed");
			mp_text(-1,55,3,"Please t.try again in t.a few moments");
		}
	}

	if (  g.mp.mode  ==  MP_SERVER_CHOOSING_FPM_TO_USE ) 
	{
		mp_text(-1,5,3,"LIST OF LEVELS");
		mp_lobbyListBox ( );
	}

	if (  g.mp.mode  ==  MP_SERVER_CHOOSING_TO_MAKE_FPS_WORKSHOP ) 
	{
		mp_text(-1,30,3,"This level contains custom content.");
		mp_text(-1,35,3,"To share this level with others you will need to create a workshop item.");
		mp_text(-1,40,3,"(This is done automatically for you)");
		mp_text(-1,50,3,"Once your level is a workshop item other players can play your level.");
		mp_text(-1,60,3,"Do you wish to create (or update if you have share this level before)");
		mp_text(-1,65,3,"A workshop item?");
		mp_text(-1,75,3,"By submitting this item, you agree to the workshop terms of service");
	
//   `mp.oldtime = 0

	}

	if (  g.mp.mode  ==  MP_CREATING_WORKSHOP_ITEM ) 
	{
		if (  Timer() - g.mp.oldtime > 150 ) 
		{
			g.mp.oldtime = Timer();
			t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
			if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
		}
		t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Building Workshop Item" + t.tSteamBuildingWorkshopItem_s;
		mp_text(-1,50,3,t.tstring_s.Get());
		t.tstring_s = "";
	
//  `mp_text(0,10,3, "mp.buildingWorkshopItemMode = " + Str(mp.buildingWorkshopItemMode) )

//  `mp_text(0,20,3, "mp.workshopid = " + mp.workshopid )

	
	}

	if (  g.mp.mode  ==  MP_MODE_LOBBY ) 
	{
		if (  g.mp.isGameHost  ==  0 ) 
		{

			if (  g.mp.isRunning  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Cannot connect to Steam (Error MP004)";
				g.mp.backtoeditorforyou = 2;
				mp_lostConnection ( );
				return;
			}
//    `print "======================================"

//    `print "Lobby list"

//    `print "======================================"

			mp_text(-1,5,3,"LIST OF LOBBIES");
			if (  SteamIsLobbyListCreated()  ==  0 ) 
			{

				if (  g.mp.lobbycount  ==  0 ) 
				{
					t.tstring_s = "Building Lobby list";
					mp_text(-1,10,1,t.tstring_s.Get());
				}
				else
				{
//      `if mp.lobbycount = 1

//       `tstring$ = "1 lobby found"

//      `else

//       `tstring$ = Str(tsize) + " lobbies found"

//      `endif

//      `mp_text(-1,15,1,tstring$)


				}

				if (  Timer() - g.mp.oldtime > 3000 ) 
				{
					SteamGetLobbyList (  );
					g.mp.oldtime = Timer();
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 3000 ) 
				{
					SteamGetLobbyList (  );
					g.mp.oldtime = Timer();
				}
			}

			mp_lobbyListBox ( );

		}
		else
		{
			//  030315 - 013 - Lobby chat
			t.tchatLobbyMode = 1;
			mp_chat ( );
			mp_text(-1,85,3,"Press Enter to chat");

			t.tUserCount = SteamGetLobbyUserCount();
			if (  Timer() - t.tempsteamlobbycreationtimeout > 5000 && t.tUserCount  ==  0 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Could not create lobby (Error MP005)";
				mp_lostConnection ( );
				return;
			}
//    `print "======================================"

//    `print mp.playerName + "'s Lobby"

//    `print "======================================"

//    `print "(S) Start Server"

//    `print "======================================"

			if (  t.tUserCount  ==  1 ) 
			{
				t.tstring_s = "There is 1 user (you!) in this lobby";
				g.mp.usersInServersLobbyAtServerCreation = 1;
			}
			else
			{
				t.tstring_s = cstr("There are ") + Str(t.tUserCount) + " users in this lobby";
			}
			if (  t.tUserCount  !=  g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.haveSentMyAvatar = 0;
			}
			if (  t.tUserCount > g.mp.usersInServersLobbyAtServerCreation ) 
			{
				g.mp.usersInServersLobbyAtServerCreation = t.tUserCount;
			}
			mp_text(-1,15,1,t.tstring_s.Get());
			t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
			t.tsteamy = t.tsteamy_f;
			for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
			{
				t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1);
				if (  SteamGetPlayerName()  !=  SteamGetLobbyUserName(t.tn-1)  )  t.mp_joined[t.tn-1]  =  SteamGetLobbyUserName(t.tn-1);
				mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
				t.tsteamy += 5;
			}
			for ( t.tn = t.tUserCount ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
			{
				t.mp_joined[t.tn] = "";
			}
				if (  g.mp.haveToldAboutSolo  ==  1 && t.tUserCount  <=  1 ) 
				{
					mp_textColor(-1,70,1,"Noone has joined your lobby yet. If you start now you will be playing alone.",255,100,100);
					mp_textColor(-1,75,1,"Press Start Server again to start anyway.",255,100,100);
				}

		if (  g.mp.launchServer  ==  1 ) 
		{
					if (  g.mp.haveToldAboutSolo  ==  0 && t.tUserCount  <=  1 ) 
					{
						g.mp.haveToldAboutSolo = 1;
						g.mp.launchServer = 0;
						return;
					}
					SteamStartServer (  );
				g.mp.mode = MP_WAITING_FOR_SERVER_CREATION;
				g.mp.oldtime = Timer();
			}
		}
	}

	if (  g.mp.mode  ==  MP_JOINING_LOBBY ) 
	{
		if (  SteamIsGameRunning()  ==  1 ) 
		{
			g.mp.mode = MP_IN_GAME_CLIENT;
			g.mp.needToResetOnStartup = 1;
			t.toldsteamfolder_s=GetDir();
			SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\gridedit").Get() );
			t.tsteamtimeoutongamerunning = Timer();
			t.tPlayerIndex = SteamGetMyPlayerIndex();
			if (  t.tPlayerIndex  >=  0 && t.tPlayerIndex < MP_MAX_NUMBER_OF_PLAYERS ) 
			{
				t.mp_health[t.tPlayerIndex] = 0;
				t.ta = MouseMoveX() + MouseMoveY();
			}
		}
		if (  t.tjoinedLobby  ==  0 ) 
		{
			if (  Timer() - g.mp.AttemptedToJoinLobbyTime > MP_JOIN_LOBBY_TIMEOUT ) 
			{
				g.mp.mode = MP_MODE_MAIN_MENU;
				t.tmsg_s = "Could not join Lobby";
				mp_setMessage ( );
			}
		}
		if (  SteamHasJoinedLobby()  ==  1 ) 
		{
			t.tjoinedLobby = 1;
			if (  t.tjoinedLobby  ==  0 ) 
			{
					t.tsteamwaitedforlobbytimer = Timer();
			}
		}
		else
		{
			t.tsteamwaitedforlobbytimer = Timer();
			t.tsteamistheownerpresenttime = t.tsteamwaitedforlobbytimer;
		}
		if (  t.tjoinedLobby  ==  0 ) 
		{
				t.tsteamwaitedforlobbytimer = Timer();
				t.tsteamistheownerpresenttime = Timer();
		}
		if (  t.tjoinedLobby  ==  1 ) 
		{
			if (  SteamHasJoinedLobby()  ==  1 ) 
			{
					//  030315 - 013 - Lobby chat
					t.tchatLobbyMode = 1;
					mp_chat ( );
					mp_text(-1,85,3,"Press Enter to chat");
					t.tsteamlobbertimer = Timer();
//     `print "======================================"

//     `print "In lobby"

//     `print "======================================"

				if (  t.tUserCount  !=  SteamGetLobbyUserCount() ) 
				{
					g.mp.haveSentMyAvatar = 0;
				}
				t.tUserCount = SteamGetLobbyUserCount();
				if (  t.tUserCount  ==  1 && Timer() - t.tsteamwaitedforlobbytimer > 15000 ) 
				{
					if (  SteamIsGameRunning()  ==  0 ) 
					{
						SteamLeaveLobby (  );
						t.tsteamlostconnectioncustommessage_s = "Lost connection to lobby (Error MP006)";
						mp_lostConnection ( );
						return;
					}
				}
				else
				{
					t.tsteamwaitedforlobbytimer = Timer();
					mp_text(-1,15,1, cstr(cstr("There are ") + Str(t.tUserCount) + " users in this lobby").Get() );
					mp_text(-1,10,1, cstr(cstr("Game being hosted is '") + g.mp.levelnametojoin + "'").Get() );

				}
				t.tsteamistheownerpresent = 0;
				t.tsteamnamewearelookingfor_s = Left(g.mp.lobbyjoinedname.Get(),Len(g.mp.lobbyjoinedname.Get())-8);

				t.tsteamy_f = 50.0 - (t.tUserCount * 2.5);
				t.tsteamy = t.tsteamy_f;

				if (  t.tsteamnamewearelookingfor_s  ==  SteamGetLobbyUserName(0) ) 
				{
					for ( t.tn = t.tUserCount ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
					{
						t.mp_joined[t.tn] = "";
					}
					for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
					{
						if (  SteamGetPlayerName()  !=  SteamGetLobbyUserName(t.tn-1)  )  t.mp_joined[t.tn-1]  =  SteamGetLobbyUserName(t.tn-1);
					}
				}
				for ( t.tn = 1 ; t.tn<=  t.tUserCount; t.tn++ )
				{
					if (  t.tn  ==  1 ) 
					{
						t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1) + " (Host)";
					}
					else
					{
						t.tstring_s = cstr("Player ") + Str(t.tn) + ": " + SteamGetLobbyUserName(t.tn-1);
					}
					mp_text(-1,t.tsteamy,1,t.tstring_s.Get());
					t.tsteamy += 5;
					if (  t.tsteamnamewearelookingfor_s  ==  SteamGetLobbyUserName(t.tn-1) ) 
					{
						t.tsteamistheownerpresent = 1;
						t.tsteamistheownerpresenttime = Timer();
					}
				}
				if (  t.tsteamistheownerpresent  ==  0 && Timer() - t.tsteamistheownerpresenttime > 10000 ) 
				{
					SteamLeaveLobby (  );
					t.tsteamlostconnectioncustommessage_s = "The host left the lobby (Code MP007)";
					mp_lostConnection ( );
					return;
				}

				t.tsteamlobbertimer = Timer();
			}
			else
			{
				mp_textDots(-1,20,3,"Game Starting...Connecting");
				if (  Timer() - t.tsteamlobbertimer > 20000 ) 
				{
					if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
					{
						t.tsteamlostconnectioncustommessage_s = "Lost connection to host (Error MP008)";
						mp_lostConnection ( );
						return;
					}
				}
			}
		}
		else
		{
//    `print "======================================"

//    `print "Joining lobby"

//    `print "======================================"

		}
	}
	if (  g.mp.mode  ==  MP_WAITING_FOR_SERVER_CREATION ) 
	{
		g.mp.dontDrawTitles = 1;
		if (  SteamIsServerRunning()  ==  1 ) 
		{
			mp_textDots(-1,10,3,"Server Started");
			if (  SteamIsGameRunning()  ==  1 ) 
			{
				g.mp.mode = MP_IN_GAME_SERVER;
				g.mp.needToResetOnStartup = 1;
				t.toldsteamfolder_s=GetDir();
				SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\gridedit").Get() );
				t.tPlayerIndex = SteamGetMyPlayerIndex();
				SteamSetSendFileCount (  1 );
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
					if (  Len(t.tStartingServerCount_s.Get()) > 5  )  t.tStartingServerCount_s  =  ".";
				}
				t.tstring_s = t.tStartingServerCount_s + "Waiting for game to start" + t.tStartingServerCount_s;
				mp_text(-1,25,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		}
		else
		{
			if (  Timer() - g.mp.oldtime > 150 ) 
			{
				g.mp.oldtime = Timer();
				t.tStartingServerCount_s = t.tStartingServerCount_s + ".";
				if (  Len(t.tStartingServerCount_s.Get()) > 5  )  t.tStartingServerCount_s  =  ".";
			}
			t.tstring_s = t.tStartingServerCount_s + "Starting server" + t.tStartingServerCount_s;
			mp_text(-1,15,3,t.tstring_s.Get());
			t.tstring_s = "";
		}
	}

	if (  g.mp.mode  ==  MP_IN_GAME_SERVER ) 
	{
			g.mp.dontDrawTitles = 1;
			if (  g.mp.iHaveSaidIAmReady  ==  0 ) 
			{
				SteamSendIAmLoadedAndReady (  );
				g.mp.iHaveSaidIAmReady = 1;
				t.tempsteamingameinitialwaitingdelay = Timer();
				while (  Timer() - t.tempsteamingameinitialwaitingdelay < 20000 ) 
				{
					g.mp.syncedWithServerMode = 0;
					g.mp.okayToLoadLevel = 0;
					SteamLoop (  );
					mp_textDots(-1,20,3,"Waiting for other players");
					if (  Timer() - t.tsteamiseveryoneloadedandreadytime > 1000 ) 
					{
						t.tsteamiseveryoneloadedandreadytime = Timer();
						if (  SteamIsEveryoneLoadedAndReady()  ==  1  )  t.tempsteamingameinitialwaitingdelay  =  -30000;
					}
				}
				t.tskipLevelSync = Timer();
			}

			//wait for everyone before starting to load, at this GetPoint (  they have all the files they need, they just have not loaded them )
			if (  g.mp.okayToLoadLevel  ==  0 && g.mp.syncedWithServerMode  ==  99 ) 
			{
				t.game.titleloop=0;
				t.game.levelloop=1;
				t.game.runasmultiplayer=1;
				t.game.levelloadprogress=0;
				t.game.cancelmultiplayer=0;
				t.game.quitflag=0;
				t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;

				g.mp.playGame = 1;
				g.mp.okayToLoadLevel = 1;
				t.tskipLevelSync = Timer();
			}
			else
			{
				if (  g.mp.playGame  ==  1 ) 
				{
					if (  t.game.titleloop == 1 ) 
					{
						t.game.titleloop=0;
						t.game.levelloop=1;
						t.game.runasmultiplayer=1;
						t.game.levelloadprogress=0;
						t.game.cancelmultiplayer=0;
						t.game.quitflag=0;
						t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
					}
				}
				if (  g.mp.okayToLoadLevel  ==  0 ) 
				{
					mp_pre_game_file_sync_server ( 0 );
				}
			}

	}
	if (  g.mp.mode  ==  MP_IN_GAME_CLIENT ) 
	{
			if (  t.titlespage  ==  11 ) 
			{
				g.mp.dontDrawTitles = 0;
			}
			else
			{
				g.mp.dontDrawTitles = 1;
			}
			g.mp.dontDrawTitles = 1;
			if (  g.mp.iHaveSaidIAmReady  ==  0 ) 
			{
				SteamSendIAmLoadedAndReady (  );
				g.mp.iHaveSaidIAmReady = 1;
				t.tempsteamingameinitialwaitingdelay = Timer();
				while (  Timer() - t.tempsteamingameinitialwaitingdelay < 20000 ) 
				{
					g.mp.syncedWithServerMode = 0;
					g.mp.okayToLoadLevel = 0;
					mp_textDots(-1,50,3,"Waiting for other players");
					SteamLoop (  );
					if (  Timer() - t.tsteamtimeoutongamerunning > 16000 ) 
					{
						if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
						{
							t.tsteamlostconnectioncustommessage_s = "Lost connection to host (Error MP009)";
							mp_lostConnection ( );
							return;
						}
					}
					t.tskipLevelSync = Timer();
					if (  SteamIsEveryoneLoadedAndReady()  ==  1  )  t.tempsteamingameinitialwaitingdelay  =  -30000;
				}   
			}

			//wait for everyone before starting to load, at this GetPoint (  they have all the files they need, they just have not loaded them )
			if (  g.mp.okayToLoadLevel  ==  0 && g.mp.syncedWithServerMode  ==  99 ) 
			{
					t.game.titleloop=0;
					t.game.levelloop=1;
					t.game.runasmultiplayer=1;
					t.game.levelloadprogress=0;
					t.game.cancelmultiplayer=0;
					t.game.quitflag=0;
					t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;

					g.mp.playGame = 1;
					g.mp.okayToLoadLevel = 1;
					t.tskipLevelSync = Timer();
			}
			else
			{
				if (  g.mp.playGame  ==  1 ) 
				{
					if (  t.game.titleloop == 1 ) 
					{
						t.game.titleloop=0;
						t.game.levelloop=1;
						t.game.runasmultiplayer=1;
						t.game.levelloadprogress=0;
						t.game.cancelmultiplayer=0;
						t.game.quitflag=0;
						t.tescapepress=0 ; t.ttitlesbuttonhighlight=0;
					}
				}
				if (  g.mp.okayToLoadLevel  ==  0 ) 
				{
					mp_pre_game_file_sync_client ( );
				}
			}

	}

	mp_message ( );
	mp_messageDots ( );
}

void mp_free ( void )
{
	//PhotonFree();
	SteamFree();
}

void mp_checkVoiceChat ( void )
{
}

void mp_spawn_objects ( void )
{
	//  Grab the list of spawned objects from the server
	//  TO DO - find out how entities are spawned in FPSC and call those routines
	//  LEE - AGREED, no need to repeat code but we can do that during clean-up ;)
	while (  SteamGetSpawnList() ) 
	{
		t.obj = SteamGetSpawnObjectNumber();
		t.sourceobj = SteamGetSpawnObjectSource();
		t.x_f = SteamGetSpawnX();
		t.y_f = SteamGetSpawnY();
		t.z_f = SteamGetSpawnZ();
		InstanceObject (  t.obj, t.sourceobj );
		//  restore any radius settings the original object might have had
		SetSphereRadius (  t.obj,-1 );
		ShowObject (  t.obj );
		PositionObject (  t.obj, t.x_f, t.y_f, t.z_f );
		SteamGetNextSpawn (  );
	}

return;

}

void mp_lua ( void )
{

	while (  SteamGetLuaList() ) 
	{
		t.steamLuaCode = SteamGetLuaCommand();
		t.e = SteamGetLuaE();
		t.v = SteamGetLuaV();
	
		t.tLuaDontSendLua = 1;
	
		switch (  t.steamLuaCode ) 
		{
		case MP_LUA_SetActivated:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_setactivated() ; ++t.activatedCount;
		break;
		case MP_LUA_SetAnimation:
			entity_lua_setanimation() ; ++t.animCount;
		break;
		case MP_LUA_PlayAnimation:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_playanimation() ; ++t.playanimCount;
		break;
		case MP_LUA_ActivateIfUsed:
			if ( mp_check_if_lua_entity_exists(t.e) == 1 ) entity_lua_activateifused() ; ++t.activateCount;
		break;
		case MP_LUA_PlaySound:
			entity_lua_playsound ( );
		break;
		case MP_LUA_StartTimer:
			entity_lua_starttimer ( );
		break;
		case MP_LUA_CollisionOff:
			entity_lua_collisionoff ( );
		break;
		case MP_LUA_CollisionOn:
			entity_lua_collisionon ( );
		break;
		case MP_LUA_ServerSetLuaGameMode:
			LuaSetInt (  "mp_gameMode",t.v );
		break;
		case MP_LUA_ServerSetPlayerKills:
			t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.e) + "] = " + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerSetPlayerDeaths:
			t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.e) + "] = " + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerSetPlayerAddKill:
			t.mp_kills[t.v] = t.mp_kills[t.v] + 1;
			SteamSendLua (  MP_LUA_ServerSetPlayerKills,t.v,t.mp_kills[t.v] );
			t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.v) + "] = " + Str(t.mp_kills[t.v])).Get() );
		break;
		case MP_LUA_ServerSetPlayerRemoveKill:
			//  check if they already have the kills needed to win
			//  because they may kill someone else first, then themselves, which has already triggered a win
			//  so we only remove a kill if they havent yet won
			if (  g.mp.setserverkillstowin  <= 0  )  g.mp.setserverkillstowin  =  100;
			if (  t.mp_kills[t.v] < g.mp.setserverkillstowin ) 
			{
				t.mp_kills[t.v] = t.mp_kills[t.v] - 1;
				SteamSendLua (  MP_LUA_ServerSetPlayerKills,t.v,t.mp_kills[t.v] );
				t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.v) + "] = " + Str(t.mp_kills[t.v])).Get() );
			}
		break;
		case MP_LUA_ServerSetPlayerAddDeath:
			t.mp_deaths[t.v] = t.mp_deaths[t.v] + 1;
			SteamSendLua (  MP_LUA_ServerSetPlayerDeaths,t.v,t.mp_deaths[t.v] );
			t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.v) + "] = " + Str(t.mp_deaths[t.v])).Get() );
		break;
		case MP_LUA_SetServerTimer:
			t.tnothing = LuaExecute( cstr(cstr("mp_servertimer = ") + Str(t.v)).Get() );
		break;
		case MP_LUA_ServerRespawnAll:
			mp_restoreEntities ( );
			mp_setLuaResetStats ( );
			mp_respawnEntities ( );
			t.playercontrol.jetpackhidden=0;
			t.playercontrol.jetpackmode=0;
			physics_no_gun_zoom ( );
			t.aisystem.processplayerlogic=1;
			g.mp.gameAlreadySpawnedBefore = 0;
			t.mp_playerHasSpawned[g.mp.me] = 0;
			if (  g.mp.myOriginalSpawnPoint  !=  -1 ) 
			{
				t.tindex = g.mp.myOriginalSpawnPoint;
			}
			else
			{
				t.tindex = 1;
			}
			if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
			{
				t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
				t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y;
				t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
				t.terrain.playerax_f=0;
				t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
				t.terrain.playeraz_f=0;

				g.mp.lastx=t.terrain.playerx_f;
				g.mp.lasty=t.terrain.playery_f;
				g.mp.lastz=t.terrain.playerz_f;
				g.mp.lastangley=t.terrain.playeray_f;

				t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
				if (  t.tobj > 0 ) 
				{
					PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-50, t.terrain.playerz_f );
					RotateObject (  t.tobj, t.terrain.playerax_f, t.terrain.playeray_f, t.terrain.playeraz_f );
				}
			}

			g.autoloadgun=0  ; gun_change ( );
			g.mp.endplay = 0;
			t.player[t.plrid].health = 0;
			t.mp_health[g.mp.me] = 0;
			physics_resetplayer_core ( );
		break;
		case MP_LUA_ServerEndPlay:
				t.playercontrol.jetpackhidden=0;
				t.playercontrol.jetpackmode=0;
				physics_no_gun_zoom ( );
				t.aisystem.processplayerlogic=0;
				g.mp.endplay = 1;
				g.autoloadgun=0 ; gun_change ( );
		break;
		case MP_LUA_AiGoToX:
			t.tSteamX_f = t.v;
		break;
		case MP_LUA_AiGoToZ:
			t.tSteamZ_f = t.v;
			if (  t.e > 0 ) 
			{
				if (  ObjectExist(t.e)  ==  1 ) 
				{
					AISetEntityActive (  t.e,1 );
					mp_COOP_aiMoveTo ( );
				}
			}
			for ( t.tee = 1 ; t.tee<=  g.entityelementlist; t.tee++ )
			{
				if (  t.entityelement[t.tee].obj  ==  t.e ) 
				{
					t.entityelement[t.tee].mp_updateOn = 1;
					t.entityelement[t.tee].active = 1;
//      `print "updating object " + Str(tee)

					break;
				}
			}
		break;
		case MP_LUA_setcharactertowalkrun:
			entity_lua_setcharactertowalkrun ( );
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
				}
			}
		break;
		case MP_LUA_CharacterControlManual:
			entity_lua_charactercontrolmanual ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_CharacterControlLimbo:
			entity_lua_charactercontrollimbo ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_CharacterControlArmed:
			entity_lua_charactercontrolarmed ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
			AISetEntityActive (  t.entityelement[t.e].obj,1 );
		break;
		case MP_LUA_CharacterControlUnarmed:
			entity_lua_charactercontrolunarmed ( );
			t.entityelement[t.e].mp_updateOn = 1;
			t.entityelement[t.e].active = 1;
		break;
		case MP_LUA_LookAtPlayer:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
					mp_entity_lua_lookatplayer ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].mp_rotateTimer = Timer();
					t.entityelement[t.e].mp_rotateType = 1;
				}
			}
		break;
		case MP_LUA_TakenAggro:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				AISetEntityActive (  t.entityelement[t.e].obj,1 );
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
//      `if mp.me  ==  0 then SteamSendLua (  MP_LUA_TakenAggro,e,v )

					t.entityelement[t.e].mp_coopControlledByPlayer = t.v;
					t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = Timer();
//      `if v  ==  mp.me then inc entityelement(e).mp_coopLastTimeSwitchedTarget,5000
					
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					//  set them to run - not totally ideal for zombies (some walk) but okay for now
					t.v = 1;
					entity_lua_setcharactertowalkrun ( );
//      `AI Entity Stop entityelement(e).obj

				}
			}
		break;
		case MP_LUA_HaveAggro:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				AISetEntityActive (  t.entityelement[t.e].obj,1 );
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
//      `if mp.me  ==  0 then SteamSendLua (  MP_LUA_TakenAggro,e,v )
					
					t.entityelement[t.e].mp_coopControlledByPlayer = t.v;
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					//  set them to run - not totally ideal for zombies (some walk) but okay for now
					t.v = 1;
					entity_lua_setcharactertowalkrun ( );
//      `AI Entity Stop entityelement(e).obj

				}
			}
		break;
		case MP_LUA_FireWeaponEffectOnly:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					mp_entity_lua_fireweaponEffectOnly ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
				}
			}
		break;
		case MP_LUA_RotateToPlayer:
			if (  t.entityelement[t.e].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{
					mp_coop_rotatetoplayer ( );
					t.entityelement[t.e].mp_updateOn = 1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].mp_rotateTimer = Timer();
					t.entityelement[t.e].mp_rotateType = 2;
					AISetEntityActive (  t.entityelement[t.e].obj,1 );
				}
			}
		break;
		case MP_LUA_SetAnimationFrames:
			entity_lua_setanimationframes ( );
		break;
		case MP_LUA_AISetEntityControl:
			AISetEntityControl (  t.e,t.v );
		break;
		case MP_LUA_AIMoveX:
			t.tsteamPosX = t.v;
		break;
		case MP_LUA_AIMoveZ:
			AISetEntityPosition (  t.e, t.tsteamPosX, BT_GetGroundHeight(t.terrain.TerrainID,t.tsteamPosX,t.v),t.v );
		break;
		case MP_LUA_SendAvatar:
			t.tsteams_s = SteamGetLuaS();
			t.mp_playerAvatars_s[t.e] = t.tsteams_s;
		break;
		case MP_LUA_SendAvatarName:
			t.tsteams_s = SteamGetLuaS();
			t.mp_playerAvatarOwners_s[t.e] = t.tsteams_s;
		break;
		}	//~   
	
		t.tLuaDontSendLua = 0;
	
		SteamGetNextLua (  );
	}

return;

}

void mp_delete_entities ( void )
{

	g.mp.ignoreDamageToEntity = 1;
	while (  SteamGetDeleteList() ) 
	{
		t.ttte = SteamGetDeleteObjectNumber();
		if (  t.ttte  <=  g.entityelementlist ) 
		{
			t.tobj = t.entityelement[t.ttte].obj;
			if (  t.tobj > 0 ) 
			{
				if (  ObjectExist(t.tobj)  ==  1 ) 
				{
					t.tdamage = t.entityelement[t.ttte].health;
					t.tdamageforce = 0;
					t.tdamagesource = 0;
					t.brayx1_f = ObjectPositionX(t.tobj);
					t.brayy1_f = ObjectPositionY(t.tobj);
					t.brayz1_f = ObjectPositionZ(t.tobj);
					t.brayx2_f = ObjectPositionX(t.tobj);
					t.brayy2_f = ObjectPositionY(t.tobj);
					t.brayz2_f = ObjectPositionZ(t.tobj);
	
					t.entityelement[t.ttte].mp_networkkill = 1;
					t.entityelement[t.ttte].mp_killedby = SteamGetDeleteSource();
					t.entityelement[t.ttte].health = 0;
	
					entity_applydamage ( );
				}
			}
		}
	
		SteamGetNextDelete (  );
	}
	g.mp.ignoreDamageToEntity = 0;

	while (  SteamGetDestroyList() ) 
	{
		t.ttte = SteamGetDestroyObjectNumber();
		if (  t.ttte  <=  g.entityelementlist ) 
		{
			t.tobj = t.entityelement[t.ttte].obj;
			if (  t.tobj > 0 ) 
			{
				if (  ObjectExist(t.tobj)  ==  1 ) 
				{
					t.entityelement[t.ttte].destroyme=1;
				}
			}
		}
		SteamGetNextDestroy (  );
	}
}

void mp_pre_game_file_sync ( void )
{
	if (  g.mp.isGameHost  ==  1 ) 
	{
		mp_pre_game_file_sync_server ( 0 );
	}
	else
	{
		mp_pre_game_file_sync_client ( );
	}
}

void mp_pre_game_file_sync_server ( int iOnlySendMapToSpecificPlayer )
{
//  if we have lost connection, head back to main menu
t.tconnectionStatus = SteamGetClientServerConnectionStatus();
if (  t.tconnectionStatus  ==  0 ) 
{
	t.tsteamconnectionlostmessage_s = "Lost Connection";
	mp_lostConnection ( );
	return;
}

mp_sendAvatarInfo ( );
//  check if we have finished sending and receiving textures with the server
//  (the actual process is handled by steam dll)
if (  g.mp.isGameHost  ==  0 || g.mp.me  !=  0  )  return;
if (  SteamCheckSyncedAvatarTexturesWithServer()  ==  0 ) 
{
	t.tstring_s = "Syncing Avatars";
	mp_textDots(-1,50,3,t.tstring_s.Get());
	return;
}

	switch (  g.mp.syncedWithServerMode ) 
	{

		case 0:
			//  for solo testing to prevent sending files
			if (  g.mp.usersInServersLobbyAtServerCreation  ==  1 ) 
			{
				g.mp.syncedWithServerMode = 3;
				return;
			}
	
			g.mp.serverusingworkshop = 0;
	
			SteamSetSendFileCount (  1 );
			if (  g.mp.levelContainsCustomContent  ==  0 ) 
			{
				SteamSendFileBegin (  1,"__multiplayerlevel__.fpm" );
				g.mp.serverusingworkshop = 1;
			}
			else
			{
				t.tempsteamfiletosend_s = g.mysystem.editorsGrideditAbs_s+"__multiplayerworkshopitemid__.dat";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat";
				if (  FileExist (t.tempsteamfiletosend_s.Get())  ==  1  )  DeleteAFile (  t.tempsteamfiletosend_s.Get() );
				if (  FileOpen(1)  )  CloseFile (  1 );
				OpenToWrite (  1,t.tempsteamfiletosend_s.Get() );
				WriteString (  1,g.mp.workshopid.Get() );
				CloseFile (  1 );
				SteamSendFileBegin (  1,"__multiplayerworkshopitemid__.dat" );
			}
			g.mp.syncedWithServerMode = 1;
			mp_textDots(-1,30,3,"Setting up data for clients")  ;    

		break;

		case 1:
			if (  SteamSendFileDone()  ==  1 ) 
			{
					g.mp.syncedWithServerMode = 2;
			}
				mp_textDots(-1,50,3,"Sending data to clients");
		break;

		case 2:
			mp_textDots(-1,30,3,"Waiting for clients to receive data");

			if (  SteamIsEveryoneFileSynced()  ==  1 ) 
			{
			//the client hosting the server needs to have loaded everything in also
				SteamSendIAmLoadedAndReady (  );
				g.mp.syncedWithServerMode = 3;
				g.mp.oldtime = Timer();
			}
		break;

	case 3:
		if (  SteamIsEveryoneLoadedAndReady()  ==  1 ) 
		{
			if (  g.mp.serverusingworkshop  ==  1 ) 
			{
					mp_textDots(-1,30,3,"Waiting for clients to receive data");
					if (  Timer() - g.mp.oldtime > 3000 ) 
					{
						g.mp.oldtime = Timer();
						g.mp.syncedWithServer = 1;
						g.mp.syncedWithServerMode = 99;
						SetDir (  t.toldsteamfolder_s.Get() );
						SetDir (  g.mp.originalpath.Get() );
					}
				}
				else
				{
					g.mp.oldtime = Timer();
					g.mp.syncedWithServer = 1;
					g.mp.syncedWithServerMode = 99;
					SetDir (  t.toldsteamfolder_s.Get() );
					SetDir (  g.mp.originalpath.Get() );
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				if (  Timer() - t.tempMPsendingready > 2000 ) 
				{
					SteamSendIAmLoadedAndReady (  );
					t.tempMPsendingready = Timer();
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Waiting for everyone to be ready" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
	break;
	}//~ ` 

return;

}

void mp_pre_game_file_sync_client ( void )
{
	//  if we have lost connection, head back to main menu
	t.tconnectionStatus = SteamGetClientServerConnectionStatus();
	if (  t.tconnectionStatus  ==  0 ) 
	{
		t.tsteamconnectionlostmessage_s = "Lost Connection";
		mp_lostConnection ( );
		return;
	}

	mp_sendAvatarInfo ( );
	//  check if we have finished sending and receiving textures with the server
	//  (the actual process is handled by steam dll)
	if (  g.mp.isGameHost  ==  1 || g.mp.me  ==  0  )  return;
	if (  SteamCheckSyncedAvatarTexturesWithServer()  ==  0 ) 
	{
		t.tstring_s = "Syncing Avatars";
		mp_textDots(-1,50,3,t.tstring_s.Get());
		return;
	}

	if ( SteamGetClientServerConnectionStatus()  ==  0 ) 
	{
		t.tsteamlostconnectioncustommessage_s = "Lost connect to server (Error MP010)";
		g.mp.backtoeditorforyou = 0;
		g.mp.mode = 0;
		mp_lostConnection ( );
		return;
	}

	switch (  g.mp.syncedWithServerMode ) 
	{
		case 0:
			if (  SteamAmIFileSynced()  ==  1 ) 
			{
				t.tempMPshopidfile_s = g.mysystem.editorsGrideditAbs_s+"__multiplayerworkshopitemid__.dat";//g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat";
				if (  FileExist(t.tempMPshopidfile_s.Get()) ) 
				{
					if (  FileOpen(10)  ==  1  )  CloseFile (  10 );
					OpenToRead (  10,t.tempMPshopidfile_s.Get() );
					g.mp.workshopid = ReadString ( 10 );
					CloseFile (  10 );
					cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
					if (  FileExist( mlevel_s.Get() ) )  DeleteAFile ( mlevel_s.Get() );
					SteamDownloadWorkshopItem (  g.mp.workshopid.Get() );
					g.mp.syncedWithServerMode = 2;
				}
				else
				{
					g.mp.fileLoaded = 1;
					SteamSendIAmLoadedAndReady (  );
					g.mp.syncedWithServerMode = 1;
				}
			}
			else
			{
				t.tProgress = SteamGetFileProgress();
				t.tstring_s = cstr("Receiving '")+g.mp.levelnametojoin+"': " + Str(t.tProgress) + "%";
				mp_text(-1,85,3,t.tstring_s.Get());
			}
		break;

		case 1:
			if (  SteamIsEveryoneLoadedAndReady()  ==  1 ) 
			{
				g.mp.syncedWithServer = 1;
				g.mp.syncedWithServerMode = 99;
				SetDir (  t.toldsteamfolder_s.Get() );
				SetDir (  g.mp.originalpath.Get() );
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				if (  Timer() - t.tempMPsendingready > 2000 ) 
				{
					SteamSendIAmLoadedAndReady (  );
					t.tempMPsendingready = Timer();
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Waiting for everyone to be ready" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		break;

		case 2:
			if (  SteamIsWorkshopItemDownloaded()  ==  -1 ) 
			{
				t.tsteamconnectionlostmessage_s = "Unable to join, Steam does not yet have all the files needed (Error MP011)";
				mp_lostConnection ( );
				return;
			}
			if (  SteamIsWorkshopItemDownloaded()  ==  1 ) 
			{
				cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
				if ( FileExist( mlevel_s.Get() ) ) 
				{
					g.mp.fileLoaded = 1;
					SteamSendIAmLoadedAndReady (  );
					g.mp.syncedWithServerMode = 1;
				}
				else
				{
					t.tsteamconnectionlostmessage_s = "Unable to join, Steam does not yet have all the files needed (Error MP012)";
					mp_lostConnection ( );
					return;
				}
			}
			else
			{
				if (  Timer() - g.mp.oldtime > 150 ) 
				{
					g.mp.oldtime = Timer();
					t.tSteamBuildingWorkshopItem_s = t.tSteamBuildingWorkshopItem_s + ".";
					if (  Len(t.tSteamBuildingWorkshopItem_s.Get()) > 5  )  t.tSteamBuildingWorkshopItem_s  =  ".";
				}
				t.tstring_s = t.tSteamBuildingWorkshopItem_s + "Downloading Workshop Item" + t.tSteamBuildingWorkshopItem_s;
				mp_text(-1,50,3,t.tstring_s.Get());
				t.tstring_s = "";
			}
		break;

	} 
	return;
}

void mp_sendAvatarInfo ( void )
{
		if (  g.mp.haveSentMyAvatar  ==  0 ) 
		{
			g.mp.me = SteamGetMyPlayerIndex();
			if (  g.mp.isGameHost  ==  1 || g.mp.me  !=  0 ) 
			{
				g.mp.haveSentMyAvatar = 1;
				SteamSendLuaString (  MP_LUA_SendAvatarName,g.mp.me,SteamGetPlayerName() );
				SteamSendLuaString (  MP_LUA_SendAvatar,g.mp.me,g.mp.myAvatar_s.Get() );

				//  store our own info for loading in our avatar
				t.mp_playerAvatarOwners_s[g.mp.me] = SteamGetPlayerName();
				t.mp_playerAvatars_s[g.mp.me] = g.mp.myAvatar_s;
				//  send out custom texture (mp.myAvatarHeadTexture$ will be "" if we don't have one)
				SteamSetMyAvatarHeadTextureName (  g.mp.myAvatarHeadTexture_s.Get() );
			}
		}
//  `endif

	mp_lua ( );
return;

}

void mp_animation ( void )
{

	while (  SteamGetAnimationList() ) 
	{
		t.tEnt = SteamGetAnimationIndex();
		t.astart = SteamGetAnimationStart();
		t.aend = SteamGetAnimationEnd();
		t.aspeed = SteamGetAnimationSpeed();
	
		SetObjectSpeed (  t.entityelement[t.tEnt].obj,t.aspeed );
		PlayObject (  t.entityelement[t.tEnt].obj,t.astart,t.aend );

		SteamGetNextAnimation (  );
	}

return;
//  Send our position and angle to steam
}

void mp_update_player ( void )
{
if (  g.mp.endplay  ==  1  )  return;

// once we are alive, no immunity
t.huddamage.immunity = 1000;
if (  Timer() - g.mp.invincibleTimer > 6000 ) 
{
	t.huddamage.immunity = 0;
}
else
{
	t.huddamage.immunity = 1000;
	t.tthrowawaythisdamage = SteamGetPlayerDamageAmount();
}
//  check if we have taken damage
t.tdamage = SteamGetPlayerDamageAmount();
// `if player(plrid).health > 100 then player(plrid).health  ==  100

if (  t.tdamage > 0 ) 
{

	t.tsteamlastdamageincounter = t.tsteamlastdamageincounter + 1;
	//  Receives; tdamage, te, tDrownDamageFlag
	t.te = t.mp_playerEntityID[SteamGetPlayerDamageSource()];
	t.tDrownDamageFlag = 0;
	physics_player_takedamage ( );

	if (  t.player[t.plrid].health  <=  0 ) 
	{
		g.mp.killedByPlayerFlag = 1;
		g.mp.playerThatKilledMe = SteamGetPlayerDamageSource();
		t.tsteamforce = SteamGetPlayerDamageForce();
		SteamKilledBy (  g.mp.playerThatKilledMe , SteamGetPlayerDamageX(), SteamGetPlayerDamageY(), SteamGetPlayerDamageZ(), t.tsteamforce, SteamGetPlayerDamageLimb() );


		g.mp.dyingTime = Timer();
	}
}

t.mp_health[g.mp.me] = t.player[t.plrid].health;

	//  check if we have changed guns
	if (  g.mp.gunid  !=  t.gunid ) 
	{
		//  send a server message saying we have a new gun
		t.tfound = 0;
		for ( t.ti = 0 ; t.ti<=  g.mp.gunCount; t.ti++ )
		{
			if (  t.mp_gunname[t.ti]  ==  Lower(t.gun[t.gunid].name_s.Get()) ) 
			{
				t.tfound = t.ti+1;
			}
		}
		if (  t.tfound>0 ) 
		{
			t.hasgunname_s=t.gun[t.gunid].name_s;
			t.steamhasgunname_s=t.mp_gunname[t.tfound-1];
			g.mp.appearance = t.tfound;
			t.toldappearancevariable = t.tfound;
//    `mp.gunid = gunid

		}
		else
		{
			g.mp.appearance = 0;
		}
		g.mp.gunid = t.gunid;
	}

	//  Send our positional data to the server
//  `t.tTime = Timer()

//  `if t.tTime - mp.lastSendPositionTime < MP_POSITION_UPDATE_DELAY then return
	
	
//  `mp.lastSendPositionTime = t.tTime

	
	SteamSetPlayerPositionX (  CameraPositionX() );
	g.mp.lastx = CameraPositionX();

		if (  g.mp.crouchOn  ==  0 ) 
		{
			SteamSetPlayerPositionY (  CameraPositionY()-64 );
			g.mp.lasty = CameraPositionY()-64;
		}
		else
		{
			SteamSetPlayerPositionY (  CameraPositionY()-64+30 );
			g.mp.lasty = CameraPositionY()-64+30;
		}

	SteamSetPlayerPositionZ (  CameraPositionZ() );
	g.mp.lastz = CameraPositionZ();

	SteamSetPlayerAngle (  CameraAngleY() );
	g.mp.lastangley = CameraAngleY();

	t.tpe = t.mp_playerEntityID[g.mp.me];
	t.entityelement[t.tpe].x=g.mp.lastx;
	t.entityelement[t.tpe].y=g.mp.lasty;
	t.entityelement[t.tpe].z=g.mp.lastz;
	if (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
	{
		if (  ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
		{
			PositionObject (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj, g.mp.lastx, g.mp.lasty+10, g.mp.lastz );
		}
	}
	t.te = t.tpe;
	t.tolde = t.e;
	t.e = t.tpe;
	entity_updatepos ( );
	entity_lua_rotateupdate ( );
	t.e = t.tolde;

return;

}

void mp_updatePlayerPositions ( void )
{

	if (  g.mp.endplay  ==  1  )  return;

	//  Get player data from the server
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

		if (  t.mp_forcePosition[t.c] > 0 && SteamGetPlayerAlive(t.c)  ==  1 ) 
		{
			if (  t.mp_forcePosition[t.c]  ==  1  )  t.mp_forcePosition[t.c]  =  Timer();
			if (  Timer() - t.mp_forcePosition[t.c] > 1000 ) 
			{
				t.mp_forcePosition[t.c] = 0;
				t.x_f = SteamGetPlayerPositionX(t.c);
				t.y_f = SteamGetPlayerPositionY(t.c);
				t.z_f = SteamGetPlayerPositionZ(t.c);
				SteamSetTweening (  t.c,1 );
			}
			else
			{
				SteamSetTweening (  t.c,0 );
			}

			t.x_f = SteamGetPlayerPositionX(t.c);
			t.y_f = SteamGetPlayerPositionY(t.c);
			t.z_f = SteamGetPlayerPositionZ(t.c);
			t.angle_f = SteamGetPlayerAngle(t.c);

		}
		//  Get other players tweened positional data
		t.x_f = SteamGetPlayerPositionX(t.c);
		t.y_f = SteamGetPlayerPositionY(t.c);
		t.z_f = SteamGetPlayerPositionZ(t.c);
		t.angle_f = SteamGetPlayerAngle(t.c);
		if (  t.c  !=  g.mp.me ) 
		{
			if (  SteamGetPlayerAlive(t.c)  ==  1 && t.mp_forcePosition[t.c]  ==  0 ) 
			{
				t.e = t.mp_playerEntityID[t.c];
				t.entityelement[t.e].x=t.x_f;
				t.entityelement[t.e].y=t.y_f;
				t.entityelement[t.e].z=t.z_f;
				t.entityelement[t.e].ry=t.angle_f;
				PositionObject (  t.entityelement[t.e].obj, t.entityelement[t.e].x, t.entityelement[t.e].y, t.entityelement[t.e].z );
				t.te = t.e;
				entity_updatepos ( );
				entity_lua_rotateupdate ( );
			}
		}
	}

return;

//  Display message from server
}

void mp_server_message ( void )
{

if (  g.mp.endplay  ==  1  )  return;

t.s_s = SteamGetServerMessage();
if (  g.mp.coop  ==  1 ) 
{
	t.tplayer_s = FirstToken(t.s_s.Get()," ");
	t.tcheckforkilled_s = NextToken(" ");
	if (  t.tcheckforkilled_s  ==  "was" || t.tcheckforkilled_s  ==  "killed"  )  t.s_s  =  t.tplayer_s + " died!";
}
if (  t.s_s  !=  "" ) 
{
	t.tsteamdisplaymessagetimer = Timer();
	t.s_s = Upper(t.s_s.Get());
}
if (  t.s_s  ==  ""  )  t.s_s  =  g.mp.previousMessage_s;
g.mp.previousMessage_s = t.s_s;
if (  Timer() - t.tsteamdisplaymessagetimer < 2000  )  mp_text(-1,10,3,t.s_s.Get());
// `text GetDisplayWidth()/2 - Text (  width(s$)/2, 100, s$ )


return;

}

void mp_updatePlayerNamePlates ( void )
{

//  `if mp.endplay  ==  1 then return
	

	if (  g.mp.nameplatesOff  ==  1 ) 
	{
		for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
		{
			if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
			{
				PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
			}
		}
		return;
	}
	//  Display players names and stats
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
			//  if it isnt me, display their details above their head
			if (  g.mp.sentmyname  ==  1 ) 
			{
				if (  ObjectExist(g.steamplayermodelsoffset+500+t.c)  ==  1  )  DeleteObject (  g.steamplayermodelsoffset+500+t.c );
			}
			if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
			{
				PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
			}

			if (  t.entityelement[t.mp_playerEntityID[t.c]].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].obj)  ==  1 ) 
				{
					if (  t.c  !=  g.mp.me ) 
					{
						if (  t.mp_forcePosition[t.c]  ==  0 && SteamGetPlayerAlive(t.c)  ==  1 ) 
						{
							if (  GetInScreen(t.entityelement[t.mp_playerEntityID[t.c]].obj) ) 
							{
								t.tname_s = SteamGetOtherPlayerName(t.c);
								if (  t.tname_s != "Player" ) 
								{
									t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
									if (  ObjectExist(g.steamplayermodelsoffset+500+t.c)  ==  0 ) 
									{
										t.tResult = MakeNewObjectPanel(g.steamplayermodelsoffset+500+t.c,Len(t.tname_s.Get()));
										if (  t.tResult ) 
										{
											t.index = 3;
											t.twidth=0;
											for ( t.n = 1 ; t.n<=  Len(t.tname_s.Get()); t.n++ )
											{
												t.charindex=Asc(Mid(t.tname_s.Get(),t.n));
												t.twidth += t.bitmapfont[t.index][t.charindex].w;
											}
											t.tx = -(t.twidth/2.0);

											t.timg = g.bitmapfontimagetart+t.index;
											for ( t.n = 1 ; t.n<=  Len(t.tname_s.Get()); t.n++ )
											{
												t.charindex=Asc(Mid(t.tname_s.Get(),t.n));
												t.u1_f=t.bitmapfont[t.index][t.charindex].x1;
												t.v1_f=t.bitmapfont[t.index][t.charindex].y1;
												t.u2_f=t.bitmapfont[t.index][t.charindex].x2;
												t.v2_f=t.bitmapfont[t.index][t.charindex].y2;
												t.r = 255;
												t.g = 50;
												t.b = 50;
												if (  g.mp.team  ==  1 ) 
												{
													if (  t.mp_team[t.c]  ==  t.mp_team[g.mp.me] ) 
													{
														t.r = 100;
														t.g = 255;
														t.b = 100;
													}
												}
												SetObjectPanelQuad (  g.steamplayermodelsoffset+500+t.c,t.n-1,t.tx,0,t.bitmapfont[t.index][t.charindex].w,t.bitmapfont[t.index][t.charindex].h,t.u1_f,t.v1_f,t.u2_f,t.v2_f,t.r,t.g,t.b );
												t.tx += t.bitmapfont[t.index][t.charindex].w;
											}
											FinishObjectPanel (  g.steamplayermodelsoffset+500+t.c,32,10 );

											SetCharacterCreatorTones (  g.steamplayermodelsoffset+500+t.c,0,t.r,t.g,t.b,1.0 );
											SetObjectLight (  g.steamplayermodelsoffset+500+t.c,0 );
											YRotateObject (  g.steamplayermodelsoffset+500+t.c,180 );
											FixObjectPivot (  g.steamplayermodelsoffset+500+t.c );
											SetObjectTransparency (  g.steamplayermodelsoffset+500+t.c, 6 );
											ScaleObject (  g.steamplayermodelsoffset+500+t.c,60,60,100 );
											SetSphereRadius (  g.steamplayermodelsoffset+500+t.c,0 );
											SetObjectMask (  g.steamplayermodelsoffset+500+t.c, 1 );
											//  apply special overlay_basic shader which also handles depth render for DOF avoidance
											t.teffectid=loadinternaleffect("effectbank\\reloaded\\overlay_basic.fx");
											TextureObject (  g.steamplayermodelsoffset+500+t.c,t.timg );
											SetObjectEffect (  g.steamplayermodelsoffset+500+t.c,t.teffectid );
										}
									}
									else
									{
//          `show it

										if (  SteamGetPlayerAlive(t.c)  ==  1 && g.mp.endplay  ==  0 ) 
										{
											t.tnameplatey_f = ObjectPositionY(t.tobj)+ ObjectSizeY(t.tobj,1);
											if (  t.mp_playerAvatars_s[t.c]  !=  ""  )  t.tnameplatey_f  =  t.tnameplatey_f + 15.0;
											ShowObject (  g.steamplayermodelsoffset+500+t.c );
											PositionObject((g.steamplayermodelsoffset+500+t.c), ObjectPositionX(t.tobj), t.tnameplatey_f , ObjectPositionZ(t.tobj));
											PointObject (  g.steamplayermodelsoffset+500+t.c,CameraPositionX(), CameraPositionY(), CameraPositionZ() );
										}
										else
										{
											HideObject (  g.steamplayermodelsoffset+500+t.c );
										}
									}
								}
							}
						}
						else
						{
							if (  ObjectExist(g.steamplayermodelsoffset+500+t.c) ) 
							{
								PositionObject (  g.steamplayermodelsoffset+500+t.c,500000,-500000,500000 );
							}
						}
					}
				}
			}
		}

		g.mp.sentmyname = 0;

return;

}

void mp_updatePlayerAnimations ( void )
{

	//  Update animations
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

			t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

			t.thasNade = 0;
			t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
			if ( t.gun[t.tgunid].projectileframe != 0 ) t.thasNade = 1;

			t.mp_playerShooting[t.c] = SteamGetShoot(t.c);

			//  if the player is reloading we will try and show it (only works if idle or ducking at present)
			if (  SteamGetPlayerAppearance(t.c)  ==  201  )  t.mp_reload[t.c]  =  1;

			//  update animations
			g.mp.isAnimating = 0;
			if (  SteamGetPlayerAlive(t.c)  ==  1 ) 
			{
					t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
					RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );

				if (  (SteamGetPlayerAppearance(t.c) < 102 || SteamGetPlayerAppearance(t.c) > 200) ) 
				{
					//  Melee
					if (  SteamGetKeyState(t.c,16)  ==  1 || t.mp_meleePlaying[t.c]  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						if (  t.mp_meleePlaying[t.c]  ==  0 ) 
						{
							t.mp_meleePlaying[t.c] = 1;
						}
						else
						{
							if (  GetPlaying(t.tobj)  ==  0  )  t.mp_meleePlaying[t.c]  =  0;
							if (  GetLooping(t.tobj)  ==  1  )  t.mp_meleePlaying[t.c]  =  0;
						}
					}
					//  Forwards
					if (  SteamGetKeyState(t.c,17)  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						//  are they moving left also
						if (  SteamGetKeyState(t.c,30)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						//  or perhaps they are moving right also
						if (  SteamGetKeyState(t.c,32)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						if (  SteamGetKeyState(t.c,42)  ==  0 || SteamGetKeyState(t.c,46)  ==  1 ) 
						{
							if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=300;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
						}
						else
						{
							if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=600;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=200;
							}
						}
						if (  SteamGetKeyState(t.c,46)  ==  0 ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_WALKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex=t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
								}
								mp_switchAnim ( );

								if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
								{
									t.tplaycsioranimindex=g.csi_unarmedANIM0;
									mp_switchAnim ( );
								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_WALKING;
							}
						}
						else
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
							}
						}
					}
					//  Backwards
					if (  SteamGetKeyState(t.c,31)  ==  1 ) 
					{
						g.mp.isAnimating = 1;
						//  are they moving left also
						if (  SteamGetKeyState(t.c,30)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						//  or perhaps they are moving right also
						if (  SteamGetKeyState(t.c,32)  ==  1 ) 
						{
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
						}
						if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-300;
						}
						else
						{
							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=-100;
						}
						if (  SteamGetKeyState(t.c,46)  ==  0 ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_WALKINGBACKWARDS ) 
							{
								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
								{
									t.tplaycsioranimindex=t.csi_stoodmoverunANIM[t.tweapstyle];
								}
								else
								{
									t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
								}
								mp_switchAnim ( );

								if (  SteamGetPlayerAppearance(t.c)  ==  101 ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex=t.csi_stoodnormalANIM[t.tweapstyle];
									mp_switchAnim ( );
								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_WALKINGBACKWARDS;
							}
						}
						else
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKINGBACKWARDS ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoverunANIM[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								entity_lua_loopanimation ( );
								g.mp.isAnimating = 1;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKINGBACKWARDS;
							}
						}
					}

					//  strafe left
					if (  SteamGetKeyState(t.c,30)  ==  1 ) 
					{
						if (  g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  SteamGetKeyState(t.c,42)  ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if (  SteamGetKeyState(t.c,46)  ==  1 ) 
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchmoveleftANIM[t.tweapstyle];
								mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFELEFT ) 
								{
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex=t.csi_stoodmoverunleftANIM[t.tweapstyle];
									}
									else
									{
										RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
										t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
									}
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFELEFT;
								}
							}
						}
					}

					//  strafe right
					if (  SteamGetKeyState(t.c,32)  ==  1 ) 
					{
						if (  g.mp.isAnimating  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  SteamGetKeyState(t.c,42)  ==  0 ) 
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=100;
							}
							else
							{
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.animspeed=150;
							}
							if (  SteamGetKeyState(t.c,46)  ==  1 ) 
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKINGWALKING ) 
								{

									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.tweapstyle  ==  0  )  t.tweapstyle  =  1;
									t.tplaycsioranimindex=t.csi_crouchmoverightANIM[t.tweapstyle];
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
									g.mp.isAnimating = 1;
									t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKINGWALKING;
								}
							}
							else
							{
								if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_STRAFERIGHT ) 
								{

									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{
										t.tplaycsioranimindex=t.csi_stoodmoverunrightANIM[t.tweapstyle];
									}
									else
									{
										t.tplaycsioranimindex=g.csi_unarmedmoverunANIM;
									}
									mp_switchAnim ( );

									entity_lua_setanimationframes ( );
									t.e = t.mp_playerEntityID[t.c];
									entity_lua_loopanimation ( );
								
									t.mp_playingAnimation[t.c] = MP_ANIMATION_STRAFERIGHT;
								}
							}
						}
					}

					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFELEFT ) 
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) - 45 );
						}
					}

					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_STRAFERIGHT ) 
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj  ==  0 ) 
						{
							RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX(t.tobj,t.spinelimbofcharacter),-45,LimbAngleZ(t.tobj,t.spinelimbofcharacter) );
							YRotateObject (  t.tobj, ObjectAngleY(t.tobj) + 45 );
						}
					}


					//  Ducking
					if (  SteamGetKeyState(t.c,46)  ==  1 && t.mp_jetpackOn[t.c]  ==  0 ) 
					{
						if (  g.mp.isAnimating  ==  0 && t.mp_reload[t.c]  ==  0 ) 
						{
							g.mp.isAnimating = 1;
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_DUCKING ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchidlenormalANIM1[t.tweapstyle];
								mp_switchAnim ( );

								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								t.entityelement[t.e].eleprof.animspeed=100;
								entity_lua_playanimation ( );
								g.mp.isAnimating = 1;

								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKING;

							}
						}
					}

				}

				if (  t.thasNade  ==  1 ) 
				{
					if (  t.mp_reload[t.c]  ==  1  )  t.mp_reload[t.c]  =  0;
				}

				if (  t.mp_reload[t.c]  ==  1 ) 
				{
						if (  g.mp.isAnimating  ==  0 || t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_DUCKING ) 
						{
							if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_RELOAD ) 
							{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_stoodreloadANIM[t.tweapstyle];
								mp_switchAnim ( );

								if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_DUCKING ) 
								{

								t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
								t.tweapstyle=t.gun[t.tgunid].weapontype;
								if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
								t.tplaycsioranimindex=t.csi_crouchreloadANIM[t.tweapstyle];
								mp_switchAnim ( );

								}
								entity_lua_setanimationframes ( );
								t.e = t.mp_playerEntityID[t.c];
								t.entityelement[t.e].eleprof.animspeed=200;
								entity_lua_playanimation ( );
								g.mp.isAnimating = 1;

								t.mp_playingAnimation[t.c] = MP_ANIMATION_RELOAD;
							}
						}
						g.mp.isAnimating = 1;
						//  if the reload anim has finished or the player starts shooting, turn reloading off
						if (  GetFrame(t.tobj)  ==  605 || GetFrame(t.tobj)  ==  2010 || t.mp_playerShooting[t.c]  ==  1 ) 
						{
							t.mp_reload[t.c] = 0;
							if (  GetFrame(t.tobj)  ==  2010 ) 
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_DUCKING;
							}
							else
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
								g.mp.isAnimating = 0;
							}
//        `print "DUCKING SPOT 2 <----------------------------------------------"

						}
				}

				t.tjetpacktempanim = 0;
				if (  SteamGetPlayerAppearance(t.c)  ==  102 ) 
				{
					t.tjetpacktempanim = 1;
					if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_IDLE ) 
					{
						t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						g.mp.isAnimating = 0;
					}
				}

				if (  t.thasNade  ==  1 ) 
				{
					if (  t.mp_playingAnimation[t.c]  ==  MP_ANIMATION_IDLE ) 
					{
						if (  t.mp_playerShooting[t.c]  ==  1 ) 
						{
							if (   GetFrame(t.tobj) < 2390 || GetFrame(t.tobj) > 2444 ) 
							{
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
								g.mp.isAnimating = 0;
							}
						}

						if (  t.mp_playerShooting[t.c]  ==  0 ) 
						{
							if (  GetFrame(t.tobj)  ==  2444 ) 
							{
								SetObjectFrame(t.tobj,2443);
								StopObject (  t.tobj );
								g.mp.isAnimating = 0;
								t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
							}
						}

					}
				}

				if (  g.mp.isAnimating  ==  0 ) 
				{
					mp_update_waist_rotation ( );
//      `print "IDLING"

					if (  t.mp_playingAnimation[t.c]  !=  MP_ANIMATION_IDLE ) 
					{
						if (  abs(t.mp_oldplayerx[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].x) < 1.0 || t.tjetpacktempanim  ==  1 ) 
						{
							if (  abs(t.mp_oldplayery[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].y) < 1.0 || t.tjetpacktempanim  ==  1 ) 
							{
								if (  abs(t.mp_oldplayerz[t.c] - t.entityelement[t.mp_playerEntityID[t.c]].z) < 1.0 || t.tjetpacktempanim  ==  1 ) 
								{
									t.tIsThrowingNade = 0;
									t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
									t.tweapstyle=t.gun[t.tgunid].weapontype;
									if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
									if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 && t.thasNade  ==  0 ) 
									{

										t.tplaycsioranimindex=t.csi_stoodnormalANIM[t.tweapstyle];
										mp_switchAnim ( );

									}
									else
									{

										if (  t.thasNade  ==  1 && t.mp_playerShooting[t.c]  ==  1 ) 
										{
											t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
											t.e=2390;
											t.v=2444;
											entity_lua_setanimationframes ( );
											t.e = t.mp_playerEntityID[t.c];
											t.entityelement[t.e].eleprof.animspeed=200;
											t.tLuaDontSendLua = 1;
											t.q=-1;
											entity_lua_playanimation ( );
											t.tLuaDontSendLua = 0;
											t.tIsThrowingNade = 1;
										}
										else
										{
											t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
											t.tweapstyle=t.gun[t.tgunid].weapontype;
											if (  t.tweapstyle > 5  )  t.tweapstyle  =  1;
											t.tplaycsioranimindex=g.csi_unarmedANIM0;
											mp_switchAnim ( );
										}

									}
									if (  t.tIsThrowingNade  ==  0 ) 
									{
										entity_lua_setanimationframes ( );
										t.e = t.mp_playerEntityID[t.c];
										t.entityelement[t.e].eleprof.animspeed=100;
										entity_lua_loopanimation ( );
									}
									t.mp_playingAnimation[t.c] = MP_ANIMATION_IDLE;
								}
							}
						}
					}
				}
				else
				{
					//  reset the idle turn if animating
					t.mp_lastIdleReset[t.c] = 1;
//      `print "last idle reset = 1"

				}

			}

			//  

			if (  SteamGetPlayerAlive(t.c)  ==  0 && g.mp.gameAlreadySpawnedBefore  !=  0 ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				t.mp_lastIdleReset[t.c] = 1;
				t.mp_forcePosition[t.c] = 1;

				if (  t.mp_jetpackparticles[t.c]  !=  -1 ) 
				{
					t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
					ravey_particles_delete_emitter ( );
					t.mp_jetpackparticles[t.c]=-1;
				}

				if (  t.mp_isDying[t.c]  ==  0 && t.mp_playerHasSpawned[t.c]  ==  1 ) 
				{

					t.mp_isDying[t.c] = 1;
					t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
					t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
					RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );
					t.e = t.mp_playerEntityID[t.c];
					if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
					{
						t.tweight=t.entityelement[t.e].eleprof.phyweight;
						t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
						ODECreateDynamicBox (  g.steamplayermodelsoffset+t.c+121,-1,0,t.tweight,t.tfriction,-1 );
					}

					//  NON-CHARACTER, but can still have ragdoll flagged (like Zombies)
					t.ttentid=t.entityelement[t.e].bankindex;
					t.ttte = t.e;
					t.mp_playingRagdoll[t.c] = 1;
					if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
					{
						if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj)  )  DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
						t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
					}
					t.entityprofile[t.ttentid].ragdoll=1;
					if (  t.entityprofile[t.ttentid].ragdoll == 1 ) 
					{

						//  can only ragdoll clones not instances
						t.tte=t.ttte;
						entity_converttoclone ( );

						//  create ragdoll and stop any further manipulation of the object
						t.tphye=t.ttte;
						t.tphyobj=t.entityelement[t.ttte].obj;
						t.oldc = t.c;
						ragdoll_setcollisionmask ( t.entityelement[t.ttte].eleprof.colondeath );
						ragdoll_create ( );
						t.c = t.oldc;

						/*      
						MoveObjectLeft (  t.entityelement[t.ttte].obj,60 );
						t.x_f = ObjectPositionX(t.entityelement[t.ttte].obj);
						t.y_f = ObjectPositionY(t.entityelement[t.ttte].obj)+50;
						t.z_f = ObjectPositionZ(t.entityelement[t.ttte].obj);
						MoveObjectRight (  t.entityelement[t.ttte].obj,60 );
						t.tforce_f = 1000;
						tlimb = Rnd(6);
						switch (  tlimb ) 
						{
						case 0; tlimb  =  8 ; break 
:
						case 1; tlimb  =  10 ; break 
:
						case 2; tlimb  =  12 ; break 
:
						case 3; tlimb  =  16 ; break 
:
						case 4; tlimb  =  17 ; break 
:
						case 5; tlimb  =  24 ; break 
:
						case 6; tlimb  =  25 ; break 
:
						}						//~       remend
						*/    

						//  use the real raycast if we shot them
//       `if Steam Get Player Killed Source(c) = mp.me

//        `ttx# = brayx2#-brayx1#

//        `tty# = brayy2#-brayy1#

//        `ttz# = brayz2#-brayz1#

//        `ttforce# = tforce#

//        `ttlimb = bulletraylimbhit

//       `else

							//  grab the details from the server if someone else shot them
							t.ttx_f = SteamGetPlayerKilledX(t.c);
							t.tty_f = SteamGetPlayerKilledY(t.c);
							t.ttz_f = SteamGetPlayerKilledZ(t.c);
							t.ttforce_f = SteamGetPlayerKilledForce(t.c);
							t.ttlimb = SteamGetPlayerKilledLimb(t.c);
//       `endif

	
						//  and apply bullet directional force (tforce#=from gun settings)
						t.entityelement[t.ttte].ragdollified=1;
//       `entityelement(ttte).ragdollifiedforcex#=(x#)*0.8

//       `entityelement(ttte).ragdollifiedforcey#=(y#)*1.2

//       `entityelement(ttte).ragdollifiedforcez#=(z#)*0.8

						t.entityelement[t.ttte].ragdollifiedforcex_f=(t.ttx_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcey_f=(t.tty_f)*1.2;
						t.entityelement[t.ttte].ragdollifiedforcez_f=(t.ttz_f)*0.8;
						t.entityelement[t.ttte].ragdollifiedforcevalue_f=t.ttforce_f*8000.0;
						t.entityelement[t.ttte].ragdollifiedforcelimb=t.ttlimb;
					}
				}
			}
			else
			{
				if (  t.mp_forcePosition[t.c]  ==  0 ) 
				{
					if (  t.mp_isDying[t.c]  ==  1 ) 
					{
						if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
						{
							ODEDestroyObject (  g.steamplayermodelsoffset+t.c+121 );
							RotateObject (  g.steamplayermodelsoffset+t.c+121,0,0,0 );
							PositionObject (  g.steamplayermodelsoffset+t.c+121,0,-99999,0 );
							HideObject (  g.steamplayermodelsoffset+t.c+121 );
							t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						}
						t.mp_isDying[t.c] = 0;
					}
				}
			}

			t.mp_oldplayerx[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].x;
			t.mp_oldplayery[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].y;
			t.mp_oldplayerz[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].z;

}

return;

}

void mp_switchAnim ( void )
{
	t.ttentid=t.entityelement[t.mp_playerEntityID[t.c]].bankindex;
	t.q=t.entityprofile[t.ttentid].startofaianim;
	t.e=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].start;
	t.v=t.entityanim[t.ttentid][t.q+t.tplaycsioranimindex].finish;
}

void mp_update_waist_rotation ( void )
{
	return;
	t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

	if (  t.mp_lastIdleReset[t.c]  ==  1 ) 
	{
		t.mp_lastIdleY[t.c] = t.entityelement[t.mp_playerEntityID[t.c]].ry;
		t.mp_lastIdleReset[t.c] = 0;
	}

	t.tDifference_f = t.entityelement[t.mp_playerEntityID[t.c]].ry - t.mp_lastIdleY[t.c];
	t.tAmountToRotateSpine_f = t.tDifference_f;
	t.tAmountToRotateObject_f = 0.0;

	if (  t.tAmountToRotateSpine_f > 60.0 ) 
	{
		t.tAmountToRotateObject_f = t.tAmountToRotateSpine_f - 60.0;
		t.tAmountToRotateSpine_f = 60.0;
	}

	if (  t.tAmountToRotateSpine_f < -60.0 ) 
	{
		t.tAmountToRotateObject_f = t.tAmountToRotateSpine_f + 60.0;
		t.tAmountToRotateSpine_f = -60.0;
	}

	YRotateObject (  t.tobj, t.mp_lastIdleY[t.c]+t.tAmountToRotateObject_f );
	t.mp_lastIdleY[t.c] = t.mp_lastIdleY[t.c]+t.tAmountToRotateObject_f;

	t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[t.c]].bankindex].spine;
	RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),t.tAmountToRotateSpine_f,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );

	return;

}

void mp_showdeath ( void )
{

	t.characterkitcontrol.showmyhead = 1;
	if (  g.mp.haveshowndeath  ==  0 ) 
	{
		g.mp.haveshowndeath = 1;
		t.tolddeathcamx_f = 0;
		t.tolddeathcamy_f = 0;
		t.tolddeathcamz_f = 0;
		t.tamountToMoveIn_f = 0.0;
		t.tamountToMoveUp_f = 0.0;
		t.tspawninyoffset_f = 0.0;
		t.tshowdeathlockcam = -1;
		g.mp.spectatorfollowdistance = 200;
		t.tdeathamounttotakeoffdistance = 0;
	}

	//  19032015 - 021 - prevent water affect being triggered when in 3rd person
	visuals_underwater_off ( );

		t.tobjtosee = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
//   `playercontrol.jetobjtouse=hudlayersbankoffset+1

		t.playercontrol.jetpackhidden=0;
		t.playercontrol.jetpackmode=0;
//   `plrzoomin# = 0

//   `plrzoominchange = 1

//   `gunzoommode = 0

		//  new subroutine so steam can reset zoom in
		physics_no_gun_zoom ( );
		if (  g.mp.endplay  ==  1 && g.mp.respawnLeft > 3 ) 
		{
				g.mp.respawnLeft = 100;
				return;
		}
		//  if dead switch to 3rd person view to see the action
			t.e = t.mp_playerEntityID[g.mp.me];
			t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
			if (  t.tobj > 0 ) 
			{
				if ( ObjectExist (t.tobj) ) 
				{
//     `if health(mp.me) <= 0

					ShowObject (  t.tobj );
					t.tpe = t.mp_playerEntityID[g.mp.me];
					if (  g.mp.ragdollon  ==  0 ) 
					{
						g.mp.ragdollon = 1;

					if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
					{

						//  turn off jetpack sound, turn off particles and reset thrust
						if (  SoundExist(t.playercontrol.soundstartindex+18) == 1  )  StopSound (  t.playercontrol.soundstartindex+18 );
						t.playercontrol.jetpackthrust_f=0.0;
						//  stop particle emitter
						if (  t.playercontrol.jetpackparticleemitterindex>0 ) 
						{
							t.tRaveyParticlesEmitterID=t.playercontrol.jetpackparticleemitterindex;
							ravey_particles_delete_emitter ( );
							t.playercontrol.jetpackparticleemitterindex=0;
						}

						t.spinelimbofcharacter=t.entityprofile[t.entityelement[t.mp_playerEntityID[g.mp.me]].bankindex].spine;
						RotateLimb (  t.tobj,t.spinelimbofcharacter,LimbAngleX( t.tobj,t.spinelimbofcharacter),0,LimbAngleZ( t.tobj,t.spinelimbofcharacter) );
						if (  ObjectExist(g.steamplayermodelsoffset+g.mp.me+121)  ==  1 ) 
						{
							t.tweight=t.entityelement[t.e].eleprof.phyweight;
							t.tfriction=t.entityelement[t.e].eleprof.phyfriction;
							ODECreateDynamicBox (  g.steamplayermodelsoffset+g.mp.me+121,-1,0,t.tweight,t.tfriction,-1 );
						}
						t.tme = g.mp.me;

						//  NON-CHARACTER, but can still have ragdoll flagged (like Zombies)
						t.ttentid=t.entityelement[t.e].bankindex;
						t.ttte = t.e;
						t.mp_playingRagdoll[t.tme] = 1;
						if (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj > 0 ) 
						{
							if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj)  )  DeleteObject (  t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj );
							t.entityelement[t.mp_playerEntityID[t.tme]].attachmentobj = 0;
						}
						t.entityprofile[t.ttentid].ragdoll=1;
						if (  t.entityprofile[t.ttentid].ragdoll == 1 ) 
						{

							//  can only ragdoll clones not instances
							t.tte=t.ttte;
							entity_converttoclone ( );

							//  create ragdoll and stop any further manipulation of the object
							t.tphye=t.ttte;
							t.tphyobj=t.entityelement[t.ttte].obj;
							t.oldc = t.c;
							ragdoll_setcollisionmask ( t.entityelement[t.ttte].eleprof.colondeath );
							ragdoll_create ( );
							t.c = t.oldc;

							//  use the real raycast if we shot them
							if (  SteamGetPlayerDamageSource()  ==  g.mp.me ) 
							{
								t.ttx_f = t.brayx2_f-t.brayx1_f;
								t.tty_f = t.brayy2_f-t.brayy1_f;
								t.ttz_f = t.brayz2_f-t.brayz1_f;
								t.ttforce_f = t.tforce_f;
								t.ttlimb = t.bulletraylimbhit;
							}
							else
							{
								//  grab the details from the server if someone else shot them
								t.ttx_f = SteamGetPlayerDamageX();
								t.tty_f = SteamGetPlayerDamageY();
								t.ttz_f = SteamGetPlayerDamageZ();
								t.ttforce_f = SteamGetPlayerDamageForce();
								t.ttlimb = SteamGetPlayerDamageLimb();
							}
	
							//  and apply bullet directional force (tforce#=from gun settings)
							t.entityelement[t.ttte].ragdollified=1;
//        `entityelement(ttte).ragdollifiedforcex#=(x#)*0.8

//        `entityelement(ttte).ragdollifiedforcey#=(y#)*1.2

//        `entityelement(ttte).ragdollifiedforcez#=(z#)*0.8

							t.entityelement[t.ttte].ragdollifiedforcex_f=(t.ttx_f)*0.8;
							t.entityelement[t.ttte].ragdollifiedforcey_f=(t.tty_f)*1.2;
							t.entityelement[t.ttte].ragdollifiedforcez_f=(t.ttz_f)*0.8;
							t.entityelement[t.ttte].ragdollifiedforcevalue_f=t.ttforce_f*8000.0;
//        `entityelement(ttte).ragdollifiedforcelimb=tlimb

							t.entityelement[t.ttte].ragdollifiedforcelimb=t.ttlimb;
//        `bulletraylimbhit=-1


						}
					}
					}
//entity_updatepos ( );
//entity_lua_rotateupdate ( );
					if (  1 ) 
					{
						t.tsteamlimb=t.entityprofile[t.entityelement[t.tpe].bankindex].spine2;
						if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
						{
							if (  g.mp.initialSpawnmoveDownCharacterFlag  ==  1 ) 
							{
								PositionObject (  t.entityelement[t.tpe].obj, ObjectPositionX(t.entityelement[t.tpe].obj), ObjectPositionY(t.entityelement[t.tpe].obj)-50, ObjectPositionZ(t.entityelement[t.tpe].obj) );
								if (  ObjectPositionY(t.entityelement[t.tpe].obj) < BT_GetGroundHeight(t.terrain.TerrainID,ObjectPositionX(t.entityelement[t.tpe].obj),ObjectPositionZ(t.entityelement[t.tpe].obj)) ) 
								{
									PositionObject (  t.entityelement[t.tpe].obj, ObjectPositionX(t.entityelement[t.tpe].obj), BT_GetGroundHeight(t.terrain.TerrainID,ObjectPositionX(t.entityelement[t.tpe].obj),ObjectPositionZ(t.entityelement[t.tpe].obj)) , ObjectPositionZ(t.entityelement[t.tpe].obj) );
								}
								g.mp.initialSpawnmoveDownCharacterFlag = 0;
							}
						}
						t.x_f = LimbPositionX(t.tobjtosee,t.tsteamlimb);
						t.y_f = LimbPositionY(t.tobjtosee,t.tsteamlimb);
						t.z_f = LimbPositionZ(t.tobjtosee,t.tsteamlimb);
						PositionCamera (  t.x_f,t.y_f+100,t.z_f );
						RotateCamera (  0,g.mp.camrotate,0 );
						g.mp.camrotate = g.mp.camrotate + (0.5*g.timeelapsed_f);


						t.x_f = LimbPositionX(t.tobjtosee,t.tsteamlimb);
						t.y_f = LimbPositionY(t.tobjtosee,t.tsteamlimb)+10;
						t.z_f = LimbPositionZ(t.tobjtosee,t.tsteamlimb);

						MoveCamera (  -g.mp.spectatorfollowdistance );

						t.tXOldPos_f = CameraPositionX();
						t.tYOldPos_f = CameraPositionY();
						t.tZOldPos_f = CameraPositionZ();

						MoveCamera (  g.mp.spectatorfollowdistance );
						if (g.lightmappedobjectoffset >= g.lightmappedobjectoffsetfinish)
							t.ttt = IntersectAll(87000, 87000 + g.merged_new_objects - 1, 0, 0, 0, 0, 0, 0, -123);
						else
							t.ttt=IntersectAll(g.lightmappedobjectoffset,g.lightmappedobjectoffsetfinish,0,0,0,0,0,0,-123);
//       `tEndEntity = entityviewstartobj+entityelementlist

						t.tHitObj=IntersectAll(g.entityviewstartobj,g.entityviewendobj,t.x_f,t.y_f,t.z_f,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tobjtosee);
						t.tdistancewecanmovecam_f = g.mp.spectatorfollowdistance;
						if (  t.tHitObj > 0 ) 
						{
							t.tHitX_f = ChecklistFValueA(6);
							t.tHitY_f = ChecklistFValueB(6);
							t.tHitZ_f = ChecklistFValueC(6);
							t.dx_f = t.x_f - t.tHitX_f;
							t.dy_f = t.y_f - t.tHitY_f;
							t.dz_f = t.z_f - t.tHitZ_f;
							t.tdistancewecanmovecam_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f)) - 30;
						}
						MoveCamera (  -t.tdistancewecanmovecam_f );

//       `move camera -mp.spectatorfollowdistance

						PointCamera (  t.x_f,t.y_f,t.z_f );

//       `tEndEntity = entityviewstartobj+entityelementlist


						t.tXOldPos_f = CameraPositionX();
						t.tYOldPos_f = CameraPositionY();
						t.tZOldPos_f = CameraPositionZ();

						t.tXNewPos_f = t.x_f;
						t.tYNewPos_f = t.y_f;
						t.tZNewPos_f = t.z_f;

						//tobjtosee = entityelement(tpe).obj;
						/*      
						t.tHitObj=IntersectAll(g.entityviewstartobj,t.tEndEntity,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tXNewPos_f,t.tYNewPos_f,t.tZNewPos_f,t.entityelement[t.tpe].obj);
						if (  t.tHitObj>0 ) 
						{
							if (  g.mp.spectatorfollowdistance > 10.0 ) 
							{
								g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 10.0;
								g.mp.spectatorfollowdistancedelay = Timer();
							}
						}
						else
						{
							if (  g.mp.spectatorfollowdistance < 200.0 && Timer() - g.mp.spectatorfollowdistancedelay > 1000 ) 
							{
								g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance + 10.0;
							}
						}
						*/    
					}
					else
					{
						t.twhokilledme = SteamGetPlayerDamageSource();
						if (  t.twhokilledme  !=  g.mp.me ) 
						{
							t.tsteamlimb=t.entityprofile[t.entityelement[t.tpe].bankindex].spine2;
							t.x_f = LimbPositionX(t.entityelement[t.tpe].obj,t.tsteamlimb);
							t.y_f = LimbPositionY(t.entityelement[t.tpe].obj,t.tsteamlimb);
							t.z_f = LimbPositionZ(t.entityelement[t.tpe].obj,t.tsteamlimb);
							PositionCamera (  t.x_f,t.y_f+100,t.z_f );
							PointCamera (  SteamGetPlayerPositionX(t.twhokilledme),SteamGetPlayerPositionY(t.twhokilledme)+50,SteamGetPlayerPositionZ(t.twhokilledme) );
							/*      
							tcamheight = (200 - g.mp.spectatorfollowdistance) / 2;
							PositionCamera (  SteamGetPlayerPositionX(t.twhokilledme), SteamGetPlayerPositionY(t.twhokilledme)+tcamheight+50, SteamGetPlayerPositionZ(t.twhokilledme) );
							SteamSetPlayerPositionX (  SteamGetPlayerPositionX(t.twhokilledme) );
							SteamSetPlayerPositionY (  SteamGetPlayerPositionY(t.twhokilledme) );
							SteamSetPlayerPositionZ (  SteamGetPlayerPositionZ(t.twhokilledme) );
							RotateCamera (  0,SteamGetPlayerAngle(t.twhokilledme),0 );
							MoveCamera (  -g.mp.spectatorfollowdistance );
							PointCamera (  SteamGetPlayerPositionX(t.twhokilledme),SteamGetPlayerPositionY(t.twhokilledme)+50,SteamGetPlayerPositionZ(t.twhokilledme) );

							t.tEndEntity = g.entityviewstartobj+g.entityelementlist;

							t.tXOldPos_f = CameraPositionX();
							t.tYOldPos_f = CameraPositionY();
							t.tZOldPos_f = CameraPositionZ();

							t.tXNewPos_f = SteamGetPlayerPositionX(t.twhokilledme);
							t.tYNewPos_f = SteamGetPlayerPositionY(t.twhokilledme);
							t.tZNewPos_f = SteamGetPlayerPositionZ(t.twhokilledme);

							t.tHitObj=IntersectAll(g.entityviewstartobj,t.tEndEntity,t.tXOldPos_f,t.tYOldPos_f,t.tZOldPos_f,t.tXNewPos_f,t.tYNewPos_f,t.tZNewPos_f,t.entityelement[t.tpe].obj);
							if (  t.tHitObj>0 ) 
							{
								if (  g.mp.spectatorfollowdistance > 10.0 ) 
								{
									g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 10.0;
									g.mp.spectatorfollowdistancedelay = Timer();
								}
							}
							else
							{
								if (  g.mp.spectatorfollowdistance < 200.0 && Timer() - g.mp.spectatorfollowdistancedelay > 1000 ) 
								{
									g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance + 10.0;
								}
							}
						*/    
						}
//       `if mp.respawnLeft  ==  5 && toldrespawnleft  ==  6 then mp.spectatorfollowdistance  ==  200
						
						t.toldrespawnleft = g.mp.respawnLeft;
					}
				}
				if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
				{
					if (  CameraPositionY()  <=  BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) ) 
					{
						PositionCamera (  CameraPositionX(), BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) + 50, CameraPositionZ() );
					}
					if (  CameraPositionY() < t.terrain.waterliney_f ) 
					{
						t.tshowdeathlockcam = 0;
					}
					if (  t.tshowdeathlockcam > -1 ) 
					{
						if (  t.tshowdeathlockcam  ==  0 ) 
						{
							PositionCamera (  CameraPositionX(), CameraPositionY() + t.tspawninyoffset_f , CameraPositionZ() );
							if (  CameraPositionY() < t.terrain.waterliney_f ) 
							{
								PositionCamera (  CameraPositionX(), t.terrain.waterliney_f+100 , CameraPositionZ() );
								t.tshowdeathlockcam = 1;
								t.tshowdeathlockcamx_f = CameraPositionX();
								t.tshowdeathlockcamy_f = CameraPositionY();
								t.tshowdeathlockcamz_f = CameraPositionZ();
								t.tshowdeathlockcamrotx_f = CameraAngleX();
								t.tshowdeathlockcamroty_f = CameraAngleY();
								t.tshowdeathlockcamrotz_f = CameraAngleZ();
							}
						}
						else
						{
							PositionCamera (  t.tshowdeathlockcamx_f,t.tshowdeathlockcamy_f,t.tshowdeathlockcamz_f );
							RotateCamera (  t.tshowdeathlockcamrotx_f,t.tshowdeathlockcamroty_f, t.tshowdeathlockcamrotz_f );
						}
					}
				}
				else
				{
					if (  CameraPositionY()  <=  BT_GetGroundHeight(t.terrain.TerrainID,CameraPositionX(),CameraPositionZ()) ) 
					{
						MoveCamera (  1.0 );
						t.tdeathamounttotakeoffdistance = t.tdeathamounttotakeoffdistance + 20;
					}
					if (  t.tdeathamounttotakeoffdistance > 0 && g.mp.spectatorfollowdistance > 40 ) 
					{
						g.mp.spectatorfollowdistance = g.mp.spectatorfollowdistance - 1.0;
						t.tdeathamounttotakeoffdistance = t.tdeathamounttotakeoffdistance - 1;
					}
				}

			}

			//  update any character creator people
			t.entityelement[t.mp_playerEntityID[g.mp.me]].x = ObjectPositionX(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].y = ObjectPositionY(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			t.entityelement[t.mp_playerEntityID[g.mp.me]].z = ObjectPositionZ(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj);
			//  for initial spawn in
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 && ( g.mp.realfirsttimespawn  ==  1 || g.mp.coop  ==  1 ) ) 
			{
				characterkit_checkForCharacters ( );
				characterkit_updateAllCharacterCreatorEntitiesInMapFirstSpawn ( );
			}

return;

}

void mp_respawn ( void )
{

	t.characterkitcontrol.showmyhead = 1;
	if ( g.autoloadgun != 0 ) { g.autoloadgun=0 ; gun_change ( ); }
	if (  t.player[t.plrid].health < 100  )  t.player[t.plrid].health  =  100;
	if (  g.mp.myOriginalSpawnPoint  !=  -1 ) 
	{
		t.tindex = g.mp.me+1;
	}
	else
	{
		t.tindex = g.mp.myOriginalSpawnPoint;
	}

	g.mp.invincibleTimer = Timer();
	t.huddamage.immunity=1000;

	g.mp.damageWasFromAI = 0;

	if (  g.mp.coop  ==  1 ) 
	{
//  `remstart

		if (  g.mp.originalEntitycount  ==  0 ) 
		{
			//  Store the count here incase other elements get added later (like guns)
			g.mp.originalEntitycount = g.entityelementlist;
			Dim (  t.steamStoreentityelement,g.entityelementlist );
			for ( t.te = 1 ; t.te<=  g.entityelementlist; t.te++ )
			{
				t.steamStoreentityelement[t.te]=t.entityelement[t.te];
			}
		}
//   `remend

	}

	t.playercontrol.deadtime = Timer() + 2000;
	t.playercontrol.redDeathFog_f = 0;
//physics_disableplayer ( );
	t.aisystem.processplayerlogic=0;
	g.mp.noplayermovement = 1;
	
	if (  g.mp.syncedWithServer  ==  0 ) 
	{
		SteamSendIAmReadyToPlay (  );
		g.mp.syncedWithServer = 1;
		g.mp.sentreadytime = Timer();
//   `print "SENDING I AM READY"

		//  are we the server? if so, let lua know
		if (  g.mp.isGameHost  ==  1 ) 
		{
			LuaSetInt (  "mp_isServer",1 );
		}
		else
		{
			LuaSetInt (  "mp_isServer",0 );
		}
		LuaSetInt (  "mp_coop", g.mp.coop );
		mp_howManyEnemiesLeftToKill ( );
		LuaSetInt (  "mp_me",g.mp.me+1 );
		mp_setLuaResetStats ( );
	}
	
	ravey_particles_delete_all_emitters ( );
	
	if (  g.mp.maxHealth  ==  0  )  g.mp.maxHealth  =  t.player[t.plrid].health;
	
	if (  g.mp.gameAlreadySpawnedBefore  ==  0 || Timer() - g.mp.dyingTime > 1500 ) 
	{
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
			{

				//  13032015 0XX - Team Multiplayer
				if (  g.mp.team  ==  1 ) 
				{
					for ( t.tteam = 1 ; t.tteam<=  MP_MAX_NUMBER_OF_PLAYERS; t.tteam++ )
					{
						t.tnothing = LuaExecute( cstr(cstr("mp_playerTeam[") + Str(t.tteam) + "] = " + Str(t.mp_team[t.tteam-1])).Get() );
					}
						t.tnothing = LuaExecute( cstr(cstr("mp_teambased = ") + Str(g.mp.team)).Get() );
				}

				t.tindex = g.mp.me+1;
				g.mp.myOriginalSpawnPoint = t.tindex;

				if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
				{
					t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
					t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y+20;
					t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
					t.terrain.playerax_f=0;
					t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
					t.terrain.playeraz_f=0;

					g.mp.lastx=t.terrain.playerx_f;
					g.mp.lasty=t.terrain.playery_f;
					g.mp.lastz=t.terrain.playerz_f;
					g.mp.lastangley=t.terrain.playeray_f;

				}
				else
				{
					t.tfound = 0;
					t.ttempindex = t.tindex/2;
					if (  t.ttempindex > 0 ) 
					{
						if (  t.mpmultiplayerstart[t.ttempindex].active == 1 ) 
						{
							g.mp.myOriginalSpawnPoint = t.ttempindex;
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[t.ttempindex].x;
							t.terrain.playery_f=t.mpmultiplayerstart[t.ttempindex].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[t.ttempindex].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[t.ttempindex].angle;
							t.terrain.playeraz_f=0;

							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						if (  t.mpmultiplayerstart[1].active == 1 ) 
						{
							g.mp.myOriginalSpawnPoint = 1;
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
							t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
							t.terrain.playeraz_f=0;
	
							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						physics_resetplayer_core ( );
					}
				}
			}

			SteamSetPlayerPositionX (  t.terrain.playerx_f );
			SteamSetPlayerPositionY (  t.terrain.playery_f );
			SteamSetPlayerPositionZ (  t.terrain.playerz_f );
			SteamSetPlayerAngle (  t.terrain.playeray_f );
	}

	if (  SteamIsEveryoneReadyToPlay()  ==  0 || g.mp.syncedWithServer  ==  0 ) 
	{
		mp_textDots(-1,30,3,"Waiting for other players to join");
		if (  Timer() - g.mp.sentreadytime > 30*1000 ) 
		{
				g.mp.syncedWithServer = 0;
		}
		t.typos = 40;
		for ( t.tn = 0 ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
		{
			if (  t.mp_joined[t.tn]  !=  "" ) 
			{
				if (  cstr(Right(t.mp_joined[t.tn].Get(),6 ))  ==  "Joined" ) 
				{
					mp_textColor(-1,t.typos,1,t.mp_joined[t.tn].Get(),100,255,100);
				}
				else
				{
					mp_textColor(-1,t.typos,1, cstr(t.mp_joined[t.tn] + " - Waiting").Get(),255,200,100);
				}
				t.typos += 5;
			}
		}
		return;
	}

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
			t.x_f = SteamGetPlayerPositionX(t.c);
			t.y_f = SteamGetPlayerPositionY(t.c);
			t.z_f = SteamGetPlayerPositionZ(t.c);
			t.angle_f = SteamGetPlayerAngle(t.c);
	}

	t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;

	if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
	{
		//  if we have not died to another player, we take one off our kills instead since we killed ourself
		if (  g.mp.checkedWhoKilledMe  ==  0 ) 
		{
			g.mp.checkedWhoKilledMe = 1;
			if (  g.mp.killedByPlayerFlag  ==  0 ) 
			{
				if (  g.mp.coop  ==  0 ) 
				{
					SteamSendLua (  MP_LUA_ServerSetPlayerRemoveKill,0,g.mp.me+1 );
					SteamKilledSelf (  );
				}
			}
			else
			{
				if (  g.mp.coop  ==  0 ) 
				{
					SteamSendLua (  MP_LUA_ServerSetPlayerAddKill,0,g.mp.playerThatKilledMe+1 );
				}
			}
			SteamSendLua (  MP_LUA_ServerSetPlayerAddDeath,0,g.mp.me+1 );
		}
	}
	
	if (  SteamReadyToSpawn()  ==  0 ) 
	{
		mp_text(-1,20,3,"WAITING FOR PLAYERS");
		return;
	}
	if (  g.mp.syncedWithServer  ==  0 ) 
	{
		mp_pre_game_file_sync ( );
		if (  SteamGetClientServerConnectionStatus()  ==  0 ) 
		{
			t.tsteamlostconnectioncustommessage_s = "Lost connection with server (Error MP013)";
			mp_lostConnection ( );
			return;
		}
		return;
	}

	if (  g.mp.endplay  ==  0 && g.mp.showscoresdelay  ==  -2000 ) 
	{
		mp_panel(40,45,60,65);
		mp_text(-1,52,3,"SPAWNING IN");

		t.s_s = Str(5-g.mp.respawnLeft);
		mp_text(-1,58,3,t.s_s.Get());

		if (  g.mp.coop  ==  0 ) 
		{
			if (  g.mp.killedByPlayerFlag  ==  1 ) 
			{
				t.s_s = cstr("YOU WERE KILLED BY ") + Upper(SteamGetOtherPlayerName(g.mp.playerThatKilledMe));
				mp_text(-1,30,3,t.s_s.Get());
			}
			else
			{
				if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
				{
					t.s_s = "YOU KILLED YOURSELF!";
					mp_text(-1,30,3,t.s_s.Get());
				}
			}
		}
		else
		{
			if (  g.mp.gameAlreadySpawnedBefore  ==  1 ) 
			{
				t.s_s = "YOU DIED!";
				mp_text(-1,30,3,t.s_s.Get());
			}
		}
	}
	if (  g.mp.oldSpawnTimeLeft  ==  0  )  g.mp.oldSpawnTimeLeft  =  Timer();

	if (  Timer() - g.mp.oldSpawnTimeLeft  >=  1000 ) 
	{
		++g.mp.respawnLeft;
		g.mp.oldSpawnTimeLeft = 0;
		if (  g.mp.respawnLeft  >=  5 ) 
		{

			g.mp.haveshowndeath = 0;
			weapon_mp_projectile_reset ( );
			ravey_particles_delete_all_emitters ( );
			lua_removeplayerweapons ( );
			t.tsteamwasnetworkdamage = 0;
			g.mp.checkedWhoKilledMe = 0;
			g.mp.killedByPlayerFlag = 0;
			g.plrreloading = 0;
			t.playercontrol.pushforce_f = 0.0;
			t.playercontrol.camerashake_f = 0.0;
			g.mp.lastSendTime = 0;
			g.mp.spectatorfollowdistance = 200.0;
			t.tme = g.mp.me;
			if (  t.mp_playingRagdoll[t.tme]  ==  1 ) 
			{

				t.tphyobj=t.entityelement[t.mp_playerEntityID[t.tme]].obj;
				ragdoll_destroy ( );
				RotateObject (  t.entityelement[t.mp_playerEntityID[t.tme]].obj,0,180,0 );
				FixObjectPivot (  t.entityelement[t.mp_playerEntityID[t.tme]].obj );
				t.mp_playingRagdoll[t.tme] = 0;

			}

			if (  ObjectExist(g.steamplayermodelsoffset+t.tme+121)  ==  1 ) 
			{
				ODEDestroyObject (  g.steamplayermodelsoffset+t.tme+121 );
				RotateObject (  g.steamplayermodelsoffset+t.tme+121,0,0,0 );
				PositionObject (  g.steamplayermodelsoffset+t.tme+121,0,-99999,0 );
				HideObject (  g.steamplayermodelsoffset+t.tme+121 );
			}

			g.mp.ragdollon = 0;
			if (  g.mp.endplay  ==  0 ) 
			{
				t.aisystem.processplayerlogic=1;
			}
			t.playercontrol.deadtime = 0;
			t.playercontrol.redDeathFog_f = 0;

			if (  g.mp.maxHealth  ==  0  )  g.mp.maxHealth  =  100;
			g.mp.reloading = 0;

			t.mp_health[g.mp.me] = g.mp.maxHealth;
			t.entityelement[t.mp_playerEntityID[g.mp.me]].health = g.mp.maxHealth;
			t.player[t.plrid].health = g.mp.maxHealth;
			g.mp.killedByPlayer = 0;
			g.mp.playedMyDeathAnim = 0;
	
			// courtesy of Ravey
			t.playercontrol.regenrate = 2;
			t.playercontrol.regenspeed = 100;
			t.playercontrol.regendelay = 3000;
			t.playercontrol.regentime = 3000;

			//  16032015 - 020 - MP Team code
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 && g.mp.team  ==  0 ) 
			{
				t.tindex = g.mp.me+1;
	
				if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
				{
					t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
					t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y+20;
					t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
					t.terrain.playerax_f=0;
					t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
					t.terrain.playeraz_f=0;

					physics_resetplayer_core ( );
				}
				else
				{
					t.tfound = 0;
					t.ttempindex = t.tindex/2;
					if (  t.ttempindex > 0 ) 
					{
						if (  t.mpmultiplayerstart[t.ttempindex].active == 1 ) 
						{
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[t.ttempindex].x;
							t.terrain.playery_f=t.mpmultiplayerstart[t.ttempindex].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[t.ttempindex].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[t.ttempindex].angle;
							t.terrain.playeraz_f=0;
	
							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  ==  0 ) 
					{
						if (  t.mpmultiplayerstart[1].active == 1 ) 
						{
							t.tfound = 1;
							t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
							t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
							t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
							t.terrain.playerax_f=0;
							t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
							t.terrain.playeraz_f=0;

							g.mp.lastx=t.terrain.playerx_f;
							g.mp.lasty=t.terrain.playery_f;
							g.mp.lastz=t.terrain.playerz_f;
							g.mp.lastangley=t.terrain.playeray_f;
						}
					}
					if (  t.tfound  !=  0 ) 
					{
						physics_resetplayer_core ( );
					}
	
//      `endif

				}
			}
			else
			{
				t.tsteamnumberofmarkers = 0;
				for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
				{
					if (  t.mpmultiplayerstart[t.tc].active == 1 ) 
					{
						++t.tsteamnumberofmarkers;
					}
				}
				if (  g.mp.spawnrnd  ==  -1 && t.tsteamnumberofmarkers > 0  )  g.mp.spawnrnd  =  Rnd(t.tsteamnumberofmarkers-1)+1;
				//  13032015 0XX - Team Multiplayer
				if (  g.mp.team  ==  1 && g.mp.coop  ==  0 ) 
				{
					if (  t.tsteamnumberofmarkers  >=  8 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = Rnd(4-1)+1;
						}
						else
						{
							g.mp.spawnrnd = Rnd(4-1)+1+4;
						}
					}
					if (  t.tsteamnumberofmarkers  ==  4 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = Rnd(2-1)+1;
						}
						else
						{
							g.mp.spawnrnd = Rnd(2-1)+1+2;
						}
					}
					if (  t.tsteamnumberofmarkers  ==  2 ) 
					{
						if (  t.mp_team[g.mp.me]  ==  0 ) 
						{
							g.mp.spawnrnd = 0;
						}
						else
						{
							g.mp.spawnrnd = 1;
						}
					}
				}
				if (  t.tsteamnumberofmarkers  ==  1  )  g.mp.spawnrnd  =  0;
				mp_getPlaceToSpawn ( );
				if (  t.mpmultiplayerstart[g.mp.spawnrnd].active == 1 ) 
				{
					t.terrain.playerx_f=t.mpmultiplayerstart[g.mp.spawnrnd].x;
					t.terrain.playery_f=t.mpmultiplayerstart[g.mp.spawnrnd].y+20;
					t.terrain.playerz_f=t.mpmultiplayerstart[g.mp.spawnrnd].z;
					t.terrain.playerax_f=0;
					t.terrain.playeray_f=t.mpmultiplayerstart[g.mp.spawnrnd].angle;
					t.terrain.playeraz_f=0;
				}
				else
				{
					if (  t.mpmultiplayerstart[1].active == 1 ) 
					{
						t.terrain.playerx_f=t.mpmultiplayerstart[1].x;
						t.terrain.playery_f=t.mpmultiplayerstart[1].y+20;
						t.terrain.playerz_f=t.mpmultiplayerstart[1].z;
						t.terrain.playerax_f=0;
						t.terrain.playeray_f=t.mpmultiplayerstart[1].angle;
						t.terrain.playeraz_f=0;
					}
				}

				physics_resetplayer_core ( );

				SteamSetPlayerPositionX (  t.terrain.playerx_f );
				SteamSetPlayerPositionY (  t.terrain.playery_f );
				SteamSetPlayerPositionZ (  t.terrain.playerz_f );
				SteamSetPlayerAngle (  t.terrain.playeray_f );
			}
	
			g.mp.spawnrnd = -1;
	
			mp_getInitialPlayerCount ( );
	
			if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
			{
				//  send our name on first respawn to ensure everyone gets it
				//  as this is the moment everyone is definately there
				//  steam can sometimes fail to get the name for a while
				//  so we will send it a few times
//     `if Timer() - mp.lastsendmynametime > 1000

//      `mp.lastsendmynametime = Timer()

					SteamSendMyName (  );
					g.mp.sentmyname = 1;
//     `endif


				if (  t.game.runasmultiplayer == 1 && g.mp.coop  ==  1 ) 
				{
					for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
					{
						t.entid=t.entityelement[t.e].bankindex;
						if (  t.entid>0 ) 
						{
							if (  t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1 ) 
							{
								t.entityelement[t.e].mp_coopControlledByPlayer = -1;
								t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = 0;
//         `if entityelement(e).speedmodulator# < 1.0 then entityelement(e).speedmodulator#  ==  1.0

							}
						}
					}
				}

			}

			g.mp.realfirsttimespawn = 0;
			g.mp.gameAlreadySpawnedBefore = 1;
			for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
			{
					t.mp_forcePosition[t.c] = 1;
			}
			g.mp.respawnLeft = 0;

			for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
			{
				t.mp_playingAnimation[t.tc] = MP_ANIMATION_NONE;
			}

		}
		//Much like mouse move x, calling get player damage amount will wipe it out after
		t.a=SteamGetPlayerDamageAmount();
		t.entityelement[t.mp_playerEntityID[g.mp.me]].eleprof.hasweapon = 0;
		g.mp.noplayermovement = 0;
		g.mp.invincibleTimer = Timer();
		g.mp.lastSpawnedTime = g.mp.invincibleTimer;


	}

return;

}

void mp_getPlaceToSpawn ( void )
{
	t.found = -1;
	
	if (  g.mp.team  ==  1  )  t.tdisttocheck  =  100; else t.tdisttocheck  =  300;
	
	//  check if the spawnpoint picked is clear, if it is, just use that
	t.failed = 0;
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{

		if (  t.c  !=  g.mp.me ) 
		{
			t.tpx_f = SteamGetPlayerPositionX(t.c);
			t.tpy_f = SteamGetPlayerPositionY(t.c);
			t.tpz_f = SteamGetPlayerPositionZ(t.c);

			t.tsx_f = t.mpmultiplayerstart[g.mp.spawnrnd].x;
			t.tsy_f = t.mpmultiplayerstart[g.mp.spawnrnd].y-50;
			t.tsz_f = t.mpmultiplayerstart[g.mp.spawnrnd].z;

			t.dx_f = t.tpx_f - t.tsx_f;
			t.dy_f = t.tpy_f - t.tsy_f;
			t.dz_f = t.tpz_f - t.tsz_f;

			t.dist_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f));

			if (  t.dist_f < t.tdisttocheck  )  t.failed  =  1;
		}

	}

	//  is no one is here, lets use it
	if (  t.failed  ==  0  )  return;
	
	t.tstart = 1;
	t.tend = MP_MAX_NUMBER_OF_PLAYERS;

	if (  g.mp.team  ==  1 ) 
	{

		t.tsteamnumberofmarkers = 0;
		for ( t.tc = 1 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS; t.tc++ )
		{
			if (  t.mpmultiplayerstart[t.tc].active == 1 ) 
			{
				++t.tsteamnumberofmarkers;
			}
		}

		if (  t.tsteamnumberofmarkers  >=  8 ) 
		{
			if (  t.mp_team[g.mp.me]  ==  0 ) 
			{
				t.tstart = 1;
				t.tend = 4;
			}
			else
			{
				t.tstart = 5;
				t.tend = 8;
			}
		}
		if (  t.tsteamnumberofmarkers  ==  4 ) 
		{
			if (  t.mp_team[g.mp.me]  ==  0 ) 
			{
				t.tstart = 1;
				t.tend = 2;
			}
			else
			{
				t.tstart = 3;
				t.tend = 4;
			}
		}
		if (  t.tsteamnumberofmarkers  <=  2 ) 
		{
			return;
		}
	}

	if (  g.mp.coop  ==  1 ) 
	{
		t.tstart = 1;
		t.tend = t.tsteamnumberofmarkers;
	}
	//  it failed so lets look for an alternative
	for ( t.tspawnpoints = t.tstart ; t.tspawnpoints<=  t.tend; t.tspawnpoints++ )
	{
		if (  t.tspawnpoints  !=  g.mp.spawnrnd ) 
		{
			t.failed = 0;
			if (  t.mpmultiplayerstart[t.tspawnpoints].active == 1 ) 
			{

				for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
				{

					if (  t.c  !=  g.mp.me ) 
					{
						t.tpx_f = SteamGetPlayerPositionX(t.c);
						t.tpy_f = SteamGetPlayerPositionY(t.c);
						t.tpz_f = SteamGetPlayerPositionZ(t.c);

						t.tsx_f = t.mpmultiplayerstart[t.tspawnpoints].x;
						t.tsy_f = t.mpmultiplayerstart[t.tspawnpoints].y-50;
						t.tsz_f = t.mpmultiplayerstart[t.tspawnpoints].z;

						t.dx_f = t.tpx_f - t.tsx_f;
						t.dy_f = t.tpy_f - t.tsy_f;
						t.dz_f = t.tpz_f - t.tsz_f;

						t.dist_f = Sqrt((t.dx_f*t.dx_f)+(t.dy_f*t.dy_f)+(t.dz_f*t.dz_f));

						if (  t.dist_f < t.tdisttocheck  )  t.failed  =  1;
					}

				}

				//  if noone is here lets use this
				if (  t.failed  ==  0 ) 
				{
					g.mp.spawnrnd = t.tspawnpoints;
					return;
				}
			}
		}
	}
	
return;

}

void mp_getInitialPlayerCount ( void )
{
	g.mp.howmanyjoinedatstart = 0;
	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		t.tname_s = SteamGetOtherPlayerName(t.c);
		if (  t.tname_s != "Player"  )  ++g.mp.howmanyjoinedatstart;
	}
return;

}

void mp_nukeTestmap ( void )
{
	mp_deleteFile ("levelbank\\cfg.cfg");
	mp_deleteFile ("levelbank\\conkit.dat");
	mp_deleteFile ("levelbank\\header.dat");
	mp_deleteFile ("levelbank\\m.dat");
	mp_deleteFile ("levelbank\\map.ele");
	mp_deleteFile ("levelbank\\map.ent");
	mp_deleteFile ("levelbank\\map.way");
	mp_deleteFile ("levelbank\\playerconfig.dat");
	mp_deleteFile ("levelbank\\temparea.txt");
	mp_deleteFile ("levelbank\\vegmaskgrass.dat");
	mp_deleteFile ("levelbank\\visuals.ini");
	mp_deleteFile ("levelbank\\watermask.dds");
	//mp_deleteFile ("editors\\gridedit\\__multiplayerlevel__.fpm");
	//mp_deleteFile ("editors\\gridedit\\__multiplayerworkshopitemid__.dat");
	mp_deleteFile (cstr(g.mysystem.editorsGridedit_s+"__multiplayerlevel__.fpm").Get());
	mp_deleteFile (cstr(g.mysystem.editorsGridedit_s+"__multiplayerworkshopitemid__.dat").Get());
}

void mp_respawnEntities ( void )
{
	if (  g.mp.destroyedObjectCount > 0 ) 
	{
		for ( t.i = 0 ; t.i<=  g.mp.destroyedObjectCount-1; t.i++ )
		{
			t.e = t.mp_destroyedObjectList[t.i];
			t.entityelement[t.e].active = 1;
			entity_lua_spawn ( );
			entity_lua_collisionon ( );
			t.entityelement[t.e].lua.firsttime=0;
			t.entityelement[t.e].activated = 0;
			t.entityelement[t.e].collected = 0;
			t.entityelement[t.e].explodefusetime = 0;
			StopObject (  t.entityelement[t.e].obj );
			SetObjectFrame (  t.entityelement[t.e].obj,0 );
			ShowObject (  t.entityelement[t.e].obj );
		}
	}
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.ttentid=t.entityelement[t.e].bankindex;
		if (  t.entityprofile[t.ttentid].strength > 0 || t.entityelement[t.e].activated  !=  0 || t.entityelement[t.e].collected  !=  0 || t.entityelement[t.e].eleprof.strength > 0 ) 
		{
			if (  t.entityelement[t.e].obj > 0 && t.entityelement[t.e].staticflag  ==  0 ) 
			{
				if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
				{

					t.entityelement[t.e].lua.flagschanged=1;
					if (  g.mp.coop  ==  0 ) 
					{
						entity_lua_spawn ( );
					}
					else
					{
						entity_lua_spawn_core ( );
					}
					t.entityelement[t.e].lua.firsttime=0;
					//120916 - seems collisionON sets SetObjectCollisionProperty to 0 (needed for exploding barrel) - left as Dave coded it just in case
					if ( t.entityelement[t.e].eleprof.explodable  ==  0 ) 
					{
						entity_lua_collisionon ( );
					}
					else
					{
						t.tphyobj = t.entityelement[t.e].obj;
						t.entid = t.ttentid;
						physics_setupobject ( );
					}
					t.aisystem.cumilativepauses=0;
					t.entityelement[t.e].mp_updateOn = 0;
					t.entityelement[t.e].mp_coopControlledByPlayer = -1;
					t.entityelement[t.e].active = 1;
					t.entityelement[t.e].activated = 0;
					t.entityelement[t.e].collected = 0;
					t.entityelement[t.e].explodefusetime = 0;
					t.entityelement[t.e].health = t.entityelement[t.e].eleprof.strength;
					StopObject (  t.entityelement[t.e].obj );
					SetObjectFrame (  t.entityelement[t.e].obj,0 );
				}
			}
		}
	}
	g.mp.destroyedObjectCount = 0;
}

void mp_addDestroyedObject ( void )
{
	//  if (  it has a quantity  )  we will respawn it after so much time has passed
	if (  t.entityelement[t.e].eleprof.quantity > 0 ) 
	{
		mp_add_respawn_timed ( );
	}
	if (  g.mp.destroyedObjectCount < MP_DESTROYED_OBJECT_LIST_SIZE ) 
	{
		t.mp_destroyedObjectList[g.mp.destroyedObjectCount] = t.e;
		++g.mp.destroyedObjectCount;
	}
return;

}

void mp_add_respawn_timed ( void )
{
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
			if (  t.mp_respawn_timed[t.i].inuse  ==  0 ) 
			{
				t.mp_respawn_timed[t.i].inuse = 1;
				t.mp_respawn_timed[t.i].e = t.e;
				t.mp_respawn_timed[t.i].time = Timer();
				break;
			}
	}
return;

}

void mp_setLuaPlayerNames ( void )
{
	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		if (  t.i  ==  g.mp.me ) 
		{
			t.tsteamname_s = g.mp.playerName;
		}
		else
		{
			t.tsteamname_s = SteamGetOtherPlayerName(t.i);
		}
		//  ensure the string isnt null before doing anything to it
		if (  t.tsteamname_s  !=  "" ) 
		{
			t.tsteamnameNoApos_s = "";
			for ( t.tloop = 1 ; t.tloop<=  Len(t.tsteamname_s.Get()); t.tloop++ )
			{
				if (  cstr(Mid(t.tsteamname_s.Get(),t.tloop))  !=  "'" && cstr(Mid(t.tsteamname_s.Get(),t.tloop))  !=  Chr(34) ) 
				{
					t.tsteamnameNoApos_s = t.tsteamnameNoApos_s + Mid(t.tsteamname_s.Get(),t.tloop);
				}
				else
				{
					t.tsteamnameNoApos_s = t.tsteamnameNoApos_s + "_";
				}
			}
			t.tnothing = LuaExecute( cstr(cstr("mp_playerNames[") + Str(t.i+1) + "] = '" + t.tsteamnameNoApos_s + "'").Get() );
			if (  t.tsteamname_s  ==  "Player" || t.tsteamname_s  ==  "" ) 
			{
				t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 0").Get() );
			}
			else
			{
				t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 1").Get() );
			}
		}
	}
return;

}

void mp_setLuaResetStats ( void )
{
	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(t.i+1) + "] = 0").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerDeaths[") + Str(t.i+1) + "] = 0").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerNames[") + Str(t.i+1) + "] = ''").Get() );
		t.tnothing = LuaExecute( cstr(cstr("mp_playerConnected[") + Str(t.i+1) + "] = 0").Get() );
		t.mp_kills[t.i] = 0;
		t.mp_deaths[t.i] = 0;
	}
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
		t.mp_respawn_timed[t.i].inuse = 0;
	}

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
			t.entityelement[t.e].mp_networkkill = 0;
	}

	t.tsteamwasnetworkdamage = 0;

return;

}

void mp_updatePlayerInput ( void )
{

//  `print "RELOADING: " + Str(plrreloading)

	
	if (  t.playercontrol.plrhitfloormaterial  ==  0 ) 
	{
		if (  g.mp.oldfootfloortime  ==  0  )  g.mp.oldfootfloortime  =  Timer();
		if (  Timer()-g.mp.oldfootfloortime > 100  )  g.mp.footfloor  =  0;
	}
	else
	{
		g.mp.oldfootfloortime = 0;
		g.mp.footfloor = 1;
	}
	
	if (  g.plrreloading  ==  0 ) 
	{
		g.mp.reloadingCount = 0;
	}

	t.tTime = Timer();
	
	if (  t.tTime - g.mp.lastSendTimeAppearance > MP_APPEARANCE_UPDATE_DELAY ) 
	{
	
		if (  g.plrreloading  !=  0 ) 
		{
			++g.mp.reloadingCount;
			if (  g.mp.reloadingCount < 4 ) 
			{
				g.mp.reloading = 1;
			}
			else
			{
				g.mp.reloading = 0;
			}
		}
	
		g.mp.lastSendTimeAppearance = t.tTime;
		if (  t.playercontrol.jetpackmode  !=  2 && g.mp.reloading  ==  0 ) 
		{
			SteamSetPlayerAppearance (  g.mp.appearance );
		}
		else
		{
			if (  t.playercontrol.jetpackmode  !=  0 ) 
			{
				if (  g.mp.footfloor  ==  1 ) 
				{
					SteamSetPlayerAppearance (  101 );
				}
				else
				{
					SteamSetPlayerAppearance (  102 );
				}
				g.mp.reloading = 0;
			}
			else
			{
				if (  g.mp.reloading  ==  1 ) 
				{
						SteamSetPlayerAppearance (  201 );
				}
				if (  g.plrreloading  ==  0 ) 
				{
					g.mp.reloading = 0;
				}
			}
		}
//   `print "sending appearance update"

	}
	
	if (  t.tTime - g.mp.lastSendTime < MP_INPUT_UPDATE_DELAY  )  return;
	
	g.mp.lastSendTime = t.tTime;
	
//  `print "sending input update"

	
	if (  g.mp.meleeOn  ==  0 ) 
	{
		if (  KeyState(g.keymap[17])  ==  1 || KeyState(g.keymap[200])  ==  1 ) 
		{
			SteamSetKeyState (  17,1 );
		}
		else
		{
			SteamSetKeyState (  17,0 );
		}
		if (  KeyState(g.keymap[31])  ==  1 || KeyState(g.keymap[208])  ==  1 ) 
		{
			SteamSetKeyState (  31,1 );
		}
		else
		{
			SteamSetKeyState (  31,0 );
		}
		if (  KeyState(g.keymap[30])  ==  1 || KeyState(g.keymap[203])  ==  1 ) 
		{
			SteamSetKeyState (  30,1 );
		}
		else
		{
			SteamSetKeyState (  30,0 );
		}
		if (  KeyState(g.keymap[32])  ==  1 || KeyState(g.keymap[205])  ==  1 ) 
		{
			SteamSetKeyState (  32,1 );
		}
		else
		{
			SteamSetKeyState (  32,0 );
		}
	}
if (  KeyState(g.keymap[46])  ==  1 || KeyState(g.keymap[29])  ==  1 || KeyState(g.keymap[157])  ==  1 ) 
{
	SteamSetKeyState (  46,1 );
	g.mp.crouchOn = 1;
}
else
{
	SteamSetKeyState (  46,0 );
	g.mp.crouchOn = 0;
}
//  shift keys
if (  KeyState(g.keymap[42])  ==  1 || KeyState(g.keymap[54])  ==  1 ) 
{
	SteamSetKeyState (  42,1 );
}
else
{
	SteamSetKeyState (  42,0 );
}

return;

}

void mp_load_guns ( void )
{

/*       Debug info
for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
	if (  t.gun[t.tgindex].activeingame == 1 ) 
	{
		t.tweaponname_s=t.gun[t.tgindex].t.name_s;
		tactiveGun = t.tgindex;
		Print (  t.tweaponname_s );
	}
}
*/    

g.mp.gunCount = 0;

	//  all vweaps (that are active)
	for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
	{
		if (  t.gun[t.tgindex].activeingame == 1 ) 
		{
			t.tweaponname_s=t.gun[t.tgindex].name_s;
			if (  t.tweaponname_s != "" ) 
			{

				//  go and load this gun (attached to calling entity instance)
				t.ttobj=g.mp.gunCount+g.steamplayermodelsoffset;
				t.mp_gunobj[g.mp.gunCount] = t.ttobj;
				t.mp_gunname[g.mp.gunCount] = Lower(t.tweaponname_s.Get());
				++g.mp.gunCount;
				if (  ObjectExist(t.ttobj) == 1  )  DeleteObject (  t.ttobj );

				//  replaced X file load with optional DBO convert/load
				t.tfile_s=cstr("gamecore\\guns\\")+t.tweaponname_s+"\\vweap.x";
				deleteOutOfDateDBO(t.tfile_s.Get());
				if (  cstr(Lower(Right(t.tfile_s.Get(),2))) == ".x"  )  t.tdbofile_s = cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-2))+cstr(".dbo"); else t.tdbofile_s = "";
				if (  FileExist(t.tfile_s.Get()) == 1 || FileExist(t.tdbofile_s.Get()) == 1 ) 
				{
					if (  FileExist(t.tdbofile_s.Get()) == 1 ) 
					{
						t.tfile_s=t.tdbofile_s;
						t.tdbofile_s="";
					}
					LoadObject (  t.tfile_s.Get(),t.ttobj );
					SetObjectFilter (  t.ttobj,2 );
					if ( Len(t.tdbofile_s.Get())>1 ) 
					{
						if ( FileExist( t.tdbofile_s.Get()) == 0 ) 
						{
							// unnecessary now as LoadObject auto creates DBO file!
							SaveObject ( t.tdbofile_s.Get(), t.ttobj );
						}
						if (  FileExist(t.tdbofile_s.Get()) == 1 ) 
						{
							DeleteObject (  t.ttobj );
							LoadObject (  t.tdbofile_s.Get(),t.ttobj );
							SetObjectFilter (  t.ttobj,2 );
							t.tfile_s=t.tdbofile_s;
						}
					}
				}
				else
				{
					MakeObjectTriangle (  t.ttobj,0,0,0,0,0,0,0,0,0 );
				}

				//  Apply object settings
				SetObjectTransparency (  t.ttobj,1 );
				SetObjectCollisionOff (  t.ttobj );
				HideObject (  t.ttobj );

				//  VWEAP is NOT part of collision universe (prevents rocket hitting launcher)
				SetObjectCollisionProperty (  t.ttobj,1 );

				//  apply texture to vweap
				if (  g.gdividetexturesize == 0 ) 
				{
					t.texuseid=loadinternaltexture("effectbank\\reloaded\\media\\white_D.dds");
				}
				else
				{
					t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_D.dds").Get() );
				}
				TextureObject (  t.ttobj,0,t.texuseid );
				t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_N.dds").Get() );
				TextureObject (  t.ttobj,1,loadinternaltexture( "effectbank\\reloaded\\media\\blank_O.dds" ));
				TextureObject (  t.ttobj,2,t.texuseid );
				t.texuseid=loadinternaltexture( cstr(cstr("gamecore\\guns\\")+t.tweaponname_s+"\\gun_S.dds").Get() );
				TextureObject (  t.ttobj,3,t.texuseid );
				TextureObject (  t.ttobj,4,t.terrain.imagestartindex );
				TextureObject (  t.ttobj,5,g.postprocessimageoffset+5 );
				TextureObject (  t.ttobj,6,loadinternaltexture( "effectbank\\reloaded\\media\\blank_I.dds") );

				//  Apply entity shader to vweap model
				t.teffectid=loadinternaleffect("effectbank\\reloaded\\entity_basic.fx");
				SetObjectEffect (  t.ttobj,t.teffectid );

				//  07032015 - 016 - ensure the gun orders are the same on all machines
				for ( t.i = 0 ; t.i<=  g.mp.gunCount-2; t.i++ )
				{
					for ( t.j = t.i ; t.j<=  g.mp.gunCount-1; t.j++ )
					{
						if (  SteamStrCmp(t.mp_gunname[t.i].Get(),t.mp_gunname[t.j].Get()) > 0 ) 
						{
							t.ttemp_s = t.mp_gunname[t.i];
							t.mp_gunname[t.i] = t.mp_gunname[t.j];
							t.mp_gunname[t.j]=t.ttemp_s;

							t.ttemp = t.mp_gunobj[t.i];
							t.mp_gunobj[t.i] = t.mp_gunobj[t.j];
							t.mp_gunobj[t.j]=t.ttemp;
						}
					}
				} 
			}

		}
		SteamLoop (  );
	}
}

void mp_check_for_attachments ( void )
{

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if (  t.c  !=  g.mp.me ) 
		{
		
			//  Jetpack
			if (  SteamGetPlayerAppearance(t.c)  !=  t.mp_oldAppearance[t.c] ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				if (  SteamGetPlayerAppearance(t.c)  ==  101 || SteamGetPlayerAppearance(t.c)  ==  102 ) 
				{
					t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
					t.e = t.mp_playerEntityID[t.c];
					entity_freeattachment ( );

					if (  t.mp_jetpackparticles[t.c]  ==  -1 && SteamGetPlayerAppearance(t.c)  ==  102 ) 
					{
						mp_addJetpackParticles ( );
					}
					if (  SteamGetPlayerAppearance(t.c)  ==  101 && t.mp_jetpackparticles[t.c]  !=  -1 ) 
					{
							t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
							ravey_particles_delete_emitter ( );
							t.mp_jetpackparticles[t.c]=-1;
					}

					if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
					{
						DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
						t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
						t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
					}

					if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  0 ) 
					{
						if (  t.playercontrol.jetobjtouse > 0 ) 
						{
							if (  ObjectExist (g.steamplayermodelsoffset+120)  ) 
							{
								CloneObject (  g.steamplayermodelsoffset+t.c+121,g.steamplayermodelsoffset+120 );
							}
						}
					}
				}
			}
			if (  SteamGetPlayerAppearance(t.c)  !=  101 && SteamGetPlayerAppearance(t.c)  !=  102 ) 
			{
				if (  t.mp_jetpackparticles[t.c]  !=  -1 ) 
				{
					t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.c];
					ravey_particles_delete_emitter ( );
					t.mp_jetpackparticles[t.c] = -1;
				}
			}
			//  Gun
			if (  SteamGetPlayerAppearance(t.c)  !=  t.mp_oldAppearance[t.c] && SteamGetPlayerAppearance(t.c) < 101 ) 
			{
				t.mp_playingAnimation[t.c] = MP_ANIMATION_NONE;
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
				{
					HideObject (  g.steamplayermodelsoffset+t.c+121 );
				}
				t.e = t.mp_playerEntityID[t.c];
				entity_freeattachment ( );
				if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
				{
					DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
					t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = 0;
				}
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+100)  ==  0 ) 
				{

					t.tobj = 0;
					if (  SteamGetPlayerAppearance(t.c) > 0 ) 
					{
						t.tobj = t.mp_gunobj[SteamGetPlayerAppearance(t.c)-1];
					}

					if (  t.tobj > 0 ) 
					{
						if (  ObjectExist(t.tobj)  ==  1 ) 
						{
							CloneObject (  g.steamplayermodelsoffset+t.c+100,t.tobj );
							ShowObject (  g.steamplayermodelsoffset+t.c+100 );
							SetObjectMask (  g.steamplayermodelsoffset+t.c+100,1 );
							t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj = g.steamplayermodelsoffset+t.c+100;

							t.tfound = 0;
							for ( t.tgindex = 1 ; t.tgindex<=  g.gunmax; t.tgindex++ )
							{
								if (  t.gun[t.tgindex].activeingame == 1 ) 
								{
									if (  t.mp_gunname[SteamGetPlayerAppearance(t.c)-1] == Lower(t.gun[t.tgindex].name_s.Get()) ) 
									{
										t.tfound = t.tgindex;
									}
								}
							}

							t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon = t.tfound;

						//  Find firespot for this vweap
						t.entityelement[t.e].attachmentobjfirespotlimb=0;
						PerformCheckListForLimbs (  t.tobj );
						for ( t.tc = 1 ; t.tc<=  ChecklistQuantity(); t.tc++ )
						{
							if (  cstr(Lower(ChecklistString(t.tc))) == "firespot" ) 
							{
								t.entityelement[t.e].attachmentobjfirespotlimb=t.tc-1;
								t.tc=ChecklistQuantity()+1;
							}
						}

						}
					}
					else
					{
						if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
						{
							if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj) ) 
							{
								DeleteObject (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj );
								t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon = 0;
							}
						}
					}

				}
			}

			//  update jetpack appearance
			if (  SteamGetPlayerAppearance(t.c)  ==  101 || SteamGetPlayerAppearance(t.c)  ==  102 ) 
			{
				if (  ObjectExist(g.steamplayermodelsoffset+t.c+121)  ==  1 ) 
				{
					ShowObject (  g.steamplayermodelsoffset+t.c+121 );
					t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
					if (  SteamGetKeyState(t.c,46)  ==  1 ) 
					{
						PositionObject (  g.steamplayermodelsoffset+t.c+121, ObjectPositionX(t.tobj), ObjectPositionY(t.tobj)+20, ObjectPositionZ(t.tobj) );
					}
					else
					{
						PositionObject (  g.steamplayermodelsoffset+t.c+121, ObjectPositionX(t.tobj), ObjectPositionY(t.tobj)+40, ObjectPositionZ(t.tobj) );
					}
					YRotateObject (  g.steamplayermodelsoffset+t.c+121,ObjectAngleY(t.tobj) );
				}
			}

			//  update gun appearance
			if (  t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj > 0 ) 
			{
				t.e = t.mp_playerEntityID[t.c];
				entity_controlattachments ( );

				if (  t.mp_playerShooting[t.c]  ==  1 ) 
				{
					t.tgunid=t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
					t.tattachedobj=t.entityelement[t.mp_playerEntityID[t.c]].attachmentobj;
					t.te = t.mp_playerEntityID[t.c];

					t.tgunid = t.entityelement[t.mp_playerEntityID[t.c]].eleprof.hasweapon;
					t.ttrr=Rnd(1);
					for ( t.tt = t.ttrr+0 ; t.tt<=  t.ttrr+1; t.tt++ )
					{
						t.ttsnd=t.gunsoundcompanion[t.tgunid][1][t.tt].soundid;
						if (  t.ttsnd>0 ) 
						{
							if (  SoundExist(t.ttsnd) == 1 ) 
							{
								if (  SoundPlaying(t.ttsnd) == 0 || t.tt == t.ttrr+1 ) 
								{
									t.charanimstate.firesoundindex=t.ttsnd ; t.tt=3;
									t.charanimstate.firesoundexpiry=Timer()+200;
								}
							}
						}
					}

					if (  t.charanimstate.firesoundindex>0 ) 
					{
						darkai_shooteffect ( );
					}
				}

			}
		}

		if (  t.mp_oldAppearance[t.c]  !=  SteamGetPlayerAppearance(t.c)  )  t.mp_playingAnimation[t.c]  =  MP_ANIMATION_NONE;
		t.mp_oldAppearance[t.c] = SteamGetPlayerAppearance(t.c);

	}
	return;
}

void mp_addJetpackParticles ( void )
{

	t.tpartObj = t.entityelement[t.mp_playerEntityID[t.c]].obj;

	ravey_particles_get_free_emitter ( );
	if (  t.tResult>0 ) 
	{
		t.mp_jetpackparticles[t.c]=t.tResult;
		g.tEmitter.id = t.tResult;
		g.tEmitter.emitterLife = 0;
		g.tEmitter.parentObject = t.tpartObj;
		g.tEmitter.parentLimb = 0;
		g.tEmitter.isAnObjectEmitter = 0;
		g.tEmitter.imageNumber = RAVEY_PARTICLES_IMAGETYPE_LIGHTSMOKE + g.particlesimageoffset;
		g.tEmitter.isAnimated = 1;
		g.tEmitter.animationSpeed = 1/64.0;
		g.tEmitter.isLooping = 1;
		g.tEmitter.frameCount = 64;
		g.tEmitter.startFrame = 0;
		g.tEmitter.endFrame = 63;
		g.tEmitter.startsOffRandomAngle = 1;
		g.tEmitter.offsetMinX = -20;
		g.tEmitter.offsetMinY = 50;
		g.tEmitter.offsetMinZ = -20;
		g.tEmitter.offsetMaxX = 20;
		g.tEmitter.offsetMaxY = 50;
		g.tEmitter.offsetMaxZ = 20;
		g.tEmitter.scaleStartMin = 5;
		g.tEmitter.scaleStartMax = 10;
		g.tEmitter.scaleEndMin = 90;
		g.tEmitter.scaleEndMax = 100;
		g.tEmitter.movementSpeedMinX = -0.1f;
		g.tEmitter.movementSpeedMinY = -0.9f;
		g.tEmitter.movementSpeedMinZ = -0.1f;
		g.tEmitter.movementSpeedMaxX = 0.1f;
		g.tEmitter.movementSpeedMaxY = -0.1f;
		g.tEmitter.movementSpeedMaxZ = 0.1f;
		g.tEmitter.rotateSpeedMinZ = -0.1f;
		g.tEmitter.rotateSpeedMaxZ = 0.1f;
		g.tEmitter.startGravity = 0;
		g.tEmitter.endGravity = 0;
		g.tEmitter.lifeMin = 1000;
		g.tEmitter.lifeMax = 2000;
		g.tEmitter.alphaStartMin = 40;
		g.tEmitter.alphaStartMax = 75;
		g.tEmitter.alphaEndMin = 0;
		g.tEmitter.alphaEndMax = 0;
		g.tEmitter.frequency = 25;
		ravey_particles_add_emitter ( );
	}

return;

}

void mp_NearOtherPlayers ( void )
{

	for ( t.c = 0 ; t.c<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.c++ )
	{
		if (  t.c  !=  g.mp.me ) 
		{
				t.tobj = t.entityelement[t.mp_playerEntityID[t.c]].obj;
				if (  t.tobj > 0 ) 
				{
					if (  ObjectExist(t.tobj) ) 
					{
						if (  SteamGetPlayerAlive(t.c)  ==  1 ) 
						{
							t.tplrproxx_f=CameraPositionX()-ObjectPositionX(t.tobj);
							if (  g.mp.crouchOn  ==  0 ) 
							{
								t.tplrproyy_f=(CameraPositionY()-64)-ObjectPositionY(t.tobj);
							}
							else
							{
								t.tplrproyy_f=(CameraPositionY()-64+30)-ObjectPositionY(t.tobj);
							}
							t.tplrproxz_f=CameraPositionZ()-ObjectPositionZ(t.tobj);
							t.tplrproxd_f=Sqrt(abs(t.tplrproxx_f*t.tplrproxx_f)+abs(t.tplrproyy_f*t.tplrproyy_f)+abs(t.tplrproxz_f*t.tplrproxz_f));
							t.tplrproxa_f=atan2deg(t.tplrproxx_f,t.tplrproxz_f);
							if (  t.tplrproxd_f<50.0 ) 
							{
								t.playercontrol.pushforce_f=0.5;
								t.playercontrol.pushangle_f=t.tplrproxa_f;
							}
						}
					}
				}
		}
	}

return;

}

void mp_check_respawn_objects ( void )
{
	t.tTime = Timer();
	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
			if (  t.mp_respawn_timed[t.i].inuse  ==  1 ) 
			{
				if (  t.tTime - t.mp_respawn_timed[t.i].time > MP_RESPAWN_TIME_DELAY ) 
				{
					t.mp_respawn_timed[t.i].inuse = 0;

					t.e = t.mp_respawn_timed[t.i].e;
					t.entityelement[t.e].active = 1;
					entity_lua_spawn ( );
					entity_lua_collisionon ( );
					t.entityelement[t.e].activated = 0;
					t.entityelement[t.e].collected = 0;
					StopObject (  t.entityelement[t.e].obj );
					SetObjectFrame (  t.entityelement[t.e].obj,0 );
					ShowObject (  t.entityelement[t.e].obj );

				}
			}
	}
return;

}

void mp_checkForEveryoneLeft ( void )
{
		if (  g.mp.howmanyjoinedatstart > 1 ) 
		{
			t.tsteamhowmanynow = 0;
			for ( t.tcount = 0 ; t.tcount<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tcount++ )
			{
				t.tname_s = SteamGetOtherPlayerName(t.tcount);
				if (  t.tname_s != "Player"  )  ++t.tsteamhowmanynow;
			}

			if (  t.tsteamhowmanynow  <= 1 ) 
			{
				t.tsteamlostconnectioncustommessage_s = "Everyone else left the game! (Code MP014)";
				g.mp.backtoeditorforyou = 1;
				mp_lostConnection ( );
				return;
			}
		}
return;

}

void mp_lostConnection ( void )
{
		t.tTime = Timer();
		editor_hideall3d ( );
		SetDir (  cstr(g.fpscrootdir_s + "\\Files").Get() );
		if (  t.tsteamconnectionlostmessage_s  ==  "GAMEOVER"  )  g.mp.backtoeditorforyou  =  1;
		t.tsteamconnectionlostmessage_s = "Lost connection to server";
		if (  t.tsteamlostconnectioncustommessage_s != ""  )  t.tsteamconnectionlostmessage_s  =  t.tsteamlostconnectioncustommessage_s;
		while (  Timer() - t.tTime < 5000 ) 
		{
			Cls (  );
			mp_text(-1,30,3,t.tsteamconnectionlostmessage_s.Get());
			if (  t.tsteamconnectionlostmessage_s  ==  "Could not build workshop item (Error MP015)" ) 
			{
				mp_text(-1,40,3,"The workshop item did not upload to Steam");
				mp_text(-1,45,3,"Please t.try again in t.a few moments.");
				mp_text(-1,50,3,"If the problem persists t.try closing");
				mp_text(-1,55,3,"Game Guru and restarting Steam.");
			}
			SteamLoop (  );
			Sync (  );
		}
		t.tsteamlostconnectioncustommessage_s = "";
//mp_free_game ( );
		mp_setMessage ( );
		if (  g.mp.mode  ==  MP_IN_GAME_CLIENT || g.mp.mode  ==  MP_IN_GAME_SERVER || g.mp.backtoeditorforyou > 0 ) 
		{
			mp_resetGameStats ( );
			if (  g.mp.backtoeditorforyou  !=  2 ) 
			{
				mp_setLuaResetStats ( );
			}
			else
			{
				g.mp.goBackToEditor = 1;
			}
		}
		g.mp.backtoeditorforyou = 0;
		mp_resetGameStats ( );
		mp_quitGame ( );
}

void mp_gameLoop ( void )
{

//  check we have finished loading, if not exit out
if (  g.mp.finishedLoadingMap  ==  0  )  return;

//  HideMouse (  when menu finished )
if (  t.thaveShownMouse >0 ) 
{
	game_hidemouse ( );
	--t.thaveShownMouse;
}

mp_updateAIForCOOP ( );
mp_howManyEnemiesLeftToKill ( );

//  some debug stuff
// `print "destroycount = " + Str(tempsteamdestroycount)

// `if UpKey() then tempsteamdestroycount  ==  0

// `if guntimercount  ==  0 then guntimercount  ==  6

//  `print guntimercount

//  `print ttemppotato

//  `print gunmode


	mp_NearOtherPlayers ( );

	//  if we have lost connection, head back to main menu
	t.tconnectionStatus = SteamGetClientServerConnectionStatus();
	if (  t.tconnectionStatus  ==  0 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		mp_lostConnection ( );
		return;
	}
	if (  t.tconnectionStatus  ==  2 ) 
	{
		t.tsteamconnectionlostmessage_s = "GAMEOVER";
		t.tsteamlostconnectioncustommessage_s = "Game Over. The host closed the server.";
		mp_lostConnection ( );
		return;
	}

	mp_lua ( );
	mp_setLuaPlayerNames ( );
	mp_check_respawn_objects ( );

//mp_checkVoiceChat ( );

	if (  Timer() - g.mp.showscoresdelay > 2000 ) 
	{
		if (  KeyState(g.keymap[15])  ==  1 && g.mp.chaton  ==  0 ) 
		{
			t.tnothing = LuaExecute("mp_showscores = 1");
			g.mp.showscoresdelay = Timer();
		}
		else
		{
			t.tnothing = LuaExecute("mp_showscores = 0");
			g.mp.showscoresdelay = -2000;
		}
	}

	//  Find out which index we are (server will always be 0)
	g.mp.me = SteamGetMyPlayerIndex();
	//  Hide our own player model but show everyone elses
	//  TO DO; if a player has d/c or never joined, need to hide their model rather than show a zombie standing there doing nothing
	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
			if (  t.mp_playingRagdoll[t.a]  ==  1 && SteamGetPlayerAlive(t.a)  ==  1 ) 
			{
				t.mp_playingRagdoll[t.a] = 0;
				t.tphyobj=t.entityelement[t.mp_playerEntityID[t.a]].obj;
				ragdoll_destroy ( );
				RotateObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj,0,180,0 );
				FixObjectPivot (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
				t.e = t.mp_playerEntityID[t.a];
				t.entityelement[t.e].health=g.mp.maxHealth;
				//  set appearance back to default so they repick the gun up they had before
				t.mp_oldAppearance[t.a] = 0;
				t.mp_playingAnimation[t.a] = MP_ANIMATION_NONE;
			}
			if (  t.a  ==  g.mp.me ) 
			{
				if (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.mp_playerEntityID[g.mp.me]].obj) ) 
					{
						HideObject (  t.entityelement[t.mp_playerEntityID[g.mp.me]].obj );
					}
				}
			}
			else
			{
				if (  t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
					{
						if (  t.mp_forcePosition[t.a] > 0 && SteamGetPlayerAlive(t.a)  ==  1 ) 
						{
							t.mp_playerHasSpawned[t.a]=1;
							HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
							if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
							{
								if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
							}
						}
						else
						{
							if (  t.mp_playerHasSpawned[t.a]  ==  1 ) 
							{
								ShowObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
								if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  ShowObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
								}
							}
							else
							{
								HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj );
								if (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj)  ==  1  )  HideObject (  t.entityelement[t.mp_playerEntityID[t.a]].attachmentobj );
								}
							}
						}
					}
				}
			}
	}

	//  Player is respawning or dead
	t.characterkitcontrol.showmyhead = 0;
	if (  t.mp_health[g.mp.me]  <=  0 ) 
	{
		t.tTime = Timer();
		if (  t.tTime - g.mp.lastSendAliveTime > MP_ALIVE_UPDATE_DELAY ) 
		{
			g.mp.lastSendAliveTime = t.tTime;
			SteamSetPlayerAlive (  0 );
		}
		mp_showdeath ( );
		mp_respawn ( );

		mp_updatePlayerPositions ( );
		mp_updatePlayerNamePlates ( );
		mp_updatePlayerAnimations ( );
		mp_delete_entities ( );
		mp_loop ( );
		mp_check_for_attachments ( );
		mp_update_all_projectiles ( );
		if (  g.mp.gameAlreadySpawnedBefore  ==  0 ) 
		{
			mp_dontShowOtherPlayers ( );
		}

		if (  t.mp_health[g.mp.me] > 0  )  g.mp.lastSendAliveTime  =  0;
		return;
	}

	//  Player is alive
	t.tTime = Timer();
	if (  t.tTime - g.mp.lastSendAliveTime > MP_ALIVE_UPDATE_DELAY ) 
	{
		g.mp.lastSendAliveTime = t.tTime;
		SteamSetPlayerAlive (  1 );
	}
	mp_update_player ( );
	mp_updatePlayerPositions ( );
	mp_updatePlayerInput ( );
	mp_updatePlayerNamePlates ( );
	mp_updatePlayerAnimations ( );
	mp_delete_entities ( );
	mp_loop ( );
	mp_server_message ( );
	mp_check_for_attachments ( );
	mp_update_all_projectiles ( );

	if ( g.mp.endplay == 1 ) mp_ending_game ( );

	if (  t.mp_health[g.mp.me]  <=  0  )  g.mp.lastSendAliveTime  =  0;

	t.tTime = Timer();

if ( g.mp.isGameHost == 1 ) mp_checkForEveryoneLeft ( );

return;

}

//  used when restarting a match so you don't see everyone dropping out of the sky
void mp_dontShowOtherPlayers ( void )
{

	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		if (  t.a  !=  g.mp.me ) 
		{
			if (  t.entityelement[t.mp_playerEntityID[t.a]].obj > 0 ) 
			{
				if (  ObjectExist(t.entityelement[t.mp_playerEntityID[t.a]].obj) ) 
				{
					PositionObject (  t.entityelement[t.mp_playerEntityID[t.a]].obj,-100000,-100000,-100000 );
				}
			}
		}
	}

return;
	
}

void mp_ending_game ( void )
{
	PositionCamera (  25500,2000,25500 );
	RotateCamera (  90,t.tendofgamerotate_f,0 );
	t.tendofgamerotate_f = t.tendofgamerotate_f + (0.25*g.timeelapsed_f);
	for ( t.a = 0 ; t.a<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.a++ )
	{
		t.tobj=t.entityelement[t.mp_playerEntityID[t.a]].obj;
		if (  t.tobj > 0 ) 
		{
				if (  ObjectExist(t.tobj) ) 
				{
					PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-20, t.terrain.playerz_f );
					HideObject (  t.tobj );
				}
		}
	}
return;

}

void mp_free_game ( void )
{
	if (  t.tspritetouse > 0 ) 
	{
		if (  SpriteExist(t.tspritetouse)  ==  1  )  DeleteSprite (  t.tspritetouse );
		t.tspritetouse = 0;
	}

	if (  g.mp.coop  ==  1 && g.mp.originalEntitycount > 0 ) 
	{
		UnDim (  t.steamStoreentityelement );
		g.mp.originalEntitycount = 0;
	}

	if (  g.mp.gunCount > 0 ) 
	{
		for ( t.i = 0 ; t.i<=  g.mp.gunCount-1; t.i++ )
		{
			if (  t.mp_gunobj[t.i] > 0 ) 
			{
				if (  ObjectExist(t.mp_gunobj[t.i])  )  DeleteObject (  t.mp_gunobj[t.i] );
			}
		}
	}

	g.mp.gunCount = 0;
	for ( t.i = 0 ; t.i<=  599; t.i++ )
	{
		if (  ObjectExist (g.steamplayermodelsoffset+t.i)  ==  1  )  DeleteObject (  g.steamplayermodelsoffset+t.i ) ;
	}

	for ( t.tbulletloop = 0 ; t.tbulletloop<=  159; t.tbulletloop++ )
	{

		t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
		if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
		{
			if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			DeleteSound (  t.tSteamSoundID );
		}

		t.tSteamSoundID = g.steamsoundoffset+t.tbulletloop;
		if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
		{
			if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			if (  SoundLooping(t.tSteamSoundID)  ==  0  )  StopSound(t.tSteamSoundID);
			DeleteSound (  t.tSteamSoundID );
		}

	}

	for ( t.i = 0 ; t.i<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.i++ )
	{
		if (  t.mp_jetpackparticles[t.i]  !=  -1 ) 
		{
			t.tRaveyParticlesEmitterID=t.mp_jetpackparticles[t.i];
			ravey_particles_delete_emitter ( );
			t.mp_jetpackparticles[t.i] = -1;
		}
	}

	mp_resetGameStats ( );

	SteamWorkshopModeOff (  );

	g.mp.message = "";
	g.mp.messageTime = 0;

	t.game.gameloop = 0;

	if (  ImageExist(g.panelimageoffset+10)  )  DeleteImage (  g.panelimageoffset+10 );
	if (  SpriteExist(g.steamchatpanelsprite)  ==  1  )  DeleteSprite (  g.steamchatpanelsprite );

return;

//  needs tlobbytring$ to be set to the lobby name
}

void mp_subbedToItem ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		if (  t.mp_subbedItems[t.tloop]  ==  "" ) 
		{
			t.mp_subbedItems[t.tloop] = t.tlobbytring_s;
			break;
		}
	}
return;

//  needs tlobbytring$ to be set to the lobby name
}

void mp_checkItemSubbed ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		if (  t.mp_subbedItems[t.tloop]  ==  t.tlobbytring_s && t.tlobbytring_s != "" ) 
		{
			if (  Timer() - t.tsteaminstallingdotstime > 150 ) 
			{
				t.tsteaminstallingdotstime = Timer();
				t.tsteamInstallingDots_s = t.tsteamInstallingDots_s + ".";
				if (  Len(t.tsteamInstallingDots_s.Get()) > 3  )  t.tsteamInstallingDots_s  =  "";
			}
			t.tlobbytring_s = t.tlobbytring_s + " - Installing." + t.tsteamInstallingDots_s;
			break;
		}
	}
return;

}

void mp_resetGameStats ( void )
{

	mp_nukeTestmap ( );
	//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get())  )  DeleteAFile (  cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
	//if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat").Get())  )  DeleteAFile (  cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerworkshopitemid__.dat").Get() );
	cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
	if ( FileExist( mlevel_s.Get())  )  DeleteAFile ( mlevel_s.Get() );
	cstr mlevelworkshop_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerworkshopitemid__.dat";
	if ( FileExist( mlevelworkshop_s.Get())  )  DeleteAFile ( mlevelworkshop_s.Get() );

	//  empty messages
	for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-1; t.tloop++ )
	{
		t.mp_chat[t.tloop] = "";
	}

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
			t.entityelement[t.e].mp_networkkill = 0;
	}

	for ( t.tloop = 0 ; t.tloop<=  20; t.tloop++ )
	{
		t.mp_subbedItems[t.tloop] = "";
	}

	if ( SteamGetPlayerName() != NULL )
	{
		g.mp.playerName = SteamGetPlayerName();
		g.mp.playerID = SteamGetPlayerID();
	}

	g.mp.mode = MP_MODE_MAIN_MENU;
	g.mp.launchServer = 0;
	g.mp.maxHealth = 0;
	g.mp.isLobbyCreated = 0;
	g.mp.isServerCreated = 0;
	g.mp.isGameHost= 0;
	g.mp.voiceChatOn = 0;
	g.mp.lobbycount = 0;
	g.mp.lobbyscrollbarOn = 0;
	g.mp.gameAlreadySpawnedBefore = 0;
	g.mp.killedByPlayer = 0;
	g.mp.previousMessage_s = "";
	g.mp.syncedWithServer = 0;
	g.mp.syncedWithServerMode = 0;
	g.mp.oldtime = 0;
	g.mp.me = 0;
	g.mp.playedMyDeathAnim = 0;
	g.mp.fileLoaded = 0;
	g.mp.playGame = 0;
	g.mp.oldSpawnTimeLeft = 0;
	g.mp.respawnLeft = 0;
	g.mp.crouchOn = 0;
	g.mp.meleeOn = 0;
	g.mp.isAnimating = 0;
	g.mp.okayToLoadLevel = 0;
	g.mp.iHaveSaidIAmReady = 0;
	g.mp.attachmentcount = 0;
	g.mp.gunCount = 0;
	g.mp.gunid = 0;
	g.mp.lastSendTime = 0;
	g.mp.lastSendTimeAppearance = 0;
	g.mp.appearance = 0;
	g.mp.dyingTime = 0;
	g.mp.spawnrnd = -1;
	g.mp.reloading = 0;
	g.mp.syncedWithServer = 0;
	g.mp.sentreadytime = 0;
	g.mp.AttemptedToJoinLobbyTime = 0;
	g.mp.lastSendProjectileTime = 0;
	g.mp.dontApplyDamage = 0;
	g.mp.ragdollon = 0;
	g.mp.spectatorfollowdistance = 200.0;
	g.mp.ignoreDamageToEntity = 0;
	g.mp.endplay = 0;
	g.mp.destroyedObjectCount = 0;
	g.mp.message = "";
	g.mp.messageTime = 0;
	g.mp.oldfootfloortime = 0;
	g.mp.footfloor = 0;
	g.mp.resetcore = 0;
	g.mp.levelContainsCustomContent = 0;
	g.mp.workshopid = "0";
	g.mp.initialSpawnmoveDownCharacterFlag=1;
	g.mp.usersInServersLobbyAtServerCreation = 0;
	g.mp.dontDrawTitles = 0;
	g.mp.haveshowndeath = 0;
	g.mp.checkiflobbiesavailablemode = 0;
	g.mp.noplayermovement = 0;
	g.mp.team = 0;
	g.mp.friendlyfireoff = 0;
	g.mp.nameplatesOff = 0;
	g.mp.damageWasFromAI = 0;
	g.mp.haveSentMyAvatar = 0;
	g.mp.myOriginalSpawnPoint = -1;
	g.mp.realfirsttimespawn = 1;

	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.mp_kills[t.tc] = 0;
		t.mp_deaths[t.tc] = 0;
		t.mp_reload[t.tc] = 0;
		t.mp_playerShooting[t.tc] = 0;
		t.mp_playerAttachmentIndex[t.tc] = 0;
		t.mp_playerIsRagdoll[t.tc] = 0;
		t.mp_playerAttachmentObject[t.tc] = 0;
		t.mp_playerHasSpawned[t.tc] = 0;
		t.mp_oldAppearance[t.tc] = 0;
		t.mp_playingAnimation[t.tc] = 0;
		t.mp_playingRagdoll[t.tc] = 0;
		t.mp_oldplayerx[t.tc] = 0;
		t.mp_oldplayery[t.tc] = 0;
		t.mp_oldplayerz[t.tc] = 0;
		t.mp_meleePlaying[t.tc] = 0;

		t.mp_isDying[t.tc] = 0;
		t.mp_jetpackOn[t.tc] = 0;
		t.mp_lobbies_s[t.tc] = "";
		t.mp_playerEntityID[t.tc] = 0;
		t.mp_forcePosition[t.tc] = 0;
		t.mp_health[t.tc] = 0;
		t.mp_lastIdleReset[t.tc] = 1;
		t.mp_jetpackparticles[t.tc] = -1;
		t.mp_joined[t.tc] = "";
	}

	t.twhichteam = 1;
	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.twhichteam = 1-t.twhichteam;
		t.mp_team[t.tc] = t.twhichteam;
	}

	for ( t.tc = 0 ; t.tc<=  99; t.tc++ )
	{
		t.mp_attachmentobjects[t.tc] = 0;
		t.mp_gunobj[t.tc] = 0;
		t.mp_gunname[t.tc] = "";
	}

	for ( t.tc = 0 ; t.tc<=  79; t.tc++ )
	{
		t.mp_bullets[t.tc].on = 0;
		t.mp_bullets[t.tc].particles = -1;
		t.mp_bullets[t.tc].sound = 0;
	}

	for ( t.i = 0 ; t.i<=  MP_RESPAWN_TIME_OBJECT_LIST_SIZE; t.i++ )
	{
		t.mp_respawn_timed[t.i].inuse = 0;
	}
	
	t.characterkitcontrol.showmyhead = 0;
	

return;

}

void mp_update_all_projectiles ( void )
{
t.debugHowManyInUse = 0;
	for ( t.tbulletloop = 0 ; t.tbulletloop<=  159; t.tbulletloop++ )
	{
			if (  SteamGetBulletOn(t.tbulletloop)  ==  0 ) 
			{
				//  clean up particles
				if (  t.mp_bullets[t.tbulletloop].particles  !=  -1 ) 
				{
						t.tRaveyParticlesEmitterID=t.mp_bullets[t.tbulletloop].particles;
						ravey_particles_delete_emitter ( );
						t.mp_bullets[t.tbulletloop].particles=-1;
				}
			}
			if (  t.tbulletloop < g.mp.me*20 || t.tbulletloop > (g.mp.me*20)+19 ) 
			{
				t.tSteamSoundID = g.steamsoundoffset+t.tbulletloop;
				if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
				{
					if (  SoundLooping(t.tSteamSoundID)  ==  0 ) 
					{
						DeleteSound (  t.tSteamSoundID );
						t.mp_bullets[t.tbulletloop].sound = 0;
					}
				}
				t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
				if (  SoundExist(t.tSteamSoundID)  ==  1 ) 
				{
					if (  SoundPlaying(t.tSteamSoundID)  ==  0  )  DeleteSound (  t.tSteamSoundID );
				}
				if (  SteamGetBulletOn(t.tbulletloop)  ==  1 ) 
				{
					++t.debugHowManyInUse;
					t.tsteamBObj = g.steamplayermodelsoffset+200+t.tbulletloop;
					t.mp_bullets[t.tbulletloop].btype = SteamGetBulletType(t.tbulletloop);
					if (  ObjectExist(t.tsteamBObj)  ==  0 ) 
					{
						t.tfindObj = t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].baseObj;
						if (  t.tfindObj  !=  0 ) 
						{
							CloneObject (  t.tsteamBObj, t.tfindObj );
						}
						else
						{
							MakeObjectBox (  t.tsteamBObj,20,20,20 );
						}

						//  setup particle emitters for this projectile
						//  but only if entities not set to LOWEST as particle trails are expensive!
						t.mp_bullets[t.tbulletloop].particles = -1;
						t.tokay = 1 ; if (  t.visuals.shaderlevels.entities == 3  )  t.tokay = 0;
						if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].particleType>0 && t.tokay == 1 ) 
						{
							ravey_particles_get_free_emitter ( );
							if (  t.tResult>0 ) 
							{
								t.tobj = t.tsteamBObj;
								weapon_add_projectile_particles ( );
								t.mp_bullets[t.tbulletloop].particles = t.tResult;
							}
						}

						//  setup sound
						t.mp_bullets[t.tbulletloop].sound = g.steamsoundoffset+t.tbulletloop;
						if (  SoundExist(t.mp_bullets[t.tbulletloop].sound) == 1 ) 
						{
							if (  SoundPlaying(t.mp_bullets[t.tbulletloop].sound)  ==  1  )  StopSound (  t.mp_bullets[t.tbulletloop].sound );
							if (  SoundLooping(t.mp_bullets[t.tbulletloop].sound)  ==  1  )  StopSound (  t.mp_bullets[t.tbulletloop].sound );
							DeleteSound (  t.mp_bullets[t.tbulletloop].sound );
						}
						//  if this projectile has a sound that loops, start it now
						if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].sound  ==  0  )  t.mp_bullets[t.tbulletloop].sound  =  0;
						if (  t.mp_bullets[t.tbulletloop].sound > 0 ) 
						{
							CloneSound (  t.mp_bullets[t.tbulletloop].sound,t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].sound );
							if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundLoopFlag  ==  1 ) 
							{
								PositionSound (  t.mp_bullets[t.tbulletloop].sound,SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
								SetSoundSpeed (  t.mp_bullets[t.tbulletloop].sound, t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerBaseSpeed );
								LoopSound (  t.mp_bullets[t.tbulletloop].sound );
								if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerFlag  ==  1 ) 
								{
									t.txDist_f = CameraPositionX(0) - SteamGetBulletX(t.tbulletloop);
									t.tyDist_f = CameraPositionY(0) - SteamGetBulletY(t.tbulletloop);
									t.tzDist_f = CameraPositionZ(0) - SteamGetBulletZ(t.tbulletloop);
									t.mp_bullets[t.tbulletloop].soundDistFromPlayer = Sqrt(t.txDist_f*t.txDist_f + t.tyDist_f*t.tyDist_f + t.tzDist_f*t.tzDist_f);
								}
							}
						}

					}
					if (  ObjectExist(t.tsteamBObj)  ==  1 ) 
					{
						PositionObject (  t.tsteamBObj, SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
						RotateObject (  t.tsteamBObj, SteamGetBulletAngleX(t.tbulletloop), SteamGetBulletAngleY(t.tbulletloop), SteamGetBulletAngleZ(t.tbulletloop) );

						//  do we need to reposition the 3D sound?
						t.tSndID = t.mp_bullets[t.tbulletloop].sound;
						if (  t.tSndID > 0 ) 
						{
							if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundLoopFlag  ==  1 ) 
							{
								//  position the sound
								PositionSound (  t.tSndID,SteamGetBulletX(t.tbulletloop), SteamGetBulletY(t.tbulletloop), SteamGetBulletZ(t.tbulletloop) );
								//  calculate and set doppler pitch
								if (  t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerFlag  ==  1 ) 
								{
									t.tOldDist_f = t.mp_bullets[t.tbulletloop].soundDistFromPlayer;
									t.txDist_f = CameraPositionX(0) - SteamGetBulletX(t.tbulletloop);
									t.tyDist_f = CameraPositionY(0) - SteamGetBulletY(t.tbulletloop);
									t.tzDist_f = CameraPositionZ(0) - SteamGetBulletZ(t.tbulletloop);
									t.mp_bullets[t.tbulletloop].soundDistFromPlayer = Sqrt(t.txDist_f*t.txDist_f + t.tyDist_f*t.tyDist_f + t.tzDist_f*t.tzDist_f);
									t.tDistDiff_f = t.tOldDist_f - t.mp_bullets[t.tbulletloop].soundDistFromPlayer;
									t.tSoundMultiplier_f = 1 + (t.tDistDiff_f/t.ElapsedTime_f)*0.00015;
									if (  t.tSoundMultiplier_f < 0.5  )  t.tSoundMultiplier_f  =  0.5;
									if (  t.tSoundMultiplier_f > 2  )  t.tSoundMultiplier_f  =  2;
									t.mp_bullets[t.tbulletloop].btype = SteamGetBulletType(t.tbulletloop);
									SetSoundSpeed (  t.tSndID, t.WeaponProjectileBase[t.mp_bullets[t.tbulletloop].btype].soundDopplerBaseSpeed * t.tSoundMultiplier_f );
								}
							}
						}

					}
				}
				else
				{
					//  projectile ended, show result

					//  clean up particles
					if (  t.mp_bullets[t.tbulletloop].particles  !=  -1 ) 
					{
							t.tRaveyParticlesEmitterID=t.mp_bullets[t.tbulletloop].particles;
							ravey_particles_delete_emitter ( );
							t.mp_bullets[t.tbulletloop].particles=-1;
					}

					if (  ObjectExist(g.steamplayermodelsoffset+200+t.tbulletloop)  ==  1 ) 
					{

						t.tSteamProjectileType = t.mp_bullets[t.tbulletloop].btype;

						//  stop any looping sound
						if (  t.mp_bullets[t.tbulletloop].sound > 0 ) 
						{
							StopSound (  t.mp_bullets[t.tbulletloop].sound );
							t.mp_bullets[t.tbulletloop].sound = 0;
						}

						t.tsteamBObj = g.steamplayermodelsoffset+200+t.tbulletloop;

						t.tDeathSoundSoundID = t.WeaponProjectileBase[t.tSteamProjectileType].soundDeath;
						if (  t.tDeathSoundSoundID > 0 ) 
						{
							t.tSteamSoundID = g.steamsoundoffset+200+t.tbulletloop;
							if (  SoundExist(t.tSteamSoundID)  ==  0 ) 
							{
								CloneSound (  t.tSteamSoundID,t.tDeathSoundSoundID );
							}
							if (SoundExist(t.tSteamSoundID) == 1) {
								PositionSound(t.tSteamSoundID, ObjectPositionX(t.tsteamBObj), ObjectPositionY(t.tsteamBObj), ObjectPositionZ(t.tsteamBObj));
								SetSoundSpeed(t.tSteamSoundID, 38000 + Rnd(8000));
								PlaySound(t.tSteamSoundID);
							}
						}

						t.texplodex_f=ObjectPositionX(t.tsteamBObj);
						t.texplodey_f=ObjectPositionY(t.tsteamBObj);
						t.texplodez_f=ObjectPositionZ(t.tsteamBObj);
//       `texploderadius#=300.0

//       `mp.dontApplyDamage = 1

						explosion_rocket(t.texplodex_f,t.texplodey_f,t.texplodez_f);
//physics_explodesphere ( );
//       `mp.dontApplyDamage = 0

						DeleteObject (  t.tsteamBObj );

					}
				}
			}
	}
	
//  `print "Multiplayer rockets in use: " + Str(debugHowManyInUse)

return;


//Direct Multiplayer Instructions


}

void mp_destroyentity ( void )
{
	//  takes ttte
	SteamDeleteObject (  t.ttte );
return;

}

void mp_refresh ( void )
{
	SteamLoop (  );
return;

}

void mp_setMessage ( void )
{
	//  takes tmsg$
	g.mp.message = t.tmsg_s;
	g.mp.messageTime = Timer();
return;

}

void mp_setMessageDots ( void )
{
	//  takes tmsg$
	g.mp.messageDots = t.tmsg_s;
	g.mp.messageTimeDots = Timer();
return;

}

void mp_message ( void )
{
	if (  g.mp.message  !=  "" ) 
	{
//   `x = (GetDisplayWidth()/2) - (Text (  width(mp.message)/2) )

//   `text x,200,mp.message

		mp_text(-1,15,3,g.mp.message.Get());
		if (  Timer() - g.mp.messageTime > MP_MESSAGE_TIMOUT ) 
		{
			g.mp.message = "";
		}
	}
return;

}

void mp_messageDots ( void )
{
	if (  g.mp.messageDots  !=  "" ) 
	{
//   `x = (GetDisplayWidth()/2) - (Text (  width(mp.message)/2) )

//   `text x,200,mp.message

		mp_textDots(-1,15,3,g.mp.messageDots.Get());
		if (  Timer() - g.mp.messageTimeDots > MP_MESSAGE_TIMOUT ) 
		{
			g.mp.messageDots = "";
		}
	}
return;

}

void mp_update_projectile ( void )
{
	if (  t.tProj > 19  )  return;

	t.tSteamBullet = (g.mp.me*20) + t.tProj;

	t.tTime = Timer();
	if (  t.tTime - t.mp_bullets_send_time[t.tSteamBullet] < MP_PROJECTILE_UPDATE_DELAY && t.tSteamBulletOn  ==  1  )  return;

	t.mp_bullets_send_time[t.tSteamBullet] = t.tTime;
	
	t.mp_bullets[t.tSteamBullet].on = t.tSteamBulletOn;
	SteamSetBullet (  t.tSteamBullet , t.WeaponProjectile[t.tProj].xPos_f, t.WeaponProjectile[t.tProj].yPos_f, t.WeaponProjectile[t.tProj].zPos_f, t.WeaponProjectile[t.tProj].xAng_f, t.WeaponProjectile[t.tProj].yAng_f, t.WeaponProjectile[t.tProj].zAng_f, t.tProjType, t.tSteamBulletOn );

	return;
}

void mp_serverSetLuaGameMode ( void )
{
	SteamSendLua (  MP_LUA_ServerSetLuaGameMode,0,t.v );
return;

}

void mp_setServerTimer ( void )
{
	SteamSendLua (  MP_LUA_SetServerTimer,0,t.v );
return;

}

void mp_serverRespawnAll ( void )
{
	SteamSendLua (  MP_LUA_ServerRespawnAll,0,0 );
	mp_restoreEntities ( );
	mp_setLuaResetStats ( );
	mp_respawnEntities ( );
	t.playercontrol.jetpackhidden=0;
	t.playercontrol.jetpackmode=0;
	physics_no_gun_zoom ( );
	t.aisystem.processplayerlogic=1;
	g.mp.gameAlreadySpawnedBefore = 0;
	t.tindex = g.mp.me+1;

	//  Find start position for player
	t.tfoundone = 0;
	if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
	{
		t.tfoundone = 1;
	}
	else
	{
		t.tonetotry = t.tindex/2;
		if (  t.tonetotry > 0 ) 
		{
			if (  t.mpmultiplayerstart[t.tonetotry].active == 1 ) 
			{
				t.tfoundone = 1;
				t.tindex = t.tonetotry;
			}
		}
	}
	if (  t.tfoundone  ==  0 ) 
	{
		if (  t.mpmultiplayerstart[1].active == 1 ) 
		{
			t.tindex = 1;
			t.tfoundone = 1;
		}
	}

	if (  t.mpmultiplayerstart[t.tindex].active == 1 ) 
	{

		t.terrain.playerx_f=t.mpmultiplayerstart[t.tindex].x;
		t.terrain.playery_f=t.mpmultiplayerstart[t.tindex].y+20;
		t.terrain.playerz_f=t.mpmultiplayerstart[t.tindex].z;
		t.terrain.playerax_f=0;
		t.terrain.playeray_f=t.mpmultiplayerstart[t.tindex].angle;
		t.terrain.playeraz_f=0;

		g.mp.lastx=t.terrain.playerx_f;
		g.mp.lasty=t.terrain.playery_f;
		g.mp.lastz=t.terrain.playerz_f;
		g.mp.lastangley=t.terrain.playeray_f;
	}
	physics_resetplayer_core ( );
	t.tobj = t.entityelement[t.mp_playerEntityID[g.mp.me]].obj;
	if (  t.tobj > 0 ) 
	{
		PositionObject (  t.tobj, t.terrain.playerx_f, t.terrain.playery_f-70, t.terrain.playerz_f );
	}
	g.mp.gameAlreadySpawnedBefore = 0;
	t.player[t.plrid].health = 0;
	t.mp_health[g.mp.me] = 0;
	g.mp.endplay = 0;
	g.autoloadgun=0 ; gun_change ( );
return;

//  Put entities back to the original "first played" state
}

void mp_restoreEntities ( void )
{
//  `remstart

	for ( t.te = 1 ; t.te<=  g.mp.originalEntitycount; t.te++ )
	{
		t.tentid=t.entityelement[t.te].bankindex;
		if (  t.tentid>0 ) 
		{
			if (  t.entityprofile[t.tentid].ischaracter  ==  1 || t.entityelement[t.te].mp_isLuaChar  ==  1 ) 
			{
//     `entityelement(te) = steamStoreentityelement(te)

				t.entityelement[t.te].x = t.steamStoreentityelement[t.te].x;
				t.entityelement[t.te].y = t.steamStoreentityelement[t.te].y;
				t.entityelement[t.te].z = t.steamStoreentityelement[t.te].z;
				ScaleObject (  t.entityelement[t.te].obj,100,100,100 );
				t.entityelement[t.te].health = 100;
				PositionObject (  t.entityelement[t.te].obj,t.entityelement[t.te].x,t.entityelement[t.te].y,t.entityelement[t.te].z );
				t.entityelement[t.te].mp_coopControlledByPlayer = -1;
				t.entityelement[t.te].mp_updateOn = 0;
				t.entityelement[t.te].active = 1;
				AISetEntityActive (  t.entityelement[t.te].obj,1 );
			}
		}
	}
//  `remend

return;

}

void mp_serverEndPlay ( void )
{
	SteamSendLua (  MP_LUA_ServerEndPlay,0,0 );
	t.playercontrol.jetpackhidden=0;
	t.playercontrol.jetpackmode=0;
	physics_no_gun_zoom ( );
	g.mp.endplay = 1;
	t.aisystem.processplayerlogic = 0;
	g.autoloadgun=0 ; gun_change ( );
return;

}

void mp_setServerKillsToWin ( void )
{
	g.mp.setserverkillstowin = t.v;
return;

}

void mp_networkkill ( void )
{

	//  get damage amount to set it back to 0
	t.tdamage = SteamGetPlayerDamageAmount();

	t.tsteamlastdamageincounter = t.tsteamlastdamageincounter + 1;
	t.tsource = t.entityelement[t.texplodesourceEntity].mp_killedby;
	t.te = t.mp_playerEntityID[t.tsource];
	
	g.mp.killedByPlayerFlag = 1;
	g.mp.playerThatKilledMe = t.tsource;
	t.tsteamforce = 500;
	SteamKilledBy (  g.mp.playerThatKilledMe , CameraPositionX(), CameraPositionY(), CameraPositionZ(), t.tsteamforce, 0 );
	g.mp.dyingTime = Timer();

return;

}

void mp_lobbyListBox ( void )
{
	t.tluaTextCenterX = 0;
	if (  g.mp.listboxmode  ==  0 ) 
	{
		t.tsize = SteamGetLobbyListSize();

		t.tLeft = 5;
		t.tTop = 5;
		t.tRight = (26*10)+10;
		t.tBottom = 98+110+70;
		InkEx ( 20, 20, 20 );// Rgb(20,20,20),Rgb(20,20,20) );
		BoxEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
		InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 );
		LineEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
		LineEx (  t.tLeft,t.tTop,t.tLeft,t.tBottom );
		LineEx (  t.tLeft,t.tBottom,t.tRight,t.tBottom );
		LineEx (  t.tRight,t.tTop,t.tRight,t.tBottom );

		InkEx (  255, 255, 255 );//Rgb ( 255,255,255),0 );
		BoxEx (  20,25,40,45 );
		InkEx (  255, 255, 50 );//Rgb (255,255,50),0 );
		BoxEx (  20,60,40,80 );
		InkEx (  255, 100, 100 );//Rgb (255,100,100),0 );
		BoxEx (  20,195,40,215 );

		t.tluarealcoords = 1;
		mp_textColor(50,20,1,"You can join this Lobby",255,255,255);
		t.tluarealcoords = 1;
		mp_textColor(50,55,1,"Join to subscribe and",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,80,1,"download the content",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,105,1,"for this game. The",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,130,1,"lobby will turn white",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,155,1,"when downloaded",255,255,50);
		t.tluarealcoords = 1;
		mp_textColor(50,190,1,"Please restart",255,100,100);
		t.tluarealcoords = 1;
		mp_textColor(50,215,1,"GameGuru to",255,100,100);
		t.tluarealcoords = 1;
		mp_textColor(50,240,1,"update this game",255,100,100);
	}
	if (  g.mp.listboxmode  ==  1 ) 
	{
		t.tsize = t.tempsteamhowmanyfpmsarethere;
	}
	

	t.tTop_f = 20.0 * (GetDisplayHeight() / 100.0);
	t.tleft_f = 30.0 * (GetDisplayWidth() / 100.0);
	t.tBottom_f = 75.0 * (GetDisplayHeight() / 100.0);
	t.tright_f = 70.0 * (GetDisplayWidth() / 100.0);

	t.tTop = t.tTop_f;
	t.tLeft = t.tleft_f;
	t.tBottom = t.tBottom_f;
	t.tRight = t.tright_f;

	t.tempsteamyminY_f = (GetDisplayHeight() * 0.25) - (GetDisplayHeight() * 0.025);
	t.tempsteamymaxY_f = GetDisplayHeight() * 0.75;
	t.tempsteamyminX_f = GetDisplayWidth() * 0.30;
	t.tempsteamymaxX_f = GetDisplayWidth() * 0.65;
	t.tempsteamselected_f = g.mp.selectedLobby - g.mp.lobbyoffset;
	t.tempmissthisone_f = t.tempsteamselected_f;
	if (  t.tempsteamselected_f  >=  0 && t.tempsteamselected_f  <= 9 ) 
	{
		t.tempsteamselectedY_f = t.tempsteamyminY_f + (t.tempsteamselected_f * (GetDisplayHeight() * 0.05));
		InkEx ( 128, 128, 128 );//  Rgb ( 128,128,128),0 );
		BoxEx (  t.tLeft,t.tempsteamselectedY_f,t.tRight,t.tempsteamselectedY_f+(GetDisplayHeight() * 0.05) );
		if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0  )  g.mp.selectedLobby  =  t.tempsteamselected_f+g.mp.lobbyoffset;
	}

	t.tempsteamyminY_f = (GetDisplayHeight() * 0.25) - (GetDisplayHeight() * 0.025);
	t.tempsteamymaxY_f = GetDisplayHeight() * 0.75;
	t.tempsteamyminX_f = GetDisplayWidth() * 0.30;
	t.tempsteamymaxX_f = GetDisplayWidth() * 0.65;
	if (  t.mx  >=  t.tempsteamyminX_f && t.mx  <=  t.tempsteamymaxX_f ) 
	{
		if (  t.my  >=  t.tempsteamyminY_f && t.my  <=  t.tempsteamymaxY_f ) 
		{
			t.my_f = t.my;
			t.tempsteamselected_f = Floor((t.my_f - t.tempsteamyminY_f) / (GetDisplayHeight() * 0.05));
			if (  t.tempsteamselected_f  >=  0 && t.tempsteamselected_f < 10 && t.tempsteamselected_f  !=  t.tempmissthisone_f ) 
			{
				if (  t.tempsteamselected_f+g.mp.lobbyoffset < t.tsize ) 
				{
					t.tempsteamselectedY_f = t.tempsteamyminY_f + (t.tempsteamselected_f * (GetDisplayHeight() * 0.05));
					InkEx ( 64, 64, 64 );//  Rgb ( 64,64,64),0 );
					BoxEx (  t.tLeft,t.tempsteamselectedY_f,t.tRight,t.tempsteamselectedY_f+(GetDisplayHeight() * 0.05) );
					if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0  )  g.mp.selectedLobby  =  t.tempsteamselected_f+g.mp.lobbyoffset;
				}
			}
		}
	}

	InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 );
	LineEx (  t.tLeft,t.tTop,t.tRight,t.tTop );
	LineEx (  t.tLeft,t.tTop,t.tLeft,t.tBottom );
	LineEx (  t.tLeft,t.tBottom,t.tRight,t.tBottom );
	LineEx (  t.tRight,t.tTop,t.tRight,t.tBottom );

	t.toffx_f = (1.0 * GetDisplayWidth()) / 100.0;
	t.toffx = t.toffx_f;
	InkEx ( 30, 30, 30 );// Rgb ( 30,30,30),0 );
	BoxEx (  t.tRight-(t.toffx*2)-8,t.tTop+1,t.tRight-1,t.tBottom-1 );

	t.tTop += 4;
	t.tBottom -= 4;
	t.tRight -= 4;

	t.toffx_f = (1.0 * GetDisplayWidth()) / 100.0;
	t.toffx = t.toffx_f;
	t.toffy_f = (1.0 * GetDisplayHeight()) / 100.0;
	t.toffy = t.toffy_f;

	InkEx ( 255, 255, 255 );//  Rgb (255,255,255),0 );
	if (  t.mx > t.tLeft && t.mx < t.tRight ) 
	{
		if (  t.my > t.tTop && t.my < t.tTop+(t.toffy_f*2) ) 
		{
			InkEx ( 128, 128, 128 );//  Rgb (128,128,128),0 ) ;
			if (  t.mc  ==  1 ) 
			{
				if (  Timer() - t.tempsteamscrollclicktimer > 100 ) 
				{
					--g.mp.lobbyoffset;
					t.tempsteamscrollclicktimer = Timer();
				}
			}
		}
	}

	LineEx (  t.tRight-t.toffx,t.tTop,t.tRight-(t.toffx*2), t.tTop+(t.toffy*2) );
	LineEx (  t.tRight-t.toffx,t.tTop,t.tRight, t.tTop+(t.toffy*2) );
	LineEx (  t.tRight-(t.toffx*2),t.tTop+(t.toffy*2),t.tRight,t.tTop+(t.toffy*2) );

	InkEx ( 255, 255, 255 );// Rgb (255,255,255),0 ) ;
	if (  t.mx > t.tLeft && t.mx < t.tRight ) 
	{
		if (  t.my > t.tBottom-(t.toffy*2) && t.my < t.tBottom ) 
		{
			InkEx ( 128, 128, 128 );// Rgb (128,128,128),0 ) ;
			if (  t.mc  ==  1 ) 
			{
				if (  Timer() - t.tempsteamscrollclicktimer > 100 ) 
				{
					++g.mp.lobbyoffset;
					t.tempsteamscrollclicktimer = Timer();
				}
			}
		}
	}
	

	LineEx (  t.tRight-t.toffx,t.tBottom,t.tRight-(t.toffx*2), t.tBottom-(t.toffy*2) );
	LineEx (  t.tRight-t.toffx,t.tBottom,t.tRight, t.tBottom-(t.toffy*2) );
	LineEx (  t.tRight-(t.toffx*2),t.tBottom-(t.toffy*2),t.tRight,t.tBottom-(t.toffy*2) );

	if (  g.mp.lobbyscrollbarOn  ==  0 || t.mc  ==  0 ) 
	{
		g.mp.lobbyscrollbarOn = 0;
		t.tboxsize_f = (10.0 * GetDisplayHeight()) / 100.0;
		t.tboxsize = t.tboxsize_f;
		t.tloboffset_f = g.mp.lobbyoffset;
		t.tsize_f = t.tsize;
		t.tboxoffset_f = (t.tloboffset_f / t.tsize_f) * 100.0;
		if (  t.tboxoffset_f < 0.0  )  t.tboxoffset_f  =  0.0;
		if (  t.tboxoffset_f > 100.0  )  t.tboxoffset_f  =  100.0;
		t.tboxoffset_f = (t.tboxoffset_f * (GetDisplayHeight() * 0.42)) / 100.0;
		t.tboxoffset = t.tboxoffset_f;
		t.tboxtop = t.tTop+(t.toffy*2) + t.tboxoffset + 2;
		InkEx ( 255, 255, 255 );//  Rgb (255,255,255),0 ) ;
		if (  t.mx > t.tRight-(t.toffx*2) && t.mx < t.tRight ) 
		{
			if (  t.my > t.tboxtop && t.my < t.tboxtop+t.tboxsize ) 
			{
				InkEx ( 160, 160, 160 );// Rgb (160,160,160),0 ) ;
				if (  t.mc  ==  1 && t.tempsteamoldmc  ==  0 ) 
				{
					g.mp.lobbyscrollbarOn = 1;
					g.mp.lobbyscrollbarOffset = t.tboxtop-t.my;
					g.mp.lobbyscrollbarOldY = t.my;
				}
			}
		}

		BoxEx (  t.tRight-(t.toffx*2),t.tboxtop,t.tRight,t.tboxtop+t.tboxsize );

	}
	else
	{
		g.mp.lobbyscrollbarOldY = t.my;
		t.tboxtop = t.my+g.mp.lobbyscrollbarOffset;
		if (  t.tboxtop < t.tTop+(t.toffy*2)+2  )  t.tboxtop  =  t.tTop+(t.toffy*2)+2;
		if (  t.tboxtop > t.tTop+(t.toffy*2)+2 + ((100.0 * (GetDisplayHeight() * 0.40)) / 100.0)  )  t.tboxtop  =  t.tTop+(t.toffy*2)+2 + ((100.0 * (GetDisplayHeight() * 0.40)) / 100.0);
		t.tboxsize_f = (10.0 * GetDisplayHeight()) / 100.0;
		t.tboxsize = t.tboxsize_f;
		InkEx ( 160, 160, 160 );// Rgb (160,160,160),0 ) ;
		BoxEx (  t.tRight-(t.toffx*2),t.tboxtop,t.tRight,t.tboxtop+t.tboxsize );

		//  update the list to reflect where the bar is
		t.tboxtop_f = t.tboxtop - (t.tTop+(t.toffy*2)+2);
		t.tempsteamperc_f = (t.tboxtop_f / (GetDisplayHeight() * 0.42)) * 100.0;
		if (  t.tempsteamperc_f < 0.0  )  t.tempsteamperc_f  =  0.0;
		if (  t.tempsteamperc_f > 100.0  )  t.tempsteamperc_f  =  100.0;
		t.tsize_f = t.tsize;
		t.tempsteamnewoffset_f = (t.tempsteamperc_f * t.tsize_f) / 100.0;
		g.mp.lobbyoffset = t.tempsteamnewoffset_f;

	}

	if (  g.mp.lobbyoffset > t.tsize-10  )  g.mp.lobbyoffset  =  t.tsize-10;
	if (  g.mp.lobbyoffset < 0  )  g.mp.lobbyoffset  =  0;

	InkEx ( 255, 255, 255 );// Rgb(255,255,255),0 );

	t.tempsteamoldmc = t.mc;

	if (  g.mp.listboxmode  ==  0 ) 
	{
		if (  t.tsize  ==  1 ) 
		{
			t.tstring_s = "1 lobby found";
		}
		else
		{
			t.tstring_s = cstr(cstr(Str(t.tsize)) + " lobbies found");
		}
	}
	if (  g.mp.listboxmode  ==  1 ) 
	{
		t.tstring_s = cstr(cstr(Str(t.tsize)) + " levels found");
	}
	mp_text(-1,15,1,t.tstring_s.Get());

	if (  t.tsize > 0  )  g.mp.lobbycount  =  t.tsize;
	t.teampsteamy = 25;
	t.tlobbycount = 0;
	for ( t.c = 0 ; t.c<=  t.tsize-1; t.c++ )
	{
			if (  t.c  >=  g.mp.lobbyoffset && t.c < g.mp.lobbyoffset+10 ) 
			{
				if (  g.mp.listboxmode  ==  0 ) 
				{
					t.mp_lobbies_s[t.tlobbycount] = SteamGetLobbyListName(t.c);

					if (  cstr(Left(t.mp_lobbies_s[t.tlobbycount].Get(),5))  ==  "Lobby" || Len(t.mp_lobbies_s[t.tlobbycount].Get()) < 8 ) 
					{
						t.mp_lobbies_s[t.tlobbycount] = "Waiting for lobby details...";
					}
					if (  g.mp.selectedLobby  ==  t.c  )  g.mp.selectedLobbyName  =  t.mp_lobbies_s[t.tlobbycount];

					t.tempMPLobbyNameFromList_s = t.mp_lobbies_s[t.tlobbycount];
					mp_canIJoinThisLobby ( );
					t.tsteamstring_s = g.mp.lobbyjoinedname;
					if (  t.tsteamcanjoinlobby  ==  1 ) 
					{
						t.tr = 255;
						t.tg = 255;
						t.tb = 255;
					}
					else
					{
						if (  t.tsteamcanjoinlobby  ==  2 ) 
						{
							t.tr = 255;
							t.tg = 100;
							t.tb = 100;
						}
						else
						{
							t.tr = 255;
							t.tg = 255;
							t.tb = 50;
						}
					}
				}
				if (  g.mp.listboxmode  ==  1 ) 
				{
					t.mp_lobbies_s[t.tlobbycount] = t.tfpmfilelist_s[t.c];
					t.tsteamstring_s = t.mp_lobbies_s[t.tlobbycount];
					t.tr = 255;
					t.tg = 255;
					t.tb = 255;
				}

				t.tlobbytring_s = t.tsteamstring_s;
				if ( t.tsteamcanjoinlobby == 0  ) mp_checkItemSubbed ( );
				mp_textColor(32,t.teampsteamy,1,t.tlobbytring_s.Get(),t.tr,t.tg,t.tb);
				t.teampsteamy += 5;
				++t.tlobbycount;
			}
	}
}

void mp_createLobby ( void )
{
	g.mp.haveToldAboutSolo = 0;
	t.tempsteamhostlobbyname_s = cstr(SteamGetPlayerName()) + cstr("'s Lobby:");
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		t.tempsteamlevelname_s = cstr(SteamGetPlayerName()) + cstr("'s Level:");
	}
	else
	{
		t.tempsteamlevelname_s = cstr(Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-4)) + cstr(":");
	}
	//  grab version number
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s + "worklevel.dat";		
	}
	else
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
	}

	t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
	mp_grabWorkshopChangedFlagAndVersion ( );

	SteamSetLobbyName (  cstr(t.tempsteamhostlobbyname_s+t.tempsteamlevelname_s+g.mp.workshopid+":"+Str(t.tMPshopTheVersionNumber)).Get() );
	SteamCreateLobby (  );
	g.mp.isGameHost = 1;
	g.mp.mode = MP_WAITING_FOR_LOBBY_CREATION;
	t.tempsteamlobbycreationtimeout = Timer();
return;

}

void mp_searchForLobbies ( void )
{
	SteamGetLobbyList (  );
	g.mp.mode = MP_MODE_LOBBY;
	g.mp.isGameHost = 0;
return;

}

void mp_searchForFpms ( void )
{
	g.mp.mode = MP_SERVER_CHOOSING_FPM_TO_USE;
	t.told_s=GetDir();
	//SetDir (  cstr(g.fpscrootdir_s + "\\Files\\mapbank").Get() );
	SetDir ( g.mysystem.mapbankAbs_s.Get() );
	ChecklistForFiles (  );
	Dim (  t.tfpmfilelist_s,ChecklistQuantity( ) );
	t.tempsteamhowmanyfpmsarethere = 0;
	for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
	{
		if (  ChecklistValueA(t.c) == 0 ) 
		{
			t.tfile_s=ChecklistString(t.c);
			if (  t.tfile_s != "." && t.tfile_s != ".." ) 
			{
				if (  cstr(Lower(Right(t.tfile_s.Get(),4)))  ==  ".fpm" ) 
				{
					t.tfpmfilelist_s[t.tempsteamhowmanyfpmsarethere] = t.tfile_s;
					++t.tempsteamhowmanyfpmsarethere;
				}
			}
		}
	}
	SetDir (  t.told_s.Get() );
}

void mp_launchGame ( void )
{
	g.mp.launchServer = 1;
}

void mp_backToStart ( void )
{
	mp_resetGameStats ( );
}

void mp_selectedALevel ( void )
{
	cstr mlevel_s = g.mysystem.editorsGrideditAbs_s + "__multiplayerlevel__.fpm";
	if ( FileExist( mlevel_s.Get())  ) DeleteAFile ( mlevel_s.Get() );
	g.mp.fpmpicked = t.tfpmfilelist_s[g.mp.selectedLobby];
	mp_checkIfLevelHasCustomContent ( );
	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		cstr worklevel_s = g.mysystem.editorsGrideditAbs_s + "worklevel.fpm";
		CopyAFile ( worklevel_s.Get(),mlevel_s.Get() );
	}
	else
	{
		//CopyAFile (  cstr(g.fpscrootdir_s+"\\Files\\mapbank\\"+g.mp.fpmpicked).Get(),cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
		CopyAFile ( cstr(g.mysystem.mapbankAbs_s+g.mp.fpmpicked).Get(), cstr(g.fpscrootdir_s+"\\Files\\editors\\gridedit\\__multiplayerlevel__.fpm").Get() );
	}
	if (  g.mp.levelContainsCustomContent  ==  1 ) 
	{
		//  first we check if the changed flag is set (they have saved since hosting) if not, we dont need to upload to steam
		if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
		{
			//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
			t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s + "worklevel.dat";
		}
		else
		{
			//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
			t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		}

		t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
		mp_grabWorkshopChangedFlagAndVersion ( );
		g.mp.workshopItemChangedFlag = t.tMPshopHasItemChangedFlag;
		if (  t.tMPshopHasItemChangedFlag  ==  1 ) 
		{
			g.mp.mode = MP_SERVER_CHOOSING_TO_MAKE_FPS_WORKSHOP;
		}
		else
		{
			g.mp.workshopid = t.tMPshopTheIDNumber_s;
		}
	}

return;

}

void mp_checkIfLevelHasCustomContent ( void )
{

	if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";
	}
	else
	{
		//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
		t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
	}

	if (  FileExist(t.tempsteammaptocheck_s.Get()) ) 
	{
		g.mp.levelContainsCustomContent = 1;
	}
	else
	{
		g.mp.levelContainsCustomContent = 0;
	}

return;

}

void mp_buildWorkShopItem ( void )
{
	g.mp.mode = MP_CREATING_WORKSHOP_ITEM;
	switch (  g.mp.buildingWorkshopItemMode ) 
	{
		case 0:
			if (  PathExist( cstr(g.fpscrootdir_s+"\\Files\\editors\\workshopItem").Get() )  ==  0 ) 
			{
				MakeDirectory ( cstr(g.fpscrootdir_s+"\\Files\\editors\\workshopItem").Get() );
			}
			else
			{
				t.told_s=GetDir();
				SetDir (  cstr(g.fpscrootdir_s + "\\Files\\editors\\workshopItem").Get() );
				ChecklistForFiles (  );

				for ( t.c = 1 ; t.c<=  ChecklistQuantity(); t.c++ )
				{
					if (  ChecklistValueA(t.c) == 0 ) 
					{
						t.tfile_s=ChecklistString(t.c);
						if (  t.tfile_s != "." && t.tfile_s != ".." ) 
						{
							DeleteAFile (  t.tfile_s.Get() );
						}
					}
				}
				SetDir (  t.told_s.Get() );
			}

			if (  FileOpen(1)  ==  1  )  CloseFile (  1 );

			if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
			{
				t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";//g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
				t.tempsteamleveltocopy_s = g.mysystem.mapbankAbs_s+"worklevel.fpm";//g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.fpm";
			}
			else
			{
				t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";//g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
				t.tempsteamleveltocopy_s = g.mysystem.mapbankAbs_s+g.mp.fpmpicked;//g.fpscrootdir_s+"\\Files\\mapbank\\" + g.mp.fpmpicked;
			}

			t.tempsteamdestworkshopfolder_s = g.fpscrootdir_s + "\\Files\\editors\\workshopItem\\";
			CopyAFile (  t.tempsteamleveltocopy_s.Get(), cstr(t.tempsteamdestworkshopfolder_s + "editors_gridedit___multiplayerlevel__.fpm").Get() );
			//  grab clanged flag and version number (we dont need the changed flag here tho)
			t.tmphopitemtocheckifchangedandversion_s = t.tempsteammaptocheck_s;
			mp_grabWorkshopChangedFlagAndVersion ( );

			OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
			g.mp.buildingWorkshopItemMode = 1;
			//  skip over the warning, change flag, version number and workshop id
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			t.tempsteamstring_s = ReadString ( 1 );
			//  skip over the fpm in the file
			t.tempsteamstring_s = ReadString ( 1 );

		break;
//   ``````````````````````````````````

		case 1:
			t.tempsteamdestworkshopfolder_s = g.fpscrootdir_s + "\\Files\\editors\\workshopItem\\";
			//  Write out version number
			if (  FileOpen(2)  )  CloseFile (  2 );
			OpenToWrite (  2, cstr(t.tempsteamdestworkshopfolder_s+"version.dat").Get() );
			WriteString (  2,Str(t.tMPshopTheVersionNumber) );
			CloseFile (  2 );

			t.tempsteamstring_s = ReadString ( 1 );
			t.tempMPshopfilename_s = "";
			for ( t.c = 1 ; t.c<=  Len(t.tempsteamstring_s.Get()); t.c++ )
			{
				if (  cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "\\" || cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "/" ) 
				{
					t.tempMPshopfilename_s=t.tempMPshopfilename_s+"_";
				}
				else
				{
					if (  cstr(Mid(t.tempsteamstring_s.Get(),t.c))  ==  "_" ) 
					{
						t.tempMPshopfilename_s=t.tempMPshopfilename_s+"@";
					}
					else
					{
						t.tempMPshopfilename_s=t.tempMPshopfilename_s+Mid(t.tempsteamstring_s.Get(),t.c);
					}
				}
			}

			if (  t.tempsteamstring_s  !=  "" ) 
			{
				CopyAFile (  cstr(g.fpscrootdir_s+"\\Files\\"+t.tempsteamstring_s).Get() , cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
				t.tsteamyesencrypt = 0;
				//  models
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".dbo"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),2))  ==  ".x"  )  t.tsteamyesencrypt  =  1;
				//  images
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".png"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".jpg"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".dds"  )  t.tsteamyesencrypt  =  1;
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".bmp"  )  t.tsteamyesencrypt  =  1;
				//  sounds and music
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".wav"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4))  ==  ".mp3"  )  t.tsteamyesencrypt  =  1;
				if ( cstr( Right(t.tempMPshopfilename_s.Get(),4) ) ==  ".ogg"  )  t.tsteamyesencrypt  =  1;
				//  scripts
				if (  cstr(Right(t.tempMPshopfilename_s.Get(),4))  ==  ".fpe"  )  t.tsteamyesencrypt  =  1;

				if (  t.tsteamyesencrypt  ==  1 ) 
				{
					EncryptWorkshopDBPro ( cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
					DeleteAFile (  cstr(t.tempsteamdestworkshopfolder_s + t.tempMPshopfilename_s).Get() );
				}
			}
			if (  FileEnd(1) ) 
			{
				g.mp.buildingWorkshopItemMode = 2;
			}
		break;
//   ``````````````````````````````````

		case 2:

			if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
			{
				t.tempMPshopname_s = "My Level";
			}
			else
			{
				t.tempMPshopname_s = Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-4);
			}

			SteamSetRoot( cstr( g.fpscrootdir_s+"\\Files\\" ).Get() );
			SteamCreateWorkshopItem (  t.tempMPshopname_s.Get() );

			g.mp.buildingWorkshopItemMode = 3;
		break;
//   ``````````````````````````````````

		case 3:
			if (  SteamIsWorkshopItemUploaded()  ==  1 ) 
			{
				//  set changed flag to 0
				if (  g.mp.fpmpicked  ==  "Level I just worked on" ) 
				{
					//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\worklevel.dat";
					t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+"worklevel.dat";
				}
				else
				{
					//t.tempsteammaptocheck_s = g.fpscrootdir_s+"\\Files\\mapbank\\"+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
					t.tempsteammaptocheck_s = g.mysystem.mapbankAbs_s+Left(g.mp.fpmpicked.Get(),Len(g.mp.fpmpicked.Get())-3)+"dat";
				}
				if (  FileOpen(1)  ==  1  )  CloseFile (  1 );
				//  Work out how many lines there are so we can Dim (  the right amount )
				t.thowmanylines = 0;
				OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
					while (  FileEnd(1) == 0 ) 
					{
						t.tthrowawaystring_s = ReadString ( 1 );
						++t.thowmanylines;
					}
				CloseFile (  1 );
				////  now read in the whole thing
				Dim (  t.templines,t.thowmanylines );
				t.thowmanylines = 0;
				OpenToRead (  1,t.tempsteammaptocheck_s.Get() );
					while (  FileEnd(1) == 0 ) 
					{
						t.templines[t.thowmanylines] = ReadString ( 1 );
						++t.thowmanylines;
					}
				CloseFile (  1 );
				DeleteAFile (  t.tempsteammaptocheck_s.Get() );
				OpenToWrite (  1,t.tempsteammaptocheck_s.Get() );
				g.mp.workshopid = SteamGetWorkshopID();

				t.templines[1] = "0";
				t.templines[3] = g.mp.workshopid;
				for ( t.tloop = 0 ; t.tloop<=  t.thowmanylines-1; t.tloop++ )
				{
					WriteString (  1,t.templines[t.tloop].Get() );
				}
				CloseFile (  1 );

				//  get rid of the array
				UnDim (  t.templines );

				g.mp.buildingWorkshopItemMode = 99;
			}

			if (  SteamIsWorkshopItemUploaded()  ==  -1  )  g.mp.buildingWorkshopItemMode  =  98;
		break;
	}//~ return
return;

}

void mp_buildingWorkshopItemFailed ( void )
{
	t.tsteamlostconnectioncustommessage_s = "Could not build workshop item (Error MP015)";
	mp_lostConnection ( );
	g.mp.mode = MP_SERVER_CHOOSING_FPM_TO_USE;
return;

}

void mp_joinALobby ( void )
{
	t.a = g.mp.selectedLobby;
	if (  g.mp.selectedLobbyName  !=  "Getting Lobby details..." ) 
	{
		g.mp.lobbyjoinedname = g.mp.selectedLobbyName;
		t.tempsteamstringlobbyname_s = "";
		t.tempsteamgotto = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			++t.tempsteamgotto;
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" ) { t.tempsteamgotto+=2 ; break; }
			t.tempsteamstringlobbyname_s = t.tempsteamstringlobbyname_s + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
		}
		g.mp.levelnametojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 1 ) 
				{
					g.mp.levelnametojoin = g.mp.levelnametojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopidtojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 2 ) 
				{
					g.mp.workshopidtojoin = g.mp.workshopidtojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopVersionNumberToJoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 3 ) 
				{
					g.mp.workshopVersionNumberToJoin = g.mp.workshopVersionNumberToJoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.lobbyjoinedname = t.tempsteamstringlobbyname_s;

		//  Check here if there is a workshop item, if the user has subbed and downloaded
		if (  g.mp.workshopidtojoin  !=  "0" ) 
		{

			t.tempMPLobbyNameFromList_s = g.mp.selectedLobbyName;
			mp_canIJoinThisLobby ( );
			t.tsteamstring_s = g.mp.lobbyjoinedname;

			if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  0 ) 
			{
				//  show screen asking if they want to subscribe
				g.mp.mode = MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM;
				titles_steamdoyouwanttosubscribetoworkshopitem ( );
				return;
			}
			else
			{
				if (  t.tsteamcanjoinlobby  ==  2 ) 
				{
					//  show screen asking if they want to subscribe
					g.mp.mode = MP_TELLING_THEY_NEED_TO_RESTART;
					titles_steamdTellingToRestart ( );
					return;
				}
			}
		}

		SteamJoinLobby(t.a);
		g.mp.mode = MP_JOINING_LOBBY;
		t.tsteamwaitedforlobbytimer = Timer();
		g.mp.AttemptedToJoinLobbyTime = Timer();
		g.mp.lobbycount = 0;
	}
return;

}

void mp_canIJoinThisLobby ( void )
{
	if (  g.mp.selectedLobbyName  !=  "Getting Lobby details..." ) 
	{
		g.mp.lobbyjoinedname = t.tempMPLobbyNameFromList_s;
		t.tempsteamstringlobbyname_s = "";
		t.tempsteamgotto = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			++t.tempsteamgotto;
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc) ) ==  ":" ) { t.tempsteamgotto += 2 ; break; }
			t.tempsteamstringlobbyname_s = t.tempsteamstringlobbyname_s + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
		}
		g.mp.levelnametojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ")" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 1 ) 
				{
					g.mp.levelnametojoin = g.mp.levelnametojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.workshopidtojoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 2 ) 
				{
					g.mp.workshopidtojoin = g.mp.workshopidtojoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		//  grab version number
		g.mp.workshopVersionNumberToJoin = "";
		t.tempsteamfoundone = 0;
		for ( t.tc = 1 ; t.tc<=  Len(g.mp.lobbyjoinedname.Get()); t.tc++ )
		{
			if (  cstr(Mid(g.mp.lobbyjoinedname.Get(),t.tc))  ==  ":" )
			{
				++t.tempsteamfoundone;
			}
			else
			{
				if (  t.tempsteamfoundone == 3 ) 
				{
					g.mp.workshopVersionNumberToJoin = g.mp.workshopVersionNumberToJoin + Mid(g.mp.lobbyjoinedname.Get(),t.tc);
				}
			}
		}
		g.mp.lobbyjoinedname = t.tempsteamstringlobbyname_s;

		//  Check here if there is a workshop item, if the user has subbed and downloaded
		if (  g.mp.workshopidtojoin  !=  "0" && g.mp.workshopidtojoin  !=  "" ) 
		{
			if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  0 ) 
			{
				t.tsteamcanjoinlobby = 0;
			}
			else
			{
				t.tsteamcanjoinlobby = 0;
				if (  SteamIsWorkshopItemInstalled(g.mp.workshopidtojoin.Get())  ==  2  )  t.tsteamcanjoinlobby  =  2;
				if (  t.tsteamcanjoinlobby  ==  0 ) 
				{
					t.tpath_s = SteamGetWorkshopItemPath();
					t.tfiletocheck_s = t.tpath_s + "\\version.dat";
					if (  FileExist(t.tfiletocheck_s.Get())  ==  1 ) 
					{
						if (  FileOpen(1)  )  CloseFile (  1 );
						OpenToRead (  1,t.tfiletocheck_s.Get() );
						t.tversioncheck_s = ReadString ( 1 );
						CloseFile (  1 );
						if (  t.tversioncheck_s  ==  g.mp.workshopVersionNumberToJoin ) 
						{
							t.tsteamcanjoinlobby = 1;
						}
						else
						{
							t.tsteamcanjoinlobby = 2;
						}
					}
				}
			}
		}
		else
		{
			t.tsteamcanjoinlobby = 1;
		}

	}
	else
	{
		t.tsteamcanjoinlobby = 0;
	}
return;

}

void mp_leaveALobby ( void )
{
	SteamLeaveLobby (  );
	mp_resetGameStats ( );
return;

}

void mp_SubscribeToWorkShopItem ( void )
{
	t.tlobbytring_s = g.mp.lobbyjoinedname;
	mp_subbedToItem ( );
	g.mp.mode = MP_ASKING_IF_SUBSCRIBE_TO_WORKSHOP_ITEM_WAITING_FOR_RESULTS;
}

void mp_save_workshop_files_needed ( void )
{
	cstr toriginalMasterLevelFile_s = "";
	cstr toriginalprojectname_s = "";

	toriginalMasterLevelFile_s = t.tmasterlevelfile_s;
	toriginalprojectname_s = g.projectfilename_s;
	//  If there is no baseList.dat file we cant proceed
	if (  FileExist("editors\\baseList.dat")  ==  0  )  return;

	//  Work out how many lines there are so we can Dim (  the right amount )
	t.thowmanyfpefiles = 0;
	OpenToRead (  1,"editors\\baseList.dat" );
	while (  FileEnd(1) == 0 ) 
	{
		t.tthrowawaystring_s = ReadString ( 1 );
		++t.thowmanyfpefiles;
	}
	CloseFile (  1 );

	//  Store the count in our global steamworks type
	g.mp.howmanyfpefiles = t.thowmanyfpefiles;

	Dim (  t.tallfpefiles_s,t.thowmanyfpefiles  );
	t.thowmanyfpefiles = 0;
	OpenToRead (  1,"editors\\baseList.dat" );
	while (  FileEnd(1) == 0 ) 
	{
		t.tallfpefiles_s[t.thowmanyfpefiles] = ReadString ( 1 );
		++t.thowmanyfpefiles;
	}
	CloseFile (  1 );

	t.exename_s=t.tsteamsavefilename_s;
	if (  cstr(Lower(Right(t.exename_s.Get(),4))) == ".fpm" ) 
	{
		t.exename_s=Left(t.exename_s.Get(),Len(t.exename_s.Get())-4);
	}
	for ( t.n = Len(t.exename_s.Get()) ; t.n >= 1 ; t.n+= -1 )
	{
		if (  cstr(Mid(t.exename_s.Get(),t.n)) == "\\" || cstr(Mid(t.exename_s.Get(),t.n)) == "/" ) 
		{
			t.exename_s=Right(t.exename_s.Get(),Len(t.exename_s.Get())-t.n);
			break;
		}
	}
	if (  Len(t.exename_s.Get())<1  )  t.exename_s = "sample";

	//  the level
	t.tmasterlevelfile_s=cstr("mapbank\\")+t.exename_s+".fpm";
	t.strwork = "" ; t.strwork = t.strwork + "Saving required files list for "+ t.tmasterlevelfile_s;
	timestampactivity(0, t.strwork.Get() );

	//  Get absolute My Games folder
	g.exedir_s="?";
	t.told_s=GetDir();
	t.tworkshoplistfile_s = t.told_s+"\\mapbank\\" + t.exename_s + ".dat";
	t.tMPshopTheVersionNumber = 1;
	if (  FileExist(t.tworkshoplistfile_s.Get())  ==  1 ) 
	{
		t.tmphopitemtocheckifchangedandversion_s = t.tworkshoplistfile_s;
		mp_grabWorkshopChangedFlagAndVersion ( );
		++t.tMPshopTheVersionNumber;
		DeleteAFile (  t.tworkshoplistfile_s.Get() );
	}
	OpenToWrite (  1,t.tworkshoplistfile_s.Get() );

	//  set the changed flag since we are saving, this way we dont rely on workshop info to know if a file is new or not
	WriteString (  1,"DO NOT MANUALY EDIT THIS FILE" );
	WriteString (  1,"1" );
	WriteString (  1,Str(t.tMPshopTheVersionNumber) );
	WriteString (  1,"0" );

	//  Collect ALL files in string array list
	g.filecollectionmax = 0;
	Dim (  t.filecollection_s,500  );

	//  include original FPM
	addtocollection(t.tmasterlevelfile_s.Get());

	//  Stage 2 - collect all files
	t.tlevelfile_s="";
	g.projectfilename_s=t.tmasterlevelfile_s;

	//  load in level FPM
	if (  Len(t.tlevelfile_s.Get())>1 ) 
	{
		g.projectfilename_s=t.tlevelfile_s;
	}

	//  chosen sky, terrain and veg
	addfoldertocollection( cstr(cstr("skybank\\")+t.skybank_s[g.skyindex]).Get() );
	addfoldertocollection("skybank\\night");
	if ( stricmp ( g.terrainstyle_s.Get(), "CUSTOM" ) != NULL )
	{
		addfoldertocollection( cstr(cstr("terrainbank\\")+g.terrainstyle_s).Get() );
	}
	addfoldertocollection( cstr(cstr("vegbank\\")+g.vegstyle_s).Get() );

	//  choose all entities and associated files
	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			//  check for lua scripts
			if (  t.entityelement[t.e].eleprof.aimain_s  !=  "" ) 
			{
				if (  mp_check_if_entity_is_from_install(t.entityelement[t.e].eleprof.aimain_s.Get())  ==  0 ) 
				{
					addtocollection( cstr(cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s).Get() );
				}
			}
			//  entity profile file
			t.tentityname1_s=cstr("entitybank\\")+t.entitybank_s[t.entid];
			t.tentityname2_s=cstr(Left(t.tentityname1_s.Get(),Len(t.tentityname1_s.Get())-4))+".fpe";
			if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tentityname2_s).Get() ) == 1 ) 
			{
				t.tentityname_s=t.tentityname2_s;
			}
			else
			{
				t.tentityname_s=t.tentityname1_s;
			}
			//  Check to see if the entity is part of the base install
			//  If it is, we can skip checking any further with it
			if (  mp_check_if_entity_is_from_install(t.tentityname_s.Get())  ==  0 ) 
			{

				addtocollection(t.tentityname_s.Get());
				//  entity files in folder
				t.tentityfolder_s=t.tentityname_s;
				for ( t.n = Len(t.tentityname_s.Get()) ; t.n >=  1 ; t.n+= -1 )
				{
					if (  cstr(Mid(t.tentityname_s.Get(),t.n)) == "\\" || cstr(Mid(t.tentityname_s.Get(),t.n)) == "/" ) 
					{
						t.tentityfolder_s=Left(t.tentityfolder_s.Get(),t.n);
						break;
					}
				}
				//  model file
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(t.entityprofile[t.entid].model_s.Get()); t.n++ )
				{
					if (  cstr(Mid(t.entityprofile[t.entid].model_s.Get(),t.n)) == "\\" || cstr(Mid(t.entityprofile[t.entid].model_s.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile1_s=t.tentityfolder_s+t.entityprofile[t.entid].model_s;
				}
				else
				{
					t.tfile1_s=t.entityprofile[t.entid].model_s;
				}
				t.tfile2_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dbo";
				if (  FileExist( cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile2_s).Get() ) == 1 ) 
				{
					t.tfile_s=t.tfile2_s;
				}
				else
				{
					t.tfile_s=t.tfile1_s;
				}
				t.tmodelfile_s=t.tfile_s;
				addtocollection(t.tmodelfile_s.Get());
				//  entity characterpose file (if any)
				t.tfile3_s=cstr(Left(t.tfile1_s.Get(),Len(t.tfile1_s.Get())-2))+".dat";
				if (  FileExist(cstr(g.fpscrootdir_s+"\\Files\\"+t.tfile3_s).Get()) == 1 ) 
				{
					addtocollection(t.tfile3_s.Get());
				}

				//  texture files
				t.tlocaltofpe=1;
				for ( t.n = 1 ; t.n<=  Len(t.entityelement[t.e].eleprof.texd_s.Get()); t.n++ )
				{
					if (  cstr(Mid(t.entityelement[t.e].eleprof.texd_s.Get(),t.n)) == "\\" || cstr(Mid(t.entityelement[t.e].eleprof.texd_s.Get(),t.n)) == "/" ) 
					{
						t.tlocaltofpe=0 ; break;
					}
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+t.entityelement[t.e].eleprof.texd_s;
				}
				else
				{
					t.tfile_s=t.entityelement[t.e].eleprof.texd_s;
				}
				addtocollection(t.tfile_s.Get());
				timestampactivity(0, cstr(cstr("Exporting ")+t.entitybank_s[t.entid]+" texd:"+t.tfile_s).Get() );
				if (  cstr(Left(Lower(Right(t.tfile_s.Get(),6)),2)) == "_d" ) 
				{
					t.tfileext_s=Right(t.tfile_s.Get(),3);
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_n."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_s."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_i."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
					t.tfile_s=cstr(Left(t.tfile_s.Get(),Len(t.tfile_s.Get())-6))+"_o."+t.tfileext_s ; addtocollection(t.tfile_s.Get());
				}
				if (  t.tlocaltofpe == 1 ) 
				{
					t.tfile_s=t.tentityfolder_s+t.entityelement[t.e].eleprof.texaltd_s;
				}
				else
				{
					t.tfile_s=t.entityelement[t.e].eleprof.texaltd_s;
				}
				addtocollection(t.tfile_s.Get());
				//  if entity did not specify texture it is multi-texture, so interogate model file
				findalltexturesinmodelfile(t.tmodelfile_s.Get(),t.tentityfolder_s.Get(),t.entityprofile[t.entityelement[t.e].bankindex].texpath_s.Get());
				//  shader file
				t.tfile_s=t.entityelement[t.e].eleprof.effect_s ; addtocollection(t.tfile_s.Get());
				//  script files
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aiinit_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aimain_s ; addtocollection(t.tfile_s.Get());
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aidestroy_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				// t.tfile_s=cstr("scriptbank\\")+t.entityelement[t.e].eleprof.aishoot_s ; addtocollection(t.tfile_s.Get()); //PE: Not used anymore.
				//  sound files
				t.tfile_s=t.entityelement[t.e].eleprof.soundset_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset1_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset2_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset3_s ; addtocollection(t.tfile_s.Get());
				t.tfile_s=t.entityelement[t.e].eleprof.soundset4_s ; addtocollection(t.tfile_s.Get());
				//  collectable guns
				if (  Len(t.entityprofile[t.entid].isweapon_s.Get())>1 ) 
				{
					t.tfile_s=cstr("gamecore\\guns\\")+t.entityprofile[t.entid].isweapon_s ; addfoldertocollection(t.tfile_s.Get());
					t.foundgunid=t.entityprofile[t.entid].isweapon;
					if (  t.foundgunid>0 ) 
					{
						for ( t.x = 0 ; t.x<=  1; t.x++ )
						{
							t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
							if (  t.tpoolindex>0 ) 
							{
								t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s ; addfoldertocollection(t.tfile_s.Get());
							}
						}
					}
				}
				//  associated guns and ammo
				if (  Len(t.entityelement[t.e].eleprof.hasweapon_s.Get())>1 ) 
				{
					t.tfile_s=cstr("gamecore\\guns\\")+t.entityelement[t.e].eleprof.hasweapon_s ; addfoldertocollection(t.tfile_s.Get());
					t.foundgunid=t.entityelement[t.e].eleprof.hasweapon;
					if (  t.foundgunid>0 ) 
					{
						for ( t.x = 0 ; t.x<=  1; t.x++ )
						{
							t.tpoolindex=g.firemodes[t.foundgunid][t.x].settings.poolindex;
							if (  t.tpoolindex>0 ) 
							{
								t.tfile_s=cstr("gamecore\\ammo\\")+t.ammopool[t.tpoolindex].name_s ; addfoldertocollection(t.tfile_s.Get());
							}
						}
					}
				}
				//  zone marker can reference other levels to jump to
				if (  t.entityprofile[t.entid].ismarker == 3 ) 
				{
					t.tlevelfile_s=t.entityelement[t.e].eleprof.ifused_s;
					if (  Len(t.tlevelfile_s.Get())>1 ) 
					{
						t.tlevelfile_s=cstr("mapbank\\")+t.tlevelfile_s+".fpm";
						addtocollection(t.tlevelfile_s.Get());
					}
				}
			}

		}
	}

	//  fill in the .dat file
	SetDir (  cstr(g.fpscrootdir_s+"\\Files\\").Get() );
	t.filesmax=g.filecollectionmax;
	t.thowmanyadded = 0;
	for ( t.fileindex = 1 ; t.fileindex<=  t.filesmax; t.fileindex++ )
	{
		t.name_s=t.filecollection_s[t.fileindex];
		if (  cstr(Left(t.name_s.Get(),12))  ==  "entitybank\\\\"  )  t.name_s  =  cstr("entitybank\\") + Right(t.name_s.Get(), Len(t.name_s.Get())-12);
		if (  cstr(Left(t.name_s.Get(),12))  ==  "scriptbank\\\\"  )  t.name_s  =  cstr("scriptbank\\") + Right(t.name_s.Get(), Len(t.name_s.Get())-12);
		if (  FileExist(t.name_s.Get()) == 1 ) 
		{
			if (  mp_check_if_entity_is_from_install(t.name_s.Get())  ==  0 ) 
			{
				WriteString (  1,t.name_s.Get() );
				//  check if it is character creator, if it is, check for the existance of a texture
				if (  cstr(Lower(Left(t.name_s.Get(),32)))  ==  "entitybank\\user\\charactercreator" ) 
				{
					t.tname_s = cstr(Left(t.name_s.Get(), Len(t.name_s.Get())-4)) + "_cc.dds";
					if (  FileExist(t.tname_s.Get())  ==  1 ) 
					{
						WriteString (  1,t.tname_s.Get() );
						++t.thowmanyadded;
					}
				}

				++t.thowmanyadded;
				//  09032015 - 017 - If its a gun, grab the muzzleflash, decals and include them
				if (  cstr(Right(t.name_s.Get(),11))  ==  "gunspec.txt" ) 
				{
					if (  FileOpen(3)  )  CloseFile (  3 );
					t.tfoundflash = 0;
					OpenToRead (  3,t.name_s.Get() );
					t.tfoundflash = 0;
					while (  FileEnd(3)  ==  0 && t.tfoundflash  ==  0 ) 
					{
						t.tisthisflash_s = ReadString ( 3 );
						if (  cstr(Left(t.tisthisflash_s.Get(),11))  ==  "muzzleflash" ) 
						{
							t.tlocationofequals = FindLastChar(t.tisthisflash_s.Get(),"=");
							if (  t.tlocationofequals > 1 ) 
							{
								if (  cstr(Mid(t.tisthisflash_s.Get(),t.tlocationofequals+1))  ==  " " ) 
								{
									t.tflash_s = Right(t.tisthisflash_s.Get(),Len(t.tisthisflash_s.Get())-(t.tlocationofequals+1));
								}
								else
								{
									t.tflash_s = Right(t.tisthisflash_s.Get(),Len(t.tisthisflash_s.Get())-(t.tlocationofequals));
								}
								t.tfext_s = "";
								if (  FileExist( cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+".png").Get() )  ==  1  )  t.tfext_s  =  ".png";
								if (  FileExist( cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+".dds").Get() )  ==  1  )  t.tfext_s  =  ".dds";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\muzzleflash\\flash")+t.tflash_s+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfext_s = "";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal.png").Get() )  ==  1  )  t.tfext_s  =  ".png";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal.dds").Get() )  ==  1  )  t.tfext_s  =  ".dds";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decal"+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfext_s = "";
								if (  FileExist(cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decalspec.txt").Get() )  ==  1  )  t.tfext_s  =  ".txt";
								if (  t.tfext_s  !=  "" ) 
								{
									WriteString (  1,cstr(cstr("gamecore\\decals\\muzzleflash")+t.tflash_s+"\\decalspec"+t.tfext_s).Get() );
									++t.thowmanyadded;
								}
								t.tfoundflash = 1;
							}
						}
					}
					CloseFile (  3 );
				}
			}
		}
	}

	CloseFile (  1 );

	//  if (  it is just the fpm  )  there are is no custom media with this level
	if (  t.thowmanyadded  <=  1  )  DeleteAFile (  t.tworkshoplistfile_s.Get() );

	//  cleanup file array
	UnDim (  t.filecollection_s );

	//  Restore directory
	SetDir (  t.told_s.Get() );

	UnDim (  t.tallfpefiles );

	t.tmasterlevelfile_s = toriginalMasterLevelFile_s;
	g.projectfilename_s = toriginalprojectname_s;

}

void mp_grabWorkshopChangedFlagAndVersion ( void )
{
	if (  FileExist(t.tmphopitemtocheckifchangedandversion_s.Get())  ==  1 ) 
	{
		if (  FileOpen(1)  )  CloseFile (  1 );
		OpenToRead (  1,t.tmphopitemtocheckifchangedandversion_s.Get() );
		//  skip the warning message
		t.tnothing_s = ReadString ( 1 );
		//  read in flag changed
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopHasItemChangedFlag = ValF(t.tnothing_s.Get());
		//  read in version number
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopTheVersionNumber = ValF(t.tnothing_s.Get());
		//  read in workshop id
		t.tnothing_s = ReadString ( 1 );
		t.tMPshopTheIDNumber_s = t.tnothing_s;
		CloseFile (  1 );
	}
return;

//  Check to see if this file is part of the base install
}

int mp_check_if_entity_is_from_install ( char* name_s )
{
	int ttemploop = 0;
	int ttresult = 0;
	ttresult = 0;
	if (  cstr(Left(name_s,12))  ==  "entitybank\\\\"  )  strcpy ( name_s  , cstr(cstr("entitybank\\") + Right(name_s, Len(name_s)-12)).Get() );
	if (  cstr(Right(name_s,3))  ==  "bin" ) 
	{
		strcpy ( name_s , cstr(cstr(Lower(Left(name_s,Len(name_s)-3))) + cstr("fpe")).Get() );
	}
	else
	{
		name_s = Lower(name_s);
	}
	cstr nameCheck = cstr(name_s);
	for ( ttemploop = 0 ; ttemploop<=  g.mp.howmanyfpefiles-1; ttemploop++ )
	{
		cstr listFile = cstr( _strlwr(t.tallfpefiles_s[ttemploop].Get()) );
		if (  nameCheck  ==  listFile ) 
		{
			ttresult = 1;
			break;
		}
	}
//endfunction ttresult
	return ttresult;
}

void mp_resetSteam ( void )
{
		mp_free ( );
		mp_init ( );
		mp_resetGameStats ( );
		g.mp.needToResetOnStartup = 0;
return;

}

void mp_shoot ( void )
{
	if (  t.weaponammo[g.weaponammoindex+g.ammooffset]>0 ) 
	{
		SteamShoot (  );
	}
return;

}

void mp_chat ( void )
{

	//  check for chat
	t.tchat_s = SteamGetChat();
	if (  t.tchat_s  !=  "" ) 
	{
		mp_chatNew ( );
	}

	t.tscancode = ScanCode();
	if (  KeyState(g.keymap[28])  ==  1 && t.oldchatscancode  !=  28 ) 
	{
		g.mp.chaton = 1-g.mp.chaton;
		if (  g.mp.chaton  ==  1 ) 
		{
			//  start a new chat message
			ClearEntryBuffer (  );
			g.mp.chatstring = "";
		}
		else
		{
			//  send the chat message
			//  local send
			if (  Len(g.mp.chatstring.Get())  ==  1 ) 
			{
				if (  Asc(Mid(g.mp.chatstring.Get(),1))  <=  31  )  g.mp.chatstring  =  "";
			}
			else
			{
				if (  Asc(Mid(g.mp.chatstring.Get(),1))  <=  31  )  g.mp.chatstring  =  Right(g.mp.chatstring.Get(),Len(g.mp.chatstring.Get())-1);
			}
			if (  g.mp.chatstring !=  "" ) 
			{
				t.tchat_s = cstr(cstr(Str(g.mp.me)) + Left( cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80)).Get();
				mp_chatNew ( );
				g.mp.chatstring = "";
				if (  t.tchatLobbyMode  ==  0 ) 
				{
					SteamSendChat (  t.tchat_s.Get() );
				}
				else
				{
					//  030315 - 013 - Lobby chat
					SteamSendLobbyChat (  t.tchat_s.Get() );
				}
			}
		}
	}
	if (  g.mp.chaton  ==  1 ) 
	{
		if (  Timer() - g.mp.lastSpawnedTime > 1000 ) 
		{
			t.aisystem.processplayerlogic=0;
		}
		else
		{
			t.aisystem.processplayerlogic=1;
		}
		g.mp.chattimer = Timer();

		g.mp.chatstring = Entry(1);
		//  show the Text (  we have typed )
		if (  Timer() - t.chatcursortime > 250 ) 
		{
			t.chatcursortime = Timer();
			g.mp.cursoron = 1-g.mp.cursoron;
		}
		if (  g.mp.cursoron  ==  0 ) 
		{
			t.tstringtoshow_s = cstr(Left(cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80));
		}
		else
		{
			t.tstringtoshow_s = cstr(Left(cstr(cstr("<") + SteamGetPlayerName() + "> " + g.mp.chatstring).Get(),80)) + cstr("|");
		}
	}
	else
	{
		t.aisystem.processplayerlogic=1;
	}
	t.oldchatscancode = t.tscancode;

	if (  Timer() - g.mp.chattimer  <=  MP_CHAT_DELAY+2550 ) 
	{
		t.ttimegone = Timer()-t.toldchattime;
		if (  t.ttimegone > 50 ) 
		{
			t.toldchattime = Timer();
			t.ttimegone = 16;
		}
		t.toldchattime = Timer();
		if (  Timer() - g.mp.chattimer  >=  MP_CHAT_DELAY ) 
		{
			t.tsteamalpha = t.tsteamalpha - t.ttimegone;
		}
		else
		{
			t.tsteamalpha = t.tsteamalpha + t.ttimegone;
		}
		if (  t.tsteamalpha < 0  )  t.tsteamalpha  =  0;
		if (  t.tsteamalpha > 255  )  t.tsteamalpha  =  255;
		
		if (  t.tsteamalpha > 0 ) 
		{
			InkEx ( 20, 20, 20 );//  Rgb(20,20,20),Rgb(20,20,20) );
			BoxEx (  5,5,(40*10)+5,((MP_MAX_CHAT_LINES+1)*15)+10 );
		}

		t.tsteamy = 10;
		for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-1; t.tloop++ )
		{
			if (  t.mp_chat[t.tloop] != "" ) 
			{
				if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "0" ) { t.r  =  255 ; t.g  =  255 ; t.b  =  50; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "1" ) { t.r  =  255  ; t.g  =  100 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "2" ) { t.r  =  100  ; t.g  =  255 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "3" ) { t.r  =  100  ; t.g  =  100 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "4" ) { t.r  =  255  ; t.g  =  255 ; t.b  =  100; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "5" ) { t.r  =  255  ; t.g  =  100 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "6" ) { t.r  =  100  ; t.g  =  255 ; t.b  =  255; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "7" ) { t.r  =  200  ; t.g  =  255 ; t.b  =  200; }
				else if (  cstr(Left(t.mp_chat[t.tloop].Get(),1))  ==  "s" ) { t.r  =  255  ; t.g  =  255 ; t.b  =  255; }
				t.tluarealcoords = 1;
				t.tluatextalpha = t.tsteamalpha;
				mp_textColor(10,t.tsteamy,2,Right(t.mp_chat[t.tloop].Get(),Len(t.mp_chat[t.tloop].Get())-1),t.r,t.g,t.b);
			}
			t.tsteamy += 14;
		}

		if (  g.mp.chaton  ==  1 ) 
		{
			t.tluarealcoords = 1;
			t.tluatextalpha = t.tsteamalpha;
			t.tsteamy = 10+((MP_MAX_CHAT_LINES)*15);
			mp_textColor(10,t.tsteamy,2,t.tstringtoshow_s.Get(),255,255,255);
		}
	}
	InkEx ( 255, 255, 255 );// Rgb(255,255,255),Rgb(0,0,0) );
}

void mp_chatNew ( void )
{
	for ( t.tloop = 0 ; t.tloop<=  MP_MAX_CHAT_LINES-2; t.tloop++ )
	{
		t.mp_chat[t.tloop] = t.mp_chat[t.tloop+1];
	}
	if (  Len(t.tchat_s.Get()) > 80  )  t.tchat_s  =  Left(t.tchat_s.Get(),80);
	if (  cstr(Left(t.tchat_s.Get(),1))  !=  "s" ) 
	{
		t.mp_chat[MP_MAX_CHAT_LINES-1] = t.tchat_s;
		g.mp.chattimer = Timer();
	}
	//  200315 - 021 - pick up users joining the game from the server message sent
	if (  cstr(Left(t.tchat_s.Get(),1))  ==  "s" ) 
	{
		t.tnametocheckforjoining_s = Right(t.tchat_s.Get(),Len(t.tchat_s.Get())-1);
		for ( t.tn = 0 ; t.tn<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tn++ )
		{
			if (  cstr(Left(t.tnametocheckforjoining_s.Get(),Len(t.mp_joined[t.tn].Get())))  ==  t.mp_joined[t.tn] && t.mp_joined[t.tn]  !=  "" && cstr(Right(t.mp_joined[t.tn].Get(),6))  !=  "Joined" ) 
			{
				t.mp_joined[t.tn] = t.mp_joined[t.tn] + " - Joined";
//     `if tlastchatline$  !=  tchat$ then mp_chat(MP_MAX_CHAT_LINES-1)  ==  tchat$

//     `tlastchatline$ = tchat$

			}
		}
	}
}

void mp_quitGame ( void )
{
	// exit current game and return to multiplayer menu
	// 110315 - 019 - first lets fade out nice
	t.tstartoffade = Timer();
	t.tfadestealpha_f = 0.0;
	t.tspritetouse = 0;
	for ( t.tloop = 2000 ; t.tloop<=  3000; t.tloop++ )
	{
		if (  SpriteExist(t.tloop)  ==  0 ) { t.tspritetouse  =  t.tloop  ; break; }
	}
	while (  Timer() - t.tstartoffade < 500 ) 
	{
		t.tfadestealpha_f = (Timer() - t.tstartoffade)*2;
		if (  t.tfadestealpha_f < 0  )  t.tfadestealpha_f  =  0.0;
		if (  t.tfadestealpha_f > 255.0  )  t.tfadestealpha_f  =  255.0;
		if (  t.tspritetouse > 0 && ImageExist(g.panelimageoffset+1)  ==  1 ) 
		{
			Sprite (  t.tspritetouse,0,0,g.panelimageoffset+1 );
			SizeSprite (  t.tspritetouse,GetDisplayWidth()*10, GetDisplayHeight()*10 );
			SetSpriteDiffuse (  t.tspritetouse,0,0,0 );
			SetSpriteAlpha (  t.tspritetouse,t.tfadestealpha_f );
		}
		SteamLoop (  );
		Sync (  );
	}
	//  if the server, let everyone know instantly the server is dropping
	//  020315 - 012 - Send an end game message when the host decides to leave
	if (  g.mp.isGameHost  ==  1 ) 
	{
		SteamEndGame (  );
	}

	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
}

void mp_freefadesprite ( void )
{
	// 240316 - v1.13b1 - free sprite now finished with fade
	if ( t.tspritetouse > 0 )
	{
		DeleteSprite ( t.tspritetouse );
		t.tspritetouse = 0;
	}
}

void mp_backToEditor ( void )
{
	t.game.gameloop=0;
	t.game.levelloop=0;
	t.game.titleloop=0;
	t.game.quitflag=1;
	g.mp.goBackToEditor = 1;
return;

//  remove all entities and lightmaps that are left from our gaming session
}

void mp_cleanupGame ( void )
{

	//  default start position is edit-camera XZ
	t.terrain.playerx_f=t.cx_f;
	t.terrain.playerz_f=t.cy_f;
	if (  t.terrain.TerrainID>0 ) 
	{
		t.terrain.playery_f=BT_GetGroundHeight(t.terrain.TerrainID,t.terrain.playerx_f,t.terrain.playerz_f)+150.0;
	}
	else
	{
		t.terrain.playery_f=1000.0+150.0;
	}
	t.terrain.playerax_f=0.0;
	t.terrain.playeray_f=0.0;
	t.terrain.playeraz_f=0.0;

	//  remove light map objects for return to IDE editor
	lm_restoreall ( );

	//  remove all entities
	if (  g.entityelementlist>0 ) 
	{
		for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
		{
			t.obj=t.entityelement[t.e].obj;
			if (  t.obj>0 ) 
			{
				if (  ObjectExist(t.obj) == 1 ) 
				{
					DeleteObject (  t.obj );
				}
			}
			t.entityelement[t.e].obj=0;
			t.entityelement[t.e].bankindex=0;
		}
		g.entityelementlist=0;
	}

return;

//  Send Steam User ID to editor via file mapping

}

void mp_sendSteamIDToEditor ( void )
{

	if (  g.mp.isRunning  ==  0 ) 
	{
		//  was 60*1000, changing to 5 to keep try and connecting
		if (  Timer() - g.mp.lastTimeTriedToConnectToSteamFromEditor > 5*1000 ) 
		{
			g.mp.lastTimeTriedToConnectToSteamFromEditor = Timer();
			mp_resetSteam ( );
		}
		if (  g.mp.isRunning  ==  0 ) 
		{
			return;
		}
	}
	else
	{
		//  send user id

		//  Debug code put it for a user that had issues with the downloader (which needs to know the steam id from steam, if steam doesnt give it, it wont work)
//   `set cursor 0,30

//   `print "steam id = " + tSteamGetID$


//   `print Timer()

//   `print mp.lastTimeISentMySteamID


		if (  Timer() - g.mp.lastTimeISentMySteamID > 5000 ) 
		{

			t.tSteamGetID_s = SteamGetPlayerID();

			if (  t.tSteamGetID_s  !=  "" ) 
			{
//     `print "sending id"

				g.mp.lastTimeISentMySteamID = Timer();

				OpenFileMap (  1, "FPSEXCHANGE" );

				//  params; filemap number, offset in bytes, value
				SetFileMapDWORD (  1, 6145, 1 );
				SetFileMapString (  1, 6149 , t.tSteamGetID_s.Get() );
				SetEventAndWait (  1 );

				//  Close when set all defaults
				//CloseFileMap (  1 );

				g.mp.haveSentSteamIDToEditor = 1;
			}
			else
			{
				mp_resetSteam ( );
				g.mp.lastTimeISentMySteamID = Timer()-3000;
//     `print "resetting steam"

			}


		}
	}

return;

//  020315 - 012 - enable check for lobbies while in editor
}

void mp_checkIfLobbiesAvailable ( void )
{
	if (  t.thowlongbetweenlobbychecks  <=  0  )  t.thowlongbetweenlobbychecks  =  15*1000;
	if (  Timer() - t.tlasttimecheckedforlobbiestimer > t.thowlongbetweenlobbychecks ) 
	{
		SteamLoop (  );
		t.tlasttimecheckedforlobbiestimer = Timer();
		if (  g.mp.checkiflobbiesavailablemode  ==  0 ) 
		{
			SteamGetLobbyList (  );
			g.mp.checkiflobbiesavailablemode = 1;
			return;
		}
		if (  g.mp.checkiflobbiesavailablemode  ==  1 ) 
		{
			g.mp.checkiflobbiesavailablemode = 0;

			if (  SteamIsLobbyListCreated()  ==  0 ) 
			{
				return;
			}


			t.thowmanylobbiesavailable = SteamGetLobbyListSize();
			if (  t.thowmanylobbiesavailable > 0 ) 
			{
				if (  t.thowmanylobbiesavailable > 10 ) 
				{
					t.thowlongbetweenlobbychecks = 60*1000;
				}
				if (  t.thowmanylobbiesavailable > 20 ) 
				{
					t.thowlongbetweenlobbychecks = 150*1000;
				}
				if (  t.thowmanylobbiesavailable > 40 ) 
				{
					t.thowlongbetweenlobbychecks = 300*1000;
				}
				t.steamStatusBar_s = "        |        Multiplayer Lobbies available to join";
			}
			else
			{
				t.thowlongbetweenlobbychecks = 15*1000;
				t.steamStatusBar_s = "";
			}
		}
	}
return;

//  200315 - 021 - flashlight of when starting a game
}

void mp_flashLightOff ( void )
{
	t.playerlight.flashlightcontrol_f=0.0;
return;

//  set everyone to team A for coop mode
}

void mp_setupCoopTeam ( void )
{
	for ( t.tc = 0 ; t.tc<=  MP_MAX_NUMBER_OF_PLAYERS-1; t.tc++ )
	{
		t.mp_team[t.tc] = 0;
	}
return;

//  requires; e, tSteamX# and tSteamZ#
}

void mp_COOP_aiMoveTo ( void )
{

	if (  g.mp.endplay  ==  1  )  return;

	if (  t.game.runasmultiplayer  ==  1 && g.mp.coop  ==  1 && t.tLuaDontSendLua  ==  0 ) 
	{

		SteamSendLua (  MP_LUA_AiGoToX,t.e,t.tSteamX_f );
		SteamSendLua (  MP_LUA_AiGoToZ,t.e,t.tSteamZ_f );
	}
	else
	{
		//  lets check if the distance is worth the effort of moving
		t.tdx_f=t.tSteamX_f-ObjectPositionX(t.e);
		t.tdz_f=t.tSteamZ_f-ObjectPositionZ(t.e);
		t.tdist_f=Sqrt((t.tdx_f*t.tdx_f)+(t.tdz_f*t.tdz_f));

		//  is it isn't very far, lets just stop the ai so it doesnt jerk about
		if (  t.tdist_f < 75.0 ) 
		{
//    `AI Set Entity Position e,ObjectPositionX(e),ObjectPositionY(e),ObjectPositionZ(e)

			AISetEntityPosition (  t.e,t.tSteamX_f,BT_GetGroundHeight(t.terrain.TerrainID,t.tSteamX_f,t.tSteamZ_f),t.tSteamZ_f );
			AIEntityStop (  t.e );
		//  if it is a real destination, lets head there
		}
		else
		{
			AIEntityGoToPosition (  t.e, t.tSteamX_f, t.tSteamZ_f );
		}

	}
return;

}

void mp_entity_lua_lookatplayer ( void )
{
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex != -1 ) 
	{

		//  Simply look in direction of player
		t.ee = t.mp_playerEntityID[t.v];
		t.tdx_f= ObjectPositionX (t.entityelement[t.ee].obj) - ObjectPositionX(t.entityelement[t.e].obj);
		t.tdz_f= ObjectPositionZ (t.entityelement[t.ee].obj) - ObjectPositionZ(t.entityelement[t.e].obj);
		AISetEntityAngleY (  t.charanimstate.obj,atan2deg(t.tdx_f,t.tdz_f) );
		
		//  If angle beyond 'look angle range', perform full rotation
		t.tangley_f=AIGetEntityAngleY(t.charanimstate.obj) ;
		t.headangley_f=t.tangley_f-ObjectAngleY(t.charanimstate.obj) ;
		if (  t.headangley_f<-180  )  t.headangley_f = t.headangley_f+360;
		if (  t.headangley_f>180  )  t.headangley_f = t.headangley_f-360;
		if (  t.headangley_f<-75 || t.headangley_f>75 ) 
		{
			t.charanimstate.currentangle_f=t.tangley_f;
			t.charanimstate.updatemoveangle=1;
			AISetEntityAngleY (  t.charanimstate.obj,t.charanimstate.currentangle_f );
			t.charanimstates[t.tcharanimindex] = t.charanimstate;
		}

	}
return;

}

void mp_entity_lua_fireweaponEffectOnly ( void )
{
	//  update gun appearance
	if (  t.entityelement[t.e].attachmentobj > 0 ) 
	{
		entity_controlattachments ( );

		t.tgunid=t.entityelement[t.e].eleprof.hasweapon;
		t.tattachedobj=t.entityelement[t.e].attachmentobj;
		t.te = t.e;

		t.tgunid = t.entityelement[t.e].eleprof.hasweapon;
		t.ttrr=Rnd(1);
		for ( t.tt = t.ttrr+0 ; t.tt<=  t.ttrr+1; t.tt++ )
		{
			t.ttsnd=t.gunsoundcompanion[t.tgunid][1][t.tt].soundid;
			if (  t.ttsnd>0 ) 
			{
				if (  SoundExist(t.ttsnd) == 1 ) 
				{
					if (  SoundPlaying(t.ttsnd) == 0 || t.tt == t.ttrr+1 ) 
					{
						t.charanimstate.firesoundindex=t.ttsnd ; t.tt=3;
						t.charanimstate.firesoundexpiry=Timer()+200;
					}
				}
			}
		}

		if (  t.charanimstate.firesoundindex>0 ) 
		{
			entity_lua_findcharanimstate ( );
			darkai_shooteffect ( );
		}

	}

	//  charanimstate is purely temporary, the firesoundindex will NOT be persistent!!
	t.charanimstate.firesoundindex=0;

return;

//  cycle through entities, pick out the ai and either take aggro or update depending on distance/ownership
}

void mp_updateAIForCOOP ( void )
{

		if (  g.mp.endplay  ==  1  )  return;

		t.tsentone = 0;
//   `set cursor 0,0


		if (  t.game.runasmultiplayer == 1 && g.mp.coop  ==  1 ) 
		{
			t.thowManyDoIHave = 0;
			//  only send one update to other players max
			//  so everyone gets a chance to update we keep track of where we were up to in the list last time we sent
			if (  t.tcoopyentityupdatetostartat  ==  0 || t.tcoopyentityupdatetostartat > g.entityelementlist ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
					{
						if (  t.entityelement[t.e].health  ==  0 ) 
						{
							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f > 3000  )  ScaleObject (  t.entityelement[t.e].obj,0,0,0 );
						}
						if (  t.entityelement[t.e].mp_coopControlledByPlayer  !=  g.mp.me && t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
						{
							// rotate or look at player for 1 second after receiving lua command, to cut down packets being sent
							if (  t.entityelement[t.e].mp_rotateType  ==  1 ) 
							{
								if (  Timer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}
							if (  t.entityelement[t.e].mp_rotateType  ==  2 ) 
							{
								if (  Timer() - t.entityelement[t.e].mp_rotateTimer > 1000  )  t.entityelement[t.e].mp_rotateType  =  0;
								if (  t.entityelement[t.e].obj > 0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										if (  t.entityelement[t.e].mp_coopControlledByPlayer > -1 && t.entityelement[t.e].mp_coopControlledByPlayer < MP_MAX_NUMBER_OF_PLAYERS ) 
										{
											t.v = t.entityelement[t.e].mp_coopControlledByPlayer;
											mp_entity_lua_lookatplayer ( );
										}
									}
								}
							}

							t.distx_f = CameraPositionX() - ObjectPositionX(t.entityelement[t.e].obj);
							t.distz_f = CameraPositionZ() - ObjectPositionZ(t.entityelement[t.e].obj);
							t.tdist_f = Sqrt((t.distx_f*t.distx_f)+(t.distz_f*t.distz_f));
							if (  t.tdist_f < 1200 || ( AIGetEntityHeardSound(t.entityelement[t.e].obj)  ==  1 && t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) ) 
							{
								if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1 ) 
								{
									t.tsteamplayeralive = 0;
								}
								else
								{
									t.tsteamplayeralive = SteamGetPlayerAlive(t.entityelement[t.e].mp_coopControlledByPlayer);
								}
								if (  Timer() - t.entityelement[t.e].mp_coopLastTimeSwitchedTarget > 5000 || t.tsteamplayeralive  ==  0 ) 
								{
									t.tthrowaway = Rnd(1);
									if (  t.tsteamplayeralive  ==  0 || t.entityelement[t.e].mp_coopControlledByPlayer  ==  -1  )  t.tthrowaway  =  1;
									if (  t.tthrowaway  ==  1 ) 
									{
										t.entityelement[t.e].mp_coopControlledByPlayer = g.mp.me;
										SteamSendLua (  MP_LUA_TakenAggro,t.e,g.mp.me );
										SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
										SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
//           `AI Entity Stop entityelement(e).obj

										t.entityelement[t.e].mp_updateOn = 1;
										t.entityelement[t.e].mp_lastUpdateSent = 0;
									}
									t.entityelement[t.e].mp_coopLastTimeSwitchedTarget = Timer()+5000;
								}
							}
						}
						else
						{
							if (  t.entityelement[t.e].mp_coopControlledByPlayer  ==  g.mp.me ) 
							{
								if (  t.entityelement[t.e].mp_updateOn  ==  1 && t.tsentone  ==  0 ) 
								{
									if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
									{
										AISetEntityActive (  t.entityelement[t.e].obj,1 );
									}
									if (  t.tcoopyentityupdatetostartat  <=  t.e ) 
									{
//           ``

										if (  t.entityelement[t.e].active == 1 && t.entityelement[t.e].health > 0 ) 
										{

											if (  Timer() - t.tcoopLastUpdateSent > 500 || t.tcoopLastUpdateSent < 0 ) 
											{
													t.tsentone = 1;
													SteamSendLua (  MP_LUA_HaveAggro,t.e,g.mp.me );
													SteamSendLua (  MP_LUA_AiGoToX,t.entityelement[t.e].obj,ObjectPositionX(t.entityelement[t.e].obj) );
													SteamSendLua (  MP_LUA_AiGoToZ,t.entityelement[t.e].obj,ObjectPositionZ(t.entityelement[t.e].obj) );
													t.entityelement[t.e].mp_lastUpdateSent = Timer();
													t.tcoopLastUpdateSent = Timer();
													t.tcoopyentityupdatetostartat = t.e+1;

//             `endif

											}
											else
											{
												//  pretend way have sent one this time, since we need to wait a little while
												t.tsentone = 1;
											}

										}
//           ``

									}
								}
							}

						}
					}
				}
			}
			//  if we havent sent anything, reset the list
			if (  t.tsentone  ==  0 ) 
			{
				t.tcoopyentityupdatetostartat = 1;
				++t.tcoopSendPositionUpdate;
				if (  t.tcoopSendPositionUpdate > 3  )  t.tcoopSendPositionUpdate  =  0;
			}
		}

return;

}

void mp_coop_rotatetoplayer ( void )
{
	//  only rotate to player if enemy ai with proper rig
	if (  t.entityelement[t.e].mp_isLuaChar  ==  1  )  return;
	entity_lua_findcharanimstate ( );
	if (  t.tcharanimindex == -1 ) 
	{
		//  regular entity
		t.entityelement[t.e].ry=t.v;
		entity_lua_rotateupdate ( );
	}
	else
	{
		// character subsystem
		t.charanimstate.currentangle_f=t.v;
		t.charanimstate.updatemoveangle=1;
		AISetEntityAngleY ( t.charanimstate.obj,t.charanimstate.currentangle_f );
		t.charanimstates[t.tcharanimindex] = t.charanimstate;
		t.entityelement[t.e].ry=t.v;

		// 240217 - and update visually
		entity_lua_rotateupdate ( );
	}
}

void mp_storeOldEntityPositions ( void )
{

	Dim (  t.mp_oldEntityPositionsX,g.entityelementlist );
	Dim (  t.mp_oldEntityPositionsZ,g.entityelementlist );

	for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
	{
		t.entid=t.entityelement[t.e].bankindex;
		if (  t.entid>0 ) 
		{
			if (  (t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1) && t.entityprofile[t.entid].ismultiplayercharacter  ==  0 ) 
			{
				if (  t.entityelement[t.e].obj > 0 ) 
				{
					if (  ObjectExist(t.entityelement[t.e].obj)  ==  1 ) 
					{
						t.mp_oldEntityPositionsX[t.e] = ObjectPositionX(t.entityelement[t.e].obj);
						t.mp_oldEntityPositionsZ[t.e] = ObjectPositionZ(t.entityelement[t.e].obj);
					}
				}
			}
		}
	}

return;

}

void mp_howManyEnemiesLeftToKill ( void )
{
		if (  g.mp.coop  ==  1 ) 
		{
			t.thowmanyenemies = 0;
			for ( t.e = 1 ; t.e<=  g.entityelementlist; t.e++ )
			{
				t.entid=t.entityelement[t.e].bankindex;
				if (  t.entid>0 ) 
				{
					if (  t.entityprofile[t.entid].ischaracter  ==  1 || t.entityelement[t.e].mp_isLuaChar  ==  1 ) 
					{
						if (  t.entityelement[t.e].active  ==  1 && t.entityelement[t.e].health > 0 ) 
						{
							++t.thowmanyenemies;
						}
					}
				}
			}
			LuaSetInt (  "mp_enemiesLeftToKill", t.thowmanyenemies );
		}
return;

}

void mp_IKilledAnAI ( void )
{
	t.mp_kills[g.mp.me+1] = t.mp_kills[g.mp.me+1] + 1;
	SteamSendLua (  MP_LUA_ServerSetPlayerKills,g.mp.me+1,t.mp_kills[g.mp.me+1] );
	t.tnothing = LuaExecute( cstr(cstr("mp_playerKills[") + Str(g.mp.me+1) + "] = " + Str(t.mp_kills[g.mp.me+1])).Get());
return;

// `////////////////////////////////////////////////////////////////////////////////////////////////////////////


}

void mp_text ( int x, int y, int size, char* txt_s )
{
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
//endfunction

}

void mp_textDots ( int x, int y, int size, char* txt_s )
{

	if (  Timer() - g.mp.steamdotsoldtime > 150 ) 
	{
		g.mp.steamdotsoldtime = Timer();
		g.mp.buildingDots = g.mp.buildingDots + ".";
		if (  Len(g.mp.buildingDots.Get()) > 5  )  g.mp.buildingDots  =  ".";
	}

	t.luaText.txt = g.mp.buildingDots + txt_s + g.mp.buildingDots;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	lua_text ( );
//endfunction

}

void mp_textColor ( int x, int y, int size, char* txt_s, int r, int gg, int b )
{
	g.mp.steamDoColorText = 1;
	t.luaText.txt = txt_s;
	t.luaText.x = x;
	t.luaText.y = y;
	t.luaText.size = size;
	g.mp.steamColorRed = r;
	g.mp.steamColorGreen = gg;
	g.mp.steamColorBlue = b;
	lua_text ( );
//endfunction

}

void mp_panel ( int x, int y, int x2, int y2 )
{
	t.luaPanel.x = x;
	t.luaPanel.y = y;
	t.luaPanel.x2 = x2;
	t.luaPanel.y2 = y2;
	lua_panel ( );
//endfunction

}

void mp_deleteFile ( char* tempFileToDelete_s )
{
	cstr fileToDelete;
	fileToDelete =  cstr(g.fpscrootdir_s + "\\Files\\" + tempFileToDelete_s).Get();
	if (  FileExist(fileToDelete.Get() ) )  DeleteAFile (  fileToDelete.Get() );
//endfunction

}

int mp_check_if_lua_entity_exists ( int tentitytocheck )
{
	int tcheckobj = 0;
	int result = 0;
	result = 0;

	if (  tentitytocheck  <=  g.entityelementlist ) 
	{
		tcheckobj = t.entityelement[tentitytocheck].obj;
		if (  tcheckobj > 0 ) 
		{
			if (  ObjectExist(tcheckobj)  ==  1 ) 
			{
				result = 1;
			}
		}
	}
	return result;
}

void mp_sendlua ( int code, int e, int v )
{
	SteamSendLua ( code, e, v );
}

#endif
