/*----------------------------------------------------------------------
 *
 * Desc: Everything that has to do with the takeover game of Paradroid
 * 	is contained in this file.
 *
 *----------------------------------------------------------------------*/

/* 
 *
 *   Copyright (c) 1994, 2002 Johannes Prix
 *   Copyright (c) 1994, 2002 Reinhard Prix
 *
 *
 *  This file is part of Freedroid
 *
 *  Freedroid is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Freedroid is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Freedroid; see the file COPYING. If not, write to the 
 *  Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 *  MA  02111-1307  USA
 *
 */
#define _takeover_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"
#include "takeover.h"
#include "map.h"

Uint32 cur_time;  		/* current time in ms */

SDL_Surface *to_blocks;      /* the global surface containing all game-blocks */

/* the rectangles containing the blocks */
SDL_Rect FillBlocks[NUM_FILL_BLOCKS];
SDL_Rect CapsuleBlocks[NUM_CAPS_BLOCKS];
SDL_Rect ToGameBlocks[NUM_TO_BLOCKS];
SDL_Rect ToGroundBlocks[NUM_GROUND_BLOCKS];
SDL_Rect ToColumnBlock;
SDL_Rect ToLeaderBlock;

/* Class seperation of the blocks */
int BlockClass[TO_BLOCKS] = {
  CONNECTOR,			/* KABEL */
  NON_CONNECTOR,		/* KABELENDE */
  CONNECTOR,			/* VERSTAERKER */
  CONNECTOR,			/* FARBTAUSCHER */
  CONNECTOR,			/* VERZWEIGUNG_O */
  NON_CONNECTOR,		/* VERZWEIGUNG_M */
  CONNECTOR,			/* VERZWEIGUNG_U */
  NON_CONNECTOR,		/* GATTER_O */
  CONNECTOR,			/* GATTER_M */
  NON_CONNECTOR,		/* GATTER_U */
  NON_CONNECTOR			/* LEER */
};

/* Probability of the various elements */
#define MAX_PROB		100
int ElementProb[TO_ELEMENTS] = {
  100,				/* EL_KABEL */
  2,				/* EL_KABELENDE */
  5,				/* EL_VERSTAERKER */
  5,				/* EL_FARBTAUSCHER: only on last layer */
  5,				/* EL_VERZWEIGUNG */
  5				/* EL_GATTER */
};


int NumCapsules[TO_COLORS] = {
  0, 0
};

point LeftCapsulesStart[TO_COLORS] = {
  {GELB_LEFT_CAPSULES_X, GELB_LEFT_CAPSULES_Y},
  {VIOLETT_LEFT_CAPSULES_X, VIOLETT_LEFT_CAPSULES_Y}
};

point CurCapsuleStart[TO_COLORS] = {
  {GELB_CUR_CAPSULE_X, GELB_CUR_CAPSULE_Y},
  {VIOLETT_CUR_CAPSULE_X, VIOLETT_CUR_CAPSULE_Y}
};


point PlaygroundStart[TO_COLORS] = {
  {GELB_PLAYGROUND_X, GELB_PLAYGROUND_Y},
  {VIOLETT_PLAYGROUND_X, VIOLETT_PLAYGROUND_Y}
};

point DruidStart[TO_COLORS] = {
  {GELB_DRUID_X, GELB_DRUID_Y},
  {VIOLETT_DRUID_X, VIOLETT_DRUID_Y}
};

int CapsuleCurRow[TO_COLORS] = { 0, 0 };


int LeaderColor = GELB;		/* momentary leading color */
int YourColor = GELB;
int OpponentColor = VIOLETT;
int OpponentType;		/* The druid-type of your opponent */
int DroidNum;

/* the display  column */
int DisplayColumn[NUM_LINES] = {
  GELB, VIOLETT, GELB, VIOLETT, GELB, VIOLETT, GELB, VIOLETT, GELB, VIOLETT,
  GELB, VIOLETT
};

SDL_Color to_bg_color = {199, 43, 43};

playground_t ToPlayground;
playground_t ActivationMap;
playground_t CapsuleCountdown;

/*-----------------------------------------------------------------
 * @Desc: Initialise the Takeover game
 *
 * @Ret: void
 *
 *-----------------------------------------------------------------*/
void
InitTakeover (void)
{
  if (GetTakeoverGraphics () != OK)
    {
      DebugPrintf ("Error !");
      Terminate (-1);
    }

  return;
}

/*-----------------------------------------------------------------
 * @Desc: play takeover-game against a druid 
 *
 * @Ret: TRUE/FALSE:  user has won/lost
 *
 *-----------------------------------------------------------------*/
