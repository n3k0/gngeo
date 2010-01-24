#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "SDL.h"
#include "screen.h"
#include "event.h"
#include "conf.h"
#include "emu.h"
#include "memory.h"

static int get_mapid(char *butid) {
	printf("Get mapid %s\n",butid);
	if (!strcmp(butid,"A")) return GN_A;
	if (!strcmp(butid,"B")) return GN_B;
	if (!strcmp(butid,"C")) return GN_C;
	if (!strcmp(butid,"D")) return GN_D;

	if (!strcmp(butid,"UP")) return GN_UP;
	if (!strcmp(butid,"DOWN")) return GN_DOWN;
	if (!strcmp(butid,"UPDOWN")) return GN_UP;

	if (!strcmp(butid,"LEFT")) return GN_LEFT;
	if (!strcmp(butid,"RIGHT")) return GN_RIGHT;
	if (!strcmp(butid,"LEFTRIGHT")) return GN_LEFT;

	if (!strcmp(butid,"JOY")) return GN_UP;

	if (!strcmp(butid,"START")) return GN_START;
	if (!strcmp(butid,"COIN")) return GN_SELECT_COIN;

	if (!strcmp(butid,"MENU")) return GN_MENU_KEY;

	if (!strcmp(butid,"HOTKEY1")) return GN_HOTKEY1;
	if (!strcmp(butid,"HOTKEY2")) return GN_HOTKEY2;
	if (!strcmp(butid,"HOTKEY3")) return GN_HOTKEY3;
	if (!strcmp(butid,"HOTKEY4")) return GN_HOTKEY4;

	return GN_NONE;
}

int create_joymap_from_string(int player,char *jconf) {
	char *v;
	char butid[32]={0,};
	char jevt;
	int code;
	int jid;
	int rc;
	char type;
	//printf("Jconf=%s\n",jconf);
	if (jconf==NULL) return 0;
	v=strdup(jconf);
	v=strtok(v,",");
	//printf("V1=%s\n",v);
	while(v) {
		rc=sscanf(v,"%[A-Z1-4]=%c%d%c%d",butid,&type,&jid,&jevt,&code);
		if (rc==3 && type=='K') { /* Keyboard */
			//printf("%s | keycode %d\n",butid,jid);
			code=jid;
			if (code<SDLK_LAST) {
				jmap->key[code].player=player;
				jmap->key[code].map=get_mapid(butid);
			}
			//printf("%d\n",get_mapid(butid));
		}
		if (rc==5 && type=='J') {
			//printf("%d, %s | joy no %d | evt %c | %d\n",
			//rc,butid,jid,jevt,code);
			if (jid<conf.nb_joy) {
				switch(jevt) {
				case 'A':
					if (code<SDL_JoystickNumAxes(conf.joy[jid])) {
						jmap->jaxe[jid][code].player=player;
						jmap->jaxe[jid][code].map=get_mapid(butid);
						jmap->jaxe[jid][code].dir=1;
					}
					break;
				case 'a': /* Inverted axis */
					if (code<SDL_JoystickNumAxes(conf.joy[jid])) {
						jmap->jaxe[jid][code].player=player;
						jmap->jaxe[jid][code].map=get_mapid(butid);
						jmap->jaxe[jid][code].dir=-1;
					}
					break;
				case 'H':
					if (code<SDL_JoystickNumHats(conf.joy[jid])) {
						jmap->jhat[jid][code].player=player;
						jmap->jhat[jid][code].map=get_mapid(butid);
					}
					break;
				case 'B':
					if (code<SDL_JoystickNumButtons(conf.joy[jid])) {
						jmap->jbutton[jid][code].player=player;
						jmap->jbutton[jid][code].map=get_mapid(butid);
					}
					break;
				}
			}
		}

		v=strtok(NULL,",");
	}
	return 1;
}

