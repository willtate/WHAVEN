#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <string.h>
#include <stdlib.h>
#include <dos.h>
#include "build.h"
#include "names.h"

#define TICSPERFRAME 3
#define MOVEFIFOSIZ 256

/***************************************************************************
	KEN'S TAG DEFINITIONS:      (Please define your own tags for your games)

 sector[?].lotag = 0   Normal sector
 sector[?].lotag = 1   If you are on a sector with this tag, then all sectors
								  with same hi tag as this are operated.  Once.
 sector[?].lotag = 2   Same as sector[?].tag = 1 but this is retriggable.
 sector[?].lotag = 3   A really stupid sector that really does nothing now.
 sector[?].lotag = 4   A sector where you are put closer to the floor
								  (such as the slime in DOOM1.DAT)
 sector[?].lotag = 5   A really stupid sector that really does nothing now.
 sector[?].lotag = 6   A normal door - instead of pressing D, you tag the
								  sector with a 6.  The reason I make you edit doors
								  this way is so that can program the doors
								  yourself.
 sector[?].lotag = 7   A door the goes down to open.
 sector[?].lotag = 8   A door that opens horizontally in the middle.
 sector[?].lotag = 9   A sliding door that opens vertically in the middle.
								  -Example of the advantages of not using BSP tree.
 sector[?].lotag = 10  A warping sector with floor and walls that shade.
 sector[?].lotag = 11  A sector with all walls that do X-panning.
 sector[?].lotag = 12  A sector with walls using the dragging function.
 sector[?].lotag = 13  A sector with some swinging doors in it.
 sector[?].lotag = 14  A revolving door sector.
 sector[?].lotag = 15  A subway track.
 sector[?].lotag = 16  A true double-sliding door.
 sector[?].lotag = 17  A true double-sliding door for subways only.

	wall[?].lotag = 0   Normal wall
	wall[?].lotag = 1   Y-panning wall
	wall[?].lotag = 2   Switch - If you flip it, then all sectors with same hi
								  tag as this are operated.
	wall[?].lotag = 3   Marked wall to detemine starting dir. (sector tag 12)
	wall[?].lotag = 4   Mark on the shorter wall closest to the pivot point
								  of a swinging door. (sector tag 13)
	wall[?].lotag = 5   Mark where a subway should stop. (sector tag 15)
	wall[?].lotag = 6   Mark for true double-sliding doors (sector tag 16)
	wall[?].lotag = 7   Water fountain
	wall[?].lotag = 8   Bouncy wall!

 sprite[?].lotag = 0   Normal sprite
 sprite[?].lotag = 1   If you press space bar on an AL, and the AL is tagged
								  with a 1, he will turn evil.
 sprite[?].lotag = 2   When this sprite is operated, a bomb is shot at its
								  position.
 sprite[?].lotag = 3   Rotating sprite.
 sprite[?].lotag = 4   Sprite switch.
 sprite[?].lotag = 5   Basketball hoop score.

KEN'S STATUS DEFINITIONS:  (Please define your own statuses for your games)
 status = 0            Inactive sprite
 status = 1            Active monster sprite
 status = 2            Monster that becomes active only when it sees you
 status = 3            Smoke on the wall for chainguns
 status = 4            Splashing sprites (When you shoot slime)
 status = 5            Explosion!
 status = 6            Travelling bullet
 status = 7            Bomb sprial-out explosion
 status = 8            Player!
 status = MAXSTATUS    Non-existent sprite (this will be true for your
														  code also)
**************************************************************************/

typedef struct
{
	long x, y, z;
} point3d;

void (__interrupt __far *oldtimerhandler)();
void __interrupt __far timerhandler(void);

#define KEYFIFOSIZ 64
void (__interrupt __far *oldkeyhandler)();
void __interrupt __far keyhandler(void);
volatile char keystatus[256], keyfifo[KEYFIFOSIZ], keyfifoplc, keyfifoend;
volatile char readch, oldreadch, extended, keytemp;

static long vel, svel, angvel;
static long vel2, svel2, angvel2;

extern volatile long recsnddone, recsndoffs;
static long recording = -2;

#define NUMOPTIONS 8
#define NUMKEYS 19
static long chainxres[4] = {256,320,360,400};
static long chainyres[11] = {200,240,256,270,300,350,360,400,480,512,540};
static long vesares[7][2] = {320,200,640,400,640,480,800,600,1024,768,
									  1280,1024,1600,1200};
static char option[NUMOPTIONS] = {0,0,0,0,0,0,1,0};
static char keys[NUMKEYS] =
{
	0xc8,0xd0,0xcb,0xcd,0x2a,0x9d,0x1d,0x39,
	0x1e,0x2c,0xd1,0xc9,0x33,0x34,
	0x9c,0x1c,0xd,0xc,0xf,
};

static long digihz[7] = {6000,8000,11025,16000,22050,32000,44100};

static char frame2draw[MAXPLAYERS];
static long frameskipcnt[MAXPLAYERS];

static char gundmost[320];

#define LAVASIZ 128
#define LAVALOGSIZ 7
#define LAVAMAXDROPS 32
static char lavabakpic[(LAVASIZ+2)*(LAVASIZ+2)], lavainc[LAVASIZ];
static long lavanumdrops, lavanumframes;
static long lavadropx[LAVAMAXDROPS], lavadropy[LAVAMAXDROPS];
static long lavadropsiz[LAVAMAXDROPS], lavadropsizlookup[LAVAMAXDROPS];
static long lavaradx[32][128], lavarady[32][128], lavaradcnt[32];

	//Shared player variables
static long posx[MAXPLAYERS], posy[MAXPLAYERS], posz[MAXPLAYERS];
static long horiz[MAXPLAYERS], zoom[MAXPLAYERS], hvel[MAXPLAYERS];
static short ang[MAXPLAYERS], cursectnum[MAXPLAYERS], ocursectnum[MAXPLAYERS];
static short playersprite[MAXPLAYERS], deaths[MAXPLAYERS];
static long lastchaingun[MAXPLAYERS];
static long health[MAXPLAYERS], score[MAXPLAYERS], saywatchit[MAXPLAYERS];
static short numbombs[MAXPLAYERS], oflags[MAXPLAYERS];
static char dimensionmode[MAXPLAYERS];
static char revolvedoorstat[MAXPLAYERS];
static short revolvedoorang[MAXPLAYERS], revolvedoorrotang[MAXPLAYERS];
static long revolvedoorx[MAXPLAYERS], revolvedoory[MAXPLAYERS];

	//ENGINE CONTROLLED MULTIPLAYER VARIABLES:
extern short numplayers, myconnectindex;
extern short connecthead, connectpoint2[MAXPLAYERS];   //Player linked list variables (indeces, not connection numbers)

	//Local multiplayer variables
static long locselectedgun;
static signed char locvel, olocvel;
static signed char locsvel, olocsvel;
static signed char locangvel, olocangvel;
static short locbits, olocbits;

	//Local multiplayer variables for second player
static long locselectedgun2;
static signed char locvel2, olocvel2;
static signed char locsvel2, olocsvel2;
static signed char locangvel2, olocangvel2;
static short locbits2, olocbits2;

  //Multiplayer syncing variables
static signed char fsyncvel[MAXPLAYERS], osyncvel[MAXPLAYERS], syncvel[MAXPLAYERS];
static signed char fsyncsvel[MAXPLAYERS], osyncsvel[MAXPLAYERS], syncsvel[MAXPLAYERS];
static signed char fsyncangvel[MAXPLAYERS], osyncangvel[MAXPLAYERS], syncangvel[MAXPLAYERS];
static short fsyncbits[MAXPLAYERS], osyncbits[MAXPLAYERS], syncbits[MAXPLAYERS];

static char frameinterpolate = 1, detailmode = 0, ready2send = 0;
static long ototalclock = 0, gotlastpacketclock = 0, smoothratio;
static long oposx[MAXPLAYERS], oposy[MAXPLAYERS], oposz[MAXPLAYERS];
static long ohoriz[MAXPLAYERS], ozoom[MAXPLAYERS];
static short oang[MAXPLAYERS];

static point3d osprite[MAXSPRITESONSCREEN];

static long movefifoplc, movefifoend;
static signed char baksyncvel[MOVEFIFOSIZ][MAXPLAYERS];
static signed char baksyncsvel[MOVEFIFOSIZ][MAXPLAYERS];
static signed char baksyncangvel[MOVEFIFOSIZ][MAXPLAYERS];
static short baksyncbits[MOVEFIFOSIZ][MAXPLAYERS];

	//MULTI.OBJ sync state variables
extern char syncstate;
	//GAME.C sync state variables
static short syncstat = 0;
static long syncvalplc, othersyncvalplc;
static long syncvalend, othersyncvalend;
static long syncvalcnt, othersyncvalcnt;
static short syncval[MOVEFIFOSIZ], othersyncval[MOVEFIFOSIZ];

extern long crctable[256];
#define updatecrc16(dacrc,dadat) dacrc = (((dacrc<<8)&65535)^crctable[((((unsigned short)dacrc)>>8)&65535)^dadat])
static char playerreadyflag[MAXPLAYERS];

	//Game recording variables
static long reccnt, recstat = 1;
static signed char recsyncvel[16384][2];
static signed char recsyncsvel[16384][2];
static signed char recsyncangvel[16384][2];
static short recsyncbits[16384][2];

	//Miscellaneous variables
static char tempbuf[max(576,MAXXDIM)], boardfilename[80];
static short screenpeek = 0, oldmousebstatus = 0, brightness = 0;
static short screensize, screensizeflag = 0;
static short neartagsector, neartagwall, neartagsprite;
static long lockclock, neartagdist, neartaghitdist;
static long masterslavetexttime;
extern long frameplace, pageoffset, ydim16, chainnumpages;
static long globhiz, globloz, globhihit, globlohit;
extern long stereofps, stereowidth, stereopixelwidth;

	//Board animation variables
static short rotatespritelist[16], rotatespritecnt;
static short warpsectorlist[64], warpsectorcnt;
static short xpanningsectorlist[16], xpanningsectorcnt;
static short ypanningwalllist[64], ypanningwallcnt;
static short floorpanninglist[64], floorpanningcnt;
static short dragsectorlist[16], dragxdir[16], dragydir[16], dragsectorcnt;
static long dragx1[16], dragy1[16], dragx2[16], dragy2[16], dragfloorz[16];
static short swingcnt, swingwall[32][5], swingsector[32];
static short swingangopen[32], swingangclosed[32], swingangopendir[32];
static short swingang[32], swinganginc[32];
static long swingx[32][8], swingy[32][8];
static short revolvesector[4], revolveang[4], revolvecnt;
static long revolvex[4][16], revolvey[4][16];
static long revolvepivotx[4], revolvepivoty[4];
static short subwaytracksector[4][128], subwaynumsectors[4], subwaytrackcnt;
static long subwaystop[4][8], subwaystopcnt[4];
static long subwaytrackx1[4], subwaytracky1[4];
static long subwaytrackx2[4], subwaytracky2[4];
static long subwayx[4], subwaygoalstop[4], subwayvel[4], subwaypausetime[4];
static short waterfountainwall[MAXPLAYERS], waterfountaincnt[MAXPLAYERS];
static short slimesoundcnt[MAXPLAYERS];

	//Variables that let you type messages to other player
static char getmessage[162], getmessageleng;
static long getmessagetimeoff;
static char typemessage[162], typemessageleng = 0, typemode = 0;
static char scantoasc[128] =
{
	0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
	'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
	'd','f','g','h','j','k','l',';',39,'`',0,92,'z','x','c','v',
	'b','n','m',',','.','/',0,'*',0,32,0,0,0,0,0,0,
	0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
	'2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
static char scantoascwithshift[128] =
{
	0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,0,
	'Q','W','E','R','T','Y','U','I','O','P','{','}',0,0,'A','S',
	'D','F','G','H','J','K','L',':',34,'~',0,'|','Z','X','C','V',
	'B','N','M','<','>','?',0,'*',0,32,0,0,0,0,0,0,
	0,0,0,0,0,0,0,'7','8','9','-','4','5','6','+','1',
	'2','3','0','.',0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

	//These variables are for animating x, y, or z-coordinates of sectors,
	//walls, or sprites (They are NOT to be used for changing the [].picnum's)
	//See the setanimation(), and getanimategoal() functions for more details.
#define MAXANIMATES 512
static long *animateptr[MAXANIMATES], animategoal[MAXANIMATES];
static long animatevel[MAXANIMATES], animateacc[MAXANIMATES], animatecnt = 0;

	//Here are some nice in-line assembly pragmas that make life easier.
	//(at least for Ken)
#pragma aux setvmode =\
	"int 0x10",\
	parm [eax]\

#pragma aux clearbuf =\
	"rep stosd",\
	parm [edi][ecx][eax]\

#pragma aux clearbufbyte =\
	"shr ecx, 1",\
	"jnc skip1",\
	"stosb",\
	"skip1: shr ecx, 1",\
	"jnc skip2",\
	"stosw",\
	"skip2: test ecx, ecx",\
	"jz skip3",\
	"rep stosd",\
	"skip3:",\
	parm [edi][ecx][eax]\

#pragma aux copybuf =\
	"rep movsd",\
	parm [esi][edi][ecx]\

#pragma aux copybufbyte =\
	"shr ecx, 1",\
	"jnc skip1",\
	"movsb",\
	"skip1: shr ecx, 1",\
	"jnc skip2",\
	"movsw",\
	"skip2: test ecx, ecx",\
	"jz skip3",\
	"rep movsd",\
	"skip3:",\
	parm [esi][edi][ecx]\

#pragma aux copybufreverse =\
	"shr ecx, 1",\
	"jnc skipit1",\
	"mov al, byte ptr [esi]",\
	"dec esi",\
	"mov byte ptr [edi], al",\
	"inc edi",\
	"skipit1: shr ecx, 1",\
	"jnc skipit2",\
	"mov ax, word ptr [esi-1]",\
	"sub esi, 2",\
	"ror ax, 8",\
	"mov word ptr [edi], ax",\
	"add edi, 2",\
	"skipit2: test ecx, ecx",\
	"jz endloop",\
	"begloop: mov eax, dword ptr [esi-3]",\
	"sub esi, 4",\
	"bswap eax",\
	"mov dword ptr [edi], eax",\
	"add edi, 4",\
	"dec ecx",\
	"jnz begloop",\
	"endloop:",\
	parm [esi][edi][ecx]\

#pragma aux klabs =\
	"test eax, eax",\
	"jns skipnegate",\
	"neg eax",\
	"skipnegate:",\
	parm [eax]\

#pragma aux ksgn =\
	"add ebx, ebx",\
	"sbb eax, eax",\
	"cmp eax, ebx",\
	"adc eax, 0",\
	parm [ebx]\

#pragma aux koutp =\
	"out dx, al",\
	parm [edx][eax]\

#pragma aux koutpw =\
	"out dx, ax",\
	parm [edx][eax]\

#pragma aux kinp =\
	"in al, dx",\
	parm [edx]\

#pragma aux mulscale =\
	"imul ebx",\
	"shrd eax, edx, cl",\
	parm [eax][ebx][ecx]\
	modify [edx]\

#pragma aux divscale =\
	"mov edx, eax",\
	"sar edx, 31",\
	"shld edx, eax, cl",\
	"sal eax, cl",\
	"idiv ebx",\
	parm [eax][ebx][ecx]\
	modify [edx]\

#pragma aux scale =\
	"imul ebx",\
	"idiv ecx",\
	parm [eax][ebx][ecx]\
	modify [eax edx]\

	//These parameters are in exact order of sprite structure in BUILD.H
#define spawnsprite(newspriteindex2,x2,y2,z2,cstat2,shade2,pal2,       \
		clipdist2,xrepeat2,yrepeat2,xoffset2,yoffset2,picnum2,ang2,      \
		xvel2,yvel2,zvel2,owner2,sectnum2,statnum2,lotag2,hitag2,extra2) \
{                                                                      \
	spritetype *spr2;                                                   \
	newspriteindex2 = insertsprite(sectnum2,statnum2);                  \
	spr2 = &sprite[newspriteindex2];                                    \
	spr2->x = x2; spr2->y = y2; spr2->z = z2;                           \
	spr2->cstat = cstat2; spr2->shade = shade2;                         \
	spr2->pal = pal2; spr2->clipdist = clipdist2;                       \
	spr2->xrepeat = xrepeat2; spr2->yrepeat = yrepeat2;                 \
	spr2->xoffset = xoffset2; spr2->yoffset = yoffset2;                 \
	spr2->picnum = picnum2; spr2->ang = ang2;                           \
	spr2->xvel = xvel2; spr2->yvel = yvel2; spr2->zvel = zvel2;         \
	spr2->owner = owner2;                                               \
	spr2->lotag = lotag2; spr2->hitag = hitag2; spr2->extra = extra2;   \
	copybuf(&spr2->x,&osprite[newspriteindex2].x,3);                    \
}                                                                      \

main(short int argc,char **argv)
{
	long i, j, k, l, fil, waitplayers, x1, y1, x2, y2;
	short other, tempbufleng;
	char *ptr;

	initgroupfile("stuff.dat");

	if ((argc >= 2) && (stricmp("-net",argv[1]) != 0) && (stricmp("/net",argv[1]) != 0))
	{
		strcpy(&boardfilename,argv[1]);
		if(strchr(boardfilename,'.') == 0)
			strcat(boardfilename,".map");
	}
	else
		strcpy(&boardfilename,"nukeland.map");

	if ((fil = open("setup.dat",O_BINARY|O_RDWR,S_IREAD)) != -1)
	{
		read(fil,&option[0],NUMOPTIONS);
		read(fil,&keys[0],NUMKEYS);
		close(fil);
	}
	if (option[3] != 0) initmouse();

	switch(option[0])
	{
		case 0: initengine(0,chainxres[option[6]&15],chainyres[option[6]>>4]); break;
		case 1: initengine(1,vesares[option[6]&15][0],vesares[option[6]&15][1]); break;
		case 2: initengine(2,320L,200L); break;
		case 3: initengine(3,320L,200L); break;
		case 4: initengine(4,320L,200L); break;
		case 5: initengine(5,320L,200L); break;
		case 6: initengine(6,320L,200L); break;
	}
	initkeys();
	inittimer();
	initmultiplayers(option[4],option[5]);

	pskyoff[0] = 0; pskyoff[1] = 0; pskybits = 1;

	initsb(option[1],option[2],digihz[option[7]>>4],((option[7]&4)>0)+1,((option[7]&2)>0)+1,60,option[7]&1);
	if (strcmp(boardfilename,"klab.map") == 0)
		loadsong("klabsong.kdm");
	else
		loadsong("neatsong.kdm");
	musicon();

	loadpics("tiles000.art");                      //Load artwork
	initlava();

		//Get dmost table for canon
	if (waloff[GUNONBOTTOM] == 0) loadtile(GUNONBOTTOM);
	x1 = tilesizx[GUNONBOTTOM]; y1 = tilesizy[GUNONBOTTOM];
	ptr = (char *)(waloff[GUNONBOTTOM]);
	for(i=0;i<x1;i++)
	{
		y2 = y1-1; while ((ptr[y2] != 255) && (y2 >= 0)) y2--;
		gundmost[i] = y2+1;
		ptr += y1;
	}

		//Try making a title screen & uncommenting this!
	//setgamemode();
	//setview(0L,0L,xdim-1,(ydim-1)>>detailmode);
	//loadtile(TITLESCREEN);
	//i = 0; j = 1621;
	//for(k=0;k<256;k++)
	//{
	//   for(l=0;l<256;l++)
	//   {
	//      ptr = (char *)(waloff[TITLESCREEN]+i);
	//      *ptr = 159;
	//      i = (i+j)&65535; j = (j+4)&65535;
	//   }
	//   overwritesprite(0L,0L,TITLESCREEN,0,0,0);
	//   nextpage();
	//}

		//Here's an example of TRUE ornamented walls
		//The allocatepermanenttile should be called right after loadpics
		//Since it resets the tile cache for each call.
	if (allocatepermanenttile(4095,64,64) >= 0)    //If enough memory
	{
			//My face with an explosion written over it
		copytilepiece(KENPICTURE,0,0,64,64,4095,0,0);
		copytilepiece(EXPLOSION,0,0,64,64,4095,0,0);
	}
	if (allocatepermanenttile(SLIME,128,128) < 0)    //If enough memory
	{
		printf("Not enough memory for slime!\n");
		exit(0);
	}

	for(j=0;j<256;j++)
		tempbuf[j] = ((j+32)&255);  //remap colors for screwy palette sectors
	makepalookup(16,tempbuf,0,0,0,1);

	for(j=0;j<256;j++) tempbuf[j] = j;
	makepalookup(17,tempbuf,24,24,24,1);

	for(j=0;j<256;j++) tempbuf[j] = j; //(j&31)+32;
	makepalookup(18,tempbuf,8,8,48,1);

	prepareboard(boardfilename);                   //Load board

	if (option[4] > 0)
	{
		x1 = (xdim>>1)-(screensize>>1);
		x2 = x1+screensize-1;
		y1 = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
		y2 = y1 + ((screensize*(ydim-32))/xdim)-1;
		permanentwritespritetile(0L,0L,BACKGROUND,0,x1,y1,x2,y2,0);

		sendlogon();
		if (option[4] < 5) waitplayers = 2; else waitplayers = option[4]-3;
		while (numplayers < waitplayers)
		{
			sprintf(tempbuf,"%ld of %ld players in...",numplayers,waitplayers);
			printext256(68L,84L,31,0,tempbuf,0);
			nextpage();

			if (getpacket(&other,tempbuf) > 0)
				if (tempbuf[0] == 255)
					keystatus[1] = 1;

			if (keystatus[1] > 0)
			{
				sendlogoff();         //Signing off
				musicoff();
				uninitmultiplayers();
				uninittimer();
				uninitkeys();
				uninitengine();
				uninitsb();
				uninitgroupfile();
				setvmode(0x3);        //Set back to text mode
				exit(0);
			}
		}
		screenpeek = myconnectindex;
	}

	reccnt = 0;
	for(i=connecthead;i>=0;i=connectpoint2[i]) initplayersprite((short)i);

	//waitforeverybody();
	resettiming(); ototalclock = 0; gotlastpacketclock = 0;

	ready2send = 1;
	while (keystatus[1] == 0)       //Main loop starts here
	{
			// backslash (useful only with KDM)
		if (keystatus[0x2b] > 0) { keystatus[0x2b] = 0; preparesndbuf(); }

		while (movefifoplc != movefifoend) domovethings();
		drawscreen(screenpeek,(totalclock-gotlastpacketclock)*(65536/TICSPERFRAME));
	}
	ready2send = 0;

	sendlogoff();         //Signing off
	musicoff();
	uninitmultiplayers();
	uninittimer();
	uninitkeys();
	uninitengine();
	uninitsb();
	uninitgroupfile();
	setvmode(0x3);        //Set back to text mode
	showengineinfo();     //Show speed statistics

	return(0);
}

operatesector(short dasector)
{     //Door code
	long i, j, k, s, nexti, good, cnt, datag;
	long dax, day, daz, dax2, day2, daz2, centx, centy;
	short startwall, endwall, wallfind[2];

	datag = sector[dasector].lotag;

	startwall = sector[dasector].wallptr;
	endwall = startwall + sector[dasector].wallnum - 1;
	centx = 0L, centy = 0L;
	for(i=startwall;i<=endwall;i++)
	{
		centx += wall[i].x;
		centy += wall[i].y;
	}
	centx /= (endwall-startwall+1);
	centy /= (endwall-startwall+1);

		//Simple door that moves up  (tag 8 is a combination of tags 6 & 7)
	if ((datag == 6) || (datag == 8))    //If the sector in front is a door
	{
		i = getanimationgoal((long)&sector[dasector].ceilingz);
		if (i >= 0)      //If door already moving, reverse its direction
		{
			if (datag == 8)
				daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
			else
				daz = sector[dasector].floorz;

			if (animategoal[i] == daz)
				animategoal[i] = sector[nextsectorneighborz(dasector,sector[dasector].floorz,-1,-1)].ceilingz;
			else
				animategoal[i] = daz;
			animatevel[i] = 0;
		}
		else      //else insert the door's ceiling on the animation list
		{
			if (sector[dasector].ceilingz == sector[dasector].floorz)
				daz = sector[nextsectorneighborz(dasector,sector[dasector].floorz,-1,-1)].ceilingz;
			else
			{
				if (datag == 8)
					daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
				else
					daz = sector[dasector].floorz;
			}
			if ((j = setanimation(&sector[dasector].ceilingz,daz,6L,6L)) >= 0)
				wsayfollow("updowndr.wav",4096L+(krand()&255)-128,256L,&centx,&centy,0);
		}
	}
		//Simple door that moves down
	if ((datag == 7) || (datag == 8)) //If the sector in front's elevator
	{
		i = getanimationgoal((long)&sector[dasector].floorz);
		if (i >= 0)      //If elevator already moving, reverse its direction
		{
			if (datag == 8)
				daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
			else
				daz = sector[dasector].ceilingz;

			if (animategoal[i] == daz)
				animategoal[i] = sector[nextsectorneighborz(dasector,sector[dasector].ceilingz,1,1)].floorz;
			else
				animategoal[i] = daz;
			animatevel[i] = 0;
		}
		else      //else insert the elevator's ceiling on the animation list
		{
			if (sector[dasector].floorz == sector[dasector].ceilingz)
				daz = sector[nextsectorneighborz(dasector,sector[dasector].ceilingz,1,1)].floorz;
			else
			{
				if (datag == 8)
					daz = ((sector[dasector].ceilingz+sector[dasector].floorz)>>1);
				else
					daz = sector[dasector].ceilingz;
			}
			if ((j = setanimation(&sector[dasector].floorz,daz,6L,6L)) >= 0)
				wsayfollow("updowndr.wav",4096L+(krand()&255)-128,256L,&centx,&centy,0);
		}
	}

	if (datag == 9)   //Smooshy-wall sideways double-door
	{
		//find any points with either same x or same y coordinate
		//  as center (centx, centy) - should be 2 points found.
		wallfind[0] = -1;
		wallfind[1] = -1;
		for(i=startwall;i<=endwall;i++)
			if ((wall[i].x == centx) || (wall[i].y == centy))
			{
				if (wallfind[0] == -1)
					wallfind[0] = i;
				else
					wallfind[1] = i;
			}

		for(j=0;j<2;j++)
		{
			if ((wall[wallfind[j]].x == centx) && (wall[wallfind[j]].y == centy))
			{
				//find what direction door should open by averaging the
				//  2 neighboring points of wallfind[0] & wallfind[1].
				i = wallfind[j]-1; if (i < startwall) i = endwall;
				dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
				day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					dax2 = wall[wall[wall[wallfind[j]].point2].point2].x;
					dax2 -= wall[wall[wallfind[j]].point2].x;
					setanimation(&wall[wallfind[j]].x,wall[wallfind[j]].x+dax2,4L,0L);
					setanimation(&wall[i].x,wall[i].x+dax2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].x,wall[wall[wallfind[j]].point2].x+dax2,4L,0L);
				}
				else if (day2 != 0)
				{
					day2 = wall[wall[wall[wallfind[j]].point2].point2].y;
					day2 -= wall[wall[wallfind[j]].point2].y;
					setanimation(&wall[wallfind[j]].y,wall[wallfind[j]].y+day2,4L,0L);
					setanimation(&wall[i].y,wall[i].y+day2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].y,wall[wall[wallfind[j]].point2].y+day2,4L,0L);
				}
			}
			else
			{
				i = wallfind[j]-1; if (i < startwall) i = endwall;
				dax2 = ((wall[i].x+wall[wall[wallfind[j]].point2].x)>>1)-wall[wallfind[j]].x;
				day2 = ((wall[i].y+wall[wall[wallfind[j]].point2].y)>>1)-wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					setanimation(&wall[wallfind[j]].x,centx,4L,0L);
					setanimation(&wall[i].x,centx+dax2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].x,centx+dax2,4L,0L);
				}
				else if (day2 != 0)
				{
					setanimation(&wall[wallfind[j]].y,centy,4L,0L);
					setanimation(&wall[i].y,centy+day2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].y,centy+day2,4L,0L);
				}
			}
		}
		wsayfollow("updowndr.wav",4096L-256L,256L,&centx,&centy,0);
		wsayfollow("updowndr.wav",4096L+256L,256L,&centx,&centy,0);
	}

	if (datag == 13)  //Swinging door
	{
		for(i=0;i<swingcnt;i++)
		{
			if (swingsector[i] == dasector)
			{
				if (swinganginc[i] == 0)
				{
					if (swingang[i] == swingangclosed[i])
					{
						swinganginc[i] = swingangopendir[i];
						wsayfollow("opendoor.wav",4096L+(krand()&511)-256,256L,&centx,&centy,0);
					}
					else
						swinganginc[i] = -swingangopendir[i];
				}
				else
					swinganginc[i] = -swinganginc[i];
			}
		}
	}

	if (datag == 16)  //True sideways double-sliding door
	{
		 //get 2 closest line segments to center (dax, day)
		wallfind[0] = -1;
		wallfind[1] = -1;
		for(i=startwall;i<=endwall;i++)
			if (wall[i].lotag == 6)
			{
				if (wallfind[0] == -1)
					wallfind[0] = i;
				else
					wallfind[1] = i;
			}

		for(j=0;j<2;j++)
		{
			if ((((wall[wallfind[j]].x+wall[wall[wallfind[j]].point2].x)>>1) == centx) && (((wall[wallfind[j]].y+wall[wall[wallfind[j]].point2].y)>>1) == centy))
			{     //door was closed
					//find what direction door should open
				i = wallfind[j]-1; if (i < startwall) i = endwall;
				dax2 = wall[i].x-wall[wallfind[j]].x;
				day2 = wall[i].y-wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					dax2 = wall[wall[wall[wall[wallfind[j]].point2].point2].point2].x;
					dax2 -= wall[wall[wall[wallfind[j]].point2].point2].x;
					setanimation(&wall[wallfind[j]].x,wall[wallfind[j]].x+dax2,4L,0L);
					setanimation(&wall[i].x,wall[i].x+dax2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].x,wall[wall[wallfind[j]].point2].x+dax2,4L,0L);
					setanimation(&wall[wall[wall[wallfind[j]].point2].point2].x,wall[wall[wall[wallfind[j]].point2].point2].x+dax2,4L,0L);
				}
				else if (day2 != 0)
				{
					day2 = wall[wall[wall[wall[wallfind[j]].point2].point2].point2].y;
					day2 -= wall[wall[wall[wallfind[j]].point2].point2].y;
					setanimation(&wall[wallfind[j]].y,wall[wallfind[j]].y+day2,4L,0L);
					setanimation(&wall[i].y,wall[i].y+day2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].y,wall[wall[wallfind[j]].point2].y+day2,4L,0L);
					setanimation(&wall[wall[wall[wallfind[j]].point2].point2].y,wall[wall[wall[wallfind[j]].point2].point2].y+day2,4L,0L);
				}
			}
			else
			{    //door was not closed
				i = wallfind[j]-1; if (i < startwall) i = endwall;
				dax2 = wall[i].x-wall[wallfind[j]].x;
				day2 = wall[i].y-wall[wallfind[j]].y;
				if (dax2 != 0)
				{
					setanimation(&wall[wallfind[j]].x,centx,4L,0L);
					setanimation(&wall[i].x,centx+dax2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].x,centx,4L,0L);
					setanimation(&wall[wall[wall[wallfind[j]].point2].point2].x,centx+dax2,4L,0L);
				}
				else if (day2 != 0)
				{
					setanimation(&wall[wallfind[j]].y,centy,4L,0L);
					setanimation(&wall[i].y,centy+day2,4L,0L);
					setanimation(&wall[wall[wallfind[j]].point2].y,centy,4L,0L);
					setanimation(&wall[wall[wall[wallfind[j]].point2].point2].y,centy+day2,4L,0L);
				}
			}
		}
		wsayfollow("updowndr.wav",4096L-64L,256L,&centx,&centy,0);
		wsayfollow("updowndr.wav",4096L+64L,256L,&centx,&centy,0);
	}
}