int
Takeover (int enemynum)
{
  int row;
  int FinishTakeover = FALSE;
  int waiter = 0;
  static int RejectEnergy = 0;	/* your energy if you're rejected */
  char *message;


  /* Prevent distortion of framerate by the delay coming from 
   * the time spend in the menu.
   */
  Activate_Conservative_Frame_Computation ();

  while (SpacePressed ()) ;  /* make sure space is release before proceed */

  Switch_Background_Music_To (TAKEOVER_BACKGROUND_MUSIC_SOUND);

  /* Get a new Internfenster without any robots, blasts bullets etc
     for use as background in transparent version of Takeover-game */
  //  GetInternFenster (SHOW_MAP);

  DisplayBanner (NULL, NULL,  BANNER_FORCE_UPDATE );
  
  Fill_Rect (User_Rect, to_bg_color);

  Me.status = MOBILE; /* the new status _after_ the takeover game */

  //  SetPalCol (INFLUENCEFARBWERT, Mobilecolor.rot, Mobilecolor.gruen,Mobilecolor.blau);

  while (!FinishTakeover)
    {
      /* Init Color-column and Capsule-Number for each opponenet and your color */
      for (row = 0; row < NUM_LINES; row++)
	{
	  DisplayColumn[row] = (row % 2);
	  CapsuleCountdown[GELB][0][row] = -1;
	  CapsuleCountdown[VIOLETT][0][row] = -1;
	}			/* for row */

      YourColor = GELB;
      OpponentColor = VIOLETT;

      CapsuleCurRow[GELB] = 0;
      CapsuleCurRow[VIOLETT] = 0;
      
      DroidNum = enemynum;
      OpponentType = AllEnemys[enemynum].type;
      NumCapsules[YOU] = 3 + ClassOfDruid (Me.type);
      NumCapsules[ENEMY] = 4 + ClassOfDruid (OpponentType);

      InventPlayground ();

      ShowPlayground ();

      ChooseColor ();

      PlayGame ();

      /* Ausgang beurteilen und returnen */
      if (InvincibleMode || (LeaderColor == YourColor))
	{
	  Switch_Background_Music_To (SILENCE);
	  Takeover_Game_Won_Sound ();
	  if (Me.type == DRUID001)
	    {
	      RejectEnergy = Me.energy;
	      PreTakeEnergy = Me.energy;
	    }

	  if (Me.energy > Druidmap[DRUID001].maxenergy) Me.energy = Druidmap[DRUID001].maxenergy;
	  // Me.energy = AllEnemys[enemynum].energy;
	  Me.energy += AllEnemys[enemynum].energy;
	  // Me.health = Druidmap[OpponentType].maxenergy;
	  Me.health += Druidmap[OpponentType].maxenergy;

	  Me.type = AllEnemys[enemynum].type;
	  Me.Marker = AllEnemys[enemynum].Marker;

	  RealScore += Druidmap[OpponentType].score;
	  if (LeaderColor != YourColor)	/* only won because of InvincibleMode */
	    message = "You cheat";
	  else				/* won the proper way */
	    message = "Complete";

	  FinishTakeover = TRUE;
	}				/* LeaderColor == YourColor */
      else if (LeaderColor == OpponentColor)
	{
	  Switch_Background_Music_To (SILENCE);
	  Takeover_Game_Lost_Sound ();
	  if (Me.type != DRUID001)
	    {
	      message = "Rejected";
	      Me.type = DRUID001;
	      Me.energy = RejectEnergy;
	    }
	  else
	    {
	      message = "Burnt Out";
	      Me.energy = 0;
	    }
	  FinishTakeover = TRUE;
	}			/* LeadColor == OpponentColor */
      else
	{
	  Takeover_Game_Deadlock_Sound ();
	  message = "Deadlock";
	}			/* LeadColor == REMIS */

      // NONSENSE FROM THE OLD ENGINE RedrawInfluenceNumber ();

      /* don't display enemy if we're finished */
      if (FinishTakeover) 
	{
	  AllEnemys[enemynum].Status = OUT;
	  OpponentType = -1;	/* dont display enemy any more */
	}

      /* Wait a turn */
      waiter = WAIT_AFTER_GAME;
      while (waiter != 0)
	{
	  usleep (30000);
	  waiter--;
	  DisplayBanner (message, NULL , 0 );	
	  ShowPlayground ();
	} /* WHILE waiter */

    }	/* while !FinishTakeover */

  ClearGraphMem();

  Switch_Background_Music_To (COMBAT_BACKGROUND_MUSIC_SOUND);

  if (LeaderColor == YourColor)
    return TRUE;
  else
    return FALSE;

} /* Takeover() */


