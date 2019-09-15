/**************************************************************************

	WOW/Votrax SC-01 Emulator

 	Mike@Dissfulfils.co.uk

        Modified to match phonemes to words

        Ajudd@quantime.co.uk

**************************************************************************

wow_sh_start  - Start emulation, load samples from Votrax subdirectory
wow_sh_stop   - End emulation, free memory used for samples
wow_sh_w      - Write data to votrax port
wow_sh_status - Return busy status (-1 = busy)
wow_port_2_r  - Returns status of voice port
wow_sh_ update- Null

If you need to alter the base frequency (i.e. Qbert) then just alter
the variable wowBaseFrequency, this is defaulted to 8000

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"

int	wowBaseFrequency;		/* Some games (Qbert) change this */
int 	wowBaseVolume;
int 	wowChannel = 0;
struct  GameSamples *wowSamples;

/****************************************************************************
 * 64 Phonemes - currently 1 sample per phoneme, will be combined sometime!
 ****************************************************************************/

static const char *PhonemeTable[65] =
{
 "EH3","EH2","EH1","PA0","DT" ,"A1" ,"A2" ,"ZH",
 "AH2","I3" ,"I2" ,"I1" ,"M"  ,"N"  ,"B"  ,"V",
 "CH" ,"SH" ,"Z"  ,"AW1","NG" ,"AH1","OO1","OO",
 "L"  ,"K"  ,"J"  ,"H"  ,"G"  ,"F"  ,"D"  ,"S",
 "A"  ,"AY" ,"Y1" ,"UH3","AH" ,"P"  ,"O"  ,"I",
 "U"  ,"Y"  ,"T"  ,"R"  ,"E"  ,"W"  ,"AE" ,"AE1",
 "AW2","UH2","UH1","UH" ,"O2" ,"O1" ,"IU" ,"U1",
 "THV","TH" ,"ER" ,"EH" ,"E1" ,"AW" ,"PA1","STOP",
 0
};

/* Missing samples : ready.sam from.sam one.sam bite.sam youl.sam explode.sam if.sam myself.sam back.sam
   cant.sam do.sam wait.sam worlings.sam very.sam babies.sam breath.sam fire.sam beat.sam rest.sam
   then.sam never.sam worlock.sam escape.sam door.sam try.sam any.sam harder.sam only.sam meet.sam with.sam
   doom.sam pop.sam
   Problems with YOU and YOU'LL and YOU'DD */

static const char *wowWordTable[] =
{
"AH1I3Y1", "UH1GA1EH1N", "AHAH2", "AE1EH3M", "AE1EH3ND",
"anew.sam", "AH1NUHTHER", "AE1NY", "anyone.sam", "appear.sam", "AH1UH3R", "UHR", "BABYY1S", "BAE1EH3K",
"BE1T", "become.sam", "BEHST", "BEH1TER", "BUH3AH2YT", "bones.sam", "BRE1YTH", "but.sam", "can.sam", "KAE1EH3NT",
"chance.sam", "CHEHST", "KO1O2I3Y1N", "dance.sam", "DE1STRO1UH3I3AY",
"DE1VEH1LUH3PT", "DIUU", "DONT", "DUUM", "DOO1R", "draw.sam", "DUHNJEH1N", "DUHNJEH1NZ",
"each.sam", "eaten.sam", "EHSPA0KA2I3Y1P", "EHKPA0SPLOU1D", "fear.sam", "FAH1I3YND", "FAH1I3Y1ND", "FAH1EH3AYR", "FOR", "FRUHMM",
"garwor.sam", "GEHT", "GEH1T", "GEHEH3T", "GEHTING", "good.sam", "HAH1HAH1HAH1HAH1", "HAH1RDER",
"hasnt.sam", "have.sam", "HEH1I3VE1WA1I3Y1TS", "HAI1Y1", "HOP",
"HUHNGRY", "HUHNGGRY", "HERRY", "AH1EH3I3Y", "AH1UH3I3Y", "IF", "I1F", "AH1I3YM", "AH1EH3I3YL", "AH1I3Y1L", "IN1",
"INSERT", "invisibl.sam", "IT", "lie.sam", "MAE1EH3DJI1KUH1L",
"MAE1EH3DJI1KUH1L", "MEE1", "MEE1T", "months.sam",
"MAH1EH3I3Y", "MAH2AH2EH3I3Y", "MAH1I1Y", "MAH1I3Y1", "MAH1I3Y", "MAH1I3YSEHLF", "near.sam", "NEH1VER",
"NAH1UH3U1", "UHV", "AWF", "WUHN", "O1NLY", "UHVEHN", "PA1", "PEHTS", "PAH1WERFUH1L", "PAH1P",
"radar.sam", "REHDY",
"REHST", "say.sam", "SAH1I3AYEHNS", "SE1Y", "PA0", "start.sam", "THVAYAY", "THVUH", "THVUH1", "THUH1", "THVEH1N",
"THVRU", "thurwor.sam", "time.sam", "TU1", "TUU1", "TIUU1", "TREH1ZHERT", "TRAH1EH3I3Y", "VEHEH3RY", "WA2AYYT",
"WOO1R", "WORYER", "watch.sam", "WE1Y", "WEHLKUHM",
"WERR", "WAH1EH3I3L", "WIL", "WITH", "WIZERD", "wont.sam",
"WO1O2R", "WO1ERLD", "WORLINGS", "WORLUHK",
"YI3U", "Y1IUU", "YIUUI", "Y1IUU1U1", "YI3U1", "Y1IUUL", "YIUU1L", "Y1IUUD", "YO2O2R",0
};