operatesprite(short dasprite)
{
	long datag;

	datag = sprite[dasprite].lotag;

	if (datag == 2)    //A sprite that shoots a bomb
	{
		shootgun(dasprite,
					sprite[dasprite].x,sprite[dasprite].y,sprite[dasprite].z,
					sprite[dasprite].ang,100L,sprite[dasprite].sectnum,2);
	}
}

changehealth(short snum, short deltahealth)
{
	long dax, day;
	short good, k, startwall, endwall, s;

	if (health[snum] > 0)
	{
		health[snum] += deltahealth;
		if (health[snum] > 999) health[snum] = 999;

		if (health[snum] <= 0)
		{
			health[snum] = -1;
			wsayfollow("death.wav",4096L+(krand()&127)-64,256L,&posx[snum],&posy[snum],1);
			sprite[playersprite[snum]].picnum = SKELETON;
		}

		if ((snum == screenpeek) && (screensize <= xdim))
		{
			if (health[snum] > 0)
				sprintf(&tempbuf,"Health: %3d",health[snum]);
			else
				sprintf(&tempbuf,"YOU STINK!!");

			printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-24,tempbuf,ALPHABET,80);
		}
	}
	return(health[snum] <= 0);      //You were just injured
}

changescore(short snum, short deltascore)
{
	score[snum] += deltascore;

	if ((snum == screenpeek) && (screensize <= xdim))
	{
		sprintf(&tempbuf,"Score:%ld",score[snum]);
		printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-8,tempbuf,ALPHABET,80);
	}
}

prepareboard(char *daboardfilename)
{
	short startwall, endwall, dasector;
	long i, j, k, s, dax, day, daz, dax2, day2;

	getmessageleng = 0;
	typemessageleng = 0;

	randomseed = 17L;

		//Clear (do)animation's list
	animatecnt = 0;
	typemode = 0;
	locselectedgun = 0;
	locselectedgun2 = 0;

	if (loadboard(daboardfilename,&posx[0],&posy[0],&posz[0],&ang[0],&cursectnum[0]) == -1)
	{
		musicoff();
		uninitmultiplayers();
		uninittimer();
		uninitkeys();
		uninitengine();
		uninitsb();
		uninitgroupfile();
		setvmode(0x3);        //Set back to text mode
		printf("Board not found\n");
		exit(0);
	}
	precache();

	for(i=0;i<MAXPLAYERS;i++)
	{
		posx[i] = posx[0];
		posy[i] = posy[0];
		posz[i] = posz[0];
		ang[i] = ang[0];
		cursectnum[i] = cursectnum[0];
		ocursectnum[i] = cursectnum[0];
		horiz[i] = 100;
		lastchaingun[i] = 0;
		health[i] = 100;
		score[i] = 0L;
		dimensionmode[i] = 3;
		numbombs[i] = -1;
		zoom[i] = 768L;
		deaths[i] = 0L;
		playersprite[i] = -1;
		saywatchit[i] = -1;
		screensize = xdim;

		oposx[i] = posx[0];
		oposy[i] = posy[0];
		oposz[i] = posz[0];
		ohoriz[i] = horiz[0];
		ozoom[i] = zoom[0];
		oang[i] = ang[0];
	}

	movefifoplc = 0; movefifoend = 0;
	syncvalplc = 0L; othersyncvalplc = 0L;
	syncvalend = 0L; othersyncvalend = 0L;
	syncvalcnt = 0L; othersyncvalcnt = 0L;

	setup3dscreen();

	olocvel = 0; olocvel2 = 0;
	olocsvel = 0; olocsvel2 = 0;
	olocangvel = 0; olocangvel2 = 0;
	olocbits = 0; olocbits2 = 0;
	for(i=0;i<MAXPLAYERS;i++)
	{
		fsyncvel[i] = syncvel[i] = osyncvel[i] = 0;
		fsyncsvel[i] = syncsvel[i] = osyncsvel[i] = 0;
		fsyncangvel[i] = syncangvel[i] = osyncangvel[i] = 0;
		fsyncbits[i] = syncbits[i] = osyncbits[i] = 0;
	}

		//Scan sector tags

	for(i=0;i<MAXPLAYERS;i++)
	{
		waterfountainwall[i] = -1;
		waterfountaincnt[i] = 0;
	}
	slimesoundcnt[i] = 0;
	warpsectorcnt = 0;      //Make a list of warping sectors
	xpanningsectorcnt = 0;  //Make a list of wall x-panning sectors
	floorpanningcnt = 0;    //Make a list of slime sectors
	dragsectorcnt = 0;      //Make a list of moving platforms
	swingcnt = 0;           //Make a list of swinging doors
	revolvecnt = 0;         //Make a list of revolving doors
	subwaytrackcnt = 0;     //Make a list of subways

	for(i=0;i<numsectors;i++)
	{
		switch(sector[i].lotag)
		{
			case 4:
				floorpanninglist[floorpanningcnt++] = i;
				break;
			case 10:
				warpsectorlist[warpsectorcnt++] = i;
				break;
			case 11:
				xpanningsectorlist[xpanningsectorcnt++] = i;
				break;
			case 12:
				dasector = i;
				dax = 0x7fffffff;
				day = 0x7fffffff;
				dax2 = 0x80000000;
				day2 = 0x80000000;
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dax) dax = wall[j].x;
					if (wall[j].y < day) day = wall[j].y;
					if (wall[j].x > dax2) dax2 = wall[j].x;
					if (wall[j].y > day2) day2 = wall[j].y;
					if (wall[j].lotag == 3) k = j;
				}
				if (wall[k].x == dax) dragxdir[dragsectorcnt] = -16;
				if (wall[k].y == day) dragydir[dragsectorcnt] = -16;
				if (wall[k].x == dax2) dragxdir[dragsectorcnt] = 16;
				if (wall[k].y == day2) dragydir[dragsectorcnt] = 16;

				dasector = wall[startwall].nextsector;
				dragx1[dragsectorcnt] = 0x7fffffff;
				dragy1[dragsectorcnt] = 0x7fffffff;
				dragx2[dragsectorcnt] = 0x80000000;
				dragy2[dragsectorcnt] = 0x80000000;
				startwall = sector[dasector].wallptr;
				endwall = startwall+sector[dasector].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dragx1[dragsectorcnt]) dragx1[dragsectorcnt] = wall[j].x;
					if (wall[j].y < dragy1[dragsectorcnt]) dragy1[dragsectorcnt] = wall[j].y;
					if (wall[j].x > dragx2[dragsectorcnt]) dragx2[dragsectorcnt] = wall[j].x;
					if (wall[j].y > dragy2[dragsectorcnt]) dragy2[dragsectorcnt] = wall[j].y;
				}

				dragx1[dragsectorcnt] += (wall[sector[i].wallptr].x-dax);
				dragy1[dragsectorcnt] += (wall[sector[i].wallptr].y-day);
				dragx2[dragsectorcnt] -= (dax2-wall[sector[i].wallptr].x);
				dragy2[dragsectorcnt] -= (day2-wall[sector[i].wallptr].y);

				dragfloorz[dragsectorcnt] = sector[i].floorz;

				dragsectorlist[dragsectorcnt++] = i;
				break;
			case 13:
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].lotag == 4)
					{
						k = wall[wall[wall[wall[j].point2].point2].point2].point2;
						if ((wall[j].x == wall[k].x) && (wall[j].y == wall[k].y))
						{     //Door opens counterclockwise
							swingwall[swingcnt][0] = j;
							swingwall[swingcnt][1] = wall[j].point2;
							swingwall[swingcnt][2] = wall[wall[j].point2].point2;
							swingwall[swingcnt][3] = wall[wall[wall[j].point2].point2].point2;
							swingangopen[swingcnt] = 1536;
							swingangclosed[swingcnt] = 0;
							swingangopendir[swingcnt] = -1;
						}
						else
						{     //Door opens clockwise
							swingwall[swingcnt][0] = wall[j].point2;
							swingwall[swingcnt][1] = j;
							swingwall[swingcnt][2] = lastwall(j);
							swingwall[swingcnt][3] = lastwall(swingwall[swingcnt][2]);
							swingwall[swingcnt][4] = lastwall(swingwall[swingcnt][3]);
							swingangopen[swingcnt] = 512;
							swingangclosed[swingcnt] = 0;
							swingangopendir[swingcnt] = 1;
						}
						for(k=0;k<4;k++)
						{
							swingx[swingcnt][k] = wall[swingwall[swingcnt][k]].x;
							swingy[swingcnt][k] = wall[swingwall[swingcnt][k]].y;
						}

						swingsector[swingcnt] = i;
						swingang[swingcnt] = swingangclosed[swingcnt];
						swinganginc[swingcnt] = 0;
						swingcnt++;
					}
				}
				break;
			case 14:
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				dax = 0L;
				day = 0L;
				for(j=startwall;j<=endwall;j++)
				{
					dax += wall[j].x;
					day += wall[j].y;
				}
				revolvepivotx[revolvecnt] = dax / (endwall-startwall+1);
				revolvepivoty[revolvecnt] = day / (endwall-startwall+1);

				k = 0;
				for(j=startwall;j<=endwall;j++)
				{
					revolvex[revolvecnt][k] = wall[j].x;
					revolvey[revolvecnt][k] = wall[j].y;
					k++;
				}
				revolvesector[revolvecnt] = i;
				revolveang[revolvecnt] = 0;

				revolvecnt++;
				break;
			case 15:
				subwaytracksector[subwaytrackcnt][0] = i;

				subwaystopcnt[subwaytrackcnt] = 0;
				dax = 0x7fffffff;
				day = 0x7fffffff;
				dax2 = 0x80000000;
				day2 = 0x80000000;
				startwall = sector[i].wallptr;
				endwall = startwall+sector[i].wallnum-1;
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].x < dax) dax = wall[j].x;
					if (wall[j].y < day) day = wall[j].y;
					if (wall[j].x > dax2) dax2 = wall[j].x;
					if (wall[j].y > day2) day2 = wall[j].y;
				}
				for(j=startwall;j<=endwall;j++)
				{
					if (wall[j].lotag == 5)
					{
						if ((wall[j].x > dax) && (wall[j].y > day) && (wall[j].x < dax2) && (wall[j].y < day2))
						{
							subwayx[subwaytrackcnt] = wall[j].x;
						}
						else
						{
							subwaystop[subwaytrackcnt][subwaystopcnt[subwaytrackcnt]] = wall[j].x;
							subwaystopcnt[subwaytrackcnt]++;
						}
					}
				}

				for(j=1;j<subwaystopcnt[subwaytrackcnt];j++)
					for(k=0;k<j;k++)
						if (subwaystop[subwaytrackcnt][j] < subwaystop[subwaytrackcnt][k])
						{
							s = subwaystop[subwaytrackcnt][j];
							subwaystop[subwaytrackcnt][j] = subwaystop[subwaytrackcnt][k];
							subwaystop[subwaytrackcnt][k] = s;
						}

				subwaygoalstop[subwaytrackcnt] = 0;
				for(j=0;j<subwaystopcnt[subwaytrackcnt];j++)
					if (klabs(subwaystop[subwaytrackcnt][j]-subwayx[subwaytrackcnt]) < klabs(subwaystop[subwaytrackcnt][subwaygoalstop[subwaytrackcnt]]-subwayx[subwaytrackcnt]))
						subwaygoalstop[subwaytrackcnt] = j;

				subwaytrackx1[subwaytrackcnt] = dax;
				subwaytracky1[subwaytrackcnt] = day;
				subwaytrackx2[subwaytrackcnt] = dax2;
				subwaytracky2[subwaytrackcnt] = day2;

				subwaynumsectors[subwaytrackcnt] = 1;
				for(j=0;j<numsectors;j++)
					if (j != i)
					{
						startwall = sector[j].wallptr;
						if (wall[startwall].x > subwaytrackx1[subwaytrackcnt])
							if (wall[startwall].y > subwaytracky1[subwaytrackcnt])
								if (wall[startwall].x < subwaytrackx2[subwaytrackcnt])
									if (wall[startwall].y < subwaytracky2[subwaytrackcnt])
									{
										if (sector[j].lotag == 16)
											sector[j].lotag = 17;   //Make special subway door

										if (sector[j].floorz != sector[i].floorz)
										{
											sector[j].ceilingstat |= 64;
											sector[j].floorstat |= 64;
										}
										subwaytracksector[subwaytrackcnt][subwaynumsectors[subwaytrackcnt]] = j;
										subwaynumsectors[subwaytrackcnt]++;
									}
					}

				subwayvel[subwaytrackcnt] = 64;
				subwaypausetime[subwaytrackcnt] = 720;
				subwaytrackcnt++;
				break;
		}
	}

		//Scan wall tags

	ypanningwallcnt = 0;
	for(i=0;i<numwalls;i++)
	{
		if (wall[i].lotag == 1) ypanningwalllist[ypanningwallcnt++] = i;
	}

		//Scan sprite tags&picnum's

	rotatespritecnt = 0;
	for(i=0;i<MAXSPRITES;i++)
	{
		if (sprite[i].lotag == 3) rotatespritelist[rotatespritecnt++] = i;

		if (sprite[i].statnum < MAXSTATUS)    //That is, if sprite exists
			switch(sprite[i].picnum)
			{
				case BROWNMONSTER:              //All cases here put the sprite
					if ((sprite[i].cstat&128) == 0)
					{
						sprite[i].z -= ((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);
						sprite[i].cstat |= 128;
					}
					sprite[i].clipdist = mulscale(sprite[i].xrepeat,tilesizx[sprite[i].picnum],7);
				case DOOMGUY:                   //on waiting for you (list 2)
					if (sprite[i].statnum != 1) changespritestat(i,2);
					sprite[i].lotag = 100;
				case AL:
				case EVILAL:
					sprite[i].cstat |= 0x101;    //Set the hitscan sensitivity bit
			}
	}

		//Map starts out completed
	for(i=0;i<(MAXSECTORS>>3);i++) show2dsector[i] = 0xff;
	for(i=0;i<(MAXWALLS>>3);i++) show2dwall[i] = 0xff;
	for(i=0;i<(MAXSPRITES>>3);i++) show2dsprite[i] = 0xff;

		//Map starts out blank; you map out what you see in 3D mode
	//automapping = 1;

	lockclock = 0;
	ototalclock = 0;
	gotlastpacketclock = 0;
	masterslavetexttime = 0;

	screensize = xdim;
	dax = (xdim>>1)-(screensize>>1);
	dax2 = dax+screensize-1;
	day = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
	day2 = day + ((screensize*(ydim-32))/xdim)-1;
	setview(dax,day>>detailmode,dax2,day2>>detailmode);
}