/*@Function============================================================
@Desc: ChooseColor():	Countdown zum Waehlen der Farbe 

@Ret: void
@Int:
* $Function----------------------------------------------------------*/
void
ChooseColor (void)
{
  int countdown = 100;  /* duration in 1/10 seconds given for color choosing */
  int ColorChosen = FALSE;
  char count_text[80];
  
  Uint32 prev_count_tick, count_tick_len;

  count_tick_len = 100; 	/* countdown in 1/10 second steps */

  prev_count_tick = SDL_GetTicks ();

  while (!ColorChosen)
    {
      /* wait for next countdown tick */
      while ( SDL_GetTicks() < prev_count_tick + count_tick_len ); 

      prev_count_tick += count_tick_len; /* set for next tick */
      
      if (RightPressed ())
	{
	  YourColor = VIOLETT;
	  OpponentColor = GELB;
	}
      if (LeftPressed ())
	{
	  YourColor = GELB;
	  OpponentColor = VIOLETT;
	}

      if (SpacePressed ())
	{
	  ColorChosen = TRUE;
	  while (SpacePressed ()) ;
	}

      countdown--;		/* Count down */
      sprintf (count_text, "Color? %d", countdown);

      DisplayBanner (count_text, NULL , 0);
      ShowPlayground ();


      if (countdown == 0)
	ColorChosen = TRUE;

    } /* while(!ColorChosen) */

  return;

} /* ChooseColor() */


/*-----------------------------------------------------------------
 * @Desc: the acutal Takeover game-playing is done here
 *
 *
 *-----------------------------------------------------------------*/
void
PlayGame (void)
{
  int countdown = 100;   /* lenght of Game in 1/10 seconds */
  char count_text[80];
  int FinishTakeover = FALSE;
  int row;

  Uint32 prev_count_tick, count_tick_len;  /* tick vars for count-down */
  Uint32 prev_move_tick, move_tick_len;    /* tick vars for motion */
  int wait_move_ticks;    /* number of move-ticks to wait before "key-repeat" */


  int up, down, set; 
  int up_counter, down_counter; 

  count_tick_len = 100;   /* countdown in 1/10 second steps */
  move_tick_len  = 60;    /* allow motion at this tick-speed in ms */
  
  up = down = set = FALSE;
  up_counter = down_counter = 0;

  wait_move_ticks = 1;  

  prev_count_tick = prev_move_tick = SDL_GetTicks (); /* start tick clock */
  
  while (!FinishTakeover)
    {
      cur_time = SDL_GetTicks ();
      
      /* 
       * here we register if there have been key-press events in the
       * "waiting period" between move-ticks :
       */
      up   = up   | UpPressed(); 
      down = down | DownPressed();
      set  = set  | SpacePressed();

      if (!up) up_counter = 0;    /* reset counters for released keys */
      if (!down) down_counter =0;

      /* allow for a WIN-key that give immedate victory */
      if ( WPressed () && Ctrl_Was_Pressed () && Alt_Was_Pressed () )
	{
	  LeaderColor = YourColor;   /* simple as that */
	  return;  /* leave now, to avoid changing of LeaderColor! */
	} 
	
      if ( cur_time > prev_count_tick + count_tick_len ) /* time to count 1 down */
	{
	  prev_count_tick += count_tick_len;  /* set for next countdown tick */
	  countdown--;
	  sprintf (count_text, "Finish-%d", countdown);
	  DisplayBanner (count_text, NULL , 0 );

	  if (countdown == 0)
	    FinishTakeover = TRUE;

	  AnimateCurrents ();  /* do some animation on the active cables */

	} /* if (countdown_tick has occurred) */


      /* time for movement */
      if ( cur_time > prev_move_tick + move_tick_len )  
	{
	  prev_move_tick += move_tick_len; /* set for next motion tick */
	  EnemyMovements ();

	  if (up)
	    {
	      if (!up_counter || (up_counter > wait_move_ticks) )
		{
		  CapsuleCurRow[YourColor]--;
		  if (CapsuleCurRow[YourColor] < 1)
		    CapsuleCurRow[YourColor] = NUM_LINES;
		}
	      up = FALSE;  
	      up_counter ++;
	    }
	  if (down)
	    {
	      if (!down_counter || (down_counter > wait_move_ticks))
		{
		  CapsuleCurRow[YourColor]++;
		  if (CapsuleCurRow[YourColor] > NUM_LINES)
		    CapsuleCurRow[YourColor] = 1;
		}
	      down = FALSE;
	      down_counter ++;
	    }

	  if ( set && (NumCapsules[YOU] > 0))
	    {
	      set = FALSE;
	      row = CapsuleCurRow[YourColor] - 1;
	      if ((row >= 0) &&
		  (ToPlayground[YourColor][0][row] != KABELENDE) &&
		  (ActivationMap[YourColor][0][row] == INACTIVE))
		{
		  NumCapsules[YOU]--;
		  CapsuleCurRow[YourColor] = 0;
		  ToPlayground[YourColor][0][row] = VERSTAERKER;
		  ActivationMap[YourColor][0][row] = ACTIVE1;
		  CapsuleCountdown[YourColor][0][row] = CAPSULE_COUNTDOWN * 2;

		  Takeover_Set_Capsule_Sound ();
		}	/* if (row > 0 && ... ) */
	    } /* if ( set ) */

	  ProcessCapsules ();	/* count down the lifetime of the capsules */

	  ProcessPlayground ();
	  ProcessPlayground ();
	  ProcessPlayground ();
	  ProcessPlayground ();	/* this has to be done several times to be sure */

	  ProcessDisplayColumn ();

	} /* if (motion_tick has occurred) */

      ShowPlayground ();

    }	/* while !FinishTakeover */

  /* Schluss- Countdown */
  countdown = CAPSULE_COUNTDOWN + 10;

  while (countdown--)
    {

      while ( SDL_GetTicks() < prev_count_tick + count_tick_len ) ;

      prev_count_tick += count_tick_len;
      
      ProcessCapsules ();	/* count down the lifetime of the capsules */
      ProcessCapsules ();	/* do it twice this time to be faster */

      AnimateCurrents ();

      ProcessPlayground ();
      ProcessPlayground ();
      ProcessPlayground ();
      ProcessPlayground ();	/* this has to be done several times to be sure */

      ProcessDisplayColumn ();

      ShowPlayground ();
    }	/* while (countdown) */


  return;

} /* PlayGame() */

