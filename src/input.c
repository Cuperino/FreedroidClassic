/* 
 *
 *   Copyright (c) 1994, 2002, 2003  Johannes Prix
 *   Copyright (c) 1994, 2002, 2003  Reinhard Prix
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

/*----------------------------------------------------------------------
 *
 * Desc: functions for keyboard and joystick handling
 *
 *----------------------------------------------------------------------*/

#define _input_c

#include "system.h"

#include "defs.h"
#include "struct.h"
#include "global.h"
#include "proto.h"

// earlier SDL versions didn't define these...
#ifndef SDL_BUTTON_WHEELUP 
#define SDL_BUTTON_WHEELUP 4
#endif
#ifndef SDL_BUTTON_WHEELDOWN
#define SDL_BUTTON_WHEELDOWN 5
#endif

bool show_cursor;    // show mouse-cursor or not?
#define CURSOR_KEEP_VISIBLE  3000   // ticks to keep mouse-cursor visible without mouse-input

int WheelUpEvents=0;    // count number of not read-out wheel events
int WheelDownEvents=0;
Uint32 last_mouse_event = 0;  // record when last mouse event took place (SDL ticks)
int CurrentlyMouseRightPressed=0;
int CurrentlyMouseLeftPressed = 0;


SDLMod current_modifiers;

SDL_Event event;

int input_state[INPUT_LAST];	// array of states (pressed/released) of all keys

#define FRESH_BIT   	(0x01<<8)
#define PRESSED		(TRUE|FRESH_BIT)
#define RELEASED	(FALSE|FRESH_BIT)

#define is_down(x) ((x) & (~FRESH_BIT) )
#define just_pressed(x) ( (x) & PRESSED == PRESSED)

#define clear_fresh(x) do { (x) &= ~FRESH_BIT; } while(0)



int sgn (int x)
{
  return (x ? ((x)/abs(x)) : 0);
}

void Init_Joy (void)
{
  int num_joy;

  if (SDL_InitSubSystem (SDL_INIT_JOYSTICK) == -1)
    {
      fprintf(stderr, "Couldn't initialize SDL-Joystick: %s\n", SDL_GetError());
      Terminate(ERR);
    } else
      DebugPrintf(1, "\nSDL Joystick initialisation successful.\n");


  DebugPrintf (1, " %d Joysticks found!\n", num_joy = SDL_NumJoysticks ());

  if (num_joy > 0)
    joy = SDL_JoystickOpen (0);

  if (joy)
    {
      DebugPrintf (1, "Identifier: %s\n", SDL_JoystickName (0));
      DebugPrintf (1, "Number of Axes: %d\n", joy_num_axes = SDL_JoystickNumAxes(joy));
      DebugPrintf (1, "Number of Buttons: %d\n", SDL_JoystickNumButtons(joy));

      /* aktivate Joystick event handling */
      SDL_JoystickEventState (SDL_ENABLE); 

    }
  else 
    joy = NULL;  /* signals that no yoystick is present */


  return;
}

// FIXME: remove that obsolete stuff...
void 
ReactToSpecialKeys(void)
{
  //--------------------
  // user asked for quit
  //
  if ( KeyIsPressedR('q') ) 
    QuitGameMenu();

  if ( KeyIsPressedR('c') && AltPressed() && CtrlPressed() && ShiftPressed() ) 
    Cheatmenu ();
  if ( EscapePressedR() )
    EscapeMenu ();

  if ( KeyIsPressedR ('p') )
    Pause ();

  if (KeyIsPressedR (SDLK_F12) )
    TakeScreenshot();

} // void ReactToSpecialKeys(void)