int init_event(void) {
	int i;
	jmap=calloc(sizeof(JOYMAP),1);
	
	conf.nb_joy = SDL_NumJoysticks();

	if (conf.joy!=NULL) free(conf.joy);
	conf.joy=calloc(sizeof(SDL_Joystick*),conf.nb_joy);

	SDL_JoystickEventState(SDL_ENABLE);

	jmap->jbutton=calloc(conf.nb_joy,sizeof(struct BUT_MAP*));
	jmap->jaxe=   calloc(conf.nb_joy,sizeof(struct BUT_MAPJAXIS*));
	jmap->jhat=   calloc(conf.nb_joy,sizeof(struct BUT_MAP*));


	/* Open all the available joystick */
	for (i=0;i<conf.nb_joy;i++) {
		conf.joy[i]=SDL_JoystickOpen(i);
		printf("joy \"%s\", axe:%d, button:%d\n",
		       SDL_JoystickName(i),
		       SDL_JoystickNumAxes(conf.joy[i])+ (SDL_JoystickNumHats(conf.joy[i]) * 2),
		       SDL_JoystickNumButtons(conf.joy[i]));
		jmap->jbutton[i]=calloc(SDL_JoystickNumButtons(conf.joy[i]),sizeof(struct BUT_MAP));
		jmap->jaxe[i]=calloc(SDL_JoystickNumAxes(conf.joy[i]),sizeof(struct BUT_MAPJAXIS));
		jmap->jhat[i]=calloc(SDL_JoystickNumHats(conf.joy[i]),sizeof(struct BUT_MAP));
	}

	create_joymap_from_string(1,CF_STR(cf_get_item_by_name("p1control")));
	create_joymap_from_string(2,CF_STR(cf_get_item_by_name("p2control")));
	return SDL_TRUE;
}
#ifdef GP2X
int handle_pdep_event(SDL_Event *event) {
	static int snd_volume=75;
	char volbuf[21];
	int i;
	switch (event->type) {
	case SDL_JOYBUTTONDOWN:
		//printf("Event %d %d\n",event->jbutton.which,event->jbutton.button);
		if (event->jbutton.which==0) {
			if (event->jbutton.button==GP2X_VOL_UP && conf.sound) {
				if (snd_volume<100) snd_volume+=5; else snd_volume=100;
				gp2x_sound_volume_set(snd_volume,snd_volume);
				for (i=0;i<snd_volume/5;i++) volbuf[i]='|';
				for (i=snd_volume/5;i<20;i++) volbuf[i]='-';
				volbuf[20]=0;
				draw_message(volbuf);
			}
			if (event->jbutton.button==GP2X_VOL_DOWN && conf.sound) {
				if (snd_volume>0) snd_volume-=5; else snd_volume=0;
				gp2x_sound_volume_set(snd_volume,snd_volume);
				for (i=0;i<snd_volume/5;i++) volbuf[i]='|';
				for (i=snd_volume/5;i<20;i++) volbuf[i]='-';
				volbuf[20]=0;
				draw_message(volbuf);
			}
		}
		break;
	}
	return 0;
}
#else /* Default */
int handle_pdep_event(SDL_Event *event) {
	switch (event->type) {
	case SDL_KEYDOWN:
		switch (event->key.keysym.sym) {
		case SDLK_ESCAPE:
			return 1;
			break;
		case SDLK_F12:
		    screen_fullscreen();
		    break;
		default:
			break;
		}
		break;
		default:
			break;
	}
	return 0;
}
#endif