/*-----------------------------------------------------------------
 * @Desc: animiert Gegner beim Uebernehm-Spiel
 * 
 * @Ret: void
 * @Int:
 *-----------------------------------------------------------------*/
void
EnemyMovements (void)
{
  static int Actions = 3;
  static int MoveProbability = 100;
  static int TurnProbability = 10;
  static int SetProbability = 80;

  int action;
  static int direction = 1;	/* start with this direction */
  int row = CapsuleCurRow[OpponentColor] - 1;

  if (NumCapsules[ENEMY] == 0)
    return;


  action = MyRandom (Actions);
  switch (action)
    {
    case 0:			/* Move along */
      if (MyRandom (100) <= MoveProbability)
	{
	  row += direction;
	  if (row > NUM_LINES - 1)
	    row = 0;
	  if (row < 0)
	    row = NUM_LINES - 1;
	}
      break;

    case 1:			/* Turn around */
      if (MyRandom (100) <= TurnProbability)
	{
	  direction *= -1;
	}
      break;

    case 2:			/* Try to set  capsule */
      if (MyRandom (100) <= SetProbability)
	{
	  if ((row >= 0) &&
	      (ToPlayground[OpponentColor][0][row] != KABELENDE) &&
	      (ActivationMap[OpponentColor][0][row] == INACTIVE))
	    {
	      NumCapsules[ENEMY]--;
	      Takeover_Set_Capsule_Sound ();
	      ToPlayground[OpponentColor][0][row] = VERSTAERKER;
	      ActivationMap[OpponentColor][0][row] = ACTIVE1;
	      CapsuleCountdown[OpponentColor][0][row] = CAPSULE_COUNTDOWN;
	      row = -1;		/* For the next capsule: startpos */
	    }
	} /* if MyRandom */

      break;

    default:
      break;

    }	/* switch action */

  CapsuleCurRow[OpponentColor] = row + 1;

  return;
}	/* EnemyMovements */

/*@Function============================================================
@Desc: 

@Ret: 
@Int:
* $Function----------------------------------------------------------*/
int
GetTakeoverGraphics (void)
{
  int i,j;
  int curx = 0, cury = 0;
  SDL_Rect tmp;

  Set_Rect (tmp, User_Rect.x, User_Rect.y, 0, 0);
  to_blocks = IMG_Load (TO_BLOCK_FILE);

  /* Set the fill-blocks */
  for (i=0; i<NUM_FILL_BLOCKS; i++,curx += FILL_BLOCK_LEN + 2)
    Set_Rect (FillBlocks[i], curx, cury, FILL_BLOCK_LEN, FILL_BLOCK_HEIGHT);

  /* Set the capsule Blocks */
  for (i = 0; i < NUM_CAPS_BLOCKS; i++, curx += CAPSULE_LEN + 2)
    Set_Rect (CapsuleBlocks[i], curx, cury, CAPSULE_LEN, CAPSULE_HEIGHT);
  
  curx = 0;
  cury += FILL_BLOCK_HEIGHT + 2;

  /* get the game-blocks */

  for (j = 0; j < 2*NUM_PHASES; j++)
    {
      for (i = 0; i < TO_BLOCKS; i++)
	{
	  Set_Rect (ToGameBlocks[j*TO_BLOCKS+i], curx, cury, TO_BLOCKLEN,TO_BLOCKHEIGHT);
	  curx += TO_BLOCKLEN + 2;
	}
      curx = 0;
      cury += TO_BLOCKHEIGHT + 2;
    }

  /* Get the ground, column and leader blocks */
  for (i = 0; i < NUM_GROUND_BLOCKS; i++)
    {
      Set_Rect (ToGroundBlocks[i], curx, cury, GROUNDBLOCKLEN, GROUNDBLOCKHEIGHT);
      curx += GROUNDBLOCKLEN + 2;
    }
  cury += GROUNDBLOCKHEIGHT + 2;
  curx = 0;

  Set_Rect (ToColumnBlock, curx, cury, COLUMNBLOCKLEN, COLUMNBLOCKHEIGHT);
		
  curx += COLUMNBLOCKLEN + 2;

  Set_Rect (ToLeaderBlock, curx, cury, LEADERBLOCKLEN, LEADERBLOCKHEIGHT);

  return OK;
}				// int GetTakeoverGraphics(void)