checktouchsprite(short snum, short sectnum)
{
	long i, nexti;

	if ((sectnum < 0) || (sectnum >= numsectors)) return;

	for(i=headspritesect[sectnum];i>=0;i=nexti)
	{
		nexti = nextspritesect[i];
		if ((klabs(posx[snum]-sprite[i].x)+klabs(posy[snum]-sprite[i].y) < 512) && (klabs((posz[snum]>>8)-((sprite[i].z>>8)-(tilesizy[sprite[i].picnum]>>1))) <= 40))
		{
			switch(sprite[i].picnum)
			{
				case COINSTACK:
					wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
					changescore(snum,1000);
					changehealth(snum,25);
					deletesprite((short)i);
					break;
				case GIFTBOX:
					wsayfollow("getstuff.wav",4096L+(krand()&127)-64,208L,&sprite[i].x,&sprite[i].y,0);
					changescore(snum,1000);
					changehealth(snum,10);
					deletesprite((short)i);
					break;
				case COIN:
					wsayfollow("getstuff.wav",4096L+(krand()&127)-64,192L,&sprite[i].x,&sprite[i].y,0);
					changescore(snum,100);
					changehealth(snum,5);
					deletesprite((short)i);
					break;
				case DIAMONDS:
					wsayfollow("getstuff.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
					changescore(snum,500);
					changehealth(snum,15);
					deletesprite((short)i);
					break;
				case CANON:
					if (numbombs[snum] == -1)
					{
						wsayfollow("getstuff.wav",3584L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
						changescore(snum,1000);
						if (snum == myconnectindex) keystatus[4] = 1;
						numbombs[snum] = 32767;
					}
					break;
			}
		}
	}
}

shootgun(short snum, long x, long y, long z,
			short daang, long dahoriz, short dasectnum, char guntype)
{
	short hitsect, hitwall, hitsprite, daang2;
	long i, j, daz2, hitx, hity, hitz;

	switch(guntype)
	{
		case 0:    //Shoot silver sphere bullet
			spawnsprite(j,x,y,z,1+128,0,0,16,64,64,0,0,BULLET,daang,
						  sintable[(daang+512)&2047]>>5,sintable[daang&2047]>>5,
						  (100-dahoriz)<<6,snum+4096,dasectnum,6,0,0,0);
			wsayfollow("shoot2.wav",4096L+(krand()&127)-64,184L,&sprite[j].x,&sprite[j].y,1);
			break;
		case 1:    //Shoot chain gun
			daang2 = ((daang + 2048 + (krand()&31)-16)&2047);
			daz2 = ((100-dahoriz)*2000) + ((krand()-32768)>>1);

			hitscan(x,y,z,dasectnum,                   //Start position
				sintable[(daang2+2560)&2047],           //X vector of 3D ang
				sintable[(daang2+2048)&2047],           //Y vector of 3D ang
				daz2,                                   //Z vector of 3D ang
				&hitsect,&hitwall,&hitsprite,&hitx,&hity,&hitz);

			if (wall[hitwall].picnum == KENPICTURE)
			{
				if (waloff[4095] != 0) wall[hitwall].picnum = 4095;
				wsayfollow("hello.wav",4096L+(krand()&127)-64,256L,&wall[hitwall].x,&wall[hitwall].y,0);
			}
			else if (((hitwall < 0) && (hitsprite < 0) && (hitz >= z) && (sector[hitsect].floorpicnum == SLIME)) || ((hitwall >= 0) && (wall[hitwall].picnum == SLIME)))
			{    //If you shoot slime, make a splash
				wsayfollow("splash.wav",4096L+(krand()&511)-256,256L,&hitx,&hity,0);
				spawnsprite(j,hitx,hity,hitz,2,0,0,32,64,64,0,0,SPLASH,daang,
					0,0,0,snum+4096,hitsect,4,63,0,0); //63=time left for splash
			}
			else
			{
				wsayfollow("shoot.wav",4096L+(krand()&127)-64,256L,&hitx,&hity,0);

				if ((hitsprite >= 0) && (sprite[hitsprite].statnum < MAXSTATUS))
					switch(sprite[hitsprite].picnum)
					{
						case BROWNMONSTER:
							if (sprite[hitsprite].lotag > 0) sprite[hitsprite].lotag -= 10;
							if (sprite[hitsprite].lotag > 0)
							{
								wsayfollow("hurt.wav",4096L+(krand()&511)-256,256L,&hitx,&hity,0);
								if (sprite[hitsprite].lotag <= 25)
									sprite[hitsprite].cstat |= 2;
							}
							else
							{
								wsayfollow("blowup.wav",4096L+(krand()&127)-64,256L,&hitx,&hity,0);
								sprite[hitsprite].z += ((tilesizy[sprite[hitsprite].picnum]*sprite[hitsprite].yrepeat)<<1);
								sprite[hitsprite].picnum = GIFTBOX;
								sprite[hitsprite].cstat &= ~0x83;    //Should not clip, foot-z
								changespritestat(hitsprite,0);
								changescore(snum,200);

								spawnsprite(j,hitx,hity,hitz+(32<<8),0,-4,0,32,64,64,
									0,0,EXPLOSION,daang,0,0,0,snum+4096,
									hitsect,5,31,0,0);
							}
							break;
						case EVILAL:
							wsayfollow("blowup.wav",4096L+(krand()&127)-64,256L,&hitx,&hity,0);
							sprite[hitsprite].picnum = EVILALGRAVE;
							sprite[hitsprite].cstat = 0;
							sprite[hitsprite].xrepeat = (krand()&63)+16;
							sprite[hitsprite].yrepeat = sprite[hitsprite].xrepeat;
							changespritestat(hitsprite,0);

							spawnsprite(j,hitx,hity,hitz+(32<<8),0,-4,0,32,64,64,0,
								0,EXPLOSION,daang,0,0,0,snum+4096,hitsect,5,31,0,0);
								 //31=time left for explosion

							break;
						case DOOMGUY:
							for(j=connecthead;j>=0;j=connectpoint2[j])
								if (playersprite[j] == hitsprite)
								{
									wsayfollow("ouch.wav",4096L+(krand()&127)-64,256L,&hitx,&hity,0);
									changehealth(j,-10);
									break;
								}
							break;
					}

				spawnsprite(j,hitx,hity,hitz+(8<<8),2,-4,0,32,16,16,0,0,
					EXPLOSION,daang,0,0,0,snum+4096,hitsect,3,63,0,0);

					//Sprite starts out with center exactly on wall.
					//This moves it back enough to see it at all angles.
				movesprite((short)j,-(((long)sintable[(512+daang)&2047]*TICSPERFRAME)<<4),-(((long)sintable[daang]*TICSPERFRAME)<<4),0L,4L<<8,4L<<8,1);
			}
			break;
		case 2:    //Shoot bomb
		  spawnsprite(j,x,y,z,128,0,0,12,16,16,0,0,BOMB,daang,
			  sintable[(daang+2560)&2047]>>6,sintable[(daang+2048)&2047]>>6,
			  (80-dahoriz)<<6,snum+4096,dasectnum,6,0,0,0);

			wsayfollow("shoot3.wav",4096L+(krand()&127)-64,256L,&sprite[j].x,&sprite[j].y,1);
			break;
	}
}

analyzesprites(long dax, long day)
{
	long i, j, k;
	point3d *ospr;
	spritetype *tspr;

		//This function is called between drawrooms() and drawmasks()
		//It has a list of possible sprites that may be drawn on this frame

	for(i=0,tspr=&tsprite[0];i<spritesortcnt;i++,tspr++)
	{
		switch(tspr->picnum)
		{
			case DOOMGUY:
					//Get which of the 8 angles of the sprite to draw (0-7)
					//k ranges from 0-7
				k = getangle(tspr->x-dax,tspr->y-day);
				k = (((tspr->ang+3072+128-k)&2047)>>8)&7;
					//This guy has only 5 pictures for 8 angles (3 are x-flipped)
				if (k <= 4)
				{
					tspr->picnum += (k<<2);
					tspr->cstat &= ~4;   //clear x-flipping bit
				}
				else
				{
					tspr->picnum += ((8-k)<<2);
					tspr->cstat |= 4;    //set x-flipping bit
				}
				break;
		}

		k = tspr->statnum;
		if ((k >= 1) && (k <= 8) && (k != 2))  //Interpolate moving sprite
		{
			ospr = &osprite[tspr->owner];
			k = tspr->x-ospr->x; tspr->x = ospr->x;
			if (k != 0) tspr->x += mulscale(k,smoothratio,16);
			k = tspr->y-ospr->y; tspr->y = ospr->y;
			if (k != 0) tspr->y += mulscale(k,smoothratio,16);
			k = tspr->z-ospr->z; tspr->z = ospr->z;
			if (k != 0) tspr->z += mulscale(k,smoothratio,16);
		}
	}
}

tagcode()
{
	long i, nexti, j, k, l, s, dax, day, daz, dax2, day2, cnt, good;
	short startwall, endwall, dasector, p, oldang;

	for(p=connecthead;p>=0;p=connectpoint2[p])
	{
		if (sector[cursectnum[p]].lotag == 1)
		{
			for(i=0;i<numsectors;i++)
				if (sector[i].hitag == sector[cursectnum[p]].hitag)
					if (sector[i].lotag != 1)
						operatesector(i);
			i = headspritestat[0];
			while (i != -1)
			{
				nexti = nextspritestat[i];
				if (sprite[i].hitag == sector[cursectnum[p]].hitag)
					operatesprite(i);
				i = nexti;
			}

			sector[cursectnum[p]].lotag = 0;
			sector[cursectnum[p]].hitag = 0;
		}
		if ((sector[cursectnum[p]].lotag == 2) && (cursectnum[p] != ocursectnum[p]))
		{
			for(i=0;i<numsectors;i++)
				if (sector[i].hitag == sector[cursectnum[p]].hitag)
					if (sector[i].lotag != 1)
						operatesector(i);
			i = headspritestat[0];
			while (i != -1)
			{
				nexti = nextspritestat[i];
				if (sprite[i].hitag == sector[cursectnum[p]].hitag)
					operatesprite(i);
				i = nexti;
			}
		}
	}

	for(i=0;i<warpsectorcnt;i++)
	{
		dasector = warpsectorlist[i];
		j = ((lockclock&127)>>2);
		if (j >= 16) j = 31-j;
		{
			sector[dasector].ceilingshade = j;
			sector[dasector].floorshade = j;
			startwall = sector[dasector].wallptr;
			endwall = startwall+sector[dasector].wallnum-1;
			for(s=startwall;s<=endwall;s++)
				wall[s].shade = j;
		}
	}

	for(p=connecthead;p>=0;p=connectpoint2[p])
		if (sector[cursectnum[p]].lotag == 10)  //warp sector
		{
			if (cursectnum[p] != ocursectnum[p])
			{
				warpsprite(playersprite[p]);
				posx[p] = sprite[playersprite[p]].x;
				posy[p] = sprite[playersprite[p]].y;
				posz[p] = sprite[playersprite[p]].z;
				ang[p] = sprite[playersprite[p]].ang;
				cursectnum[p] = sprite[playersprite[p]].sectnum;

				sprite[playersprite[p]].z += (32<<8);

				//warp(&posx[p],&posy[p],&posz[p],&ang[p],&cursectnum[p]);
					//Update sprite representation of player
				//setsprite(playersprite[p],posx[p],posy[p],posz[p]+(32<<8));
				//sprite[playersprite[p]].ang = ang[p];
			}
		}

	for(i=0;i<xpanningsectorcnt;i++)   //animate wall x-panning sectors
	{
		dasector = xpanningsectorlist[i];

		startwall = sector[dasector].wallptr;
		endwall = startwall+sector[dasector].wallnum-1;
		for(s=startwall;s<=endwall;s++)
			wall[s].xpanning = ((lockclock>>2)&255);
	}

	for(i=0;i<ypanningwallcnt;i++)
		wall[ypanningwalllist[i]].ypanning = ~(lockclock&255);

	for(i=0;i<rotatespritecnt;i++)
	{
		sprite[rotatespritelist[i]].ang += (TICSPERFRAME<<2);
		sprite[rotatespritelist[i]].ang &= 2047;
	}

	for(i=0;i<floorpanningcnt;i++)   //animate floor of slime sectors
	{
		sector[floorpanninglist[i]].floorxpanning = ((lockclock>>2)&255);
		sector[floorpanninglist[i]].floorypanning = ((lockclock>>2)&255);
	}

	for(i=0;i<dragsectorcnt;i++)
	{
		dasector = dragsectorlist[i];

		startwall = sector[dasector].wallptr;
		endwall = startwall+sector[dasector].wallnum-1;

		if (wall[startwall].x+dragxdir[i] < dragx1[i]) dragxdir[i] = 16;
		if (wall[startwall].y+dragydir[i] < dragy1[i]) dragydir[i] = 16;
		if (wall[startwall].x+dragxdir[i] > dragx2[i]) dragxdir[i] = -16;
		if (wall[startwall].y+dragydir[i] > dragy2[i]) dragydir[i] = -16;

		for(j=startwall;j<=endwall;j++)
			dragpoint(j,wall[j].x+dragxdir[i],wall[j].y+dragydir[i]);
		j = sector[dasector].floorz;
		sector[dasector].floorz = dragfloorz[i]+(sintable[(lockclock<<4)&2047]>>3);

		for(p=connecthead;p>=0;p=connectpoint2[p])
			if (cursectnum[p] == dasector)
			{
				posx[p] += dragxdir[i];
				posy[p] += dragydir[i];
				posz[p] += (sector[dasector].floorz-j);

					//Update sprite representation of player
				setsprite(playersprite[p],posx[p],posy[p],posz[p]+(32<<8));
				sprite[playersprite[p]].ang = ang[p];
				frameinterpolate = 0;
			}
	}

	for(i=0;i<swingcnt;i++)
	{
		if (swinganginc[i] != 0)
		{
			oldang = swingang[i];
			for(j=0;j<(TICSPERFRAME<<2);j++)
			{
				swingang[i] = ((swingang[i]+2048+swinganginc[i])&2047);
				if (swingang[i] == swingangclosed[i])
				{
					wsayfollow("closdoor.wav",4096L+(krand()&511)-256,256L,&swingx[i][0],&swingy[i][0],0);
					swinganginc[i] = 0;
				}
				if (swingang[i] == swingangopen[i]) swinganginc[i] = 0;
			}
			for(k=1;k<=3;k++)
				rotatepoint(swingx[i][0],swingy[i][0],swingx[i][k],swingy[i][k],swingang[i],&wall[swingwall[i][k]].x,&wall[swingwall[i][k]].y);

			if (swinganginc[i] != 0)
			{
				for(p=connecthead;p>=0;p=connectpoint2[p])
					if ((cursectnum[p] == swingsector[i]) || (testneighborsectors(cursectnum[p],swingsector[i]) == 1))
					{
						cnt = 256;
						do
						{
							good = 1;

								//swingangopendir is -1 if forwards, 1 is backwards
							l = (swingangopendir[i] > 0);
							for(k=l+3;k>=l;k--)
								if (clipinsidebox(posx[p],posy[p],swingwall[i][k],128L) != 0)
								{
									good = 0;
									break;
								}
							if (good == 0)
							{
								if (cnt == 256)
								{
									swinganginc[i] = -swinganginc[i];
									swingang[i] = oldang;
								}
								else
								{
									swingang[i] = ((swingang[i]+2048-swinganginc[i])&2047);
								}
								for(k=1;k<=3;k++)
									rotatepoint(swingx[i][0],swingy[i][0],swingx[i][k],swingy[i][k],swingang[i],&wall[swingwall[i][k]].x,&wall[swingwall[i][k]].y);
								if (swingang[i] == swingangclosed[i])
								{
									wsayfollow("closdoor.wav",4096L+(krand()&511)-256,256L,&swingx[i][0],&swingy[i][0],0);
									swinganginc[i] = 0;
									break;
								}
								if (swingang[i] == swingangopen[i])
								{
									swinganginc[i] = 0;
									break;
								}
								cnt--;
							}
						} while ((good == 0) && (cnt > 0));
					}
			}
		}
	}

	for(i=0;i<revolvecnt;i++)
	{
		startwall = sector[revolvesector[i]].wallptr;
		endwall = startwall + sector[revolvesector[i]].wallnum - 1;

		revolveang[i] = ((revolveang[i]+2048-(TICSPERFRAME<<2))&2047);
		for(k=startwall;k<=endwall;k++)
		{
			rotatepoint(revolvepivotx[i],revolvepivoty[i],revolvex[i][k-startwall],revolvey[i][k-startwall],revolveang[i],&dax,&day);
			dragpoint(k,dax,day);
		}
	}

	for(i=0;i<subwaytrackcnt;i++)
	{
		dasector = subwaytracksector[i][0];
		startwall = sector[dasector].wallptr;
		endwall = startwall+sector[dasector].wallnum-1;
		for(k=startwall;k<=endwall;k++)
		{
			if (wall[k].x > subwaytrackx1[i])
				if (wall[k].y > subwaytracky1[i])
					if (wall[k].x < subwaytrackx2[i])
						if (wall[k].y < subwaytracky2[i])
							wall[k].x += (subwayvel[i]&0xfffffffc);
		}

		for(j=1;j<subwaynumsectors[i];j++)
		{
			dasector = subwaytracksector[i][j];

			startwall = sector[dasector].wallptr;
			endwall = startwall+sector[dasector].wallnum-1;
			for(k=startwall;k<=endwall;k++)
				wall[k].x += (subwayvel[i]&0xfffffffc);

			s = headspritesect[dasector];
			while (s != -1)
			{
				k = nextspritesect[s];
				sprite[s].x += (subwayvel[i]&0xfffffffc);
				s = k;
			}
		}

		for(p=connecthead;p>=0;p=connectpoint2[p])
			if (cursectnum[p] != subwaytracksector[i][0])
				if (sector[cursectnum[p]].floorz != sector[subwaytracksector[i][0]].floorz)
					if (posx[p] > subwaytrackx1[i])
						if (posy[p] > subwaytracky1[i])
							if (posx[p] < subwaytrackx2[i])
								if (posy[p] < subwaytracky2[i])
								{
									posx[p] += (subwayvel[i]&0xfffffffc);

										//Update sprite representation of player
									setsprite(playersprite[p],posx[p],posy[p],posz[p]+(32<<8));
									sprite[playersprite[p]].ang = ang[p];
									frameinterpolate = 0;
								}

		subwayx[i] += (subwayvel[i]&0xfffffffc);

		k = subwaystop[i][subwaygoalstop[i]] - subwayx[i];
		if (k > 0)
		{
			if (k > 2048)
			{
				if (subwayvel[i] < 128) subwayvel[i]++;
			}
			else
				subwayvel[i] = (k>>4)+1;
		}
		else if (k < 0)
		{
			if (k < -2048)
			{
				if (subwayvel[i] > -128) subwayvel[i]--;
			}
			else
				subwayvel[i] = ((k>>4)-1);
		}

		if (((subwayvel[i]>>2) == 0) && (klabs(k) < 2048))
		{
			if (subwaypausetime[i] == 720)
			{
				for(j=1;j<subwaynumsectors[i];j++)   //Open all subway doors
				{
					dasector = subwaytracksector[i][j];
					if (sector[dasector].lotag == 17)
					{
						sector[dasector].lotag = 16;
						operatesector(dasector);
						sector[dasector].lotag = 17;
					}
				}
			}
			if ((subwaypausetime[i] >= 120) && (subwaypausetime[i]-TICSPERFRAME < 120))
			{
				for(j=1;j<subwaynumsectors[i];j++)   //Close all subway doors
				{
					dasector = subwaytracksector[i][j];
					if (sector[dasector].lotag == 17)
					{
						sector[dasector].lotag = 16;
						operatesector(dasector);
						sector[dasector].lotag = 17;
					}
				}
			}

			subwaypausetime[i] -= TICSPERFRAME;
			if (subwaypausetime[i] < 0)
			{
				subwaypausetime[i] = 720;
				if (subwayvel[i] < 0)
				{
					subwaygoalstop[i]--;
					if (subwaygoalstop[i] < 0)
						subwaygoalstop[i] = 1;
				}
				if (subwayvel[i] > 0)
				{
					subwaygoalstop[i]++;
					if (subwaygoalstop[i] >= subwaystopcnt[i])
						subwaygoalstop[i] = subwaystopcnt[i]-2;
				}
			}
		}
	}
}

statuslistcode()
{
	short p, target, hitobject, daang, osectnum, movestat;
	long i, nexti, j, nextj, k, l, dax, day, daz, dist, ox, oy, mindist;

	i = headspritestat[1];     //Go through active monster sprites
	while (i != -1)
	{
		nexti = nextspritestat[i];

			//Choose a target player
		mindist = 0x7fffffff; target = connecthead;
		for(p=connecthead;p>=0;p=connectpoint2[p])
		{
			dist = klabs(sprite[i].x-posx[p])+klabs(sprite[i].y-posy[p]);
			if (dist < mindist) mindist = dist, target = p;
		}

		switch(sprite[i].picnum)
		{
			case BROWNMONSTER:

				k = (krand()&63);  //Only get random number once
										 //Who cares if the monster won't shoot and
										 //change direction at the same time

					//brown monster decides to shoot bullet
				if ((sprite[i].lotag > 25) && (k == 0))
					if (cansee(posx[target],posy[target],posz[target],cursectnum[target],sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
					{
						wsayfollow("zipguns.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);

						spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z,128,0,0,
							16,64,64,0,0,BULLET,
							(getangle(posx[target]-sprite[j].x,
										 posy[target]-sprite[j].y)+(krand()&15)-8)&2047,
							sintable[(sprite[j].ang+512)&2047]>>6,
							sintable[sprite[j].ang&2047]>>6,
							((posz[target]+(8<<8)-sprite[j].z)<<8) /
							ksqrt((posx[target]-sprite[j].x) *
									(posx[target]-sprite[j].x) +
									(posy[target]-sprite[j].y) *
									(posy[target]-sprite[j].y)),
							i,sprite[i].sectnum,6,0,0,0);
					}

					//Move brown monster
				dax = sprite[i].x;   //Back up old x&y if stepping off cliff
				day = sprite[i].y;

				osectnum = sprite[i].sectnum;
				movestat = movesprite((short)i,(((long)sintable[(sprite[i].ang+512)&2047])*TICSPERFRAME)<<3,(((long)sintable[sprite[i].ang])*TICSPERFRAME)<<3,0L,4L<<8,4L<<8,0);
				if (globloz > sprite[i].z+(48<<8))
					{ sprite[i].x = dax; sprite[i].y = day; movestat = 1; }
				else
					sprite[i].z = globloz-((tilesizy[sprite[i].picnum]*sprite[i].yrepeat)<<1);

				if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
					{ warpsprite((short)i); movestat = 0; }

				if ((movestat != 0) || (k == 1))
				{
					daang = (getangle(posx[target]-sprite[i].x,posy[target]-sprite[i].y)&2047);
					sprite[i].ang = ((daang+(krand()&1023)-512)&2047);
				}

				break;

			case EVILAL:
				if (sprite[i].yrepeat >= 38)
				{
					if (sprite[i].yrepeat < 64)
					{
						sprite[i].xrepeat++;
						sprite[i].yrepeat++;
					}
					else
					{
						if ((krand()&63) == 0)  //Al decides to reproduce
						{
							if (cansee(posx[target],posy[target],posz[target],cursectnum[target],sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
							{
								spawnsprite(j,sprite[i].x,sprite[i].y,0,1+2+256,0,0,
									32,38,38,0,0,EVILAL,krand()&2047,0,0,0,i,
									sprite[i].sectnum,1,0,0,0);
							}
						}
						if ((krand()&63) == 0)     //Al decides to shoot bullet
						{
							if (cansee(posx[target],posy[target],posz[target],cursectnum[target],sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
							{
								wsayfollow("zipguns.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);

								spawnsprite(j,sprite[i].x,sprite[i].y,
									sector[sprite[i].sectnum].floorz-(24<<8),
									0,0,0,16,64,64,0,0,BULLET,
									(getangle(posx[target]-sprite[j].x,
										 posy[target]-sprite[j].y)+(krand()&15)-8)&2047,
									sintable[(sprite[j].ang+2560)&2047]>>6,
									sintable[(sprite[j].ang+2048)&2047]>>6,
									((posz[target]+(8<<8)-sprite[j].z)<<8) /
										ksqrt((posx[target]-sprite[j].x) *
												(posx[target]-sprite[j].x) +
												(posy[target]-sprite[j].y) *
												(posy[target]-sprite[j].y)),
									i,sprite[i].sectnum,6,0,0,0);
							}
						}

						sprite[i].z = sector[sprite[i].sectnum].floorz;

						//Move Al
						dax = ((sintable[(sprite[i].ang+512)&2047]*TICSPERFRAME)<<4);
						day = ((sintable[sprite[i].ang]*TICSPERFRAME)<<4);

						osectnum = sprite[i].sectnum;
						movestat = movesprite((short)i,dax,day,0L,4L<<8,4L<<8,0);
						if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
						{
							warpsprite((short)i);
							movestat = 0;
						}

						if (movestat != 0)
						{
							if ((krand()&1) == 0)
								sprite[i].ang = getangle(posx[target]-sprite[i].x,posy[target]-sprite[i].y);
							else
								sprite[i].ang = (krand()&2047);
						}
					}
				}
				break;
		}

		i = nexti;
	}

	i = headspritestat[6];     //Go through travelling bullet sprites
	while (i != -1)
	{
		nexti = nextspritestat[i];

			 //If the sprite is a bullet then...
		if ((sprite[i].picnum == BULLET) || (sprite[i].picnum == BOMB))
		{
			dax = ((((long)sprite[i].xvel)*TICSPERFRAME)<<11);
			day = ((((long)sprite[i].yvel)*TICSPERFRAME)<<11);
			daz = ((((long)sprite[i].zvel)*TICSPERFRAME)>>3);
			if (sprite[i].picnum == BOMB) daz = 0;

			osectnum = sprite[i].sectnum;
			hitobject = movesprite((short)i,dax,day,daz,4L<<8,4L<<8,1);
			if ((sprite[i].sectnum != osectnum) && (sector[sprite[i].sectnum].lotag == 10))
			{
				warpsprite((short)i);
				hitobject = 0;
			}

			if (sprite[i].picnum == BOMB)
			{
				sprite[i].z += sprite[i].zvel;
				sprite[i].zvel += (TICSPERFRAME<<5);
				if (sprite[i].z < globhiz+(tilesizy[BOMB]<<6))
				{
					sprite[i].z = globhiz+(tilesizy[BOMB]<<6);
					sprite[i].zvel = -(sprite[i].zvel>>1);
				}
				if (sprite[i].z > globloz-(tilesizy[BOMB]<<6))
				{
					sprite[i].z = globloz-(tilesizy[BOMB]<<6);
					sprite[i].zvel = -(sprite[i].zvel>>1);
				}
				dax = sprite[i].xvel; day = sprite[i].yvel;
				dist = dax*dax+day*day;
				if (dist < 512)
				{
					bombexplode(i);
					goto bulletisdeletedskip;
				}
				if (dist < 4096)
				{
					sprite[i].xrepeat = ((4096+2048)*16) / (dist+2048);
					sprite[i].yrepeat = sprite[i].xrepeat;
					sprite[i].xoffset = (krand()&15)-8;
					sprite[i].yoffset = (krand()&15)-8;
				}
				if (mulscale(krand(),dist,30) == 0)
				{
					sprite[i].xvel -= ksgn(sprite[i].xvel);
					sprite[i].yvel -= ksgn(sprite[i].yvel);
					sprite[i].zvel -= ksgn(sprite[i].zvel);
				}
			}

				//Check for bouncy objects before killing bullet
			if ((hitobject&0xc000) == 32768)  //Bullet hit a wall
			{
				if (wall[hitobject&4095].lotag == 8)
				{
					dax = sprite[i].xvel; day = sprite[i].yvel;
					if ((sprite[i].picnum != BOMB) || (dax*dax+day*day >= 512))
					{
						k = (hitobject&4095); l = wall[k].point2;
						j = getangle(wall[l].x-wall[k].x,wall[l].y-wall[k].y)+512;

							//k = cos(ang) * sin(ang) * 2
						k = mulscale(sintable[(j+512)&2047],sintable[j&2047],13);
							//l = cos(ang * 2)
						l = sintable[((j<<1)+512)&2047];

						ox = sprite[i].xvel; oy = sprite[i].yvel;
						dax = -ox; day = -oy;
						sprite[i].xvel = mulscale(day,k,14) + mulscale(dax,l,14);
						sprite[i].yvel = mulscale(dax,k,14) - mulscale(day,l,14);

						if (sprite[i].picnum == BOMB)
						{
							sprite[i].xvel -= (sprite[i].xvel>>3);
							sprite[i].yvel -= (sprite[i].yvel>>3);
							sprite[i].zvel -= (sprite[i].zvel>>3);
						}
						ox -= sprite[i].xvel; oy -= sprite[i].yvel;
						dist = ((ox*ox+oy*oy)>>8);
						wsayfollow("bouncy.wav",4096L+(krand()&127)-64,min(dist,256),&sprite[i].x,&sprite[i].y,1);
						hitobject = 0;
						sprite[i].owner = -1;   //Bullet turns evil!
					}
				}
			}
			else if ((hitobject&0xc000) == 49152)  //Bullet hit a sprite
			{
				if (sprite[hitobject&4095].picnum == BOUNCYMAT)
				{
					if ((sprite[hitobject&4095].cstat&48) == 0)
					{
						sprite[i].xvel = -sprite[i].xvel;
						sprite[i].yvel = -sprite[i].yvel;
						sprite[i].zvel = -sprite[i].zvel;
						dist = 255;
					}
					else if ((sprite[hitobject&4095].cstat&48) == 16)
					{
						j = sprite[hitobject&4095].ang;

							//k = cos(ang) * sin(ang) * 2
						k = mulscale(sintable[(j+512)&2047],sintable[j&2047],13);
							//l = cos(ang * 2)
						l = sintable[((j<<1)+512)&2047];

						ox = sprite[i].xvel; oy = sprite[i].yvel;
						dax = -ox; day = -oy;
						sprite[i].xvel = mulscale(day,k,14) + mulscale(dax,l,14);
						sprite[i].yvel = mulscale(dax,k,14) - mulscale(day,l,14);

						ox -= sprite[i].xvel; oy -= sprite[i].yvel;
						dist = ((ox*ox+oy*oy)>>8);
					}
					sprite[i].owner = -1;   //Bullet turns evil!
					wsayfollow("bouncy.wav",4096L+(krand()&127)-64,min(dist,256),&sprite[i].x,&sprite[i].y,1);
					hitobject = 0;
				}
			}

			if (hitobject != 0)
			{
				if (sprite[i].picnum == BOMB)
				{
					if ((hitobject&0xc000) == 49152)
						if (sprite[hitobject&4095].lotag == 5)  //Basketball hoop
						{
							wsayfollow("niceshot.wav",3840L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
							deletesprite((short)i);
							goto bulletisdeletedskip;
						}

					bombexplode(i);
					goto bulletisdeletedskip;
				}

				if ((hitobject&0xc000) == 16384)  //Hits a ceiling / floor
				{
					wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
					deletesprite((short)i);
					goto bulletisdeletedskip;
				}
				else if ((hitobject&0xc000) == 32768)  //Bullet hit a wall
				{
					if (wall[hitobject&4095].picnum == KENPICTURE)
					{
						if (waloff[4095] != 0)
							wall[hitobject&4095].picnum = 4095;
						wsayfollow("hello.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);   //Ken says, "Hello... how are you today!"
					}
					else
						wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);

					deletesprite((short)i);
					goto bulletisdeletedskip;
				}
				else if ((hitobject&0xc000) == 49152)  //Bullet hit a sprite
				{
						//Check if bullet hit a player & find which player it was...
					if (sprite[hitobject&4095].picnum == DOOMGUY)
						for(j=connecthead;j>=0;j=connectpoint2[j])
							if (sprite[i].owner != j+4096)
								if (playersprite[j] == (hitobject&4095))
								{
									wsayfollow("ouch.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
									changehealth(j,-25);
									deletesprite((short)i);
									goto bulletisdeletedskip;
								}

						//Check if bullet hit any monsters...
					j = (hitobject&4095);     //j is the spritenum that the bullet (spritenum i) hit
					if (sprite[i].owner != j)
					{
						switch(sprite[j].picnum)
						{
							case BROWNMONSTER:
								if (sprite[j].lotag > 0) sprite[j].lotag -= 25;
								if (sprite[j].lotag > 0)
								{
									if (sprite[j].lotag <= 25) sprite[j].cstat |= 2;
									wsayfollow("hurt.wav",4096L+(krand()&511)-256,256L,&sprite[i].x,&sprite[i].y,1);

									for(j=connecthead;j>=0;j=connectpoint2[j])
										if (sprite[i].owner == j+4096)
											changescore(j,200);
								}
								else
								{
									wsayfollow("blowup.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
									sprite[j].z += ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<1);
									sprite[j].picnum = GIFTBOX;
									sprite[j].cstat &= ~0x83;    //Should not clip, foot-z

									spawnsprite(k,sprite[j].x,sprite[j].y,sprite[j].z,
										0,-4,0,32,64,64,0,0,EXPLOSION,sprite[j].ang,
										0,0,0,j,sprite[j].sectnum,5,31,0,0);
											//31=Time left for explosion to stay

									changespritestat(j,0);

									for(j=connecthead;j>=0;j=connectpoint2[j])
										if (sprite[i].owner == j+4096)
											changescore(j,200);
								}
								deletesprite((short)i);
								goto bulletisdeletedskip;
							case EVILAL:
								wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
								sprite[j].picnum = EVILALGRAVE;
								sprite[j].cstat = 0;
								sprite[j].xrepeat = (krand()&63)+16;
								sprite[j].yrepeat = sprite[j].xrepeat;
								changespritestat(j,0);

								for(j=connecthead;j>=0;j=connectpoint2[j])
									if (sprite[i].owner == j+4096)
										changescore(j,200);

								deletesprite((short)i);
								goto bulletisdeletedskip;
							case AL:
								wsayfollow("blowup.wav",5144L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
								sprite[j].xrepeat += 2;
								sprite[j].yrepeat += 2;
								if (sprite[j].yrepeat >= 38)
								{
									sprite[j].picnum = EVILAL;
									sprite[j].cstat |= 2;      //Make him transluscent
								}
								changespritestat(j,1);
								deletesprite((short)i);
								goto bulletisdeletedskip;
							default:
								wsayfollow("bullseye.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
								deletesprite((short)i);
								goto bulletisdeletedskip;
						}
					}
				}
			}
		}

bulletisdeletedskip:
		i = nexti;
	}

	for(j=connecthead;j>=0;j=connectpoint2[j])
		if ((saywatchit[j] >= lockclock-TICSPERFRAME) && (saywatchit[j] < lockclock))
			wsayfollow("watchit.wav",4096L+(krand()&127)-64,256L,&posx[j],&posy[j],1);

	i = headspritestat[2];     //Go through monster waiting for you list
	while (i != -1)
	{
		nexti = nextspritestat[i];

			//Use dot product to see if monster's angle is towards a player
		for(p=connecthead;p>=0;p=connectpoint2[p])
			if (sintable[(sprite[i].ang+2560)&2047]*(posx[p]-sprite[i].x) + sintable[(sprite[i].ang+2048)&2047]*(posy[p]-sprite[i].y) >= 0)
				if (cansee(posx[p],posy[p],posz[p],cursectnum[p],sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
				{
					changespritestat(i,1);
					wsayfollow("iseeyou.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);
				}

		i = nexti;
	}

	i = headspritestat[3];     //Go through smoke sprites
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		sprite[i].z -= (TICSPERFRAME<<6);
		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0)
			deletesprite(i);

		i = nexti;
	}

	i = headspritestat[4];     //Go through splash sprites
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		sprite[i].picnum = SPLASH + ((63-sprite[i].lotag)>>4);
		if (sprite[i].lotag < 0)
			deletesprite(i);

		i = nexti;
	}

	i = headspritestat[5];     //Go through explosion sprites
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0)
			deletesprite(i);

		i = nexti;
	}

	i = headspritestat[7];     //Go through bomb spriral-explosion sprites
	while (i >= 0)
	{
		nexti = nextspritestat[i];

		sprite[i].x += ((sprite[i].xvel*TICSPERFRAME)>>4);
		sprite[i].y += ((sprite[i].yvel*TICSPERFRAME)>>4);
		sprite[i].z += ((sprite[i].zvel*TICSPERFRAME)>>4);

		sprite[i].zvel += (TICSPERFRAME<<7);
		if (sprite[i].z < sector[sprite[i].sectnum].ceilingz+(4<<8))
		{
			sprite[i].z = sector[sprite[i].sectnum].ceilingz+(4<<8);
			sprite[i].zvel = -(sprite[i].zvel>>1);
		}
		if (sprite[i].z > sector[sprite[i].sectnum].floorz-(4<<8))
		{
			sprite[i].z = sector[sprite[i].sectnum].floorz-(4<<8);
			sprite[i].zvel = -(sprite[i].zvel>>1);
		}

		sprite[i].xrepeat = (sprite[i].lotag>>2);
		sprite[i].yrepeat = (sprite[i].lotag>>2);

		sprite[i].lotag -= TICSPERFRAME;
		if (sprite[i].lotag < 0)
			deletesprite(i);

		i = nexti;
	}
}

bombexplode(long i)
{
	long j, nextj, k, daang, dax, day, dist;

	spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z,0,-4,0,
		32,64,64,0,0,EXPLOSION,sprite[i].ang,
		0,0,0,sprite[i].owner,sprite[i].sectnum,5,31,0,0);
		  //31=Time left for explosion to stay

	for(k=0;k<16;k++)
	{
		spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z+(8<<8),2,-4,0,
			32,24,24,0,0,EXPLOSION,sprite[i].ang,
			(krand()&511)-256,(krand()&511)-256,(krand()&16384)-8192,
			sprite[i].owner,sprite[i].sectnum,7,96,0,0);
				//96=Time left for smoke to be alive
	}

	for(j=connecthead;j>=0;j=connectpoint2[j])
	{
		dist = (posx[j]-sprite[i].x)*(posx[j]-sprite[i].x);
		dist += (posy[j]-sprite[i].y)*(posy[j]-sprite[i].y);
		dist += ((posz[j]-sprite[i].z)>>4)*((posz[j]-sprite[i].z)>>4);
		if (dist < 4194304)
			if (cansee(posx[j],posy[j],posz[j],cursectnum[j],sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
			{
				k = ((32768/((dist>>16)+4))>>5);
				if (j == myconnectindex)
				{
					daang = getangle(posx[j]-sprite[i].x,posy[j]-sprite[i].y);
					dax = ((k*sintable[(daang+2560)&2047])>>14);
					day = ((k*sintable[(daang+2048)&2047])>>14);
					vel += ((dax*sintable[(ang[j]+2560)&2047]+day*sintable[(ang[j]+2048)&2047])>>14);
					svel += ((day*sintable[(ang[j]+2560)&2047]-dax*sintable[(ang[j]+2048)&2047])>>14);
				}
				if (changehealth(j,-k) == 1)  //If you're dead
					if (sprite[i].owner == j+4096)
						saywatchit[j] = lockclock+120;
			}
	}

	for(k=1;k<=2;k++)         //Check for hurting monsters
	{
		j = headspritestat[k];
		while (j != -1)
		{
			nextj = nextspritestat[j];

			dist = (sprite[j].x-sprite[i].x)*(sprite[j].x-sprite[i].x);
			dist += (sprite[j].y-sprite[i].y)*(sprite[j].y-sprite[i].y);
			dist += ((sprite[j].z-sprite[i].z)>>4)*((sprite[j].z-sprite[i].z)>>4);
			if (dist < 4194304)
				if (cansee(sprite[j].x,sprite[j].y,sprite[j].z-(tilesizy[sprite[j].picnum]<<7),sprite[j].sectnum,sprite[i].x,sprite[i].y,sprite[i].z-(tilesizy[sprite[i].picnum]<<7),sprite[i].sectnum) == 1)
				{
					if (sprite[j].picnum == BROWNMONSTER)
					{
						sprite[j].z += ((tilesizy[sprite[j].picnum]*sprite[j].yrepeat)<<1);
						sprite[j].picnum = GIFTBOX;
						sprite[j].cstat &= ~0x83;    //Should not clip, foot-z
						changespritestat(j,0);
					}
					if (sprite[j].picnum == EVILAL)
					{
						sprite[j].picnum = EVILALGRAVE;
						sprite[j].cstat = 0;
						sprite[j].xrepeat = (krand()&63)+16;
						sprite[j].yrepeat = sprite[j].xrepeat;
						changespritestat(j,0);
					}
				}

			j = nextj;
		}
	}
	wsayfollow("blowup.wav",3840L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,0);
	deletesprite((short)i);
}

processinput(short snum)
{
	long oldposx, oldposy, nexti;
	long i, j, k, doubvel, xvect, yvect, goalz;
	long dax, day, dax2, day2, odax, oday, odax2, oday2;
	short startwall, endwall;
	char *ptr;

		//SHARED KEYS:
		//Movement code
	if ((syncvel[snum]|syncsvel[snum]) != 0)
	{
		doubvel = (TICSPERFRAME<<((syncbits[snum]&256)>0));

		xvect = 0, yvect = 0;
		if (syncvel[snum] != 0)
		{
			xvect += ((((long)syncvel[snum])*doubvel*(long)sintable[(ang[snum]+2560)&2047])>>3);
			yvect += ((((long)syncvel[snum])*doubvel*(long)sintable[(ang[snum]+2048)&2047])>>3);
		}
		if (syncsvel[snum] != 0)
		{
			xvect += ((((long)syncsvel[snum])*doubvel*(long)sintable[(ang[snum]+2048)&2047])>>3);
			yvect += ((((long)syncsvel[snum])*doubvel*(long)sintable[(ang[snum]+1536)&2047])>>3);
		}
		clipmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],xvect,yvect,128L,4<<8,4<<8,0);
		frameinterpolate = 1;
		revolvedoorstat[snum] = 1;
	}
	else
	{
		revolvedoorstat[snum] = 0;
	}

		//Push player away from walls if clipmove doesn't work
	if (pushmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],128L,4<<8,4<<8,0) < 0)
		changehealth(snum,-1000);  //If this screws up, then instant death!!!

			// Getzrange returns the highest and lowest z's for an entire box,
			// NOT just a point.  This prevents you from falling off cliffs
			// when you step only slightly over the cliff.
	sprite[playersprite[snum]].cstat ^= 1;
	getzrange(posx[snum],posy[snum],posz[snum],cursectnum[snum],&globhiz,&globhihit,&globloz,&globlohit,128L,0);
	sprite[playersprite[snum]].cstat ^= 1;

	if (syncangvel[snum] != 0)          //ang += angvel * constant
	{                         //ENGINE calculates angvel for you
		doubvel = TICSPERFRAME;
		if ((syncbits[snum]&256) > 0)  //Lt. shift makes turn velocity 50% faster
			doubvel += (TICSPERFRAME>>1);
		ang[snum] += ((((long)syncangvel[snum])*doubvel)>>4);
		ang[snum] = (ang[snum]+2048)&2047;
	}

	if (health[snum] < 0)
	{
		health[snum] -= TICSPERFRAME;
		if (health[snum] <= -160)
		{
			hvel[snum] = 0;
			if (snum == myconnectindex)
				vel = 0, svel = 0, angvel = 0, keystatus[3] = 1;

			deaths[snum]++;
			health[snum] = 100;
			numbombs[snum] = -1;

			findrandomspot(&posx[snum],&posy[snum],&cursectnum[snum]);
			posz[snum] = sector[cursectnum[snum]].floorz-(1<<8);
			horiz[snum] = 100;
			ang[snum] = (krand()&2047);

			setsprite(playersprite[snum],posx[snum],posy[snum],posz[snum]+(32<<8));
			sprite[playersprite[snum]].picnum = DOOMGUY;
			sprite[playersprite[snum]].ang = ang[snum];
			sprite[playersprite[snum]].xrepeat = 64;
			sprite[playersprite[snum]].yrepeat = 64;

			if ((snum == screenpeek) && (screensize <= xdim))
			{
				sprintf(&tempbuf,"Deaths: %d",deaths[snum]);
				printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-16,tempbuf,ALPHABET,80);
				sprintf(&tempbuf,"Health: %3d",health[snum]);
				printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-24,tempbuf,ALPHABET,80);
			}

			i = playersprite[snum];
			wsayfollow("zipguns.wav",4096L+(krand()&127)-64,256L,&sprite[i].x,&sprite[i].y,1);
			for(k=0;k<16;k++)
			{
				spawnsprite(j,sprite[i].x,sprite[i].y,sprite[i].z+(8<<8),2,-4,0,
					32,24,24,0,0,EXPLOSION,sprite[i].ang,
					(krand()&511)-256,(krand()&511)-256,(krand()&16384)-8192,
					sprite[i].owner,sprite[i].sectnum,7,96,0,0);
						//96=Time left for smoke to be alive
			}
		}
		else
		{
			sprite[playersprite[snum]].xrepeat = max(((128+health[snum])>>1),0);
			sprite[playersprite[snum]].yrepeat = max(((128+health[snum])>>1),0);

			hvel[snum] += (TICSPERFRAME<<2);
			horiz[snum] = max(horiz[snum]-4,0);
			posz[snum] += hvel[snum];
			if (posz[snum] > globloz-(4<<8))
			{
				posz[snum] = globloz-(4<<8);
				horiz[snum] = min(horiz[snum]+5,200);
				hvel[snum] = 0;
			}
		}
	}

	if (((syncbits[snum]&8) > 0) && (horiz[snum] > 100-(200>>1))) horiz[snum] -= 4;     //-
	if (((syncbits[snum]&4) > 0) && (horiz[snum] < 100+(200>>1))) horiz[snum] += 4;   //+

	goalz = globloz-(32<<8);         //32 pixels above floor
	if (sector[cursectnum[snum]].lotag == 4)   //slime sector
		if ((globlohit&0xc000) != 49152)            //You're not on a sprite
		{
			goalz = globloz-(8<<8);
			if (posz[snum] >= goalz-(2<<8))
			{
				clipmove(&posx[snum],&posy[snum],&posz[snum],&cursectnum[snum],-TICSPERFRAME<<14,-TICSPERFRAME<<14,128L,4<<8,4<<8,0);
				frameinterpolate = 0;

				if (slimesoundcnt[snum] >= 0)
				{
					slimesoundcnt[snum] -= TICSPERFRAME;
					while (slimesoundcnt[snum] < 0)
					{
						slimesoundcnt[snum] += 120;
						wsayfollow("slime.wav",4096L+(krand()&127)-64,256L,&posx[snum],&posy[snum],1);
					}
				}
			}
		}
	if (goalz < globhiz+(16<<8))   //ceiling&floor too close
		goalz = ((globloz+globhiz)>>1);
	//goalz += mousz;
	if (health[snum] >= 0)
	{
		if ((syncbits[snum]&1) > 0)                         //A (stand high)
		{
			if (posz[snum] >= globloz-(32<<8))
			{
				goalz -= (16<<8);
				if ((syncbits[snum]&256) > 0)                 //Either shift key
					goalz -= (24<<8);
			}
		}
		if ((syncbits[snum]&2) > 0)                         //Z (stand low)
		{
			goalz += (12<<8);
			if ((syncbits[snum]&256) > 0)                    //Either shift key
				goalz += (12<<8);
		}
	}
	if ((sector[cursectnum[snum]].floorstat&2) > 0) //Groudraw
	{
		if (waloff[sector[cursectnum[snum]].floorheinum] == 0) loadtile(sector[cursectnum[snum]].floorheinum);
		ptr = (char *)(waloff[sector[cursectnum[snum]].floorheinum]+(((posx[snum]>>4)&63)<<6)+((posy[snum]>>4)&63));
		goalz -= ((*ptr)<<8);
	}

	if (posz[snum] < goalz)
		hvel[snum] += (TICSPERFRAME<<4);
	else
		hvel[snum] = (((goalz-posz[snum])*TICSPERFRAME)>>5);

	posz[snum] += hvel[snum];
	if (posz[snum] > globloz-(4<<8)) posz[snum] = globloz-(4<<8), hvel[snum] = 0;
	if (posz[snum] < globhiz+(4<<8)) posz[snum] = globhiz+(4<<8), hvel[snum] = 0;

	if (dimensionmode[snum] != 3)
	{
		if (((syncbits[snum]&32) > 0) && (zoom[snum] > 48)) zoom[snum] -= (zoom[snum]>>4);
		if (((syncbits[snum]&16) > 0) && (zoom[snum] < 4096)) zoom[snum] += (zoom[snum]>>4);
	}

		//Update sprite representation of player
		//   -should be after movement, but before shooting code
	setsprite(playersprite[snum],posx[snum],posy[snum],posz[snum]+(32<<8));
	sprite[playersprite[snum]].ang = ang[snum];

	if ((cursectnum[snum] < 0) || (cursectnum[snum] >= numsectors))
	{       //How did you get in the wrong sector?
		wsayfollow("ouch.wav",4096L+(krand()&127)-64,64L,&posx[snum],&posy[snum],1);
		changehealth(snum,-TICSPERFRAME);
	}
	else if (globhiz+(8<<8) > globloz)
	{       //Ceiling and floor are smooshing you!
		wsayfollow("ouch.wav",4096L+(krand()&127)-64,64L,&posx[snum],&posy[snum],1);
		changehealth(snum,-TICSPERFRAME);
	}

	if ((waterfountainwall[snum] >= 0) && (health[snum] >= 0))
		if ((wall[neartagwall].lotag != 7) || ((syncbits[snum]&1024) == 0))
		{
			i = waterfountainwall[snum];
			if (wall[i].overpicnum == USEWATERFOUNTAIN)
				wall[i].overpicnum = WATERFOUNTAIN;
			else if (wall[i].picnum == USEWATERFOUNTAIN)
				wall[i].picnum = WATERFOUNTAIN;

			waterfountainwall[snum] = -1;
		}

	if ((syncbits[snum]&1024) > 0)  //Space bar
	{
			//Continuous triggers...

		neartag(posx[snum],posy[snum],posz[snum],cursectnum[snum],ang[snum],&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1024L,3);
		if (neartagsector == -1)
		{
			i = cursectnum[snum];
			if ((sector[i].lotag|sector[i].hitag) != 0)
				neartagsector = i;
		}

		if (wall[neartagwall].lotag == 7)  //Water fountain
		{
			if (wall[neartagwall].overpicnum == WATERFOUNTAIN)
			{
				wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&posx[snum],&posy[snum],1);
				wall[neartagwall].overpicnum = USEWATERFOUNTAIN;
				waterfountainwall[snum] = neartagwall;
			}
			else if (wall[neartagwall].picnum == WATERFOUNTAIN)
			{
				wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&posx[snum],&posy[snum],1);
				wall[neartagwall].picnum = USEWATERFOUNTAIN;
				waterfountainwall[snum] = neartagwall;
			}

			if (waterfountainwall[snum] >= 0)
			{
				waterfountaincnt[snum] -= TICSPERFRAME;
				while (waterfountaincnt[snum] < 0)
				{
					waterfountaincnt[snum] += 120;
					wsayfollow("water.wav",4096L+(krand()&127)-64,256L,&posx[snum],&posy[snum],1);
					changehealth(snum,2);
				}
			}
		}

			//1-time triggers...
		if ((oflags[snum]&1024) == 0)
		{
			if (neartagsector >= 0)
				if (sector[neartagsector].hitag == 0)
					operatesector(neartagsector);

			if (neartagwall >= 0)
				if (wall[neartagwall].lotag == 2)  //Switch
				{
					for(i=0;i<numsectors;i++)
						if (sector[i].hitag == wall[neartagwall].hitag)
							if (sector[i].lotag != 1)
								operatesector(i);
					i = headspritestat[0];
					while (i != -1)
					{
						nexti = nextspritestat[i];
						if (sprite[i].hitag == wall[neartagwall].hitag)
							operatesprite(i);
						i = nexti;
					}

					j = wall[neartagwall].overpicnum;
					if (j == SWITCH1ON)                     //1-time switch
					{
						wall[neartagwall].overpicnum = GIFTBOX;
						wall[neartagwall].lotag = 0;
						wall[neartagwall].hitag = 0;
					}
					if (j == GIFTBOX)                       //1-time switch
					{
						wall[neartagwall].overpicnum = SWITCH1ON;
						wall[neartagwall].lotag = 0;
						wall[neartagwall].hitag = 0;
					}
					if (j == SWITCH2ON) wall[neartagwall].overpicnum = SWITCH2OFF;
					if (j == SWITCH2OFF) wall[neartagwall].overpicnum = SWITCH2ON;
					if (j == SWITCH3ON) wall[neartagwall].overpicnum = SWITCH3OFF;
					if (j == SWITCH3OFF) wall[neartagwall].overpicnum = SWITCH3ON;

					i = wall[neartagwall].point2;
					dax = ((wall[neartagwall].x+wall[i].x)>>1);
					day = ((wall[neartagwall].y+wall[i].y)>>1);
					wsayfollow("switch.wav",4096L+(krand()&255)-128,256L,&dax,&day,0);
				}

			if (neartagsprite >= 0)
			{
				if (sprite[neartagsprite].lotag == 1)
				{  //if you're shoving innocent little AL around, he gets mad!
					if (sprite[neartagsprite].picnum == AL)
					{
						sprite[neartagsprite].picnum = EVILAL;
						sprite[neartagsprite].cstat |= 2;   //Make him transluscent
						sprite[neartagsprite].xrepeat = 38;
						sprite[neartagsprite].yrepeat = 38;
					}
					changespritestat(neartagsprite,1);
				}
				if (sprite[neartagsprite].lotag == 4)
				{
					for(i=0;i<numsectors;i++)
						if (sector[i].hitag == sprite[neartagsprite].hitag)
							if (sector[i].lotag != 1)
								operatesector(i);
					i = headspritestat[0];
					while (i != -1)
					{
						nexti = nextspritestat[i];
						if (sprite[i].hitag == sprite[neartagsprite].hitag)
							operatesprite(i);
						i = nexti;
					}

					j = sprite[neartagsprite].picnum;
					if (j == SWITCH1ON)                     //1-time switch
					{
						sprite[neartagsprite].picnum = GIFTBOX;
						sprite[neartagsprite].lotag = 0;
						sprite[neartagsprite].hitag = 0;
					}
					if (j == GIFTBOX)                       //1-time switch
					{
						sprite[neartagsprite].picnum = SWITCH1ON;
						sprite[neartagsprite].lotag = 0;
						sprite[neartagsprite].hitag = 0;
					}
					if (j == SWITCH2ON) sprite[neartagsprite].picnum = SWITCH2OFF;
					if (j == SWITCH2OFF) sprite[neartagsprite].picnum = SWITCH2ON;
					if (j == SWITCH3ON) sprite[neartagsprite].picnum = SWITCH3OFF;
					if (j == SWITCH3OFF) sprite[neartagsprite].picnum = SWITCH3ON;

					dax = sprite[neartagsprite].x;
					day = sprite[neartagsprite].y;
					wsayfollow("switch.wav",4096L+(krand()&255)-128,256L,&dax,&day,0);
				}
			}
		}
	}

	if ((syncbits[snum]&2048) > 0)      //Shoot a bullet
	{
		if ((health[snum] >= 0) || ((krand()&127) > -health[snum]))
			switch((syncbits[snum]>>13)&7)
			{
				case 0:
					if ((oflags[snum]&2048) == 0)
						shootgun(snum,posx[snum],posy[snum],posz[snum],ang[snum],horiz[snum],cursectnum[snum],0);
					break;
				case 1:
					if (lockclock > lastchaingun[snum]+8)
					{
						lastchaingun[snum] = lockclock;
						shootgun(snum,posx[snum],posy[snum],posz[snum],ang[snum],horiz[snum],cursectnum[snum],1);
					}
					break;
				case 2:
					if ((oflags[snum]&2048) == 0)
						if (numbombs[snum] > 0)
						{
							shootgun(snum,posx[snum],posy[snum],posz[snum],ang[snum],horiz[snum],cursectnum[snum],2);
							numbombs[snum]--;
						}
					break;
			}
	}

	if ((syncbits[snum]&4096) > (oflags[snum]&4096))  //Keypad enter
	{
		dimensionmode[snum]++;
		if (dimensionmode[snum] > 3) dimensionmode[snum] = 1;
		if (snum == screenpeek)
		{
			if (dimensionmode[snum] == 2) setview(0L,0L,xdim-1,(ydim-1)>>detailmode);
			if (dimensionmode[snum] == 3) setup3dscreen();
		}
	}

	oflags[snum] = syncbits[snum];
}

static char lockbyte4094;
drawscreen(short snum, long dasmoothratio)
{
	long i, j, k, charsperline, templong, dx, dy, top, bot;
	long x1, y1, x2, y2, ox1, oy1, ox2, oy2, dist, maxdist;
	long cposx, cposy, cposz, choriz, czoom, tposx, tposy, thoriz;
	short cang, tang;
	char ch, *ptr, *ptr2, *ptr3, *ptr4;

	smoothratio = max(min(dasmoothratio,65536),0);

	setears(posx[snum],posy[snum],(long)sintable[(ang[snum]+512)&2047]<<14,(long)sintable[ang[snum]&2047]<<14);

	cposx = oposx[snum]+mulscale(posx[snum]-oposx[snum],smoothratio,16);
	cposy = oposy[snum]+mulscale(posy[snum]-oposy[snum],smoothratio,16);
	cposz = oposz[snum]+mulscale(posz[snum]-oposz[snum],smoothratio,16);
	if (frameinterpolate == 0)
		{ cposx = posx[snum]; cposy = posy[snum]; cposz = posz[snum]; }
	choriz = ohoriz[snum]+mulscale(horiz[snum]-ohoriz[snum],smoothratio,16);
	czoom = ozoom[snum]+mulscale(zoom[snum]-ozoom[snum],smoothratio,16);
	cang = oang[snum]+mulscale(((ang[snum]+1024-oang[snum])&2047)-1024,smoothratio,16);

	if (dimensionmode[snum] != 2)
	{
		if ((numplayers > 1) && (option[4] == 0))
		{
				//Do not draw other views constantly if they're staying still
				//It's a shame this trick will only work in screen-buffer mode
				//At least screen-buffer mode covers all the HI hi-res modes
			if (vidoption == 1)
			{
				for(i=connecthead;i>=0;i=connectpoint2[i]) frame2draw[i] = 0;
				frame2draw[snum] = 1;

					//2-1,3-1,4-2
					//5-2,6-2,7-2,8-3,9-3,10-3,11-3,12-4,13-4,14-4,15-4,16-5
				x1 = posx[snum]; y1 = posy[snum];
				for(j=(numplayers>>2)+1;j>0;j--)
				{
					maxdist = 0x80000000;
					for(i=connecthead;i>=0;i=connectpoint2[i])
						if (frame2draw[i] == 0)
						{
							x2 = posx[i]-x1; y2 = posy[i]-y1;
							dist = mulscale(x2,x2,12) + mulscale(y2,y2,12);

							if (dist < 64) dist = 16384;
							else if (dist > 16384) dist = 64;
							else dist = 1048576 / dist;

							dist *= frameskipcnt[i];

								//Increase frame rate if screen is moving
							if ((posx[i] != oposx[i]) || (posy[i] != oposy[i]) ||
								 (posz[i] != oposz[i]) || (ang[i] != oang[i]) ||
								 (horiz[i] != ohoriz[i])) dist += dist;

							if (dist > maxdist) maxdist = dist, k = i;
						}

					for(i=connecthead;i>=0;i=connectpoint2[i])
						frameskipcnt[i] += (frameskipcnt[i]>>3)+1;
					frameskipcnt[k] = 0;

					frame2draw[k] = 1;
				}
			}
			else
			{
				for(i=connecthead;i>=0;i=connectpoint2[i]) frame2draw[i] = 1;
			}

			for(i=connecthead,j=0;i>=0;i=connectpoint2[i],j++)
				if (frame2draw[i] != 0)
				{
					if (numplayers <= 4)
					{
						switch(j)
						{
							case 0: setview(0,0,(xdim>>1)-1,(ydim>>1)-1); break;
							case 1: setview((xdim>>1),0,xdim-1,(ydim>>1)-1); break;
							case 2: setview(0,(ydim>>1),(xdim>>1)-1,ydim-1); break;
							case 3: setview((xdim>>1),(ydim>>1),xdim-1,ydim-1); break;
						}
					}
					else
					{
						switch(j)
						{
							case 0: setview(0,0,(xdim>>2)-1,(ydim>>2)-1); break;
							case 1: setview(xdim>>2,0,(xdim>>1)-1,(ydim>>2)-1); break;
							case 2: setview(xdim>>1,0,xdim-(xdim>>2)-1,(ydim>>2)-1); break;
							case 3: setview(xdim-(xdim>>2),0,xdim-1,(ydim>>2)-1); break;
							case 4: setview(0,ydim>>2,(xdim>>2)-1,(ydim>>1)-1); break;
							case 5: setview(xdim>>2,ydim>>2,(xdim>>1)-1,(ydim>>1)-1); break;
							case 6: setview(xdim>>1,ydim>>2,xdim-(xdim>>2)-1,(ydim>>1)-1); break;
							case 7: setview(xdim-(xdim>>2),ydim>>2,xdim-1,(ydim>>1)-1); break;
							case 8: setview(0,ydim>>1,(xdim>>2)-1,ydim-(ydim>>2)-1); break;
							case 9: setview(xdim>>2,ydim>>1,(xdim>>1)-1,ydim-(ydim>>2)-1); break;
							case 10: setview(xdim>>1,ydim>>1,xdim-(xdim>>2)-1,ydim-(ydim>>2)-1); break;
							case 11: setview(xdim-(xdim>>2),ydim>>1,xdim-1,ydim-(ydim>>2)-1); break;
							case 12: setview(0,ydim-(ydim>>2),(xdim>>2)-1,ydim-1); break;
							case 13: setview(xdim>>2,ydim-(ydim>>2),(xdim>>1)-1,ydim-1); break;
							case 14: setview(xdim>>1,ydim-(ydim>>2),xdim-(xdim>>2)-1,ydim-1); break;
							case 15: setview(xdim-(xdim>>2),ydim-(ydim>>2),xdim-1,ydim-1); break;
						}
					}

					if (i == snum)
						drawrooms(cposx,cposy,cposz,cang,choriz,cursectnum[i]);
					else
						drawrooms(posx[i],posy[i],posz[i],ang[i],horiz[i],cursectnum[i]);
					analyzesprites(posx[i],posy[i]);
					drawmasks();
					if (numbombs[i] > 0)
						overwritesprite(160L,184L,GUNONBOTTOM,sector[cursectnum[i]].floorshade,1|2,0);

					if (lockclock < 384)
					{
						if (lockclock < 128)
							rotatesprite(320<<15,200<<15,lockclock<<9,lockclock<<4,DEMOSIGN,(128-lockclock)>>2,0,1+2);
						else if (lockclock < 256)
							rotatesprite(320<<15,200<<15,65536,0,DEMOSIGN,0,0,2);
						else
							rotatesprite(320<<15,200<<15,(384-lockclock)<<9,lockclock<<4,DEMOSIGN,(lockclock-256)>>2,0,1+2);
					}

					if (health[i] <= 0)
						rotatesprite(320<<15,200<<15,(-health[i])<<11,(-health[i])<<5,NO,0,0,2);
				}
		}
		else
		{
				//Startdmost optimization for weapons
			if ((numbombs[screenpeek] > 0) && (windowx1 == 0) && (windowx2 == 320))
			{
				x1 = 160L-(tilesizx[GUNONBOTTOM]>>1);
				y1 = 184L-(tilesizy[GUNONBOTTOM]>>1);
				for(i=0;i<tilesizx[GUNONBOTTOM];i++)
					startdmost[i+x1] = min(windowy2+1,gundmost[i]+y1);
			}

			drawrooms(cposx,cposy,cposz,cang,choriz,cursectnum[snum]);
			analyzesprites(posx[snum],posy[snum]);
			drawmasks();
			if (numbombs[screenpeek] > 0)
			{
					//Reset startdmost to bottom of screen
				if ((windowx1 == 0) && (windowx2 == 320))
				{
					x1 = 160L-(tilesizx[GUNONBOTTOM]>>1); y1 = windowy2+1;
					for(i=0;i<tilesizx[GUNONBOTTOM];i++)
						startdmost[i+x1] = y1;
				}
				overwritesprite(160L,184L,GUNONBOTTOM,sector[cursectnum[screenpeek]].floorshade,1|2,0);
			}

			if (lockclock < 384)
			{
				if (lockclock < 128)
					rotatesprite(320<<15,200<<15,lockclock<<9,lockclock<<4,DEMOSIGN,(128-lockclock)>>2,0,1+2);
				else if (lockclock < 256)
					rotatesprite(320<<15,200<<15,65536,0,DEMOSIGN,0,0,2);
				else
					rotatesprite(320<<15,200<<15,(384-lockclock)<<9,lockclock<<4,DEMOSIGN,(lockclock-256)>>2,0,1+2);
			}

			if (health[screenpeek] <= 0)
				rotatesprite(320<<15,200<<15,(-health[screenpeek])<<11,(-health[screenpeek])<<5,NO,0,0,2);
		}
	}

		//Only animate lava if its picnum is on screen
		//gotpic is a bit array where the tile number's bit is set
		//whenever it is drawn (ceilings, walls, sprites, etc.)
	if ((gotpic[SLIME>>3]&(1<<(SLIME&7))) > 0)
	{
		gotpic[SLIME>>3] &= ~(1<<(SLIME&7));
		if (waloff[SLIME] != 0) movelava((char *)waloff[SLIME]);
	}

	if (dimensionmode[snum] != 3)
	{
			//Move back pivot point
		cposx += (sintable[(cang+512)&2047]<<6) / czoom;
		cposy += (sintable[cang&2047]<<6) / czoom;
		if (dimensionmode[snum] == 2)
		{
			clearview(0L);  //Clear screen to specified color
			drawmapview(cposx,cposy,czoom,cang);
		}
		drawoverheadmap(cposx,cposy,czoom,cang);
	}

	if (numplayers >= 2)
	{
			//Tell who's master or slave
		if (lockclock < masterslavetexttime+120)
		{
			if (myconnectindex == connecthead)
				printext256(152L,0L,31,-1,"Master",0);
			else
				printext256(152L,0L,31,-1,"Slave",0);
		}
	}

	if (typemode != 0)
	{
		charsperline = 40;
		//if (dimensionmode[snum] == 2) charsperline = 80;

		for(i=0;i<=typemessageleng;i+=charsperline)
		{
			for(j=0;j<charsperline;j++)
				tempbuf[j] = typemessage[i+j];
			if (typemessageleng < i+charsperline)
			{
				tempbuf[(typemessageleng-i)] = '_';
				tempbuf[(typemessageleng-i)+1] = 0;
			}
			else
				tempbuf[charsperline] = 0;
			//if (dimensionmode[snum] == 3)
				printext256(0L,(i/charsperline)<<3,183,-1,tempbuf,0);
			//else
			//   printext16(0L,((i/charsperline)<<3)+(pageoffset/640),10,-1,tempbuf,0);
		}
	}
	else
	{
		if (dimensionmode[myconnectindex] == 3)
		{
			templong = screensize;

			if (((locbits&32) > (screensizeflag&32)) && (screensize > 64))
			{
				ox1 = (xdim>>1)-(screensize>>1);
				ox2 = ox1+screensize-1;
				oy1 = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
				oy2 = oy1+((screensize*(ydim-32))/xdim)-1;
				screensize -= (screensize>>3);

				if (templong > xdim)
				{
					screensize = xdim;

					permanentwritesprite((xdim-320)>>1,ydim-32,STATUSBAR,0,0,0,xdim-1,ydim-1,0);
					i = ((xdim-320)>>1);
					while (i >= 8) i -= 8, permanentwritesprite(i,ydim-32,STATUSBARFILL8,0,0,0,xdim-1,ydim-1,0);
					if (i >= 4) i -= 4, permanentwritesprite(i,ydim-32,STATUSBARFILL4,0,0,0,xdim-1,ydim-1,0);
					i = ((xdim-320)>>1)+320;
					while (i <= xdim-8) permanentwritesprite(i,ydim-32,STATUSBARFILL8,0,0,0,xdim-1,ydim-1,0), i += 8;
					if (i <= xdim-4) permanentwritesprite(i,ydim-32,STATUSBARFILL4,0,0,0,xdim-1,ydim-1,0), i += 4;

					sprintf(&tempbuf,"Deaths: %d",deaths[screenpeek]);
					printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-16,tempbuf,ALPHABET,80);

					sprintf(&tempbuf,"Health: %3d",health[screenpeek]);
					printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-24,tempbuf,ALPHABET,80);

					sprintf(&tempbuf,"Score:%ld",score[screenpeek]);
					printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-8,tempbuf,ALPHABET,80);
				}

				x1 = (xdim>>1)-(screensize>>1);
				x2 = x1+screensize-1;
				y1 = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
				y2 = y1+((screensize*(ydim-32))/xdim)-1;
				setview(x1,y1>>detailmode,x2,y2>>detailmode);

				// (ox1,oy1)ÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿
				//          ³  (x1,y1)        ³
				//          ³     ÚÄÄÄÄÄ¿     ³
				//          ³     ³     ³     ³
				//          ³     ÀÄÄÄÄÄÙ     ³
				//          ³        (x2,y2)  ³
				//          ÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ(ox2,oy2)

				permanentwritespritetile(0L,0L,BACKGROUND,0,ox1,oy1,x1-1,oy2,0);
				permanentwritespritetile(0L,0L,BACKGROUND,0,x2+1,oy1,ox2,oy2,0);
				permanentwritespritetile(0L,0L,BACKGROUND,0,x1,oy1,x2,y1-1,0);
				permanentwritespritetile(0L,0L,BACKGROUND,0,x1,y2+1,x2,oy2,0);
			}
			if (((locbits&16) > (screensizeflag&16)) && (screensize <= xdim))
			{
				screensize += (screensize>>3);
				if ((screensize > xdim) && (templong == xdim))
				{
					screensize = xdim+1;
					x1 = 0; y1 = 0;
					x2 = xdim-1; y2 = ydim-1;
				}
				else
				{
					if (screensize > xdim) screensize = xdim;
					x1 = (xdim>>1)-(screensize>>1);
					x2 = x1+screensize-1;
					y1 = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
					y2 = y1+((screensize*(ydim-32))/xdim)-1;
				}
				setview(x1,y1>>detailmode,x2,y2>>detailmode);
			}
			screensizeflag = locbits;
		}
	}

	if (getmessageleng > 0)
	{
		charsperline = 40;
		//if (dimensionmode[snum] == 2) charsperline = 80;

		for(i=0;i<=getmessageleng;i+=charsperline)
		{
			for(j=0;j<charsperline;j++)
				tempbuf[j] = getmessage[i+j];
			if (getmessageleng < i+charsperline)
				tempbuf[(getmessageleng-i)] = 0;
			else
				tempbuf[charsperline] = 0;

			printext256(0L,((i/charsperline)<<3)+(200-32-8)-(((getmessageleng-1)/charsperline)<<3),151,-1,tempbuf,0);
		}
		if (totalclock > getmessagetimeoff)
			getmessageleng = 0;
	}
	if ((numplayers >= 2) && (screenpeek != myconnectindex))
	{
		strcpy(tempbuf,"Other");

		//if (dimensionmode[snum] == 3)
			printext256((xdim>>1)-(strlen(tempbuf)<<2),0,24,-1,tempbuf,0);
		//else
		//   printext16(xdim-(strlen(tempbuf)<<2),(pageoffset/640),7,-1,tempbuf,0);
	}

	if (syncstat != 0) printext256(68L,84L,31,0,"OUT OF SYNC!",0);
	if (syncstate != 0) printext256(68L,92L,31,0,"Missed Network packet!",0);

	nextpage();

	if (keystatus[0x3f] > 0)   //F5
	{
		keystatus[0x3f] = 0;
		detailmode ^= 1;
		if (detailmode == 0)
		{
			setview(windowx1,windowy1<<1,windowx2,(windowy2<<1)+1);
			outp(0x3d4,0x9); outp(0x3d5,(inp(0x3d5)&~31)|1);
		}
		else
		{
			setview(windowx1,windowy1>>detailmode,windowx2,windowy2>>detailmode);
			setaspect(yxaspect>>1);
			outp(0x3d4,0x9); outp(0x3d5,(inp(0x3d5)&~31)|3);
		}
	}
	if (keystatus[0x58] > 0)   //F12
	{
		keystatus[0x58] = 0;
		screencapture("captxxxx.pcx",keystatus[0x2a]|keystatus[0x36]);
	}
	if (stereofps != 0)  //Adjustments for red-blue / 120 crystal eyes modes
	{
		if ((keystatus[0x2a]|keystatus[0x36]) > 0)
		{
			if (keystatus[0x1a] > 0) stereopixelwidth--;   //Shift [
			if (keystatus[0x1b] > 0) stereopixelwidth++;   //Shift ]
		}
		else
		{
			if (keystatus[0x1a] > 0) stereowidth -= 512;   //[
			if (keystatus[0x1b] > 0) stereowidth += 512;   //]
		}
	}

	if (option[4] == 0)           //Single player only keys
	{
		if (keystatus[0xd2] > 0)   //Insert - Insert player
		{
			keystatus[0xd2] = 0;
			if (numplayers < MAXPLAYERS)
			{
				connectpoint2[numplayers-1] = numplayers;
				connectpoint2[numplayers] = -1;

				initplayersprite(numplayers);

				clearallviews(0L);  //Clear screen to specified color

				numplayers++;
			}
		}
		if (keystatus[0xd3] > 0)   //Delete - Delete player
		{
			keystatus[0xd3] = 0;
			if (numplayers > 1)
			{
				numplayers--;
				connectpoint2[numplayers-1] = -1;

				deletesprite(playersprite[numplayers]);
				playersprite[numplayers] = -1;

				if (myconnectindex >= numplayers) myconnectindex = 0;
				if (screenpeek >= numplayers) screenpeek = 0;

				if (numplayers < 2)
					setup3dscreen();
				else
					clearallviews(0L);  //Clear screen to specified color
			}
		}
		if (keystatus[0x46] > 0)   //Scroll Lock
		{
			keystatus[0x46] = 0;

			myconnectindex = connectpoint2[myconnectindex];
			if (myconnectindex < 0) myconnectindex = connecthead;
			screenpeek = myconnectindex;
		}
	}
}

