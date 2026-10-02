/* Build: make */

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/utils.h"
#include "../include/texture.h"
#include "../include/physics.h"

#define WHITE 0
#define BLACK 1
#define WALL_COLOR 11 /* brown */

#define BALL_RADIUS 30
#define BALL_MASS 20
#define MAX_SPAWN_SPEED 500
#define START_BALLS 3
#define NB_CARDS 4
#define CARD_COOLDOWN_MS 5000
#define AI_PERIOD_MS 2000
#define MAX_FRAME_MS 50

const int ARENA_WIDTH = 480;   /* Largeur de l'arène */
const int ARENA_HEIGHT = 854;  /* Hauteur de l'arène */
const Vect ARENA_DIMENSIONS = {480, 854};
const int SCREEN_WIDTH = 1400; /* Largeur de la fenêtre */
const int SCREEN_HEIGHT = 950; /* Hauteur de la fenêtre */
const int FPS = 100;           /* Nombre d'images par seconde */

#define ARENA_X ((SCREEN_WIDTH - ARENA_WIDTH) / 10)
#define ARENA_Y ((SCREEN_HEIGHT - ARENA_HEIGHT) / 2)

typedef struct Text
{
    SDL_Texture *texture;
    int width;
    int height;
    TTF_Font *font;
    SDL_Color color;
} Text;

typedef struct Card
{
    SDL_Texture *texture;
    SDL_Rect rect;
    bool is_selected;
    bool exist;
    Uint64 ready_at; /* Instant où la carte réapparaît */
} Card;

/* Résultat de mainGame */
enum { QUIT = -1, WHITE_WINS = 0, BLACK_WINS = 1 };

void cleanup(SDL_Window *window, SDL_Renderer *renderer, int nb_textures, SDL_Texture **textures);
void aiPlay(SDL_Renderer *renderer, int *num_objects_list, Object *objects, Vect *nb_color_balls);
Text createText(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color);
void drawText(SDL_Renderer *renderer, Text *text, SDL_Rect destRect);
void destroyText(Text *text);
int mainGame(SDL_Texture **textures, SDL_Renderer *renderer, Text titleText);
bool welcomeScreen(SDL_Renderer *renderer, Text titleText);
bool replayScreen(SDL_Renderer *renderer, Text titleText, int winner);

int initSDL(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("Erreur d'initialisation de SDL: %s\n", SDL_GetError());
        return -1;
    }
    if (TTF_Init() == -1)
    {
        printf("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
        return -1;
    }

    return 0;
}

int main(void)
{
    srand(time(NULL));

    if (initSDL() < 0)
    {
        return -1;
    }

    int max_textures = 5;
    SDL_Texture **textures = calloc(max_textures, sizeof(SDL_Texture *));

    SDL_Window *window = SDL_CreateWindow("Clash_of_balls", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = window ? SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED) : NULL;
    if (window && !renderer)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (!window || !renderer)
    {
        printf("Erreur lors de la création de la fenêtre: %s\n", SDL_GetError());
        cleanup(window, renderer, 0, textures);
        return -1;
    }

    int nb_textures = loadImagesFromDirectory("./img", renderer, textures, max_textures);
    if (nb_textures < max_textures)
    {
        printf("Images manquantes (%d/%d): lancez le jeu depuis la racine du projet.\n", nb_textures, max_textures);
        cleanup(window, renderer, max_textures, textures);
        return -1;
    }

    TTF_Font *font = TTF_OpenFont("Jersey_Sharp.ttf", 32);
    if (!font)
    {
        printf("Erreur lors du chargement de la police: %s\n", TTF_GetError());
        cleanup(window, renderer, max_textures, textures);
        return -1;
    }

    SDL_Color blackColor = {0, 0, 0, 255};
    Text titleText = createText(renderer, font, "Clash of Balls", blackColor);

    bool start = welcomeScreen(renderer, titleText);

    while (start)
    {
        int winner = mainGame(textures, renderer, titleText);
        start = replayScreen(renderer, titleText, winner);
    }

    destroyText(&titleText);
    TTF_CloseFont(font);

    cleanup(window, renderer, max_textures, textures);

    printf("Quitting !\n");

    return 0;
}