/*-----------------------------------------------------------------
 * @Desc: prepares _and displays_ (via PrepareScaledSurface(TRUE)
 *        the current Playground
 *
 *   NOTE: this function should only change the USERFENSTER part
 *         of Outline320x200 !!
 *         so that we can do Infoline-setting before this
 *
 * @Ret: void
 *
 *-----------------------------------------------------------------*/
void
ShowPlayground ()
{
  int i, j;
  int color, player;
  int block;
  SDL_Rect Target_Rect;

  SDL_SetColorKey (ne_screen, 0, 0);
  SDL_SetClipRect (ne_screen , &User_Rect);

  Fill_Rect (User_Rect, to_bg_color);

  PutInfluence (DruidStart[YourColor].x, DruidStart[YourColor].y);

  if (AllEnemys[DroidNum].Status != OUT)
    PutEnemy (DroidNum, DruidStart[!YourColor].x, DruidStart[!YourColor].y);


  Set_Rect (Target_Rect, User_Rect.x + LEFT_OFFS_X, User_Rect.y + LEFT_OFFS_Y,
	    User_Rect.w, User_Rect.h);

  SDL_BlitSurface (to_blocks, &ToGroundBlocks[GELB_OBEN],
		   ne_screen, &Target_Rect);

  Target_Rect.y += GROUNDBLOCKHEIGHT;

  for (i = 0; i < 12; i++)
    {
      SDL_BlitSurface (to_blocks, &ToGroundBlocks[GELB_MITTE],
		       ne_screen, &Target_Rect);

      Target_Rect.y += GROUNDBLOCKHEIGHT;
    }				/* for i=1 to 12 */

  SDL_BlitSurface (to_blocks, &ToGroundBlocks[GELB_UNTEN],
		   ne_screen, &Target_Rect);


  /* Mittlere Saeule */
  Set_Rect (Target_Rect, User_Rect.x + MID_OFFS_X, User_Rect.y + MID_OFFS_Y,0, 0);
  SDL_BlitSurface (to_blocks, &ToLeaderBlock,
		   ne_screen, &Target_Rect);

  Target_Rect.y += LEADERBLOCKHEIGHT;
  for (i = 0; i < 12; i++, Target_Rect.y += COLUMNBLOCKHEIGHT)
    SDL_BlitSurface (to_blocks, &ToColumnBlock,
		     ne_screen, &Target_Rect);


  /* rechte Saeule */
  Set_Rect (Target_Rect, User_Rect.x + RIGHT_OFFS_X, User_Rect.y + RIGHT_OFFS_Y,0, 0);

  SDL_BlitSurface (to_blocks, &ToGroundBlocks[VIOLETT_OBEN],
		   ne_screen, &Target_Rect);
  Target_Rect.y += GROUNDBLOCKHEIGHT;

  for (i = 0; i < 12; i++, Target_Rect.y += GROUNDBLOCKHEIGHT)
    SDL_BlitSurface (to_blocks, &ToGroundBlocks[VIOLETT_MITTE],
		     ne_screen, &Target_Rect);

  SDL_BlitSurface (to_blocks, &ToGroundBlocks[VIOLETT_UNTEN],
		   ne_screen, &Target_Rect);

  /* Fill the Leader-LED with its color */
  Set_Rect (Target_Rect, LEADERLED_X, LEADERLED_Y, 0, 0);
  SDL_BlitSurface (to_blocks, &FillBlocks[LeaderColor],
		   ne_screen, &Target_Rect);
  Target_Rect.y += FILL_BLOCK_HEIGHT;
  SDL_BlitSurface (to_blocks, &FillBlocks[LeaderColor],
		   ne_screen, &Target_Rect);

  /* Fill the Display Column with its colors */
  for (i = 0; i < NUM_LINES; i++)
    {
      Set_Rect (Target_Rect, LEDCOLUMN_X, LEDCOLUMN_Y + i*(FILL_BLOCK_HEIGHT+2),
		0, 0);
      SDL_BlitSurface (to_blocks, &FillBlocks[DisplayColumn[i]],
		       ne_screen, &Target_Rect);
    }


  /* Show the yellow playground */
  for (i = 0; i < NUM_LAYERS - 1; i++)
    for (j = 0; j < NUM_LINES; j++)
      {
	Set_Rect (Target_Rect, PlaygroundStart[GELB].x + i * TO_BLOCKLEN,
		  PlaygroundStart[GELB].y + j * TO_BLOCKHEIGHT, 0, 0);
	block = ToPlayground[GELB][i][j] + ActivationMap[GELB][i][j]*TO_BLOCKS;
	SDL_BlitSurface (to_blocks, &ToGameBlocks[block],ne_screen, &Target_Rect);
      }


  /* Show the violett playground */
  for (i = 0; i < NUM_LAYERS - 1; i++)
    for (j = 0; j < NUM_LINES; j++)
      {
	Set_Rect (Target_Rect,
		  PlaygroundStart[VIOLETT].x +(NUM_LAYERS-i-2)*TO_BLOCKLEN,
		  PlaygroundStart[VIOLETT].y + j * TO_BLOCKHEIGHT, 0, 0);
	block = ToPlayground[VIOLETT][i][j]+
	  (NUM_PHASES+ActivationMap[VIOLETT][i][j])*TO_BLOCKS;
	SDL_BlitSurface (to_blocks, &ToGameBlocks[block],ne_screen, &Target_Rect);
      }

  /* Show the capsules left for each player */
  for (player = 0; player < 2; player++)
    {
      if (player == YOU)
	color = YourColor;
      else
	color = OpponentColor;

      Set_Rect (Target_Rect, CurCapsuleStart[color].x, 
		CurCapsuleStart[color].y + CapsuleCurRow[color]*(CAPSULE_HEIGHT+2),
		0,0);
      if (NumCapsules[player])
	SDL_BlitSurface (to_blocks, &CapsuleBlocks[color], ne_screen, &Target_Rect);


      for (i = 0; i < NumCapsules[player]-1; i++)
	{
	  Set_Rect (Target_Rect, LeftCapsulesStart[color].x,
		    LeftCapsulesStart[color].y + i*CAPSULE_HEIGHT, 0, 0);
	  SDL_BlitSurface (to_blocks, &CapsuleBlocks[color],
			   ne_screen, &Target_Rect);
	} /* for capsules */
    } /* for player */

  SDL_Flip (ne_screen);

  return;

}				/* ShowPlayground */


