#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "parg.h"

#ifdef __unix__
#define WINDOWS 0
#elif defined(_WIN32) || defined(WIN32)
#define WINDOWS 1
#include <windows.h>
#endif

#define DEBUG 0

#define TITLE "MINESWEEPER"
#define HELP "minesweeper\nUsage: ms [-w WIDTH (8...26)] [-h HEIGHT (8...64)] "\
             "[-p PROBABILITY (0...100)]"

const char AZ[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct Field {
    bool hasBomb;
    bool isOpen;
    bool flag;
    int nb;     /* neighbouring bombs */
    /* struct Field *u, *ur, *r, *dr, *d, *dl, *l, *ul; */
    struct Field **nbs;
} Field;

typedef struct Coord {
    int x, y;
    char c;     /* command */
} Coord;


int initFields(Field *, int, int, double);
void printField(Field *, int, int);
int readCoord(Coord *, int, int);
bool allOpen(Field *, Field *);
bool step(Field *, Coord *, int);
void showMines(Field *, Field *);
void openFields(Field *);
int rand_one(double);
void clear();


int main(int argc, char **argv)
{
    srand(time(NULL));

    /* width, height, mine probability - default values */
    int w = 8, h = 8;
    double mp = 0.16;
    /* total fields, bombs, error variable */
    int tot, bombs, err;
    /* hit bomb? */
    bool hitBomb;
    /* struct to read and pass commands and coordinates */
    Coord next;

    /* parsing argv */
    struct parg_state ps;
    int c;
    parg_init(&ps);

    while ((c = parg_getopt(&ps, argc, argv, "w:h:p:")) != -1) {
        switch (c) {
            case 'w':
                w = atoi(ps.optarg);
                if (!(8 <= w && w <= 26)) {
                    fputs("width must be in [8, 26] ...\n", stderr);
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                h = atoi(ps.optarg);
                if (!(8 <= h && h <= 64)) {
                    fputs("height must be in [8, 64] ...\n", stderr);
                    return EXIT_FAILURE;
                }
                break;
            case 'p':
                mp = (double) atoi(ps.optarg) / 100.;
                if (!(0 <= mp && mp <= 1)) {
                    fputs("probability must be in [0, 100] (%) ...\n", stderr);
                    return EXIT_FAILURE;
                }
                break;
            default:    /* ? */
                puts(HELP);
                return EXIT_FAILURE;
        }
    }

    tot = w * h;

    /* init field */
    Field *field = malloc(tot * sizeof(*field));
    if (!field) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return EXIT_FAILURE;
    }
    /* use array of pointers to fields to store neighbours */
    Field *iter = field, *end = field + tot;
    while (iter != end) {
        iter->nbs = malloc(8 * sizeof(field));
        ++iter;
    }

    clear();
    bombs = initFields(field, w, h, mp);

    /* mainloop */
    hitBomb = false;
    for(;;) {
        printf("%d bombs\n", bombs);
        printField(field, w, h);
        err = readCoord(&next, w, h);
        clear();
        /* printf("x: %d, y: %d\n", next.x, next.y); */
        if (err) {
            printf("invalid input, try again...\n");
            continue;
        }
        hitBomb = step(field, &next, w);
        if (hitBomb) {
            printf("you lost...\n");
            break;
        }
        if (allOpen(field, field+tot)) {
            printf("you won!\n");
            break;
        }
    }

    /* game finished */
    showMines(field, field+tot);
    printField(field, w, h);

#if WINDOWS
    system("PAUSE");
#endif

    /* cleanup */
    iter = field;
    while (iter != end) {
        free(iter->nbs);
        ++iter;
    } 
    free(field);

    return EXIT_SUCCESS;
}


int initFields(Field *field, int w, int h, double prob)
{
    int i, tot, bombs;
    tot = w * h;
    bombs = 0;

    /* iterate over all cells, set neighbours and bombs */
    for (i = 0; i < tot; ++i) {
        field[i].nbs[0] = &field[i-w];      /* u  */
        field[i].nbs[1] = &field[i-w+1];    /* ur */
        field[i].nbs[2] = &field[i+1];      /* r  */
        field[i].nbs[3] = &field[i+w+1];    /* dr */
        field[i].nbs[4] = &field[i+w];      /* d  */
        field[i].nbs[5] = &field[i+w-1];    /* dl */
        field[i].nbs[6] = &field[i-1];      /* l  */
        field[i].nbs[7] = &field[i-w-1];    /* ul */

        field[i].isOpen     = false;
        field[i].flag       = false;
        field[i].hasBomb    = rand_one(prob);
        if (field[i].hasBomb)
            ++bombs;
    }

    /* UGLY HACK - set corner cases */
    /* upmost and downmost row */
    for (i = 0; i < w; ++i) {
        field[i].nbs[0] = NULL;             /* u  */
        field[i].nbs[1] = NULL;             /* ur */
        field[i].nbs[7] = NULL;             /* ul */
        field[i+w*(h-1)].nbs[4] = NULL;     /* d  */
        field[i+w*(h-1)].nbs[3] = NULL;     /* dr */
        field[i+w*(h-1)].nbs[5] = NULL;     /* dl */
    }

    /* leftmost and rightmost column */
    for (i = 0; i < tot; i+=w) {
        field[i].nbs[6] = NULL;             /* l  */
        field[i].nbs[7] = NULL;             /* ul */
        field[i].nbs[5] = NULL;             /* dl */
        field[i+w-1].nbs[2] = NULL;         /* r  */
        field[i+w-1].nbs[1] = NULL;         /* ur */
        field[i+w-1].nbs[3] = NULL;         /* dr */
    }

    /* iterate over all fields and set neighbouring bombs */
    Field *end = field + tot;
    while (field != end) {
        field->nb = 0;
        for (i = 0; i < 8; ++i)
            field->nb += ((field->nbs[i] != NULL && field->nbs[i]->hasBomb) ? 1 : 0);
        ++field;
    }

    return bombs;
}


void printField(Field *field, int w, int h)
{
    Field *cur;
    int i, j;
    printf("   ");
    for (i = 0; i < w; ++i)
        printf("  %c %s", AZ[i], (i == w-1) ? "\n" : "");
    for (i = 0; i < h; ++i) {
        printf("   ");
        for (j = 0; j < w; ++j)
            printf("|---%s", (j == w-1) ? "|\n" : "");
        printf("%s%d ", (i >= 10) ? "" : " ", i);
        for (j = 0; j < w; ++j) {
            cur = &field[w*i+j];
            if (cur->isOpen && cur->hasBomb)
                printf("| X ");
            else if (cur->isOpen)
                printf("| %d ", cur->nb);
            else if (cur->flag)
                printf("| F ");
            else
                printf("|   ");
        }
        printf("|\n");
    }
    printf("   ");
    for (j = 0; j < w; ++j)
        printf("|---%s", (j == w-1) ? "|\n" : "");
}


int readCoord(Coord *next, int w, int h)
{
    int err, c;
    int x, y;
    char cmd, xalpha;

    /* printf("Enter Coordinate (x, y): "); */
    printf("Enter command (c - uncover, f - flag) and coordinate (a-z, 0-xx): ");
/* #if WINDOWS */
/*     err = scanf_s("%c%d", &xalpha, &y); */
/* #else */
    /* err = scanf("%c%d", &xalpha, &y); */
    err = scanf("%c%c%d", &cmd, &xalpha, &y);
/* #endif */
    /* clear stdin */
    while ((c = getchar()) != '\n' && c != EOF);

    cmd = toupper(cmd);
    xalpha = toupper(xalpha);
    for (x = 0; x < 26; ++x)
        if (AZ[x] == xalpha)
            break;
    if (err != 3 || x == 26 || x >= w || y >= h \
            || !(cmd == 'C' || cmd == 'F'))
        return -1;
    next->x = x;
    next->y = y;
    next->c = cmd;

    return 0;
}


bool allOpen(Field *field, Field *end)
{
    while (field != end) {
        if (!(field->isOpen || field->hasBomb))
            return false;
        ++field;
    }
    return true;
}


bool step(Field *field, Coord *next, int w)
{
    field += (next->x + w * next->y);

    if (next->c == 'C') {
        if (field->hasBomb)
            return true;
        else
            openFields(field);
    }
    else if (next->c == 'F') {
        field->flag = !field->flag;        
    }

    return false;
}


void showMines(Field *field, Field *end)
{
    while (field != end) {
        if (field->hasBomb)
            field->isOpen = true;
        ++field;
    }
}


void openFields(Field *field)
{
    field->isOpen = true;

    int i;
    for (i = 0; i < 8; ++i) {
        if (!(field->nbs[i] == NULL         \
                || field->nbs[i]->hasBomb   \
                || field->nbs[i]->isOpen)) 
        {
            if (field->nbs[i]->nb == 0)
                openFields(field->nbs[i]);
            else
                field->nbs[i]->isOpen = true;
        }
    }
}


/* returns 1 with a probability of prob/tot
 * else returns 0 */
/* int randint(int prob, int tot)
 * {
 *     int x = tot+1;
 *     while (x > tot)
 *         x = rand() / ((RAND_MAX + 1u) / tot);
 *     return x < prob ? 1 : 0;
 * } */
int rand_one(double prob)
{
    return (rand() < prob * ((double)RAND_MAX + 1.0)) ? 1 : 0;
}


void clear()
{
#if !DEBUG
#if WINDOWS
    char fill = ' ';
    COORD t1 = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO s;
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    GetConsoleScreenBufferInfo(console, &s);
    DWORD written, cells = s.dwSize.X * s.dwSize.Y;
    FillConsoleOutputCharacter(console, fill, cells, t1, &written);
    FillConsoleOutputAttribute(console, s.wAttributes, cells, t1, &written);
    SetConsoleCursorPosition(console, t1);
#else 
    puts("\x1B[2J\x1B[H");
#endif
#endif 
}