movethings()
{
	long i;

	gotlastpacketclock = totalclock;
	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		baksyncvel[movefifoend][i] = fsyncvel[i];
		baksyncsvel[movefifoend][i] = fsyncsvel[i];
		baksyncangvel[movefifoend][i] = fsyncangvel[i];
		baksyncbits[movefifoend][i] = fsyncbits[i];
	}
	movefifoend = ((movefifoend+1)&(MOVEFIFOSIZ-1));

		//Do this for Master/Slave switching
	for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
		if (syncbits[i]&512) ready2send = 0;
}

domovethings()
{
	short i, j, startwall, endwall;
	spritetype *spr;
	walltype *wal;
	point3d *ospr;

	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		syncvel[i] = baksyncvel[movefifoplc][i];
		syncsvel[i] = baksyncsvel[movefifoplc][i];
		syncangvel[i] = baksyncangvel[movefifoplc][i];
		syncbits[i] = baksyncbits[movefifoplc][i];
	}
	movefifoplc = ((movefifoplc+1)&(MOVEFIFOSIZ-1));

	syncval[syncvalend] = getsyncstat();
	syncvalend = ((syncvalend+1)&(MOVEFIFOSIZ-1));

	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		oposx[i] = posx[i];
		oposy[i] = posy[i];
		oposz[i] = posz[i];
		ohoriz[i] = horiz[i];
		ozoom[i] = zoom[i];
		oang[i] = ang[i];
	}

	for(i=1;i<=8;i++)
		if (i != 2)
			for(j=headspritestat[i];j>=0;j=nextspritestat[j])
				copybuf(&sprite[j].x,&osprite[j].x,3);

	for(i=connecthead;i>=0;i=connectpoint2[i])
		ocursectnum[i] = cursectnum[i];

	if ((numplayers <= 2) && (recstat == 1))
	{
		j = 0;
		for(i=connecthead;i>=0;i=connectpoint2[i])
		{
			recsyncvel[reccnt][j] = syncvel[i];
			recsyncsvel[reccnt][j] = syncsvel[i];
			recsyncangvel[reccnt][j] = syncangvel[i];
			recsyncbits[reccnt][j] = syncbits[i];
			j++;
		}
		reccnt++; if (reccnt > 16383) reccnt = 16383;
	}

	lockclock += TICSPERFRAME;

	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		processinput(i);                        //Move player

		checktouchsprite(i,cursectnum[i]);      //Pick up coins
		startwall = sector[cursectnum[i]].wallptr;
		endwall = startwall + sector[cursectnum[i]].wallnum;
		for(j=startwall,wal=&wall[j];j<endwall;j++,wal++)
			if (wal->nextsector >= 0) checktouchsprite(i,wal->nextsector);
	}

	doanimations();
	tagcode();            //Door code, moving sector code, other stuff
	statuslistcode();     //Monster / bullet code / explosions

	checkmasterslaveswitch();
}