/*-----------------------------------------------------------------
 * @Desc: Clears Playground (and ActivationMap) to default start-values
 * @Ret:  void
 *
 *-----------------------------------------------------------------*/
void
ClearPlayground (void)
{
  int color, layer, row;

  for (color = GELB; color < TO_COLORS; color++)
    for (layer = 0; layer < NUM_LAYERS; layer++)
      for (row = 0; row < NUM_LINES; row++)
	{
	  ActivationMap[color][layer][row] = INACTIVE;
	  if (layer < TO_COLORS - 1)
	    ToPlayground[color][layer][row] = KABEL;
	  else
	    ToPlayground[color][layer][row] = INACTIVE;
	}

  for (row = 0; row < NUM_LINES; row++)
    DisplayColumn[row] = row % 2;

}				/* ClearPlayground */


/*-----------------------------------------------------------------
 * @Desc: generate a random Playground
 *	
 * @Ret: void
 *
 *-----------------------------------------------------------------*/
void
InventPlayground (void)
{
  int anElement;
  int newElement;
  int row, layer;
  int color = GELB;

  /* first clear the playground: we depend on this !! */
  ClearPlayground ();

  for (color = GELB; color < TO_COLORS; color++)
    {
      for (layer = 1; layer < NUM_LAYERS - 1; layer++)
	{
	  for (row = 0; row < NUM_LINES; row++)
	    {
	      if (ToPlayground[color][layer][row] != KABEL)
		continue;

	      newElement = MyRandom (TO_ELEMENTS);
	      if (MyRandom (MAX_PROB) > ElementProb[newElement])
		{
		  row--;
		  continue;
		}

	      switch (newElement)
		{
		case EL_KABEL:	/* has not to be set any more */
		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    ToPlayground[color][layer][row] = LEER;
		  break;

		case EL_KABELENDE:
		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    ToPlayground[color][layer][row] = LEER;
		  else
		    ToPlayground[color][layer][row] = KABELENDE;
		  break;

		case EL_VERSTAERKER:
		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    ToPlayground[color][layer][row] = LEER;
		  else
		    ToPlayground[color][layer][row] = VERSTAERKER;
		  break;

		case EL_FARBTAUSCHER:
		  if (layer != 2)
		    {		/* only existing on layer 2 */
		      row--;
		      continue;
		    }

		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    ToPlayground[color][layer][row] = LEER;
		  else
		    ToPlayground[color][layer][row] = FARBTAUSCHER;
		  break;

		case EL_VERZWEIGUNG:
		  if (row > NUM_LINES - 3)
		    {
		      /* try again */
		      row--;
		      break;
		    }

		  anElement = ToPlayground[color][layer - 1][row + 1];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    {
		      /* try again */
		      row--;
		      break;
		    }

		  /* dont destroy verzweigungen in prev. layer */
		  anElement = ToPlayground[color][layer - 1][row];
		  if (anElement == VERZWEIGUNG_O
		      || anElement == VERZWEIGUNG_U)
		    {
		      row--;
		      break;
		    }
		  anElement = ToPlayground[color][layer - 1][row + 2];
		  if (anElement == VERZWEIGUNG_O
		      || anElement == VERZWEIGUNG_U)
		    {
		      row--;
		      break;
		    }

		  /* cut off kabels in last layer, if any */
		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == CONNECTOR)
		    ToPlayground[color][layer - 1][row] = KABELENDE;

		  anElement = ToPlayground[color][layer - 1][row + 2];
		  if (BlockClass[anElement] == CONNECTOR)
		    ToPlayground[color][layer - 1][row + 2] = KABELENDE;

		  /* set the verzweigung itself */
		  ToPlayground[color][layer][row] = VERZWEIGUNG_O;
		  ToPlayground[color][layer][row + 1] = VERZWEIGUNG_M;
		  ToPlayground[color][layer][row + 2] = VERZWEIGUNG_U;

		  row += 2;
		  break;

		case EL_GATTER:
		  if (row > NUM_LINES - 3)
		    {
		      /* try again */
		      row--;
		      break;
		    }

		  anElement = ToPlayground[color][layer - 1][row];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    {
		      /* try again */
		      row--;
		      break;
		    }
		  anElement = ToPlayground[color][layer - 1][row + 2];
		  if (BlockClass[anElement] == NON_CONNECTOR)
		    {
		      /* try again */
		      row--;
		      break;
		    }


		  /* cut off kabels in last layer, if any */
		  anElement = ToPlayground[color][layer - 1][row + 1];
		  if (BlockClass[anElement] == CONNECTOR)
		    ToPlayground[color][layer - 1][row + 1] = KABELENDE;

		  /* set the GATTER itself */
		  ToPlayground[color][layer][row] = GATTER_O;
		  ToPlayground[color][layer][row + 1] = GATTER_M;
		  ToPlayground[color][layer][row + 2] = GATTER_U;

		  row += 2;
		  break;

		default:
		  row--;
		  break;

		}		/* switch NewElement */

	    }			/* for row */

	}			/* for layer */

    }				/* for color */

}				/* InventPlayground */