Text createText(SDL_Renderer *renderer, TTF_Font *font, const char *text, SDL_Color color)
{
    Text newText = {NULL, 0, 0, font, color};
    SDL_Surface *textSurface = TTF_RenderText_Blended(font, text, color);
    if (!textSurface)
    {
        fprintf(stderr, "Erreur lors du rendu du texte: %s\n", TTF_GetError());
        return newText;
    }
    newText.texture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if (!newText.texture)
    {
        fprintf(stderr, "Erreur lors de la création de la texture pour le texte: %s\n", SDL_GetError());
    }
    newText.width = textSurface->w;
    newText.height = textSurface->h;
    SDL_FreeSurface(textSurface);
    return newText;
}

/* destRect.x / destRect.y : centre du texte */
void drawText(SDL_Renderer *renderer, Text *text, SDL_Rect destRect)
{
    if (text->texture)
    {
        SDL_Rect dst = {destRect.x - destRect.w / 2, destRect.y - destRect.h / 2, destRect.w, destRect.h};
        SDL_RenderCopy(renderer, text->texture, NULL, &dst);
    }
}

void drawTextAt(SDL_Renderer *renderer, Text *text, int cx, int cy)
{
    drawText(renderer, text, (SDL_Rect){cx, cy, text->width, text->height});
}

void destroyText(Text *text)
{
    if (text->texture)
    {
        SDL_DestroyTexture(text->texture);
        text->texture = NULL;
    }
}

void cleanup(SDL_Window *window, SDL_Renderer *renderer, int nb_textures, SDL_Texture **textures)
{
    if (textures != NULL)
    {
        for (int i = 0; i < nb_textures; i++)
        {
            if (textures[i] != NULL)
            {
                SDL_DestroyTexture(textures[i]);
            }
        }
        free(textures);
    }
    if (renderer != NULL)
    {
        SDL_DestroyRenderer(renderer);
    }
    if (window != NULL)
    {
        SDL_DestroyWindow(window);
    }
    TTF_Quit();
    SDL_Quit();
}

/* Vitesse aléatoire de norme au plus MAX_SPAWN_SPEED, dans toutes les directions */
static Vect randomSpeed(void)
{
    double angle = (rand() % 3600) * PI / 1800.0;
    double speed = MAX_SPAWN_SPEED / 2 + rand() % (MAX_SPAWN_SPEED / 2);
    return (Vect){speed * cos(angle), speed * sin(angle)};
}

static bool spawnBall(SDL_Renderer *renderer, Object *objects, int *num_objects_list, Vect *nb_color_balls, Vect position, int color)
{
    if (!canAppear(position, 2 * BALL_RADIUS, 2 * BALL_RADIUS, objects, num_objects_list, Ball, ARENA_DIMENSIONS))
    {
        return false;
    }
    createBall(renderer, objects, BALL_RADIUS, BALL_MASS, position, randomSpeed(), color, nb_color_balls, num_objects_list, false);
    return true;
}

/* L'IA (blancs) lance une boule depuis le bord gauche */
void aiPlay(SDL_Renderer *renderer, int *num_objects_list, Object *objects, Vect *nb_color_balls)
{
    Vect position = (Vect){2 * BALL_RADIUS, BALL_RADIUS + rand() % (ARENA_HEIGHT - 2 * BALL_RADIUS)};
    spawnBall(renderer, objects, num_objects_list, nb_color_balls, position, WHITE);
}

/* Place START_BALLS boules de la couleur donnée dans la moitié de l'arène indiquée (0: gauche, 1: droite) */
static void placeStartBalls(SDL_Renderer *renderer, Object *objects, int *num_objects_list, Vect *nb_color_balls, int color, int half)
{
    int placed = 0;
    for (int attempt = 0; attempt < 500 && placed < START_BALLS; attempt++)
    {
        Vect position = {
            half * ARENA_WIDTH / 2 + BALL_RADIUS + rand() % (ARENA_WIDTH / 2 - 2 * BALL_RADIUS),
            BALL_RADIUS + rand() % (ARENA_HEIGHT - 2 * BALL_RADIUS)};
        if (spawnBall(renderer, objects, num_objects_list, nb_color_balls, position, color))
        {
            placed++;
        }
    }
}

static void updateScoreText(SDL_Renderer *renderer, Text *score, TTF_Font *font, Vect counts)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "White: %d    Black: %d", (int)counts.x, (int)counts.y);
    destroyText(score);
    *score = createText(renderer, font, buf, (SDL_Color){0, 0, 0, 255});
}