getinput()
{
	char ch, keystate, *ptr;
	long i, j, k;
	short mousx, mousy, bstatus;

	if (typemode == 0)           //if normal game keys active
	{
		if ((keystatus[0x2a]&keystatus[0x36]&keystatus[0x13]) > 0)   //Sh.Sh.R (replay)
		{
			keystatus[0x13] = 0;
			playback();
		}

		if ((keystatus[0x26]&(keystatus[0x1d]|keystatus[0x9d])) > 0) //Load game
		{
			keystatus[0x26] = 0;
			loadgame();
		}

		if ((keystatus[0x1f]&(keystatus[0x1d]|keystatus[0x9d])) > 0) //Save game
		{
			keystatus[0x1f] = 0;
			savegame();
		}

		if (keystatus[keys[15]] > 0)
		{
			keystatus[keys[15]] = 0;

			screenpeek = connectpoint2[screenpeek];
			if (screenpeek < 0) screenpeek = connecthead;
		}

		for(i=7;i>=0;i--)
			if (keystatus[i+2] > 0)
				{ keystatus[i+2] = 0; locselectedgun = i; break; }
	}


		//KEYTIMERSTUFF
	if (keystatus[keys[5]] == 0)
	{
		if (keystatus[keys[2]] > 0) angvel = max(angvel-16*TICSPERFRAME,-128);
		if (keystatus[keys[3]] > 0) angvel = min(angvel+16*TICSPERFRAME,127);
	}
	else
	{
		if (keystatus[keys[2]] > 0) svel = min(svel+8*TICSPERFRAME,127);
		if (keystatus[keys[3]] > 0) svel = max(svel-8*TICSPERFRAME,-128);
	}
	if (keystatus[keys[0]] > 0) vel = min(vel+8*TICSPERFRAME,127);
	if (keystatus[keys[1]] > 0) vel = max(vel-8*TICSPERFRAME,-128);
	if (keystatus[keys[12]] > 0) svel = min(svel+8*TICSPERFRAME,127);
	if (keystatus[keys[13]] > 0) svel = max(svel-8*TICSPERFRAME,-128);

	if (angvel < 0) angvel = min(angvel+12*TICSPERFRAME,0);
	if (angvel > 0) angvel = max(angvel-12*TICSPERFRAME,0);
	if (svel < 0) svel = min(svel+2*TICSPERFRAME,0);
	if (svel > 0) svel = max(svel-2*TICSPERFRAME,0);
	if (vel < 0) vel = min(vel+2*TICSPERFRAME,0);
	if (vel > 0) vel = max(vel-2*TICSPERFRAME,0);

	if ((option[4] == 0) && (numplayers == 2))
	{
		if (keystatus[0x4f] == 0)
		{
			if (keystatus[0x4b] > 0) angvel2 = max(angvel2-16*TICSPERFRAME,-128);
			if (keystatus[0x4d] > 0) angvel2 = min(angvel2+16*TICSPERFRAME,127);
		}
		else
		{
			if (keystatus[0x4b] > 0) svel2 = min(svel2+8*TICSPERFRAME,127);
			if (keystatus[0x4d] > 0) svel2 = max(svel2-8*TICSPERFRAME,-128);
		}
		if (keystatus[0x48] > 0) vel2 = min(vel2+8*TICSPERFRAME,127);
		if (keystatus[0x4c] > 0) vel2 = max(vel2-8*TICSPERFRAME,-128);

		if (angvel2 < 0) angvel2 = min(angvel2+12*TICSPERFRAME,0);
		if (angvel2 > 0) angvel2 = max(angvel2-12*TICSPERFRAME,0);
		if (svel2 < 0) svel2 = min(svel2+2*TICSPERFRAME,0);
		if (svel2 > 0) svel2 = max(svel2-2*TICSPERFRAME,0);
		if (vel2 < 0) vel2 = min(vel2+2*TICSPERFRAME,0);
		if (vel2 > 0) vel2 = max(vel2-2*TICSPERFRAME,0);
	}

	locvel = min(max(vel,-128+8),127-8);
	locsvel = min(max(svel,-128+8),127-8);
	locangvel = min(max(angvel,-128+16),127-16);

	getmousevalues(&mousx,&mousy,&bstatus);
	locangvel = min(max(locangvel+(mousx<<3),-128),127);
	locvel = min(max(locvel-(mousy<<3),-128),127);

	locbits = (locselectedgun<<13);
	if (typemode == 0)           //if normal game keys active
	{
		locbits |= (keystatus[0x32]<<9);                 //M (be master)
		locbits |= ((keystatus[keys[14]]==1)<<12);       //Map mode
	}
	locbits |= keystatus[keys[8]];                   //Stand high
	locbits |= (keystatus[keys[9]]<<1);              //Stand low
	locbits |= (keystatus[keys[16]]<<4);             //Zoom in
	locbits |= (keystatus[keys[17]]<<5);             //Zoom out
	locbits |= (keystatus[keys[4]]<<8);                 //Run
	locbits |= (keystatus[keys[10]]<<2);                //Look up
	locbits |= (keystatus[keys[11]]<<3);                //Look down
	locbits |= ((keystatus[keys[7]]==1)<<10);           //Space
	locbits |= ((keystatus[keys[6]]==1)<<11);           //Shoot
	locbits |= (((bstatus&6)>(oldmousebstatus&6))<<10); //Space
	locbits |= (((bstatus&1)>(oldmousebstatus&1))<<11); //Shoot

	oldmousebstatus = bstatus;
	if (((locbits&2048) > 0) && (locselectedgun == 1))
		oldmousebstatus &= ~1;     //Allow continous fire with mouse for chain gun

		//PRIVATE KEYS:
	if (keystatus[0xb7] > 0)  //Printscreen
	{
		keystatus[0xb7] = 0;
		printscreeninterrupt();
	}
	if (keystatus[0x57] > 0)  //F11 - brightness
	{
		keystatus[0x57] = 0;
		brightness++;
		if (brightness > 8) brightness = 0;
		setbrightness(brightness);
	}

	if (typemode == 0)           //if normal game keys active
	{
		if (keystatus[0x19] > 0)  //P
		{
			keystatus[0x19] = 0;
			parallaxtype++;
			if (parallaxtype > 2) parallaxtype = 0;
		}
		if ((keystatus[0x38]|keystatus[0xb8]) > 0)  //ALT
		{
			if (keystatus[0x4a] > 0)  // Keypad -
				visibility = min(visibility+(visibility>>3),16384);
			if (keystatus[0x4e] > 0)  // Keypad +
				visibility = max(visibility-(visibility>>3),128);
		}

		/*if ((option[1] == 1) && (option[4] >= 5))
		{
			if ((keystatus[0x13] > 0) && (recsnddone == 1) && (recording == -2))      //R (record)
			{
				wrec(32768L);
				recording = -1;
			}
			if ((recording == -1) && ((keystatus[0x13] == 0) || (recsnddone == 1)))
			{
				continueplay();
				recsnddone = 1;
				recording = 0;
				wsay("recordedvoice",2972L,255L,255L);
			}
			if ((recording >= 0) && (screenpeek != myconnectindex))
			{
				tempbuf[0] = 3;

				ptr = (char *)(recsndoffs+recording);
				for(i=0;i<256;i++)
					tempbuf[i+1] = *ptr++;
				recording += 256;

				sendpacket(screenpeek,tempbuf,257L);
				if (recording >= 32768)
					recording = -2;
			}
		}*/

		if ((keystatus[keys[18]]) > 0)   //Typing mode
		{
			keystatus[keys[18]] = 0;
			typemode = 1;
			keyfifoplc = keyfifoend;      //Reset keyboard fifo
		}
	}
	else
	{
		while (keyfifoplc != keyfifoend)
		{
			ch = keyfifo[keyfifoplc];
			keystate = keyfifo[(keyfifoplc+1)&(KEYFIFOSIZ-1)];
			keyfifoplc = ((keyfifoplc+2)&(KEYFIFOSIZ-1));

			if (keystate != 0)
			{
				if (ch == 0xe)   //Backspace
				{
					if (typemessageleng == 0) { typemode = 0; break; }
					typemessageleng--;
				}
				if (ch == 0xf)
				{
					keystatus[0xf] = 0;
					typemode = 0;
					break;
				}
				if ((ch == 0x1c) || (ch == 0x9c))  //Either ENTER
				{
					keystatus[0x1c] = 0; keystatus[0x9c] = 0;
					if (typemessageleng > 0)
					{
						tempbuf[0] = 2;          //Sending text is message type 4
						for(j=typemessageleng-1;j>=0;j--)
							tempbuf[j+1] = typemessage[j];

						for(i=connecthead;i>=0;i=connectpoint2[i])
							if (i != myconnectindex)
								sendpacket(i,tempbuf,typemessageleng+1);

						typemessageleng = 0;
					}
					typemode = 0;
					break;
				}

				if ((typemessageleng < 159) && (ch < 128))
				{
					if ((keystatus[0x2a]|keystatus[0x36]) != 0)
						ch = scantoascwithshift[ch];
					else
						ch = scantoasc[ch];

					if (ch != 0) typemessage[typemessageleng++] = ch;
				}
			}
		}
			//Here's a trick of making key repeat after a 1/2 second
		if (keystatus[0xe] > 0)
		{
			if (keystatus[0xe] < 30)
				keystatus[0xe] += TICSPERFRAME;
			else
			{
				if (typemessageleng == 0)
					typemode = 0;
				else
					typemessageleng--;
			}
		}
	}
}