int handle_event(void) {
	SDL_Event event;
//	int i;
	int ret;
	int jaxis_threshold=10000;

	while (SDL_PollEvent(&event)) {
	    if ((ret=handle_pdep_event(&event))!=0) {
	    	return ret;
	    }
		switch (event.type) {
		case SDL_KEYUP:
			//printf("%d\n",jmap->key[event.key.keysym.sym].player);
			switch (jmap->key[event.key.keysym.sym].player) {
			case 1:
				joy_state[0][jmap->key[event.key.keysym.sym].map]=0;
				break;
			case 2:
				joy_state[1][jmap->key[event.key.keysym.sym].map]=0;
				break;
			case 3:
				joy_state[1][jmap->key[event.key.keysym.sym].map]=0;
				joy_state[0][jmap->key[event.key.keysym.sym].map]=0;
				break;
			default:
				break;
			}
		break;
	    case SDL_KEYDOWN:
		    //printf("%d\n",jmap->key[event.key.keysym.sym].player);
		    switch (jmap->key[event.key.keysym.sym].player) {
			case 1:
				joy_state[0][jmap->key[event.key.keysym.sym].map]=1;
				break;
			case 2:
				joy_state[1][jmap->key[event.key.keysym.sym].map]=1;
				break;
			case 3:
				joy_state[1][jmap->key[event.key.keysym.sym].map]=1;
				joy_state[0][jmap->key[event.key.keysym.sym].map]=1;
				break;
			default:
				break;
		    }
		    break;
		case SDL_JOYHATMOTION: /* Hat only support Joystick map */
		{
			int player=jmap->jhat[event.jhat.which][event.jhat.hat].player;
			int map=jmap->jhat[event.jhat.which][event.jhat.hat].map;
			int i;
			if (player && map==GN_UP) {
				player-=1;
				for(i=GN_UP;i<=GN_RIGHT;i++)
					joy_state[player][i]=0;
				if (event.jhat.value&SDL_HAT_UP) joy_state[player][GN_UP]=1;
				if (event.jhat.value&SDL_HAT_DOWN) joy_state[player][GN_DOWN]=1;
				if (event.jhat.value&SDL_HAT_LEFT) joy_state[player][GN_LEFT]=1;
				if (event.jhat.value&SDL_HAT_RIGHT) joy_state[player][GN_RIGHT]=1;

			}
			
			//printf("SDL_JOYHATMOTION  %d %d %d\n",event.jhat.which,
			//event.jhat.hat,event.jhat.value);
		}
		break;
		case SDL_JOYAXISMOTION:
		{
			int player=jmap->jaxe[event.jaxis.which][event.jaxis.axis].player;
			int map=jmap->jaxe[event.jaxis.which][event.jaxis.axis].map;
			int oldvalue=jmap->jaxe[event.jaxis.which][event.jaxis.axis].value;
			int value;
			if (player) {
				player-=1;
				
				value=event.jaxis.value*jmap->jaxe[event.jaxis.which][event.jaxis.axis].dir;

				//printf("%d %d %d\n",player,map,value);
				if (map==GN_UP || map==GN_DOWN) {
					if (value>jaxis_threshold) {
						joy_state[player][GN_UP]=1;
						joy_state[player][GN_DOWN]=0;
					}
					if (value<-jaxis_threshold) {
						joy_state[player][GN_DOWN]=1;
						joy_state[player][GN_UP]=0;
					}
					if (oldvalue>jaxis_threshold && value<=jaxis_threshold && value>=-jaxis_threshold)
						joy_state[player][GN_UP]=0;
					if (oldvalue<-jaxis_threshold && value>=-jaxis_threshold && value<=jaxis_threshold)
						joy_state[player][GN_DOWN]=0;

				}
				if (map==GN_LEFT || map==GN_RIGHT) {
					if (value>jaxis_threshold) {
						joy_state[player][GN_RIGHT]=1;
						joy_state[player][GN_LEFT]=0;
					}
					if (value<-jaxis_threshold) {
						joy_state[player][GN_LEFT]=1;
						joy_state[player][GN_RIGHT]=0;
					}
					if (oldvalue>jaxis_threshold && value<=jaxis_threshold && value>=-jaxis_threshold)
						joy_state[player][GN_RIGHT]=0;
					if (oldvalue<-jaxis_threshold && value>=-jaxis_threshold && value<=jaxis_threshold)
						joy_state[player][GN_LEFT]=0;
					
				}
				
				jmap->jaxe[event.jaxis.which][event.jaxis.axis].value=value;
				
				
			}
			//printf("SDL_JOYAXISMOTION %d %d %d %d\n",event.jaxis.which,
			//event.jaxis.axis,value,jmap->jaxe[event.jaxis.which][event.jaxis.axis].dir);
		}
			break;
		case SDL_JOYBUTTONDOWN: 
		{
			int player=jmap->jbutton[event.jbutton.which][event.jbutton.button].player;
			int map=jmap->jbutton[event.jbutton.which][event.jbutton.button].map;
			//printf("player %d map %d\n",player,map);
			if (player) {
				player-=1;
				joy_state[player][map]=1;
			}
			
			//printf("SDL_JOYBUTTONDOWN %d %d\n",event.jbutton.which,event.jbutton.button);
		}
			break;
		case SDL_JOYBUTTONUP:
		{
			int player=jmap->jbutton[event.jbutton.which][event.jbutton.button].player;
			int map=jmap->jbutton[event.jbutton.which][event.jbutton.button].map;
			if (player) {
				player-=1;
				joy_state[player][map]=0;
			}
		}
			break;
		case SDL_VIDEORESIZE:
			conf.res_x=event.resize.w;
			conf.res_y=event.resize.h;
			screen_resize(event.resize.w, event.resize.h);
			break;
		case SDL_QUIT:
			return 1;
			break;
		default:
			break;
		}
	}
/*
	for(i=0;i<GN_MAX_KEY;i++)
		printf("%d",joy_state[0][i]);
	printf("|");
	for(i=0;i<GN_MAX_KEY;i++)
		printf("%d",joy_state[1][i]);
	printf("\r");
*/
	/* Update coin data */
	memory.intern_coin = 0x7;
	if (joy_state[0][GN_SELECT_COIN])
		memory.intern_coin &= 0x6;
	if (joy_state[1][GN_SELECT_COIN])
		memory.intern_coin &= 0x5;
	/* Update start data TODO: Select */
	memory.intern_start = 0x8F;
	if (joy_state[0][GN_START])
		memory.intern_start &= 0xFE;
	if (joy_state[1][GN_START])
		memory.intern_start &= 0xFB;

	/* Update P1 */
	memory.intern_p1 = 0xFF;
	if (joy_state[0][GN_UP])
	    memory.intern_p1 &= 0xFE;
	if (joy_state[0][GN_DOWN])
	    memory.intern_p1 &= 0xFD;
	if (joy_state[0][GN_LEFT])
	    memory.intern_p1 &= 0xFB;
	if (joy_state[0][GN_RIGHT])
	    memory.intern_p1 &= 0xF7;
	if (joy_state[0][GN_A])
	    memory.intern_p1 &= 0xEF;	// A
	if (joy_state[0][GN_B])
	    memory.intern_p1 &= 0xDF;	// B
	if (joy_state[0][GN_C])
	    memory.intern_p1 &= 0xBF;	// C
	if (joy_state[0][GN_D])
	    memory.intern_p1 &= 0x7F;	// D

	/* Update P1 */
	memory.intern_p2 = 0xFF;
	if (joy_state[1][GN_UP])
	    memory.intern_p2 &= 0xFE;
	if (joy_state[1][GN_DOWN])
	    memory.intern_p2 &= 0xFD;
	if (joy_state[1][GN_LEFT])
	    memory.intern_p2 &= 0xFB;
	if (joy_state[1][GN_RIGHT])
	    memory.intern_p2 &= 0xF7;
	if (joy_state[1][GN_A])
	    memory.intern_p2 &= 0xEF;	// A
	if (joy_state[1][GN_B])
	    memory.intern_p2 &= 0xDF;	// B
	if (joy_state[1][GN_C])
	    memory.intern_p2 &= 0xBF;	// C
	if (joy_state[1][GN_D])
	    memory.intern_p2 &= 0x7F;	// D

#ifdef GP2X
	if (joy_state[0][GN_HOTKEY1] && joy_state[0][GN_HOTKEY2]
	&& (joy_state[0][GN_START] || joy_state[0][GN_SELECT_COIN]))
		return 1;
#endif

	if(joy_state[0][GN_MENU_KEY]==1)
		return 1;
	else 
		return 0;

}