int mainGame(SDL_Texture **textures, SDL_Renderer *renderer, Text titleText)
{
    int num_objects_list[max_objects];
    Vect nb_color_balls = (Vect){0, 0}; /* x: blanches, y: noires */
    memset(num_objects_list, 0, sizeof(num_objects_list));

    SDL_Rect arenaViewport = {ARENA_X, ARENA_Y, ARENA_WIDTH, ARENA_HEIGHT};

    /* Bordure autour de l'arène (10 pixels de large) */
    SDL_Rect borderRect = {ARENA_X - 10, ARENA_Y - 10, ARENA_WIDTH + 20, ARENA_HEIGHT + 20};

    SDL_Rect t_ball = {0, 0, 2 * BALL_RADIUS, 2 * BALL_RADIUS}; /* Boule transparente (lorsqu'une carte est sélectionnée) */

    Object objects[max_objects];

    Card cards[NB_CARDS];
    for (int i = 0; i < NB_CARDS; i++)
    {
        int k = i % 3;
        cards[i].texture = textures[3];
        cards[i].rect.x = 900 - 375 * (k % 2) + 125 * k; /* première au centre, deuxième à gauche, troisième à droite */
        cards[i].rect.y = 150 + 400 * (i / 3);           /* on change de ligne tous les 3 */
        cards[i].rect.w = 150;
        cards[i].rect.h = 250;
        cards[i].exist = true;
        cards[i].is_selected = false;
        cards[i].ready_at = 0;
    }

    /* Obstacles */
    createWall(renderer, objects, 180, 20, (Vect){ARENA_WIDTH / 2, ARENA_HEIGHT / 2}, WALL_COLOR, num_objects_list);
    createWall(renderer, objects, 20, 120, (Vect){120, ARENA_HEIGHT / 4}, WALL_COLOR, num_objects_list);
    createWall(renderer, objects, 20, 120, (Vect){ARENA_WIDTH - 120, 3 * ARENA_HEIGHT / 4}, WALL_COLOR, num_objects_list);

    /* Boules de départ : blanches à gauche, noires à droite */
    placeStartBalls(renderer, objects, num_objects_list, &nb_color_balls, WHITE, 0);
    placeStartBalls(renderer, objects, num_objects_list, &nb_color_balls, BLACK, 1);

    TTF_Font *font = titleText.font;
    Text scoreText = {NULL, 0, 0, font, titleText.color};
    Vect shownCounts = {-1, -1};
    Text hintText = createText(renderer, font, "Pick a card, then click in the arena", titleText.color);

    int selected = -1;
    SDL_Event event;
    Uint64 lastAi = SDL_GetTicks64();
    Uint64 frameTime = SDL_GetTicks64();
    int result = QUIT;
    bool running = true;

    while (running)
    {
        Uint64 frameStart = SDL_GetTicks64();

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                result = QUIT;
                running = false;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
            {
                selected = -1;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                Vect click = {event.button.x, event.button.y};
                bool onCard = false;

                for (int i = 0; i < NB_CARDS; i++)
                {
                    if (cards[i].exist && isInBox(click, cards[i].rect))
                    {
                        selected = i;
                        onCard = true;
                    }
                }

                if (!onCard && selected >= 0)
                {
                    Vect inArena = {click.x - ARENA_X, click.y - ARENA_Y};
                    if (spawnBall(renderer, objects, num_objects_list, &nb_color_balls, inArena, BLACK))
                    {
                        cards[selected].exist = false;
                        cards[selected].ready_at = frameStart + CARD_COOLDOWN_MS;
                        selected = -1;
                    }
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                t_ball.x = event.motion.x - ARENA_X - BALL_RADIUS;
                t_ball.y = event.motion.y - ARENA_Y - BALL_RADIUS;
            }
        }

        if (!running)
        {
            break;
        }

        /* Cartes */
        for (int i = 0; i < NB_CARDS; i++)
        {
            if (!cards[i].exist && frameStart >= cards[i].ready_at)
            {
                cards[i].exist = true;
            }
            cards[i].is_selected = (i == selected);
            cards[i].texture = !cards[i].exist ? textures[0] : (cards[i].is_selected ? textures[2] : textures[3]);
        }

        /* L'IA joue : probabilité fonction du nombre de boules noires par rapport au total */
        if (frameStart - lastAi >= AI_PERIOD_MS)
        {
            lastAi = frameStart;
            double proba = sqrt(nb_color_balls.y / (nb_color_balls.x + nb_color_balls.y));
            if (rand() % 100 < proba * 100)
            {
                aiPlay(renderer, num_objects_list, objects, &nb_color_balls);
            }
        }

        /* Mise à jour des objets */
        Uint64 elapsedTime = frameStart - frameTime;
        if (elapsedTime > MAX_FRAME_MS)
        {
            elapsedTime = MAX_FRAME_MS;
        }
        frameTime = frameStart;
        updateObjects(elapsedTime, objects, num_objects_list, &nb_color_balls, ARENA_DIMENSIONS);

        /* Fin de partie */
        if (nb_color_balls.x == 0 || nb_color_balls.y == 0)
        {
            result = nb_color_balls.x == 0 ? BLACK_WINS : WHITE_WINS;
            running = false;
            break;
        }

        if (nb_color_balls.x != shownCounts.x || nb_color_balls.y != shownCounts.y)
        {
            shownCounts = nb_color_balls;
            updateScoreText(renderer, &scoreText, font, nb_color_balls);
        }

        /* Rendu */
        SDL_RenderSetViewport(renderer, NULL);
        SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &borderRect);

        SDL_RenderSetViewport(renderer, &arenaViewport);
        SDL_SetRenderDrawColor(renderer, 217, 217, 217, 255);
        SDL_RenderFillRect(renderer, NULL);

        drawObjects(renderer, objects, num_objects_list);

        if (selected >= 0)
        {
            SDL_RenderCopy(renderer, textures[4], NULL, &t_ball);
        }

        /* Éléments hors de l'arène */
        SDL_RenderSetViewport(renderer, NULL);

        for (int i = 0; i < NB_CARDS; i++)
        {
            SDL_RenderCopy(renderer, cards[i].texture, NULL, &cards[i].rect);
        }

        drawTextAt(renderer, &titleText, (SCREEN_WIDTH + ARENA_WIDTH) / 2, 50);
        drawTextAt(renderer, &scoreText, (SCREEN_WIDTH + ARENA_WIDTH) / 2, 110);
        drawTextAt(renderer, &hintText, (SCREEN_WIDTH + ARENA_WIDTH) / 2, SCREEN_HEIGHT - 60);

        SDL_RenderPresent(renderer);

        /* Limiteur d'images par seconde */
        Uint64 spent = SDL_GetTicks64() - frameStart;
        if (spent < 1000 / FPS)
        {
            SDL_Delay(1000 / FPS - spent);
        }
    }

    deleteAllObjects(objects, num_objects_list, &nb_color_balls);
    destroyText(&scoreText);
    destroyText(&hintText);
    SDL_RenderSetViewport(renderer, NULL);

    return result;
}