//----------------------------------------------------------------------
// main input-reading routine
//----------------------------------------------------------------------
int 
update_input (void)
{
  Uint8 axis; 

  // switch mouse-cursor visibility as a function of time of last activity
  if (SDL_GetTicks () - last_mouse_event > CURSOR_KEEP_VISIBLE)
    show_cursor = FALSE;
  else
    show_cursor = TRUE;

  while( SDL_PollEvent( &event ) )
    {
      switch( event.type )
	{
	case SDL_QUIT:
	  printf("\n\nUser requestet Termination...\n\nTerminating...");
	  Terminate(0);
	  break;
	  /* Look for a keypress */

	case SDL_KEYDOWN:
	  current_modifiers = event.key.keysym.mod;
	  input_state[event.key.keysym.sym] = PRESSED;
	  break;
	case SDL_KEYUP:
	  current_modifiers = event.key.keysym.mod;
	  input_state[event.key.keysym.sym] = RELEASED;
	  break;

	case SDL_JOYAXISMOTION:
	  axis = event.jaxis.axis;
	  if (axis == 0 || ((joy_num_axes >= 5) && (axis == 3)) ) /* x-axis */
	    {
	      input_axis.x = event.jaxis.value;

	      // this is a bit tricky, because we want to allow direction keys
	      // to be soft-released. When mapping the joystick->keyboard, we 
	      // therefore have to make sure that this mapping only occurs when
	      // and actual _change_ of the joystick-direction ('digital') occurs
	      // so that it behaves like "set"/"release"
	      if (joy_sensitivity*event.jaxis.value > 10000)   /* about half tilted */
		{
		  input_state[JOY_RIGHT] = PRESSED;
		  input_state[JOY_LEFT] = FALSE;
		}
	      else if (joy_sensitivity*event.jaxis.value < -10000)
		{
		  input_state[JOY_LEFT] = PRESSED;
		  input_state[JOY_RIGHT] = FALSE;
		}
	      else
		{
		  input_state[JOY_LEFT] = FALSE;
		  input_state[JOY_RIGHT] = FALSE;
		}
	    }
	  else if ((axis == 1) || ((joy_num_axes >=5) && (axis == 4))) /* y-axis */
	    {
	      input_axis.y = event.jaxis.value;

	      if (joy_sensitivity*event.jaxis.value > 10000)  
		{
		  input_state[JOY_DOWN] = PRESSED;
		  input_state[JOY_UP] =  FALSE;
		}
	      else if (joy_sensitivity*event.jaxis.value < -10000)
		{
		  input_state[JOY_UP] = PRESSED;
		  input_state[JOY_DOWN]= FALSE;
		}
	      else
		{
		  input_state[JOY_UP] = FALSE;
		  input_state[JOY_DOWN] = FALSE;
		}
	    }
		
	  break;
	  
	case SDL_JOYBUTTONDOWN: 
	  // first button
	  if (event.jbutton.button == 0)
	    input_state[JOY_BUTTON1] = PRESSED;

	  // second button
	  else if (event.jbutton.button == 1) 
	    input_state[JOY_BUTTON2] = PRESSED;

	  // and third button
	  else if (event.jbutton.button == 2) 
	    input_state[JOY_BUTTON3] = PRESSED;

	  axis_is_active = TRUE;
	  break;

	case SDL_JOYBUTTONUP:
	  // first button 
	  if (event.jbutton.button == 0)
	    input_state[JOY_BUTTON1] = FALSE;

	  // second button
	  else if (event.jbutton.button == 1) 
	    input_state[JOY_BUTTON2] = FALSE;

	  // and third button
	  else if (event.jbutton.button == 2) 
	    input_state[JOY_BUTTON3] = FALSE;

	  axis_is_active = FALSE;
	  break;

	case SDL_MOUSEMOTION:
	  input_axis.x = event.button.x - UserCenter_x + 16; 
	  input_axis.y = event.button.y - UserCenter_y + 16; 	  

	  last_mouse_event = SDL_GetTicks ();

	  break;
	  
	  /* Mouse control */
	case SDL_MOUSEBUTTONDOWN:
	  if (event.button.button == SDL_BUTTON_LEFT)
	    {
	      input_state[MOUSE_BUTTON1] = PRESSED;
	      axis_is_active = TRUE;
	    }

	  if (event.button.button == SDL_BUTTON_RIGHT)
	    input_state[MOUSE_BUTTON2] = PRESSED;

	  if (event.button.button == SDL_BUTTON_MIDDLE)  
	    input_state[MOUSE_BUTTON3] = PRESSED;

	  // wheel events are immediately released, so we rather
	  // count the number of not yet read-out events
	  if (event.button.button == SDL_BUTTON_WHEELUP)
	      WheelUpEvents ++;

	  if (event.button.button == SDL_BUTTON_WHEELDOWN)
	      WheelDownEvents ++;

	  last_mouse_event = SDL_GetTicks();
	  break;

        case SDL_MOUSEBUTTONUP:
	  if (event.button.button == SDL_BUTTON_LEFT)
	    {
	      input_state[MOUSE_BUTTON1] = FALSE;
	      axis_is_active = FALSE;
	    }

	  if (event.button.button == SDL_BUTTON_RIGHT)
	    input_state[MOUSE_BUTTON2] = FALSE;

	  if (event.button.button == SDL_BUTTON_MIDDLE)
	    input_state[MOUSE_BUTTON3] = FALSE;

	  break;

 	default:
 	  break;
 	}

    }

  return 0;
}