initplayersprite(short snum)
{
	long i;

	if (playersprite[snum] >= 0) return;

	spawnsprite(playersprite[snum],posx[snum],posy[snum],posz[snum]+(32<<8),
		1+256,0,snum,32,64,64,0,0,DOOMGUY,ang[snum],0,0,0,snum+4096,
		cursectnum[snum],8,0,0,0);

	switch(snum)
	{
		case 1: for(i=0;i<32;i++) tempbuf[i+192] = i+128; break; //green->red
		case 2: for(i=0;i<32;i++) tempbuf[i+192] = i+32; break;  //green->blue
		case 3: for(i=0;i<32;i++) tempbuf[i+192] = i+224; break; //green->pink
		case 4: for(i=0;i<32;i++) tempbuf[i+192] = i+64; break;  //green->brown
		case 5: for(i=0;i<32;i++) tempbuf[i+192] = i+96; break;
		case 6: for(i=0;i<32;i++) tempbuf[i+192] = i+160; break;
		case 7: for(i=0;i<32;i++) tempbuf[i+192] = i+192; break;
		default: for(i=0;i<256;i++) tempbuf[i] = i; break;
	}
	makepalookup(snum,tempbuf,0,0,0,1);
}

playback()
{
	long i, j, k;

	ready2send = 0;
	recstat = 0; i = reccnt;
	while (keystatus[1] == 0)
	{
		while (totalclock >= lockclock+TICSPERFRAME)
		{
			if (i >= reccnt)
			{
				prepareboard(boardfilename);
				for(i=connecthead;i>=0;i=connectpoint2[i])
					initplayersprite((short)i);
				resettiming(); ototalclock = 0; gotlastpacketclock = 0;
				i = 0;
			}

			k = 0;
			for(j=connecthead;j>=0;j=connectpoint2[j])
			{
				fsyncvel[j] = recsyncvel[i][k];
				fsyncsvel[j] = recsyncsvel[i][k];
				fsyncangvel[j] = recsyncangvel[i][k];
				fsyncbits[j] = recsyncbits[i][k];
				k++;
			}
			movethings(); domovethings();
			i++;
		}
		drawscreen(screenpeek,(totalclock-lockclock)*(65536/TICSPERFRAME));

		if (keystatus[keys[15]] > 0)
		{
			keystatus[keys[15]] = 0;

			screenpeek = connectpoint2[screenpeek];
			if (screenpeek < 0) screenpeek = connecthead;
		}
		if (keystatus[keys[14]] > 0)
		{
			keystatus[keys[14]] = 0;
			dimensionmode[screenpeek]++;
			if (dimensionmode[screenpeek] > 3) dimensionmode[screenpeek] = 1;
			if (dimensionmode[screenpeek] == 2) setview(0L,0L,xdim-1,(ydim-1)>>detailmode);
			if (dimensionmode[screenpeek] == 3) setup3dscreen();
		}
	}

	musicoff();
	uninitmultiplayers();
	uninittimer();
	uninitkeys();
	uninitengine();
	uninitsb();
	uninitgroupfile();
	setvmode(0x3);        //Set back to text mode
	exit(0);
}

setup3dscreen()
{
	long i, dax, day, dax2, day2;

	setgamemode();

	if (screensize > xdim)
	{
		dax = 0; day = 0;
		dax2 = xdim-1; day2 = ydim-1;
	}
	else
	{
		dax = (xdim>>1)-(screensize>>1);
		dax2 = dax+screensize-1;
		day = ((ydim-32)>>1)-(((screensize*(ydim-32))/xdim)>>1);
		day2 = day + ((screensize*(ydim-32))/xdim)-1;
		setview(dax,day>>detailmode,dax2,day2>>detailmode);
	}

	if (screensize < xdim)
		permanentwritespritetile(0L,0L,BACKGROUND,0,0L,0L,xdim-1,ydim-1,0);      //Draw background

	if (screensize <= xdim)
	{
		permanentwritesprite((xdim-320)>>1,ydim-32,STATUSBAR,0,0,0,xdim-1,ydim-1,0);
		i = ((xdim-320)>>1);
		while (i >= 8) i -= 8, permanentwritesprite(i,ydim-32,STATUSBARFILL8,0,0,0,xdim-1,ydim-1,0);
		if (i >= 4) i -= 4, permanentwritesprite(i,ydim-32,STATUSBARFILL4,0,0,0,xdim-1,ydim-1,0);
		i = ((xdim-320)>>1)+320;
		while (i <= xdim-8) permanentwritesprite(i,ydim-32,STATUSBARFILL8,0,0,0,xdim-1,ydim-1,0), i += 8;
		if (i <= xdim-4) permanentwritesprite(i,ydim-32,STATUSBARFILL4,0,0,0,xdim-1,ydim-1,0), i += 4;

		sprintf(&tempbuf,"Deaths: %d",deaths[screenpeek]);
		printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-16,tempbuf,ALPHABET,80);

		sprintf(&tempbuf,"Health: %3d",health[screenpeek]);
		printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-24,tempbuf,ALPHABET,80);

		sprintf(&tempbuf,"Score:%ld",score[screenpeek]);
		printext((xdim>>1)-(strlen(tempbuf)<<2),ydim-8,tempbuf,ALPHABET,80);
	}
}

findrandomspot(long *x, long *y, short *sectnum)
{
	short startwall, endwall, s;
	long dax, day, minx, maxx, miny, maxy, cnt, k;

	cnt = 256;
	while (cnt > 0)
	{
		do
		{
			k = mulscale(krand(),numsectors,16);
		} while ((sector[k].ceilingz >= sector[k].floorz) || (sector[k].lotag != 0) || ((sector[k].floorstat&2) != 0));

		startwall = sector[k].wallptr;
		endwall = startwall+sector[k].wallnum-1;
		if (endwall > startwall)
		{
			dax = 0L;
			day = 0L;
			minx = 0x7fffffff; maxx = 0x80000000;
			miny = 0x7fffffff; maxy = 0x80000000;

			for(s=startwall;s<=endwall;s++)
			{
				dax += wall[s].x;
				day += wall[s].y;
				if (wall[s].x < minx) minx = wall[s].x;
				if (wall[s].x > maxx) maxx = wall[s].x;
				if (wall[s].y < miny) miny = wall[s].y;
				if (wall[s].y > maxy) maxy = wall[s].y;
			}
			if ((maxx-minx > 256) && (maxy-miny > 256))
			{
				dax /= (endwall-startwall+1);
				day /= (endwall-startwall+1);
				if (inside(dax,day,k) == 1)
				{
					*x = dax;
					*y = day;
					*sectnum = k;
					return;
				}
			}
		}
		cnt--;
	}
}

warp(long *x, long *y, long *z, short *daang, short *dasector)
{
	short startwall, endwall, s;
	long i, j, dax, day, ox, oy;

	ox = *x; oy = *y;

	for(i=0;i<warpsectorcnt;i++)
		if (warpsectorlist[i] == *dasector)
		{
			j = sector[*dasector].hitag;
			do
			{
				i++;
				if (i >= warpsectorcnt) i = 0;
			} while (sector[warpsectorlist[i]].hitag != j);
			*dasector = warpsectorlist[i];
			break;
		}

		//Find center of sector
	startwall = sector[*dasector].wallptr;
	endwall = startwall+sector[*dasector].wallnum-1;
	dax = 0L, day = 0L;
	for(s=startwall;s<=endwall;s++)
	{
		dax += wall[s].x, day += wall[s].y;
		if (wall[s].nextsector >= 0)
			i = s;
	}
	*x = dax / (endwall-startwall+1);
	*y = day / (endwall-startwall+1);
	*z = sector[*dasector].floorz-(32<<8);
	updatesector(*x,*y,dasector);
	dax = ((wall[i].x+wall[wall[i].point2].x)>>1);
	day = ((wall[i].y+wall[wall[i].point2].y)>>1);
	*daang = getangle(dax-*x,day-*y);

	wsayfollow("warp.wav",3072L+(krand()&127)-64,192L,&ox,&oy,0);
	wsayfollow("warp.wav",4096L+(krand()&127)-64,256L,x,y,0);
}

warpsprite(short spritenum)
{
	short dasectnum;

	dasectnum = sprite[spritenum].sectnum;
	warp(&sprite[spritenum].x,&sprite[spritenum].y,&sprite[spritenum].z,
		  &sprite[spritenum].ang,&dasectnum);

	copybuf(&sprite[spritenum].x,&osprite[spritenum].x,3);
	changespritesect(spritenum,dasectnum);
}