#define num_samples (sizeof(wowWordTable)/sizeof(char *))


/* Total word to join the phonemes together - Global to make it easier to use */
/* Note the definitions for these are global and defined in src/sndhrdw/gorf.c
   (not great I know, but it will have to do for the moment ;) ) */

extern char totalword[256], *totalword_ptr;
extern char oldword[256];
extern int plural;

int wow_sh_start(void)
{
    wowBaseFrequency = 11025;
    wowBaseVolume = 230;
    wowChannel = 0;
    return 0;
}

void wow_sh_stop(void)
{
}

int wow_speech_r(int offset)
{
    int Phoneme,Intonation;
    int i = 0;

    Z80_Regs regs;
    int data;

    totalword_ptr = totalword;

    Z80_GetRegs(&regs);
    data = regs.BC.B.h;

    Phoneme = data & 0x3F;
    Intonation = data >> 6;

    if(errorlog) fprintf(errorlog,"Data : %d Speech : %s at intonation %d\n",Phoneme, PhonemeTable[Phoneme],Intonation);

    if(Phoneme==63) {
   		sample_stop(wowChannel);
                if (errorlog) fprintf(errorlog,"Clearing sample %s\n",totalword);
                totalword[0] = 0;				   /* Clear the total word stack */
                return data;
    }
    if (PhonemeTable[Phoneme] == "PA0")						   /* We know PA0 is never part of a word */
                totalword[0] = 0;				   /* Clear the total word stack */

/* Phoneme to word translation */

    if (strlen(totalword) == 0) {
       strcpy(totalword,PhonemeTable[Phoneme]);	                   /* Copy over the first phoneme */
       if (plural != 0) {
          if (errorlog) fprintf(errorlog,"found a possible plural at %d\n",plural-1);
          if (!strcmp("S",totalword)) {		   /* Plural check */
             sample_start(wowChannel, num_samples-2, 0);	   /* play the sample at position of word */
             sample_adjust(wowChannel, wowBaseFrequency, -1);    /* play at correct rate */
             totalword[0] = 0;				   /* Clear the total word stack */
             oldword[0] = 0;				   /* Clear the total word stack */
             return data;
          } else {
             plural=0;
          }
       }
    } else
       strcat(totalword,PhonemeTable[Phoneme]);	                   /* Copy over the first phoneme */

    if(errorlog) fprintf(errorlog,"Total word = %s\n",totalword);

    for (i=0; wowWordTable[i]; i++) {
       if (!strcmp(wowWordTable[i],totalword)) {		   /* Scan the word (sample) table for the complete word */
	  /* WOW has Dungeon */
          if ((!strcmp("GDTO1RFYA2N",totalword)) || (!strcmp("RO1U1BAH1T",totalword)) || (!strcmp("KO1UH3I3E1N",totalword))) {		   /* May be plural */
             plural=i+1;
             strcpy(oldword,totalword);
	     if (errorlog) fprintf(errorlog,"Storing sample position %d and copying string %s\n",plural,oldword);
          } else {
             plural=0;
          }
          sample_start(wowChannel, i, 0);	                   /* play the sample at position of word */
          sample_adjust(wowChannel, wowBaseFrequency, -1);         /* play at correct rate */
          if (errorlog) fprintf(errorlog,"Playing sample %d",i);
          totalword[0] = 0;				   /* Clear the total word stack */
          return data;
       }
    }

    /* Note : We should really also use volume in this as well as frequency */
    return data;				                   /* Return nicely */
}

int wow_status_r(void)
{
    if (errorlog) fprintf(errorlog, "asked for samples status %d\n",wowChannel);
    return !sample_playing(wowChannel);
}

/* Read from port 2 (0x12) returns speech status as 0x80 */

int wow_port_2_r(int offset)
{
    int Ans;

    Ans = (input_port_2_r(0) & 0x7F);
    if (wow_status_r() != 0) Ans += 128;
    return Ans;
}

void wow_sh_update(void)
{
}