/*-----------------------------------------------------------------
 * Desc: should do roughly what getchar() does, but in raw 
 * 	 (SLD) keyboard mode. 
 * 
 * Return: the (SDLKey) of the next key-pressed event cast to (int)
 *
 *-----------------------------------------------------------------*/
int
getchar_raw (void)
{
  SDL_Event event;
  int Returnkey;

  //  keyboard_update ();   /* treat all pending keyboard-events */

  while (1)
    {
      SDL_WaitEvent (&event);    /* wait for next event */
      
      if (event.type == SDL_KEYDOWN)
	{
	  /* 
	   * here we use the fact that, I cite from SDL_keyboard.h:
	   * "The keyboard syms have been cleverly chosen to map to ASCII"
	   * ... I hope that this design feature is portable, and durable ;)  
	   */
	  Returnkey = (int) event.key.keysym.sym;
	  if ( event.key.keysym.mod & KMOD_SHIFT ) Returnkey = toupper( (int)event.key.keysym.sym );
	  return ( Returnkey );
	}
      else
	{
	  SDL_PushEvent (&event);  /* put this event back into the queue */
	  update_input ();  /* and treat it the usual way */
	  continue;
	}

    } /* while(1) */

} /* getchar_raw() */

// forget the wheel-counters
void
ResetMouseWheel (void)
{
  WheelUpEvents = WheelDownEvents = 0;
  return;
}

bool
WheelUpPressed (void)
{
  update_input();
  if (WheelUpEvents)
    return (WheelUpEvents--);
  else
    return (FALSE);
}

bool
WheelDownPressed (void)
{
  update_input();
  if (WheelDownEvents)
    return (WheelDownEvents--);
  else
    return (FALSE);
}

bool
KeyIsPressed (SDLKey key)
{
  update_input();

  return( (input_state[key] & PRESSED) == PRESSED );
}


// does the same as KeyIsPressed, but automatically releases the key as well..
bool
KeyIsPressedR (SDLKey key)
{
  bool ret;

  ret = KeyIsPressed (key);

  ReleaseKey (key);
  return (ret);
}

void 
ReleaseKey (SDLKey key)
{
  input_state[key] = FALSE;
  return;
}

bool
ModIsPressed (SDLMod mod)
{
  bool ret;
  update_input();
  ret = ( (current_modifiers & mod) != 0) ;
  return (ret);
}

bool
NoDirectionPressed (void)
{
  if ( (axis_is_active && (input_axis.x || input_axis.y)) ||
      DownPressed () || UpPressed() || LeftPressed() || RightPressed() )
    return ( FALSE );
  else
    return ( TRUE );
} // int NoDirectionPressed(void)


//----------------------------------------------------------------------
// check if a particular key has been pressed



// check if any keys or buttons1 are pressed
bool
any_key_pressed (void)
{
  int i;
  bool ret = FALSE;

  update_input();

  for (i=0; i<SDLK_LAST; i++)
    if ( just_pressed(input_state[i]) )
      { 
	clear_fresh(input_state[i]);
	ret = TRUE; 
	break;
      }
  if ( just_pressed(input_state[JOY_BUTTON1]) )
    {
      clear_fresh (input_state[JOY_BUTTON1]);
      ret = TRUE;
    }

  if ( just_pressed(input_state[MOUSE_BUTTON1]) )
    {
      ret = TRUE;
      clear_fresh (input_state[MOUSE_BUTTON1]);
    }

  return (ret);

}  // any_key_pressed()




#undef _intput_c