initlava()
{
	long x, y, z, r;

	for(x=-16;x<=16;x++)
		for(y=-16;y<=16;y++)
		{
			r = ksqrt(x*x + y*y);
			lavaradx[r][lavaradcnt[r]] = x;
			lavarady[r][lavaradcnt[r]] = y;
			lavaradcnt[r]++;
		}

	for(z=0;z<16;z++)
		lavadropsizlookup[z] = 8 / (ksqrt(z)+1);

	for(z=0;z<LAVASIZ;z++)
		lavainc[z] = klabs((((z^17)>>4)&7)-4)+12;

	lavanumdrops = 0;
	lavanumframes = 0;
}

movelava(char *dapic)
{
	char dat, *ptr;
	long x, y, z, zx, dalavadropsiz, dadropsizlookup, offs, offs2;
	long dalavax, dalavay;

	z = 3;
	if (lavanumdrops+z >= LAVAMAXDROPS)
		z = LAVAMAXDROPS-lavanumdrops-1;
	while (z >= 0)
	{
		lavadropx[lavanumdrops] = (rand()&(LAVASIZ-1));
		lavadropy[lavanumdrops] = (rand()&(LAVASIZ-1));
		lavadropsiz[lavanumdrops] = 1;
		lavanumdrops++;
		z--;
	}

	z = lavanumdrops-1;
	while (z >= 0)
	{
		dadropsizlookup = lavadropsizlookup[lavadropsiz[z]]*(((z&1)<<1)-1);
		dalavadropsiz = lavadropsiz[z];
		dalavax = lavadropx[z]; dalavay = lavadropy[z];
		for(zx=lavaradcnt[lavadropsiz[z]]-1;zx>=0;zx--)
		{
			offs = (((lavaradx[dalavadropsiz][zx]+dalavax)&(LAVASIZ-1))<<LAVALOGSIZ);
			offs += ((lavarady[dalavadropsiz][zx]+dalavay)&(LAVASIZ-1));
			dapic[offs] += dadropsizlookup;
			if (dapic[offs] < 192) dapic[offs] = 192;
		}

		lavadropsiz[z]++;
		if (lavadropsiz[z] > 10)
		{
			lavanumdrops--;
			lavadropx[z] = lavadropx[lavanumdrops];
			lavadropy[z] = lavadropy[lavanumdrops];
			lavadropsiz[z] = lavadropsiz[lavanumdrops];
		}
		z--;
	}

		//Back up dapic with 1 pixel extra on each boundary
		//(to prevent anding for wrap-around)
	offs = ((long)dapic);
	offs2 = (LAVASIZ+2)+1+((long)lavabakpic);
	for(x=0;x<LAVASIZ;x++)
	{
		copybuf(offs,offs2,LAVASIZ>>2);
		offs += LAVASIZ;
		offs2 += LAVASIZ+2;
	}
	for(y=0;y<LAVASIZ;y++)
	{
		lavabakpic[y+1] = dapic[y+((LAVASIZ-1)<<LAVALOGSIZ)];
		lavabakpic[y+1+(LAVASIZ+1)*(LAVASIZ+2)] = dapic[y];
	}
	for(x=0;x<LAVASIZ;x++)
	{
		lavabakpic[(x+1)*(LAVASIZ+2)] = dapic[(x<<LAVALOGSIZ)+(LAVASIZ-1)];
		lavabakpic[(x+1)*(LAVASIZ+2)+(LAVASIZ+1)] = dapic[x<<LAVALOGSIZ];
	}
	lavabakpic[0] = dapic[LAVASIZ*LAVASIZ-1];
	lavabakpic[LAVASIZ+1] = dapic[LAVASIZ*(LAVASIZ-1)];
	lavabakpic[(LAVASIZ+2)*(LAVASIZ+1)] = dapic[LAVASIZ-1];
	lavabakpic[(LAVASIZ+2)*(LAVASIZ+2)-1] = dapic[0];

	for(z=(LAVASIZ+2)*(LAVASIZ+2)-4;z>=0;z-=4)
	{
		lavabakpic[z+0] &= 31;
		lavabakpic[z+1] &= 31;
		lavabakpic[z+2] &= 31;
		lavabakpic[z+3] &= 31;
	}

	for(x=LAVASIZ-1;x>=0;x--)
	{
		offs = (x+1)*(LAVASIZ+2)+1;
		ptr = (char *)((x<<LAVALOGSIZ) + (long)dapic);

		zx = ((x+lavanumframes)&(LAVASIZ-1));

		offs2 = LAVASIZ-1;
		for(y=offs;y<offs+LAVASIZ;y++)
		{
			dat = lavainc[(offs2--)&zx];
			dat += lavabakpic[y-(LAVASIZ+2)-1];
			dat += lavabakpic[y-(LAVASIZ+2)];
			dat += lavabakpic[y-(LAVASIZ+2)+1];
			dat += lavabakpic[y-1];
			dat += lavabakpic[y+1];
			dat += lavabakpic[y+(LAVASIZ+2)];
			dat += lavabakpic[y+(LAVASIZ+2)-1];
			*ptr++ = (dat>>3)+192;
		}
	}

	lavanumframes++;
}

doanimations()
{
	long i, j;

	for(i=animatecnt-1;i>=0;i--)
	{
		j = *animateptr[i];

		if (j < animategoal[i])
			j = min(j+animatevel[i]*TICSPERFRAME,animategoal[i]);
		else
			j = max(j-animatevel[i]*TICSPERFRAME,animategoal[i]);
		animatevel[i] += animateacc[i];

		*animateptr[i] = j;

		if (j == animategoal[i])
		{
			animatecnt--;
			if (i != animatecnt)
			{
				animateptr[i] = animateptr[animatecnt];
				animategoal[i] = animategoal[animatecnt];
				animatevel[i] = animatevel[animatecnt];
				animateacc[i] = animateacc[animatecnt];
			}
		}
	}
}

getanimationgoal(long animptr)
{
	long i;

	for(i=animatecnt-1;i>=0;i--)
		if (animptr == animateptr[i]) return(i);
	return(-1);
}

setanimation(long *animptr, long thegoal, long thevel, long theacc)
{
	long i, j;

	if (animatecnt >= MAXANIMATES) return(-1);

	j = animatecnt;
	for(i=animatecnt-1;i>=0;i--)
		if (animptr == animateptr[i])
			{ j = i; break; }

	animateptr[j] = animptr;
	animategoal[j] = thegoal;
	animatevel[j] = thevel;
	animateacc[j] = theacc;
	if (j == animatecnt) animatecnt++;
	return(j);
}

checkmasterslaveswitch()
{
	long i, j;

	if (option[4] == 0) return;

	i = connecthead; j = connectpoint2[i];
	while (j >= 0)
	{
		if ((syncbits[j]&512) > 0)
		{
			connectpoint2[i] = connectpoint2[j];
			connectpoint2[j] = connecthead;
			connecthead = (short)j;

			olocvel = locvel+1; olocvel2 = locvel2+1;
			olocsvel = locsvel+1; olocsvel2 = locsvel2+1;
			olocangvel = locangvel+1; olocangvel2 = locangvel2+1;
			olocbits = locbits+1; olocbits2 = locbits2+1;
			for(i=0;i<MAXPLAYERS;i++)
			{
				osyncvel[i] = fsyncvel[i]+1;
				osyncsvel[i] = fsyncsvel[i]+1;
				osyncangvel[i] = fsyncangvel[i]+1;
				osyncbits[i] = fsyncbits[i]+1;
			}

			syncvalplc = 0L; othersyncvalplc = 0L;
			syncvalend = 0L; othersyncvalend = 0L;
			syncvalcnt = 0L; othersyncvalcnt = 0L;

			totalclock = lockclock;
			ototalclock = lockclock;
			gotlastpacketclock = lockclock;
			masterslavetexttime = lockclock;
			ready2send = 1;
			return;
		}
		i = j; j = connectpoint2[i];
	}
}

inittimer()
{
	outp(0x43,54); outp(0x40,9942&255); outp(0x40,9942>>8);  //120 times/sec
	oldtimerhandler = _dos_getvect(0x8);
	_disable(); _dos_setvect(0x8, timerhandler); _enable();
}

uninittimer()
{
	outp(0x43,54); outp(0x40,255); outp(0x40,255);           //18.2 times/sec
	_disable(); _dos_setvect(0x8, oldtimerhandler); _enable();
}

void __interrupt __far timerhandler()
{
	totalclock++;
	outp(0x20,0x20);
}

initkeys()
{
	long i;

	keyfifoplc = 0; keyfifoend = 0;
	for(i=0;i<256;i++) keystatus[i] = 0;
	oldkeyhandler = _dos_getvect(0x9);
	_disable(); _dos_setvect(0x9, keyhandler); _enable();
}

uninitkeys()
{
	short *ptr;

	_dos_setvect(0x9, oldkeyhandler);
		//Turn off shifts to prevent stucks with quitting
	ptr = (short *)0x417; *ptr &= ~0x030f;
}

void __interrupt __far keyhandler()
{
	koutp(0x20,0x20);
	oldreadch = readch; readch = kinp(0x60);
	keytemp = kinp(0x61); koutp(0x61,keytemp|128); koutp(0x61,keytemp&127);
	if ((readch|1) == 0xe1) { extended = 128; return; }
	if (oldreadch != readch)
	{
		if ((readch&128) == 0)
		{
			keytemp = readch+extended;
			if (keystatus[keytemp] == 0)
			{
				keystatus[keytemp] = 1;
				keyfifo[keyfifoend] = keytemp;
				keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = 1;
				keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1));
			}
		}
		else
		{
			keytemp = (readch&127)+extended;
			keystatus[keytemp] = 0;
			keyfifo[keyfifoend] = keytemp;
			keyfifo[(keyfifoend+1)&(KEYFIFOSIZ-1)] = 0;
			keyfifoend = ((keyfifoend+2)&(KEYFIFOSIZ-1));
		}
	}
	extended = 0;
}

testneighborsectors(short sect1, short sect2)
{
	short i, startwall, num1, num2;

	num1 = sector[sect1].wallnum;
	num2 = sector[sect2].wallnum;
	if (num1 < num2) //Traverse walls of sector with fewest walls (for speed)
	{
		startwall = sector[sect1].wallptr;
		for(i=num1-1;i>=0;i--)
			if (wall[i+startwall].nextsector == sect2)
				return(1);
	}
	else
	{
		startwall = sector[sect2].wallptr;
		for(i=num2-1;i>=0;i--)
			if (wall[i+startwall].nextsector == sect1)
				return(1);
	}
	return(0);
}

loadgame()
{
	long i, fil;

	if ((fil = open("save0000.gam",O_BINARY|O_RDWR,S_IREAD)) == -1)
		return(-1);

	read(fil,&numplayers,2);
	read(fil,&myconnectindex,2);
	read(fil,&connecthead,2);
	read(fil,connectpoint2,MAXPLAYERS<<1);

		//Make sure palookups get set, sprites will get overwritten later
	for(i=connecthead;i>=0;i=connectpoint2[i]) initplayersprite((short)i);

	read(fil,posx,MAXPLAYERS<<2);
	read(fil,posy,MAXPLAYERS<<2);
	read(fil,posz,MAXPLAYERS<<2);
	read(fil,horiz,MAXPLAYERS<<2);
	read(fil,zoom,MAXPLAYERS<<2);
	read(fil,hvel,MAXPLAYERS<<2);
	read(fil,ang,MAXPLAYERS<<1);
	read(fil,cursectnum,MAXPLAYERS<<1);
	read(fil,ocursectnum,MAXPLAYERS<<1);
	read(fil,playersprite,MAXPLAYERS<<1);
	read(fil,deaths,MAXPLAYERS<<1);
	read(fil,lastchaingun,MAXPLAYERS<<2);
	read(fil,health,MAXPLAYERS<<2);
	read(fil,score,MAXPLAYERS<<2);
	read(fil,saywatchit,MAXPLAYERS<<2);
	read(fil,numbombs,MAXPLAYERS<<1);
	read(fil,&screensize,2);
	read(fil,oflags,MAXPLAYERS<<1);
	read(fil,dimensionmode,MAXPLAYERS);
	read(fil,revolvedoorstat,MAXPLAYERS);
	read(fil,revolvedoorang,MAXPLAYERS<<1);
	read(fil,revolvedoorrotang,MAXPLAYERS<<1);
	read(fil,revolvedoorx,MAXPLAYERS<<2);
	read(fil,revolvedoory,MAXPLAYERS<<2);

	read(fil,&numsectors,2);
	read(fil,sector,sizeof(sectortype)*numsectors);

	read(fil,&numwalls,2);
	read(fil,wall,sizeof(walltype)*numwalls);

		//Store all sprites (even holes) to preserve indeces
	read(fil,sprite,sizeof(spritetype)*MAXSPRITES);
	read(fil,headspritesect,(MAXSECTORS+1)<<1);
	read(fil,prevspritesect,MAXSPRITES<<1);
	read(fil,nextspritesect,MAXSPRITES<<1);
	read(fil,headspritestat,(MAXSTATUS+1)<<1);
	read(fil,prevspritestat,MAXSPRITES<<1);
	read(fil,nextspritestat,MAXSPRITES<<1);

	read(fil,&vel,4);
	read(fil,&svel,4);
	read(fil,&angvel,4);

	read(fil,&locselectedgun,4);
	read(fil,&locvel,1);
	read(fil,&olocvel,1);
	read(fil,&locsvel,1);
	read(fil,&olocsvel,1);
	read(fil,&locangvel,1);
	read(fil,&olocangvel,1);
	read(fil,&locbits,2);
	read(fil,&olocbits,2);

	read(fil,&locselectedgun2,4);
	read(fil,&locvel2,1);
	read(fil,&olocvel2,1);
	read(fil,&locsvel2,1);
	read(fil,&olocsvel2,1);
	read(fil,&locangvel2,1);
	read(fil,&olocangvel2,1);
	read(fil,&locbits2,2);
	read(fil,&olocbits2,2);

	read(fil,syncvel,MAXPLAYERS);
	read(fil,osyncvel,MAXPLAYERS);
	read(fil,syncsvel,MAXPLAYERS);
	read(fil,osyncsvel,MAXPLAYERS);
	read(fil,syncangvel,MAXPLAYERS);
	read(fil,osyncangvel,MAXPLAYERS);
	read(fil,syncbits,MAXPLAYERS<<1);
	read(fil,osyncbits,MAXPLAYERS<<1);

	read(fil,boardfilename,80);
	read(fil,&screenpeek,2);
	read(fil,&oldmousebstatus,2);
	read(fil,&brightness,2);
	read(fil,&neartagsector,2);
	read(fil,&neartagwall,2);
	read(fil,&neartagsprite,2);
	read(fil,&lockclock,4);
	read(fil,&neartagdist,4);
	read(fil,&neartaghitdist,4);
	read(fil,&masterslavetexttime,4);

	read(fil,rotatespritelist,16<<1);
	read(fil,&rotatespritecnt,2);
	read(fil,warpsectorlist,16<<1);
	read(fil,&warpsectorcnt,2);
	read(fil,xpanningsectorlist,16<<1);
	read(fil,&xpanningsectorcnt,2);
	read(fil,ypanningwalllist,64<<1);
	read(fil,&ypanningwallcnt,2);
	read(fil,floorpanninglist,64<<1);
	read(fil,&floorpanningcnt,2);
	read(fil,dragsectorlist,16<<1);
	read(fil,dragxdir,16<<1);
	read(fil,dragydir,16<<1);
	read(fil,&dragsectorcnt,2);
	read(fil,dragx1,16<<2);
	read(fil,dragy1,16<<2);
	read(fil,dragx2,16<<2);
	read(fil,dragy2,16<<2);
	read(fil,dragfloorz,16<<2);
	read(fil,&swingcnt,2);
	read(fil,swingwall,(32*5)<<1);
	read(fil,swingsector,32<<1);
	read(fil,swingangopen,32<<1);
	read(fil,swingangclosed,32<<1);
	read(fil,swingangopendir,32<<1);
	read(fil,swingang,32<<1);
	read(fil,swinganginc,32<<1);
	read(fil,swingx,(32*8)<<2);
	read(fil,swingy,(32*8)<<2);
	read(fil,revolvesector,4<<1);
	read(fil,revolveang,4<<1);
	read(fil,&revolvecnt,2);
	read(fil,revolvex,(4*16)<<2);
	read(fil,revolvey,(4*16)<<2);
	read(fil,revolvepivotx,4<<2);
	read(fil,revolvepivoty,4<<2);
	read(fil,subwaytracksector,(4*128)<<1);
	read(fil,subwaynumsectors,4<<1);
	read(fil,&subwaytrackcnt,2);
	read(fil,subwaystop,(4*8)<<2);
	read(fil,subwaystopcnt,4<<2);
	read(fil,subwaytrackx1,4<<2);
	read(fil,subwaytracky1,4<<2);
	read(fil,subwaytrackx2,4<<2);
	read(fil,subwaytracky2,4<<2);
	read(fil,subwayx,4<<2);
	read(fil,subwaygoalstop,4<<2);
	read(fil,subwayvel,4<<2);
	read(fil,subwaypausetime,4<<2);
	read(fil,waterfountainwall,MAXPLAYERS<<1);
	read(fil,waterfountaincnt,MAXPLAYERS<<1);
	read(fil,slimesoundcnt,MAXPLAYERS<<1);

		//Warning: only works if all pointers are in sector structures!
	read(fil,animateptr,MAXANIMATES<<2);
	for(i=MAXANIMATES-1;i>=0;i--)
		animateptr[i] = (long *)(animateptr[i]+((long)sector));
	read(fil,animategoal,MAXANIMATES<<2);
	read(fil,animatevel,MAXANIMATES<<2);
	read(fil,animateacc,MAXANIMATES<<2);
	read(fil,&animatecnt,4);

	read(fil,&totalclock,4);
	read(fil,&numframes,4);
	read(fil,&randomseed,4);
	read(fil,startumost,MAXXDIM<<1);
	read(fil,startdmost,MAXXDIM<<1);
	read(fil,&numpalookups,2);

	read(fil,&visibility,4);
	read(fil,&parallaxtype,1);
	read(fil,&parallaxyoffs,4);
	read(fil,pskyoff,MAXPSKYTILES<<1);
	read(fil,&pskybits,2);

	read(fil,show2dsector,MAXSECTORS>>3);
	read(fil,show2dwall,MAXWALLS>>3);
	read(fil,show2dsprite,MAXSPRITES>>3);
	read(fil,&automapping,1);

	close(fil);

	for(i=connecthead;i>=0;i=connectpoint2[i]) initplayersprite((short)i);

	totalclock = lockclock;
	ototalclock = lockclock;

	strcpy(getmessage,"Game loaded.");
	getmessageleng = strlen(getmessage);
	getmessagetimeoff = totalclock+360+(getmessageleng<<4);
	return(0);
}

savegame()
{
	long i, fil;

	if ((fil = open("save0000.gam",O_BINARY|O_TRUNC|O_CREAT|O_WRONLY,S_IWRITE)) == -1)
		return(-1);

	write(fil,&numplayers,2);
	write(fil,&myconnectindex,2);
	write(fil,&connecthead,2);
	write(fil,connectpoint2,MAXPLAYERS<<1);

	write(fil,posx,MAXPLAYERS<<2);
	write(fil,posy,MAXPLAYERS<<2);
	write(fil,posz,MAXPLAYERS<<2);
	write(fil,horiz,MAXPLAYERS<<2);
	write(fil,zoom,MAXPLAYERS<<2);
	write(fil,hvel,MAXPLAYERS<<2);
	write(fil,ang,MAXPLAYERS<<1);
	write(fil,cursectnum,MAXPLAYERS<<1);
	write(fil,ocursectnum,MAXPLAYERS<<1);
	write(fil,playersprite,MAXPLAYERS<<1);
	write(fil,deaths,MAXPLAYERS<<1);
	write(fil,lastchaingun,MAXPLAYERS<<2);
	write(fil,health,MAXPLAYERS<<2);
	write(fil,score,MAXPLAYERS<<2);
	write(fil,saywatchit,MAXPLAYERS<<2);
	write(fil,numbombs,MAXPLAYERS<<1);
	write(fil,&screensize,2);
	write(fil,oflags,MAXPLAYERS<<1);
	write(fil,dimensionmode,MAXPLAYERS);
	write(fil,revolvedoorstat,MAXPLAYERS);
	write(fil,revolvedoorang,MAXPLAYERS<<1);
	write(fil,revolvedoorrotang,MAXPLAYERS<<1);
	write(fil,revolvedoorx,MAXPLAYERS<<2);
	write(fil,revolvedoory,MAXPLAYERS<<2);

	write(fil,&numsectors,2);
	write(fil,sector,sizeof(sectortype)*numsectors);

	write(fil,&numwalls,2);
	write(fil,wall,sizeof(walltype)*numwalls);

		//Store all sprites (even holes) to preserve indeces
	write(fil,sprite,sizeof(spritetype)*MAXSPRITES);
	write(fil,headspritesect,(MAXSECTORS+1)<<1);
	write(fil,prevspritesect,MAXSPRITES<<1);
	write(fil,nextspritesect,MAXSPRITES<<1);
	write(fil,headspritestat,(MAXSTATUS+1)<<1);
	write(fil,prevspritestat,MAXSPRITES<<1);
	write(fil,nextspritestat,MAXSPRITES<<1);

	write(fil,&vel,4);
	write(fil,&svel,4);
	write(fil,&angvel,4);

	write(fil,&locselectedgun,4);
	write(fil,&locvel,1);
	write(fil,&olocvel,1);
	write(fil,&locsvel,1);
	write(fil,&olocsvel,1);
	write(fil,&locangvel,1);
	write(fil,&olocangvel,1);
	write(fil,&locbits,2);
	write(fil,&olocbits,2);

	write(fil,&locselectedgun2,4);
	write(fil,&locvel2,1);
	write(fil,&olocvel2,1);
	write(fil,&locsvel2,1);
	write(fil,&olocsvel2,1);
	write(fil,&locangvel2,1);
	write(fil,&olocangvel2,1);
	write(fil,&locbits2,2);
	write(fil,&olocbits2,2);

	write(fil,syncvel,MAXPLAYERS);
	write(fil,osyncvel,MAXPLAYERS);
	write(fil,syncsvel,MAXPLAYERS);
	write(fil,osyncsvel,MAXPLAYERS);
	write(fil,syncangvel,MAXPLAYERS);
	write(fil,osyncangvel,MAXPLAYERS);
	write(fil,syncbits,MAXPLAYERS<<1);
	write(fil,osyncbits,MAXPLAYERS<<1);

	write(fil,boardfilename,80);
	write(fil,&screenpeek,2);
	write(fil,&oldmousebstatus,2);
	write(fil,&brightness,2);
	write(fil,&neartagsector,2);
	write(fil,&neartagwall,2);
	write(fil,&neartagsprite,2);
	write(fil,&lockclock,4);
	write(fil,&neartagdist,4);
	write(fil,&neartaghitdist,4);
	write(fil,&masterslavetexttime,4);

	write(fil,rotatespritelist,16<<1);
	write(fil,&rotatespritecnt,2);
	write(fil,warpsectorlist,16<<1);
	write(fil,&warpsectorcnt,2);
	write(fil,xpanningsectorlist,16<<1);
	write(fil,&xpanningsectorcnt,2);
	write(fil,ypanningwalllist,64<<1);
	write(fil,&ypanningwallcnt,2);
	write(fil,floorpanninglist,64<<1);
	write(fil,&floorpanningcnt,2);
	write(fil,dragsectorlist,16<<1);
	write(fil,dragxdir,16<<1);
	write(fil,dragydir,16<<1);
	write(fil,&dragsectorcnt,2);
	write(fil,dragx1,16<<2);
	write(fil,dragy1,16<<2);
	write(fil,dragx2,16<<2);
	write(fil,dragy2,16<<2);
	write(fil,dragfloorz,16<<2);
	write(fil,&swingcnt,2);
	write(fil,swingwall,(32*5)<<1);
	write(fil,swingsector,32<<1);
	write(fil,swingangopen,32<<1);
	write(fil,swingangclosed,32<<1);
	write(fil,swingangopendir,32<<1);
	write(fil,swingang,32<<1);
	write(fil,swinganginc,32<<1);
	write(fil,swingx,(32*8)<<2);
	write(fil,swingy,(32*8)<<2);
	write(fil,revolvesector,4<<1);
	write(fil,revolveang,4<<1);
	write(fil,&revolvecnt,2);
	write(fil,revolvex,(4*16)<<2);
	write(fil,revolvey,(4*16)<<2);
	write(fil,revolvepivotx,4<<2);
	write(fil,revolvepivoty,4<<2);
	write(fil,subwaytracksector,(4*128)<<1);
	write(fil,subwaynumsectors,4<<1);
	write(fil,&subwaytrackcnt,2);
	write(fil,subwaystop,(4*8)<<2);
	write(fil,subwaystopcnt,4<<2);
	write(fil,subwaytrackx1,4<<2);
	write(fil,subwaytracky1,4<<2);
	write(fil,subwaytrackx2,4<<2);
	write(fil,subwaytracky2,4<<2);
	write(fil,subwayx,4<<2);
	write(fil,subwaygoalstop,4<<2);
	write(fil,subwayvel,4<<2);
	write(fil,subwaypausetime,4<<2);
	write(fil,waterfountainwall,MAXPLAYERS<<1);
	write(fil,waterfountaincnt,MAXPLAYERS<<1);
	write(fil,slimesoundcnt,MAXPLAYERS<<1);

		//Warning: only works if all pointers are in sector structures!
	for(i=MAXANIMATES-1;i>=0;i--)
		animateptr[i] = (long *)(animateptr[i]-((long)sector));
	write(fil,animateptr,MAXANIMATES<<2);
	for(i=MAXANIMATES-1;i>=0;i--)
		animateptr[i] = (long *)(animateptr[i]+((long)sector));
	write(fil,animategoal,MAXANIMATES<<2);
	write(fil,animatevel,MAXANIMATES<<2);
	write(fil,animateacc,MAXANIMATES<<2);
	write(fil,&animatecnt,4);

	write(fil,&totalclock,4);
	write(fil,&numframes,4);
	write(fil,&randomseed,4);
	write(fil,startumost,MAXXDIM<<1);
	write(fil,startdmost,MAXXDIM<<1);
	write(fil,&numpalookups,2);

	write(fil,&visibility,4);
	write(fil,&parallaxtype,1);
	write(fil,&parallaxyoffs,4);
	write(fil,pskyoff,MAXPSKYTILES<<1);
	write(fil,&pskybits,2);

	write(fil,show2dsector,MAXSECTORS>>3);
	write(fil,show2dwall,MAXWALLS>>3);
	write(fil,show2dsprite,MAXSPRITES>>3);
	write(fil,&automapping,1);

	close(fil);

	strcpy(getmessage,"Game saved.");
	getmessageleng = strlen(getmessage);
	getmessagetimeoff = totalclock+360+(getmessageleng<<4);
	return(0);
}