/*-----------------------------------------------------------------
 * @Desc: process the playground following its intrinsic logic
 *
 * @Ret: void
 *
 *-----------------------------------------------------------------*/
void
ProcessPlayground (void)
{
  int color, layer, row;
  int TurnActive = FALSE;

  for (color = GELB; color < TO_COLORS; color++)
    {
      for (layer = 1; layer < NUM_LAYERS; layer++)
	{
	  for (row = 0; row < NUM_LINES; row++)
	    {
	      if (layer == NUM_LAYERS - 1)
		{
		  if (IsActive (color, row))
		    ActivationMap[color][layer][row] = ACTIVE1;
		  else
		    ActivationMap[color][layer][row] = INACTIVE;

		  continue;
		}		/* if last layer */

	      TurnActive = FALSE;

	      switch (ToPlayground[color][layer][row])
		{
		case FARBTAUSCHER:
		case VERZWEIGUNG_M:
		case GATTER_O:
		case GATTER_U:
		case KABEL:
		  if (ActivationMap[color][layer - 1][row] >= ACTIVE1)
		    TurnActive = TRUE;
		  break;

		case VERSTAERKER:
		  if (ActivationMap[color][layer - 1][row] >= ACTIVE1)
		    TurnActive = TRUE;

		  /* Verstaerker halten sich aber auch selbst aktiv !! */
		  if (ActivationMap[color][layer][row] >= ACTIVE1)
		    TurnActive = TRUE;

		  break;

		case KABELENDE:
		  break;

		case VERZWEIGUNG_O:
		  if (ActivationMap[color][layer][row + 1] >= ACTIVE1)
		    TurnActive = TRUE;
		  break;

		case VERZWEIGUNG_U:
		  if (ActivationMap[color][layer][row - 1] >= ACTIVE1)
		    TurnActive = TRUE;
		  break;

		case GATTER_M:
		  if ((ActivationMap[color][layer][row - 1] >= ACTIVE1)
		      && (ActivationMap[color][layer][row + 1] >= ACTIVE1))
		    TurnActive = TRUE;

		  break;

		default:
		  break;
		}		/* switch */

	      if (TurnActive)
		{
		  if (ActivationMap[color][layer][row] == INACTIVE)
		    ActivationMap[color][layer][row] = ACTIVE1;
		  TurnActive = FALSE;
		}
	      else 
		ActivationMap[color][layer][row] = INACTIVE;


	    }			/* for row */

	}			/* for layer */

    }				/* for color */

  return;
}				/* ProcessPlayground */



