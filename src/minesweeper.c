#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "parg.h"

#define DEBUG 0

#define BOLD    "\x1b[1m"
#define RED     "\x1b[1;97;41m"
#define RESET   "\x1b[0m"
const char *colors[] = {"\x1b[1;32m", "\x1b[1;32m", "\x1b[1;32m",     /* green */
                        "\x1b[1;93m", "\x1b[1;93m", "\x1b[1;93m",
                        "\x1b[1;91m", "\x1b[1;91m",
                        "\x1b[1;31m"};

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


void initFields(Field *, int, int, int);
int setBombs(Field *, Field *, double, int, Coord *);
void printField(Field *, int, int);
int readCoord(Coord *, int, int);
bool allOpen(Field *, Field *);
bool step(Field *, Coord *, int, int *);
void showMines(Field *, Field *);
void openFields(Field *, int *);
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
    /* fist iter? hit bomb? */
    bool first, hitBomb;
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
    initFields(field, w, h, tot);

    /* mainloop */
    bombs = 0;
    first = true;
    hitBomb = false;

    printf("bombs unknown\n");
    for(;;) {
        printField(field, w, h);
        err = readCoord(&next, w, h);
        clear();

        if (first) {
            bombs = setBombs(field, field+tot, mp, w, &next);
            first = false;
        }
        if (err) {
            printf("invalid input, try again...\n");
            continue;
        }

        hitBomb = step(field, &next, w, &bombs);
        if (hitBomb) {
            printf("you lost...\n");
            break;
        }
        if (allOpen(field, field+tot)) {
            printf("you won!\n");
            break;
        }

        printf("%d bombs left\n", bombs);
    }

    /* game finished */
    showMines(field, field+tot);
    printField(field, w, h);

    /* cleanup */
    iter = field;
    while (iter != end) {
        free(iter->nbs);
        ++iter;
    }
    free(field);

    return EXIT_SUCCESS;
}


/* set neighbour references of each field and init members */
void initFields(Field *field, int w, int h, int tot)
{
    int i;

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
        /* field[i].hasBomb    = rand_one(prob);
         * if (field[i].hasBomb)
         *     ++bombs; */
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

}


/* randomly distribute bombs
 * make sure, the first uncovered field is empty */
int setBombs(Field *field, Field *end, double prob, int w, Coord *init)
{
    int bombs, i;
    Field *iter, *first;

    bombs = 0;
    iter = field - 1;
    first = field + init->x + w * init->y;
    while(iter != end) {
        ++iter;
        if (iter == first)
            continue;
        iter->hasBomb = rand_one(prob);
        if (iter->hasBomb)
            ++bombs;
    }

    /* iterate over all fields and set neighbouring bombs */
    iter = field;
    while (iter != end) {
        iter->nb = 0;
        for (i = 0; i < 8; ++i)
            iter->nb += ((iter->nbs[i] != NULL && iter->nbs[i]->hasBomb) ? 1 : 0);
        ++iter;
    }

    return bombs;
}


/* format and print field array to console */
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
                printf("| " BOLD "X" RESET " ");
            else if (cur->isOpen)
                printf("| " BOLD "%s" "%d" RESET " ", colors[cur->nb], cur->nb);
            else if (cur->flag)
                printf("|" BOLD RED "<F>" RESET);
            else
                printf("|   ");
        }
        printf("|\n");
    }
    printf("   ");
    for (j = 0; j < w; ++j)
        printf("|---%s", (j == w-1) ? "|\n" : "");
}


/* read command and coordinates from user input into Coord struct
 * returns errorcode */
int readCoord(Coord *next, int w, int h)
{
    int err, c;
    int x, y;
    char cmd, xalpha;

    printf("Enter command (c - uncover, f - flag) and coordinate (a-z, 0-xx): ");
    err = scanf("%c%c%d", &cmd, &xalpha, &y);
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


/* check if all fields are either uncovered or have a bomb
 * in that case the game is won */
bool allOpen(Field *field, Field *end)
{
    while (field != end) {
        if (!(field->isOpen || field->hasBomb))
            return false;
        ++field;
    }
    return true;
}


/* perform given command (uncover, flag) on given coordinates */
bool step(Field *field, Coord *next, int w, int *bombs)
{
    field += (next->x + w * next->y);

    if (next->c == 'C') {
        if (field->hasBomb)
            return true;
        else if (!field->isOpen)
            openFields(field, bombs);
    }
    else if (next->c == 'F' && !field->isOpen) {
        field->flag = !field->flag;
        *bombs += field->flag ? -1 : 1;
    }

    return false;
}


/* uncover all bombs when the game is finished */
void showMines(Field *field, Field *end)
{
    while (field != end) {
        if (field->hasBomb)
            field->isOpen = true;
        ++field;
    }
}


/* recusivly open fields which do not neighbour to a bomb */
void openFields(Field *field, int *bombs)
{
    field->isOpen = true;
    *bombs += field->flag ? 1 : 0;

    int i;
    for (i = 0; i < 8; ++i) {
        if (!(field->nbs[i] == NULL         \
                || field->nbs[i]->hasBomb   \
                || field->nbs[i]->isOpen))
        {
            if (field->nbs[i]->nb == 0)
                openFields(field->nbs[i], bombs);
            else
                field->nbs[i]->isOpen = true;
        }
    }
}


/* returns 1 with given probability
 * else returns 0 */
int rand_one(double prob)
{
    return (rand() < prob * ((double)RAND_MAX + 1.0)) ? 1 : 0;
}


/* clear console window */
/* solutions for windows and unix */
void clear()
{
#if !DEBUG
    puts("\x1B[2J\x1B[H");
#endif
}
