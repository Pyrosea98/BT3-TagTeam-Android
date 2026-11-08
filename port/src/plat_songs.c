/* The manifest of songs added to the music select (see plat_songs.h). Runs at start-up: it gives the game the
   added tracks' list offsets and names, and the file layer where their files live. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plat_songs.h"

char gPortSongNames[PORT_SONG_MAX][64];
int gPortSongCount;
int gPortSongOffsets[PORT_SONG_MAX];

typedef struct SongAlias {
    char rel[48];     /* the path the game asks for, under the data root */
    char target[192]; /* where it is served from */
} SongAlias;

static SongAlias sAliases[PORT_SONG_MAX];
static int sAliasCount;
static int sDone;

static const char *data_root(void) {
    const char *r = getenv("BT3_DATA");
    return r != NULL ? r : "gamedata";
}

static char *trim(char *s) {
    char *e;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r' || e[-1] == '\n')) {
        *--e = '\0';
    }
    return s;
}

static char sDir[256]; /* the folder the songs are in (plat_extras.c) */
static char sFiles[PORT_SONG_MAX][128];

extern int PortExtras_Dir(const char *kind, const char *env, char *out, unsigned size);
extern int PortExtras_Scan(const char *dir, const char *ext, char names[][128], int max);
extern void PortExtras_NameFromFile(const char *file, char *name, unsigned size);

void PortSongs_Init(void);

/* The order the files it does not mention follow: by name without regard to case, as PortExtras_Scan sorts them. */
static int song_cmp(const void *a, const void *b) {
    const unsigned char *x = a, *y = b;
    for (; *x != '\0' && tolower(*x) == tolower(*y); x++, y++) {
    }
    return tolower(*x) != tolower(*y) ? tolower(*x) - tolower(*y) : strcmp((const char *)a, (const char *)b);
}

const char *PortSongs_Dir(void) {
    PortSongs_Init(); /* (the overlay may ask before the game has started) */
    return sDir;
}

/* Adds one song: its place in the music list, its name, and the file the game's id for it stands for. */
static void add_song(const char *file, const char *name) {
    int i, offset, index;

    for (i = 0; i < gPortSongCount; i++) {
        if (strcmp(sFiles[i], file) == 0) {
            return;
        }
    }
    if (gPortSongCount >= PORT_SONG_MAX) {
        fprintf(stderr, "bt3: songs: more than %d added, %s is left out\n", PORT_SONG_MAX, file);
        return;
    }
    if (strlen(file) >= sizeof(sFiles[0]) || strlen(sDir) + 1 + strlen(file) >= sizeof(sAliases[0].target)) {
        /* (cut short, the name was that of no file: the song was in the list and silent) */
        fprintf(stderr, "bt3: songs: %s is left out: the folder and the name together are too long\n", file);
        return;
    }
    i = gPortSongCount;
    snprintf(sFiles[i], sizeof(sFiles[i]), "%s", file);
    offset = PORT_SONG_FIRST_OFFSET + i;
    index = PORT_SONG_FILE_INDEX + i; /* PORT_BGM_FILE(offset), as an index of the second archive */
    gPortSongOffsets[i] = offset;
    snprintf(gPortSongNames[i], 64, "%s", name);
    snprintf(sAliases[sAliasCount].rel, sizeof(sAliases[sAliasCount].rel), "pzs3us2/%05d.bin", index);
    snprintf(sAliases[sAliasCount].target, sizeof(sAliases[sAliasCount].target), "%s/%s", sDir, file);
    sAliasCount++;
    gPortSongCount++;
}

/* The ADX that earlier releases wrote next to an MP3 they converted (the mark "BT3:MP3" 1 at 0x30 of its header):
   the MP3 beside it is now played as it is, and the copy would be the same song in the list a second time. 1 when
   `file` is such a copy and its MP3 is still there (`mp3` gets the MP3's name, any case of ".mp3", as the converter
   took it); a copy without its MP3 is a song like any other ADX, and an ADX made by hand or by ffmpeg has no mark.
   The file is left where it is. */