/* Écran avec un bouton central ; renvoie true si le bouton est cliqué, false si on quitte.
   `headline` (peut être NULL) est affiché au-dessus du bouton. */
static bool buttonScreen(SDL_Renderer *renderer, Text titleText, const char *label, const char *headline)
{
    bool startGame = false;
    bool isRunning = true;
    SDL_Event event;

    SDL_Rect button = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2 - 50, 200, 100};
    Text buttonText = createText(renderer, titleText.font, label, titleText.color);
    Text headlineText = {NULL, 0, 0, titleText.font, titleText.color};
    if (headline)
    {
        headlineText = createText(renderer, titleText.font, headline, titleText.color);
    }

    while (isRunning)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
            {
                isRunning = false;
            }
            else if (event.type == SDL_KEYDOWN && (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE))
            {
                isRunning = false;
                startGame = true;
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                if (isInBox((Vect){event.button.x, event.button.y}, button))
                {
                    isRunning = false;
                    startGame = true;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 217, 217, 217, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &button);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &button);

        drawTextAt(renderer, &titleText, SCREEN_WIDTH / 2, 50);
        drawTextAt(renderer, &headlineText, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 200);
        drawTextAt(renderer, &buttonText, button.x + button.w / 2, button.y + button.h / 2);

        SDL_RenderPresent(renderer);
        SDL_Delay(1000 / FPS);
    }

    destroyText(&headlineText);
    destroyText(&buttonText);

    return startGame;
}

bool welcomeScreen(SDL_Renderer *renderer, Text titleText)
{
    return buttonScreen(renderer, titleText, "Play", "Destroy the white balls !");
}

bool replayScreen(SDL_Renderer *renderer, Text titleText, int winner)
{
    if (winner == QUIT)
    {
        return false;
    }
    return buttonScreen(renderer, titleText, "Replay", winner == BLACK_WINS ? "You win !" : "You lose !");
}
