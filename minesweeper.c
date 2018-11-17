#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

const char *AZ = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

typedef struct Field {
    bool hasBomb;
    bool isOpen;
    bool flag;
    int nb;     /* neighbouring bombs */
    struct Field *u, *ur, *r, *dr, *d, *dl, *l, *ul;
} Field;

typedef struct Coord {
    int x, y;
} Coord;

void initFields(Field *, int, int, int);

void printField(Field *, int, int);

int readCoord(Coord *);

bool allOpen(Field *, Field *);

bool step(Field *, Coord *, int);

void showMines(Field *, Field *);

void openFields(Field *);


int main(int argc, char **argv)
{
    /* width, height, mine probability */
    int w = 8, h = 8, mp = 16;
    int tot = w * h;
    Field *field = malloc(tot * sizeof(*field));
    if (!field) {
        fprintf(stderr, "Failed to allocate memory!\n");
        return EXIT_FAILURE;
    }
    initFields(field, w, h, mp);

    /* mainloop */
    int err;
    Coord next;
    bool hitMine = false;
    for(;;) {
        printField(field, w, h);
        if (allOpen(field, field+tot)) {
            printf("you won!\n");
            break;
        }
        err = readCoord(&next);
        if (err) {
            printf("invalid input, try again...\n");
            continue;
        }
        hitMine = step(field, &next, w);
        if (hitMine) {
            showMines(field, field+tot);
            printField(field, w, h);
            printf("you lost...\n");
            break;
        }
    }

    free(field);
    return EXIT_SUCCESS;
}


void initFields(Field *field, int w, int h, int prob)
{
    int i, tot;
    tot = w * h;

    /* iterate over all cells, set neighbours and bombs */
    for (i = 0; i < tot; ++i) {
        field[i].u  = &field[i-w];
        field[i].ur = &field[i-w+1];
        field[i].r  = &field[i+1];
        field[i].dr = &field[i+w+1];
        field[i].d  = &field[i+w];
        field[i].dl = &field[i+w-1];
        field[i].l  = &field[i-1];
        field[i].ul = &field[i-w-1];

        field[i].isOpen     = false;
        field[i].flag       = false;
        field[i].hasBomb    = randint(prob, tot);
    }

    /* UGLY HACK - set corner cases */
    /* upmost and downmost row */
    for (i = 0; i < w; ++i) {
        field[i].u  = NULL;
        field[i].ur = NULL;
        field[i].ul = NULL;
        field[i+w*(h-1)].d  = NULL;
        field[i+w*(h-1)].dr = NULL;
        field[i+w*(h-1)].dl = NULL;
    }

    /* leftmost and rightmost column */
    for (i = 0; i < tot; i+=w) {
        field[i].l  = NULL;
        field[i].ul = NULL;
        field[i].dl = NULL;
        field[i+w-1].r  = NULL;
        field[i+w-1].ur = NULL;
        field[i+w-1].dr = NULL;
    }

    /* iterate over all fields and set neighbouring bombs */
    Field *end = field + tot;
    while (field != end) {
        field->nb = 0;
        field->nb += ((field->u  != NULL && field->u->hasBomb)  ? 1 : 0);
        field->nb += ((field->ur != NULL && field->ur->hasBomb) ? 1 : 0);
        field->nb += ((field->r  != NULL && field->r->hasBomb)  ? 1 : 0);
        field->nb += ((field->dr != NULL && field->dr->hasBomb) ? 1 : 0);
        field->nb += ((field->d  != NULL && field->d->hasBomb)  ? 1 : 0);
        field->nb += ((field->dl != NULL && field->dl->hasBomb) ? 1 : 0);
        field->nb += ((field->l  != NULL && field->l->hasBomb)  ? 1 : 0);
        field->nb += ((field->ul != NULL && field->ul->hasBomb) ? 1 : 0);
    }
}


void printField(Field *field, int w, int h)
{
    Field *cur;
    int i, j;
    printf("   ");
    for (i = 0; i < w; ++i)
        printf("|%s%d %s", (i > 10) ? "" : " ", i, (i == w-1) ? "\n" : "");
    for (i = 0; i < h; ++i) {
        for (j = 0; j < w; ++j)
            printf("|---%s", (j == w-1) ? "|\n" : "");
        printf(" %c ", AZ[i]);
        for (j = 0; j < w; ++j) {
            cur = &field[i+j];
            if (cur->isOpen && cur->hasBomb)
                printf("| X ");
            else if (cur->isOpen)
                printf("| %c ", field[i+j].nb ? field[i+j].nb + '0' : ' ');
            else if (cur->flag)
                printf("| F ");
        }
        printf("|\n");
    }
    for (j = 0; j < w; ++j)
        printf("|---%s", (j == w-1) ? "|\n" : "");
}


int readCoord(Coord *next)
{
    int err;
    int x, y;
    char xalpha;
    printf("Enter Coordinate (x, y): ");
    err = scanf_s("%c%d", &xalpha, &y);
    for (x = 0; x < 26; ++x)
        if (AZ[x] == xalpha)
            break;
    next->x = x;
    next->y = y;
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
    if (field->hasBomb) {
        field->isOpen = true;
        return true;
    }
    openFields(field);
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
    if (!(field->u == NULL || field->u->hasBomb))
        openFields(field->u);
    if (!(field->ur == NULL || field->ur->hasBomb))
        openFields(field->ur);
    if (!(field->r == NULL || field->r->hasBomb))
        openFields(field->r);
    if (!(field->dr == NULL || field->dr->hasBomb))
        openFields(field->dr);
    if (!(field->d == NULL || field->d->hasBomb))
        openFields(field->d);
    if (!(field->dl == NULL || field->dl->hasBomb))
        openFields(field->dl);
    if (!(field->l == NULL || field->l->hasBomb))
        openFields(field->l);
    if (!(field->ul == NULL || field->ul->hasBomb))
        openFields(field->ul);
}