static int converted_copy(const char *file, char *mp3, unsigned size) {
    static const char kMark[8] = {'B', 'T', '3', ':', 'M', 'P', '3', 1};
    char path[512];
    unsigned char h[0x38];
    size_t l = strlen(file);
    FILE *fp;
    int c, got;

    if (l <= 4 || l + 1 > size || file[l - 4] != '.' || tolower((unsigned char)file[l - 3]) != 'a' || tolower((unsigned char)file[l - 2]) != 'd' ||
        tolower((unsigned char)file[l - 1]) != 'x') {
        return 0;
    }
    snprintf(path, sizeof(path), "%s/%s", sDir, file);
    if ((fp = fopen(path, "rb")) == NULL) {
        return 0;
    }
    got = fread(h, 1, sizeof(h), fp) == sizeof(h);
    fclose(fp);
    if (!got || h[0] != 0x80 || h[1] != 0 || memcmp(h + 0x30, kMark, sizeof(kMark)) != 0) {
        return 0;
    }
    for (c = 0; c < 4; c++) { /* (.mp3 .mP3 .Mp3 .MP3: on Linux the disk tells them apart) */
        snprintf(mp3, size, "%.*s.%c%c3", (int)(l - 4), file, c & 2 ? 'M' : 'm', c & 1 ? 'P' : 'p');
        snprintf(path, sizeof(path), "%s/%s", sDir, mp3);
        if ((fp = fopen(path, "rb")) != NULL) {
            fclose(fp);
            return 1;
        }
    }
    return 0;
}

/* Reads the folder and adds the songs that are not in the list yet, at its end: a song keeps its place (the game
   holds the place's number while a fight with that music is set up and runs). */