/*@Function============================================================
@Desc:  ProcessDisplayColumn(): setzt die Korrekten Werte in der Display-
        Saeule. Blinkende LEDs werden ebenfalls hier realisiert

@Ret: void
@Int:
* $Function----------------------------------------------------------*/
void
ProcessDisplayColumn (void)
{
  static int CLayer = 3;	/* the connection-layer to the Column */
  static int flicker_color = 0;
  int row;
  int GelbCounter, ViolettCounter;

  flicker_color = !flicker_color;

  for (row = 0; row < NUM_LINES; row++)
    {
      /* eindeutig gelb */
      if ((ActivationMap[GELB][CLayer][row] >= ACTIVE1) &&
	  (ActivationMap[VIOLETT][CLayer][row] == INACTIVE))
	{
	  /* Farbtauscher ??? */
	  if (ToPlayground[GELB][CLayer - 1][row] == FARBTAUSCHER)
	    DisplayColumn[row] = VIOLETT;
	  else
	    DisplayColumn[row] = GELB;
	  continue;
	}

      /* eindeutig violett */
      if ((ActivationMap[GELB][CLayer][row] == INACTIVE) &&
	  (ActivationMap[VIOLETT][CLayer][row] >= ACTIVE1))
	{
	  /* Farbtauscher ??? */
	  if (ToPlayground[VIOLETT][CLayer - 1][row] == FARBTAUSCHER)
	    DisplayColumn[row] = GELB;
	  else
	    DisplayColumn[row] = VIOLETT;

	  continue;
	}

      /* unentschieden: Flimmern */
      if ((ActivationMap[GELB][CLayer][row] >= ACTIVE1) &&
	  (ActivationMap[VIOLETT][CLayer][row] >= ACTIVE1))
	{
	  /* Farbtauscher - Faelle */
	  if ((ToPlayground[GELB][CLayer - 1][row] == FARBTAUSCHER) &&
	      (ToPlayground[VIOLETT][CLayer - 1][row] != FARBTAUSCHER))
	    DisplayColumn[row] = VIOLETT;
	  else if ((ToPlayground[GELB][CLayer - 1][row] != FARBTAUSCHER) &&
		   (ToPlayground[VIOLETT][CLayer - 1][row] == FARBTAUSCHER))
	    DisplayColumn[row] = GELB;
	  else
	    {
	      if (flicker_color == 0)
		DisplayColumn[row] = GELB;
	      else
		DisplayColumn[row] = VIOLETT;
	    }			/* if - else if - else */

	}			/* if unentschieden */

    }				/* for */

  /* Win Color beurteilen */
  GelbCounter = 0;
  ViolettCounter = 0;
  for (row = 0; row < NUM_LINES; row++)
    if (DisplayColumn[row] == GELB)
      GelbCounter++;
    else
      ViolettCounter++;

  if (ViolettCounter < GelbCounter)
    LeaderColor = GELB;
  else if (ViolettCounter > GelbCounter)
    LeaderColor = VIOLETT;
  else
    LeaderColor = REMIS;

  return;
}				/* ProcessDisplayColumn */

/*@Function============================================================
@Desc: ProcessCapsules():	does the countdown of the capsules and
									kills them if too old

@Ret: void
@Int:
* $Function----------------------------------------------------------*/
void
ProcessCapsules (void)
{
  int row;
  int color;

  for (color = GELB; color <= VIOLETT; color++)
    for (row = 0; row < NUM_LINES; row++)
      {
	if (CapsuleCountdown[color][0][row] > 0)
	  CapsuleCountdown[color][0][row]--;

	if (CapsuleCountdown[color][0][row] == 0)
	  {
	    CapsuleCountdown[color][0][row] = -1;
	    ActivationMap[color][0][row] = INACTIVE;
	    ToPlayground[color][0][row] = KABEL;
	  }

      } /* for row */

}   /* ProcessCapsules() */


/*@Function============================================================
@Desc: IsInactive(color, row): tells, wether a Column-connection
						is active or not

@Ret: TRUE/FALSE
@Int:
* $Function----------------------------------------------------------*/
int
IsActive (int color, int row)
{
  int CLayer = 3;		/* the connective Layer */
  int TestElement = ToPlayground[color][CLayer - 1][row];

  if ((ActivationMap[color][CLayer-1][row] >= ACTIVE1) &&
      (BlockClass[TestElement] == CONNECTOR))
    return TRUE;
  else
    return FALSE;
}				/* IsActive */

/*-----------------------------------------------------------------
 *
 * Animate the active cables: this is done by cycling over
 * the active phases ACTIVE1-ACTIVE3, which are represented by 
 * different pictures in the playground
 *
 *-----------------------------------------------------------------*/
void
AnimateCurrents (void)
{
  int color, layer, row;

  for (color = GELB; color <= VIOLETT; color ++)
    for (layer = 0; layer < NUM_LAYERS; layer ++)
      for (row = 0; row < NUM_LINES; row ++)
	if (ActivationMap[color][layer][row] >= ACTIVE1)
	  {
	    ActivationMap[color][layer][row] ++; 
	    if (ActivationMap[color][layer][row] == NUM_PHASES)
	      ActivationMap[color][layer][row] = ACTIVE1;
	  }

  return;
}


#undef _takeover_c