int wait_event(void) {
	SDL_Event event;
	int rc,i;
	int last=-1;
	for(i=0;i<GN_MAX_KEY;i++)
		if (joy_state[0][i]) last=i;
	SDL_WaitEvent(&event);
	switch (event.type) {
	case SDL_KEYDOWN:
		/* Some default keyboard standard key */
		switch (event.key.keysym.sym) {
		case SDLK_TAB:
			//joy_state[0][GN_MENU_KEY]=1;
			return GN_MENU_KEY;
			break;	
		case SDLK_UP:
			//joy_state[0][GN_UP]=1;
			return GN_UP;
			break;	
		case SDLK_DOWN:
			//joy_state[0][GN_DOWN]=1;
			return GN_DOWN;
			break;	
		case SDLK_LEFT:
			//joy_state[0][GN_LEFT]=1;
			return GN_LEFT;
			break;	
		case SDLK_RIGHT:
			//joy_state[0][GN_RIGHT]=1;
			return GN_RIGHT;
			break;	
		case SDLK_ESCAPE:
			//joy_state[0][GN_A]=1;
			return GN_A;
			break;
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			//joy_state[0][GN_B]=1;
			return GN_B;
			break;
		default:
			//SDL_PushEvent(&event);
			//handle_event();
			break;
		}
		break;
	case SDL_KEYUP:
		//printf("KEYUPPPPP!!!\n");

		for(i=0;i<GN_MAX_KEY;i++)
			joy_state[0][i]=0;
		last=-1;

		break;
	default:
		SDL_PushEvent(&event);
		handle_event();
		break;
	}
/*
	}
	SDL_PushEvent(&event);
	handle_event();
*/
	for(i=0;i<GN_MAX_KEY;i++)
		if (joy_state[0][i]&& i!=last) return i;
	return 0;
}