faketimerhandler()
{
	short other, tempbufleng;
	long i, j, k, l;

	if (totalclock < ototalclock+TICSPERFRAME) return;
	if (ready2send == 0) return;
	ototalclock = totalclock;

		//I am the MASTER (or 1 player game)
	if ((myconnectindex == connecthead) || (option[4] == 0))
	{
		if (option[4] != 0)
			getpackets();

		if (getoutputcirclesize() < 16)
		{
			getinput();
			fsyncvel[myconnectindex] = locvel;
			fsyncsvel[myconnectindex] = locsvel;
			fsyncangvel[myconnectindex] = locangvel;
			fsyncbits[myconnectindex] = locbits;

			if (option[4] != 0)
			{
				tempbuf[0] = 0;
				j = ((numplayers+1)>>1)+1;
				for(k=1;k<j;k++) tempbuf[k] = 0;
				k = (1<<3);
				for(i=connecthead;i>=0;i=connectpoint2[i])
				{
					l = 0;
					if (fsyncvel[i] != osyncvel[i]) tempbuf[j++] = fsyncvel[i], l |= 1;
					if (fsyncsvel[i] != osyncsvel[i]) tempbuf[j++] = fsyncsvel[i], l |= 2;
					if (fsyncangvel[i] != osyncangvel[i]) tempbuf[j++] = fsyncangvel[i], l |= 4;
					if (fsyncbits[i] != osyncbits[i])
					{
						tempbuf[j++] = (fsyncbits[i]&255);
						tempbuf[j++] = ((fsyncbits[i]>>8)&255);
						l |= 8;
					}
					tempbuf[k>>3] |= (l<<(k&7));
					k += 4;

					osyncvel[i] = fsyncvel[i];
					osyncsvel[i] = fsyncsvel[i];
					osyncangvel[i] = fsyncangvel[i];
					osyncbits[i] = fsyncbits[i];
				}

				while (syncvalplc != syncvalend)
				{
					tempbuf[j] = (char)(syncval[syncvalplc]&255);
					tempbuf[j+1] = (char)((syncval[syncvalplc]>>8)&255);
					j += 2;
					syncvalplc = ((syncvalplc+1)&(MOVEFIFOSIZ-1));
				}

				for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
					sendpacket(i,tempbuf,j);
			}
			else if (numplayers == 2)
			{
				if (keystatus[0xb5] > 0)
				{
					keystatus[0xb5] = 0;
					locselectedgun2++;
					if (locselectedgun2 >= 3) locselectedgun2 = 0;
				}

					//Second player on 1 computer mode
				locvel2 = min(max(vel2,-128+8),127-8);
				locsvel2 = min(max(svel2,-128+8),127-8);
				locangvel2 = min(max(angvel2,-128+16),127-16);
				locbits2 = (locselectedgun2<<13);
				locbits2 |= keystatus[0x45];                  //Stand high
				locbits2 |= (keystatus[0x47]<<1);             //Stand low
				locbits2 |= (1<<8);                           //Run
				locbits2 |= (keystatus[0x49]<<2);             //Look up
				locbits2 |= (keystatus[0x37]<<3);             //Look down
				locbits2 |= (keystatus[0x50]<<10);            //Space
				locbits2 |= (keystatus[0x52]<<11);            //Shoot

				other = connectpoint2[myconnectindex];
				if (other < 0) other = connecthead;

				fsyncvel[other] = locvel2;
				fsyncsvel[other] = locsvel2;
				fsyncangvel[other] = locangvel2;
				fsyncbits[other] = locbits2;
			}
			movethings();  //Move EVERYTHING (you too!)
		}
	}
	else                        //I am a SLAVE
	{
		getpackets();

		if (getoutputcirclesize() < 16)
		{
			getinput();

			tempbuf[0] = 1; k = 0;
			j = 2;

			if (locvel != olocvel) tempbuf[j++] = locvel, k |= 1;
			if (locsvel != olocsvel) tempbuf[j++] = locsvel, k |= 2;
			if (locangvel != olocangvel) tempbuf[j++] = locangvel, k |= 4;
			if ((locbits^olocbits)&0x00ff) tempbuf[j++] = (locbits&255), k |= 8;
			if ((locbits^olocbits)&0xff00) tempbuf[j++] = ((locbits>>8)&255), k |= 16;

			tempbuf[1] = k;

			olocvel = locvel;
			olocsvel = locsvel;
			olocangvel = locangvel;
			olocbits = locbits;

			sendpacket(connecthead,tempbuf,j);
		}
	}
}

getpackets()
{
	long i, j, k, l;
	short other, tempbufleng;

	if (option[4] == 0) return;

	while ((tempbufleng = getpacket(&other,tempbuf)) > 0)
	{
		switch(tempbuf[0])
		{
			case 0:  //[0] (receive master sync buffer)
				j = ((numplayers+1)>>1)+1; k = (1<<3);
				for(i=connecthead;i>=0;i=connectpoint2[i])
				{
					l = (tempbuf[k>>3]>>(k&7));
					if (l&1) fsyncvel[i] = tempbuf[j++];
					if (l&2) fsyncsvel[i] = tempbuf[j++];
					if (l&4) fsyncangvel[i] = tempbuf[j++];
					if (l&8)
					{
						fsyncbits[i] = ((short)tempbuf[j])+(((short)tempbuf[j+1])<<8);
						j += 2;
					}
					k += 4;
				}
				while (j != tempbufleng)
				{
					othersyncval[othersyncvalend] = ((long)tempbuf[j]);
					othersyncval[othersyncvalend] += (((long)tempbuf[j+1])<<8);
					j += 2;
					othersyncvalend = ((othersyncvalend+1)&(MOVEFIFOSIZ-1));
				}

				i = 0;
				while (syncvalplc != syncvalend)
				{
					if (othersyncvalcnt > syncvalcnt)
					{
						if (i == 0) syncstat = 0, i = 1;
						syncstat |= (syncval[syncvalplc]^othersyncval[syncvalplc]);
					}
					syncvalplc = ((syncvalplc+1)&(MOVEFIFOSIZ-1));
					syncvalcnt++;
				}
				while (othersyncvalplc != othersyncvalend)
				{
					if (syncvalcnt > othersyncvalcnt)
					{
						if (i == 0) syncstat = 0, i = 1;
						syncstat |= (syncval[othersyncvalplc]^othersyncval[othersyncvalplc]);
					}
					othersyncvalplc = ((othersyncvalplc+1)&(MOVEFIFOSIZ-1));
					othersyncvalcnt++;
				}

				movethings();        //Move all players and sprites
				break;
			case 1:  //[1] (receive slave sync buffer)
				j = 2; k = tempbuf[1];
				if (k&1) fsyncvel[other] = tempbuf[j++];
				if (k&2) fsyncsvel[other] = tempbuf[j++];
				if (k&4) fsyncangvel[other] = tempbuf[j++];
				if (k&8) fsyncbits[other] = ((fsyncbits[other]&0xff00)|((short)tempbuf[j++]));
				if (k&16) fsyncbits[other] = ((fsyncbits[other]&0x00ff)|(((short)tempbuf[j++])<<8));
				break;
			case 2:
				getmessageleng = tempbufleng-1;
				for(j=getmessageleng-1;j>=0;j--) getmessage[j] = tempbuf[j+1];
				getmessagetimeoff = totalclock+360+(getmessageleng<<4);
				break;
			case 3:
				wsay("getstuff.wav",4096L,63L,63L);
				break;
			case 5:
				playerreadyflag[other] = tempbuf[1];
				if ((other == connecthead) && (tempbuf[1] == 2))
					sendpacket(connecthead,tempbuf,2);
				break;
			case 255:  //[255] (logout)
				keystatus[1] = 1;
				break;
		}
	}
}

drawoverheadmap(long cposx, long cposy, long czoom, short cang)
{
	long i, j, k, l, x1, y1, x2, y2, x3, y3, x4, y4, ox, oy, xoff, yoff;
	long dax, day, cosang, sinang, xspan, yspan, sprx, spry;
	long xrepeat, yrepeat, z1, z2, startwall, endwall, tilenum, daang;
	long xvect, yvect, xvect2, yvect2;
	char col;
	walltype *wal, *wal2;
	spritetype *spr;

	xvect = sintable[(2048-cang)&2047] * czoom;
	yvect = sintable[(1536-cang)&2047] * czoom;
	xvect2 = mulscale(xvect,yxaspect,16);
	yvect2 = mulscale(yvect,yxaspect,16);

		//Draw red lines
	for(i=0;i<numsectors;i++)
	{
		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum - 1;

		z1 = sector[i].ceilingz; z2 = sector[i].floorz;

		for(j=startwall,wal=&wall[startwall];j<=endwall;j++,wal++)
		{
			k = wal->nextwall; if (k < 0) continue;

			if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;
			if ((k > j) && ((show2dwall[k>>3]&(1<<(k&7))) > 0)) continue;

			if (sector[wal->nextsector].ceilingz == z1)
				if (sector[wal->nextsector].floorz == z2)
					if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0) continue;

			col = 152;

			if (dimensionmode[screenpeek] == 2)
			{
				if (sector[i].floorz != sector[i].ceilingz)
					if (sector[wal->nextsector].floorz != sector[wal->nextsector].ceilingz)
						if (((wal->cstat|wall[wal->nextwall].cstat)&(16+32)) == 0)
							if (sector[i].floorz == sector[wal->nextsector].floorz) continue;
				if (sector[i].floorpicnum != sector[wal->nextsector].floorpicnum) continue;
				if (sector[i].floorshade != sector[wal->nextsector].floorshade) continue;
				col = 12;
			}

			ox = wal->x-cposx; oy = wal->y-cposy;
			x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
			y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

			wal2 = &wall[wal->point2];
			ox = wal2->x-cposx; oy = wal2->y-cposy;
			x2 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
			y2 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

			drawline256(x1+(xdim<<11),y1+(ydim<<11),x2+(xdim<<11),y2+(ydim<<11),col);
		}
	}

		//Draw sprites
	k = playersprite[screenpeek];
	for(i=0;i<numsectors;i++)
		for(j=headspritesect[i];j>=0;j=nextspritesect[j])
			if ((show2dsprite[j>>3]&(1<<(j&7))) > 0)
			{
				spr = &sprite[j];
				col = 56;
				if ((spr->cstat&1) > 0) col = 248;
				if (j == k) col = 31;

				sprx = spr->x;
				spry = spr->y;

				k = spr->statnum;
				if ((k >= 1) && (k <= 8) && (k != 2))  //Interpolate moving sprite
				{
					sprx = osprite[j].x+mulscale(sprx-osprite[j].x,smoothratio,16);
					spry = osprite[j].y+mulscale(spry-osprite[j].y,smoothratio,16);
				}

				switch (spr->cstat&48)
				{
					case 0:
						ox = sprx-cposx; oy = spry-cposy;
						x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
						y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

						if (dimensionmode[screenpeek] == 1)
						{
							ox = (sintable[(spr->ang+512)&2047]>>7);
							oy = (sintable[(spr->ang)&2047]>>7);
							x2 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
							y2 = mulscale(oy,xvect,16) + mulscale(ox,yvect,16);

							if (j == playersprite[screenpeek])
							{
								x2 = 0L;
								y2 = -(czoom<<5);
							}

							x3 = mulscale(x2,yxaspect,16);
							y3 = mulscale(y2,yxaspect,16);

							drawline256(x1-x2+(xdim<<11),y1-y3+(ydim<<11),
											x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
							drawline256(x1-y2+(xdim<<11),y1+x3+(ydim<<11),
											x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
							drawline256(x1+y2+(xdim<<11),y1-x3+(ydim<<11),
											x1+x2+(xdim<<11),y1+y3+(ydim<<11),col);
						}
						else
						{
							if (((gotsector[i>>3]&(1<<(i&7))) > 0) && (czoom > 192))
							{
								daang = (spr->ang-cang)&2047;
								if (j == playersprite[screenpeek])
									{ x1 = 0; y1 = (yxaspect<<2); daang = 0; }
								rotatesprite((x1<<4)+(xdim<<15),(y1<<4)+(ydim<<15),mulscale(czoom*spr->yrepeat,yxaspect,16),daang,spr->picnum,spr->shade,spr->pal,(spr->cstat&2)>>1);
							}
						}
						break;
					case 16:
						x1 = sprx; y1 = spry;
						tilenum = spr->picnum;
						xoff = (long)((signed char)((picanm[tilenum]>>8)&255))+((long)spr->xoffset);
						if ((spr->cstat&4) > 0) xoff = -xoff;
						k = spr->ang; l = spr->xrepeat;
						dax = sintable[k&2047]*l; day = sintable[(k+1536)&2047]*l;
						l = tilesizx[tilenum]; k = (l>>1)+xoff;
						x1 -= mulscale(dax,k,16); x2 = x1+mulscale(dax,l,16);
						y1 -= mulscale(day,k,16); y2 = y1+mulscale(day,l,16);

						ox = x1-cposx; oy = y1-cposy;
						x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
						y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

						ox = x2-cposx; oy = y2-cposy;
						x2 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
						y2 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

						drawline256(x1+(xdim<<11),y1+(ydim<<11),
										x2+(xdim<<11),y2+(ydim<<11),col);

						break;
					case 32:
						if (dimensionmode[screenpeek] == 1)
						{
							tilenum = spr->picnum;
							xoff = (long)((signed char)((picanm[tilenum]>>8)&255))+((long)spr->xoffset);
							yoff = (long)((signed char)((picanm[tilenum]>>16)&255))+((long)spr->yoffset);
							if ((spr->cstat&4) > 0) xoff = -xoff;
							if ((spr->cstat&8) > 0) yoff = -yoff;

							k = spr->ang;
							cosang = sintable[(k+512)&2047]; sinang = sintable[k];
							xspan = tilesizx[tilenum]; xrepeat = spr->xrepeat;
							yspan = tilesizy[tilenum]; yrepeat = spr->yrepeat;

							dax = ((xspan>>1)+xoff)*xrepeat; day = ((yspan>>1)+yoff)*yrepeat;
							x1 = sprx + mulscale(sinang,dax,16) + mulscale(cosang,day,16);
							y1 = spry + mulscale(sinang,day,16) - mulscale(cosang,dax,16);
							l = xspan*xrepeat;
							x2 = x1 - mulscale(sinang,l,16);
							y2 = y1 + mulscale(cosang,l,16);
							l = yspan*yrepeat;
							k = -mulscale(cosang,l,16); x3 = x2+k; x4 = x1+k;
							k = -mulscale(sinang,l,16); y3 = y2+k; y4 = y1+k;

							ox = x1-cposx; oy = y1-cposy;
							x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
							y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

							ox = x2-cposx; oy = y2-cposy;
							x2 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
							y2 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

							ox = x3-cposx; oy = y3-cposy;
							x3 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
							y3 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

							ox = x4-cposx; oy = y4-cposy;
							x4 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
							y4 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

							drawline256(x1+(xdim<<11),y1+(ydim<<11),
											x2+(xdim<<11),y2+(ydim<<11),col);

							drawline256(x2+(xdim<<11),y2+(ydim<<11),
											x3+(xdim<<11),y3+(ydim<<11),col);

							drawline256(x3+(xdim<<11),y3+(ydim<<11),
											x4+(xdim<<11),y4+(ydim<<11),col);

							drawline256(x4+(xdim<<11),y4+(ydim<<11),
											x1+(xdim<<11),y1+(ydim<<11),col);

						}
						break;
				}
			}

		//Draw white lines
	for(i=0;i<numsectors;i++)
	{
		startwall = sector[i].wallptr;
		endwall = sector[i].wallptr + sector[i].wallnum - 1;

		for(j=startwall,wal=&wall[startwall];j<=endwall;j++,wal++)
		{
			if (wal->nextwall >= 0) continue;

			if ((show2dwall[j>>3]&(1<<(j&7))) == 0) continue;

			if (tilesizx[wal->picnum] == 0) continue;
			if (tilesizy[wal->picnum] == 0) continue;

			ox = wal->x-cposx; oy = wal->y-cposy;
			x1 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
			y1 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

			wal2 = &wall[wal->point2];
			ox = wal2->x-cposx; oy = wal2->y-cposy;
			x2 = mulscale(ox,xvect,16) - mulscale(oy,yvect,16);
			y2 = mulscale(oy,xvect2,16) + mulscale(ox,yvect2,16);

			drawline256(x1+(xdim<<11),y1+(ydim<<11),x2+(xdim<<11),y2+(ydim<<11),24);
		}
	}
}

	//New movesprite using getzrange.  Note that I made the getzrange
	//parameters global (&globhiz,&globhihit,&globloz,&globlohit) so they
	//don't need to be passed everywhere.  Also this should make this
	//movesprite function compatible with the older movesprite functions.
movesprite(short spritenum, long dx, long dy, long dz, long ceildist, long flordist, char cliptype)
{
	long daz, zoffs, templong;
	short retval, dasectnum, tempshort;
	spritetype *spr;

	spr = &sprite[spritenum];

	if ((spr->cstat&128) == 0)
		zoffs = -((tilesizy[spr->picnum]*spr->yrepeat)<<1);
	else
		zoffs = 0;

	dasectnum = spr->sectnum;  //Can't modify sprite sectors directly becuase of linked lists
	daz = spr->z+zoffs;  //Must do this if not using the new centered centering (of course)
	retval = clipmove(&spr->x,&spr->y,&daz,&dasectnum,dx,dy,
							((long)spr->clipdist)<<2,ceildist,flordist,cliptype);

	if ((dasectnum != spr->sectnum) && (dasectnum >= 0))
		changespritesect(spritenum,dasectnum);

		//Set the blocking bit to 0 temporarly so getzrange doesn't pick up
		//its own sprite
	tempshort = spr->cstat; spr->cstat &= ~1;
	getzrange(spr->x,spr->y,spr->z-1,spr->sectnum,
				 &globhiz,&globhihit,&globloz,&globlohit,
				 ((long)spr->clipdist)<<2,cliptype);
	spr->cstat = tempshort;

	daz = spr->z+zoffs + dz;
	if ((daz <= globhiz) || (daz > globloz))
	{
		if (retval != 0) return(retval);
		return(16384+dasectnum);
	}
	spr->z = daz-zoffs;
	return(retval);
}

waitforeverybody()
{
	long i, j, oldtotalclock;

	if (numplayers < 2) return;

	if (myconnectindex == connecthead)
	{
		for(j=1;j<=2;j++)
		{
			for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
				playerreadyflag[i] = 0;
			oldtotalclock = totalclock-8;
			do
			{
				getpackets();
				if (totalclock >= oldtotalclock+8)
				{
					oldtotalclock = totalclock;
					tempbuf[0] = 5;
					tempbuf[1] = j;
					for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
						if (playerreadyflag[i] != j) sendpacket(i,tempbuf,2);
				}
				for(i=connectpoint2[connecthead];i>=0;i=connectpoint2[i])
					if (playerreadyflag[i] != j) break;
			} while (i >= 0);
		}
	}
	else
	{
		playerreadyflag[connecthead] = 0;
		while (playerreadyflag[connecthead] != 2)
		{
			getpackets();
			if (playerreadyflag[connecthead] == 1)
			{
				playerreadyflag[connecthead] = 0;
				sendpacket(connecthead,tempbuf,2);
			}
		}
	}
}

getsyncstat()
{
	long i, j;
	unsigned short crc;
	spritetype *spr;

	crc = 0;
	updatecrc16(crc,randomseed); updatecrc16(crc,randomseed>>8);
	for(i=connecthead;i>=0;i=connectpoint2[i])
	{
		updatecrc16(crc,posx[i]); //if (syncstat != 0) printf("%ld ",posx[i]);
		updatecrc16(crc,posy[i]); //if (syncstat != 0) printf("%ld ",posy[i]);
		updatecrc16(crc,posz[i]); //if (syncstat != 0) printf("%ld ",posz[i]);
		updatecrc16(crc,ang[i]); //if (syncstat != 0) printf("%ld ",ang[i]);
		updatecrc16(crc,horiz[i]); //if (syncstat != 0) printf("%ld ",horiz[i]);
		updatecrc16(crc,health[i]); //if (syncstat != 0) printf("%ld ",health[i]);
	}

	for(i=7;i>=0;i--)
		for(j=headspritestat[i];j>=0;j=nextspritestat[j])
		{
			spr = &sprite[j];
			updatecrc16(crc,spr->x); //if (syncstat != 0) printf("%ld ",spr->x);
			updatecrc16(crc,spr->y); //if (syncstat != 0) printf("%ld ",spr->y);
			updatecrc16(crc,spr->z); //if (syncstat != 0) printf("%ld ",spr->z);
			updatecrc16(crc,spr->ang); //if (syncstat != 0) printf("%ld ",spr->ang);
		}
	//if (syncstat != 0) printf("\n");
	return(crc);
}