static void collect(void) {
    char path[512];
    char line[256];
    FILE *fp;

    /* the list first (its names and its order), then whatever else is in the folder, by name */
    snprintf(path, sizeof(path), "%s/songs.txt", sDir);
    fp = fopen(path, "rb");
    while (fp != NULL && fgets(line, sizeof(line), fp) != NULL) {
        char *bar;
        if (line[0] == '\0' || line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        bar = strchr(line, '|');
        if (bar == NULL) {
            continue;
        }
        *bar = '\0';
        /* The line may name an ADX, an mp3, a wav or a flac: the file it names is the one played (gs/snd_ma.c for
           the last three), the MP3 for a converted copy of one (a list written for the converter's ADX). */
        {
            char *file = trim(line), mp3[128];
            add_song(converted_copy(file, mp3, sizeof(mp3)) ? mp3 : file, trim(bar + 1));
        }
    }
    if (fp != NULL) {
        fclose(fp);
    }
    {
        /* Everything else in the folder, of any of the formats the port plays, by name. The four scans' names are
           put together and sorted once, so the order does not depend on the extension. */
        static const char *exts[] = { ".adx", ".mp3", ".wav", ".flac" };
        static char found[PORT_SONG_MAX * 4][128];
        static char one[64][128];
        int n = 0, e, k;

        for (e = 0; e < (int)(sizeof(exts) / sizeof(exts[0])); e++) {
            int m = PortExtras_Scan(sDir, exts[e], one, 64), j;
            for (j = 0; j < m && n < (int)(sizeof(found) / sizeof(found[0])); j++) {
                snprintf(found[n++], 128, "%s", one[j]);
            }
        }
        qsort(found, (size_t)n, 128, song_cmp);
        for (k = 0; k < n; k++) {
            char name[64], mp3[128];
            if (converted_copy(found[k], mp3, sizeof(mp3))) {
                continue; /* (its MP3 is in the list instead) */
            }
            PortExtras_NameFromFile(found[k], name, sizeof(name));
            add_song(found[k], name);
        }
    }
}

static void collect_iso(void){
 char path[512],line[256];FILE *fp;const char *root=data_root();int before=gPortSongCount;
 snprintf(path,sizeof(path),"%s/beta8/iso-songs.tsv",root);fp=fopen(path,"r");if(!fp)return;
 while(fgets(line,sizeof(line),fp)&&gPortSongCount<PORT_SONG_MAX){
  char *end,*name;long entry=strtol(line,&end,10);unsigned char head[20];FILE *audio;int i;char target[192];
  if(end==line||*end!='\t'||entry<65201||entry>=65495)continue;name=trim(end+1);if(!*name)continue;
  if(snprintf(target,sizeof(target),"%s/pak3us2/%05ld.bin",root,entry)>=(int)sizeof(target))continue;
  audio=fopen(target,"rb");
  /* (a profile without the pak3us2 alias, as on Android's shared storage: the same files under pzs3us2) */
  if(!audio&&snprintf(target,sizeof(target),"%s/pzs3us2/%05ld.bin",root,entry)<(int)sizeof(target))audio=fopen(target,"rb");
  if(!audio)continue;i=fread(head,1,20,audio);fclose(audio);
  if(i!=20||head[0]!=0x80||head[1]!=0||head[7]!=2)continue;
  i=gPortSongCount++;snprintf(sFiles[i],sizeof(sFiles[i]),"iso:%05ld",entry);
  gPortSongOffsets[i]=PORT_SONG_FIRST_OFFSET+i;snprintf(gPortSongNames[i],64,"%s",name);
  snprintf(sAliases[sAliasCount].rel,sizeof(sAliases[0].rel),"pzs3us2/%05d.bin",PORT_SONG_FILE_INDEX+i);
  snprintf(sAliases[sAliasCount].target,sizeof(sAliases[0].target),"%s",target);sAliasCount++;
 }
 fclose(fp);if(gPortSongCount>before)fprintf(stderr,"bt3: songs: %d unique ISO soundtrack tracks imported\n",gPortSongCount-before);
}

static int sMadeSeen; /* PortSongs_Mp3Made() when the folder was last read */

void PortSongs_Init(void) {
    if (sDone) {
        return;
    }
    sDone = 1;
    collect_iso();
    if (!PortExtras_Dir("songs", "BT3_SONGS", sDir, sizeof(sDir))) {
        return;
    }
    collect();
    if (gPortSongCount > 0) {
        fprintf(stderr, "bt3: songs: %d added from %s\n", gPortSongCount, sDir);
    }
}

/* Kept for the game's select screens (src/menu/menu_c_e.c, menu_e_b.c), which took in the songs a background
   conversion had finished. Nothing is converted any more: the folder is read once, at start, and a song in it is
   played as it is (gs/snd_ma.c), so the list does not change while the program runs. */
void PortSongs_Refresh(void) {
}

const char *PortSongs_Name(int index) {
    return index >= 0 && index < gPortSongCount ? gPortSongNames[index] : "";
}

int PortSongs_Count(void) {
    return gPortSongCount;
}

int PortSongs_Offset(int index) {
    if (index < 0 || index >= gPortSongCount) {
        return 0;
    }
    return gPortSongOffsets[index];
}

/* Story presentation ADX slots are independent of the music list (no index shifts in saves).
   Chapter-specific files override saga families, allowing a different intro for each movie. */
#define STORY_TRACK_FIRST 75000
#define STORY_TRACK_MAX 160
static struct {char target[512];int loop,durationMs;} sStoryTracks[STORY_TRACK_MAX];
static int sStoryTrackCount;
int PortSongs_StoryBgm(const char *chapter,int saga,int title){
 char path[512];FILE*f;int i,pass,p=saga==0||saga==1?0:saga>=2&&saga<8?saga:0;
 for(pass=0;pass<3;pass++){
  int durationMs=0;
  if(pass==0){if(!chapter||!*chapter||strchr(chapter,'/')||strchr(chapter,'\\'))continue;snprintf(path,sizeof(path),"%s/tagteam-ui/history-prefight/%s-%s.adx",data_root(),chapter,title?"title":"recap");}
  else {int profile=pass==1?p:0;if(pass==2&&p==0)break;snprintf(path,sizeof(path),"%s/tagteam-ui/history-prefight/%s-%d.adx",data_root(),title?"title":"recap",profile);}
  f=fopen(path,"rb");if(!f)continue;{unsigned char h[16];int valid=fread(h,1,16,f)==16&&h[0]==0x80&&h[1]==0&&h[7]==2;fclose(f);if(!valid)continue;unsigned rate=((unsigned)h[8]<<24)|((unsigned)h[9]<<16)|((unsigned)h[10]<<8)|h[11],samples=((unsigned)h[12]<<24)|((unsigned)h[13]<<16)|((unsigned)h[14]<<8)|h[15];if(rate>=8000&&rate<=192000){unsigned long long ms=(unsigned long long)samples*1000/rate;if(ms>=500&&ms<=120000)durationMs=(int)ms;}}
  for(i=0;i<sStoryTrackCount;i++)if(!strcmp(path,sStoryTracks[i].target))return 0xD48+STORY_TRACK_FIRST+i;
  if(sStoryTrackCount>=STORY_TRACK_MAX)break;i=sStoryTrackCount++;snprintf(sStoryTracks[i].target,sizeof(sStoryTracks[i].target),"%s",path);sStoryTracks[i].loop=!title;sStoryTracks[i].durationMs=durationMs;return 0xD48+STORY_TRACK_FIRST+i;
 }
 /* Native series-specific fallback when a resource has not been supplied yet. */
 return saga==5?0x10B28:saga==6?0x10B1E:0x10B1C;
}

int PortSongs_StoryDurationMs(const char*chapter,int saga,int title){int i=PortSongs_StoryBgm(chapter,saga,title)-0xD48-STORY_TRACK_FIRST;return i>=0&&i<sStoryTrackCount?sStoryTracks[i].durationMs:0;}

/* Whether `path` is the file of a song added from outside the disc (for the sound code: such a file is played
   round and round like the disc's music, though it has no loop points of its own). */
int PortSongs_IsFile(const char *path) {
    size_t pl = strlen(path), tl;
    for(int k=0;k<sStoryTrackCount;k++)if(!strcmp(path,sStoryTracks[k].target))return sStoryTracks[k].loop;
    int i;
    for (i = 0; i < sAliasCount; i++) {
        tl = strlen(sAliases[i].target);
        if (tl != 0 && tl <= pl && strcmp(path + pl - tl, sAliases[i].target) == 0) {
            return 1;
        }
    }
    return 0;
}

int PortSongs_Alias(const char *rel, char *out, unsigned n) {
    int i;
    char canonical[256];
    /* Beta8 mounts PAK3US2; the original executable mounts PZS3US2.
       Imported music aliases must work with either partition spelling. */
    if (strncmp(rel,"pak3us2/",8)==0) {
        snprintf(canonical,sizeof(canonical),"pzs3us2/%s",rel+8);
        rel=canonical;
    }
    {int entry=-1,used=0;if(sscanf(rel,"pzs3us2/%d.bin%n",&entry,&used)==1&&rel[used]==0&&entry>=STORY_TRACK_FIRST&&entry<STORY_TRACK_FIRST+sStoryTrackCount){snprintf(out,n,"%s",sStoryTracks[entry-STORY_TRACK_FIRST].target);return 1;}}
    for (i = 0; i < sAliasCount; i++) {
        if (strcmp(sAliases[i].rel, rel) == 0) {
            snprintf(out, n, "%s", sAliases[i].target);
            return 1;
        }
    }
    return 0;
}

/* BT3_TEST_BGM=<entry>: every battle is set up with that entry of the music list, whatever the menus chose
   (testing: 0x1A is the first added song). */
int Port_TestBgm(void) {
    const char *e = getenv("BT3_TEST_BGM");
    return e != NULL && e[0] != '\0' ? (int)strtol(e, NULL, 0) : -1;
}
