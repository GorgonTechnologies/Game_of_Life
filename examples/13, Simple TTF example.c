/*
  glfont:  An example of using the SDL_ttf library with OpenGL.
  Copyright (C) 2001-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* A simple program to test the text rendering feature of the TTF library */

/* quiet windows compiler warnings */
#define _CRT_SECURE_NO_WARNINGS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <SDL3/SDL_opengl.h>

#define DEFAULT_PTSIZE  18.0f
#define DEFAULT_TEXT    "The quick brown fox jumped over the lazy dog"
// /usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"
#define DEFAULT_FONT    "./examples/resources/LiberationSans-Regular.ttf"
#define WIDTH   640
#define HEIGHT  480

#define TTF_GLFONT_USAGE \
"Usage: %s [-b] [-i] [-u] [-fgcol r,g,b] [-bgcol r,g,b] \
<font>.ttf [ptsize] [text]\n"

static void SDL_GL_Enter2DMode(int width, int height)
{
    /* Note, there may be other things you need to change,
       depending on how you have your OpenGL state set up.
    */
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_2D);

    /* This allows alpha blending of 2D textures with the scene */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(0.0, (GLdouble)width, (GLdouble)height, 0.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
}

static void SDL_GL_Leave2DMode(void)
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();
}

/* Quick utility function for texture creation */
static int power_of_two(int input)
{
    int value = 1;

    while (value < input) {
        value <<= 1;
    }
    return value;
}

static GLuint SDL_GL_LoadTexture(SDL_Surface *surface, GLfloat *texcoord)
{
    GLuint texture;
    int w, h;
    SDL_Surface *image;
    SDL_Rect area;
    Uint8  saved_alpha;
    SDL_BlendMode saved_mode;

    /* Use the surface width and height expanded to powers of 2 */
    w = power_of_two(surface->w);
    h = power_of_two(surface->h);
    texcoord[0] = 0.0f;         /* Min X */
    texcoord[1] = 0.0f;         /* Min Y */
    texcoord[2] = (GLfloat)surface->w / w;  /* Max X */
    texcoord[3] = (GLfloat)surface->h / h;  /* Max Y */

    image = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
    if (image == NULL) {
        return 0;
    }

    /* Save the alpha blending attributes */
    SDL_GetSurfaceAlphaMod(surface, &saved_alpha);
    SDL_SetSurfaceAlphaMod(surface, 0xFF);
    SDL_GetSurfaceBlendMode(surface, &saved_mode);
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

    /* Copy the surface into the GL texture image */
    area.x = 0;
    area.y = 0;
    area.w = surface->w;
    area.h = surface->h;
    SDL_BlitSurface(surface, &area, image, &area);

    /* Restore the alpha blending attributes */
    SDL_SetSurfaceAlphaMod(surface, saved_alpha);
    SDL_SetSurfaceBlendMode(surface, saved_mode);

    /* Create an OpenGL texture for the image */
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D,
             0,
             GL_RGBA,
             w, h,
             0,
             GL_RGBA,
             GL_UNSIGNED_BYTE,
             image->pixels);
    SDL_DestroySurface(image); /* No longer needed */

    return texture;
}

static void cleanup(int exitcode)
{
    TTF_Quit();
    SDL_Quit();
    exit(exitcode);
}

int main(int argc, char *argv[])
{
    char *argv0 = argv[0];
    SDL_Window *window;
    SDL_GLContext context;
    TTF_Font *font;
    SDL_Surface *text = NULL;
    float ptsize;
    int i, done;
    SDL_Color white = { 0xFF, 0xFF, 0xFF, SDL_ALPHA_OPAQUE };
    SDL_Color black = { 0x00, 0x00, 0x00, SDL_ALPHA_OPAQUE };
    SDL_Color *forecol;
    SDL_Color *backcol;
    GLenum gl_error;
    GLuint texture;
    int x, y, w, h;
    GLfloat texcoord[4];
    GLfloat texMinX, texMinY;
    GLfloat texMaxX, texMaxY;
    SDL_Event event;
    int renderstyle;
    int dump;
    char *message;

    /* Look for special execution mode */
    dump = 0;
    /* Look for special rendering types */
    renderstyle = TTF_STYLE_NORMAL;
    /* Default is black and white */
    forecol = &white;
    backcol = &black;
    for (i=1; argv[i] && argv[i][0] == '-'; ++i) {
        if (SDL_strcmp(argv[i], "-b") == 0) {
            renderstyle |= TTF_STYLE_BOLD;
        } else
        if (SDL_strcmp(argv[i], "-i") == 0) {
            renderstyle |= TTF_STYLE_ITALIC;
        } else
        if (SDL_strcmp(argv[i], "-u") == 0) {
            renderstyle |= TTF_STYLE_UNDERLINE;
        } else
        if (SDL_strcmp(argv[i], "-dump") == 0) {
            dump = 1;
        } else
        if (SDL_strcmp(argv[i], "-fgcol") == 0) {
            int r, g, b;
            if (sscanf (argv[++i], "%d,%d,%d", &r, &g, &b) != 3) {
                fprintf(stderr, TTF_GLFONT_USAGE, argv0);
                return(1);
            }
            forecol->r = (Uint8)r;
            forecol->g = (Uint8)g;
            forecol->b = (Uint8)b;
        } else
        if (SDL_strcmp(argv[i], "-bgcol") == 0) {
            int r, g, b;
            if (sscanf (argv[++i], "%d,%d,%d", &r, &g, &b) != 3) {
                fprintf(stderr, TTF_GLFONT_USAGE, argv0);
                return(1);
            }
            backcol->r = (Uint8)r;
            backcol->g = (Uint8)g;
            backcol->b = (Uint8)b;
        } else {
            fprintf(stderr, TTF_GLFONT_USAGE, argv0);
            return(1);
        }
    }
    argv += i;
    argc -= i;

    /* Check usage */
    if (!argv[0]) {
        fprintf(stderr, TTF_GLFONT_USAGE, argv0);
        return(1);
    }

    /* Initialize the TTF library */
    if (!TTF_Init()) {
        fprintf(stderr, "Couldn't initialize TTF: %s\n",SDL_GetError());
        SDL_Quit();
        return(2);
    }

    /* Open the font file with the requested point size */
    ptsize = 0.0f;
    if (argc > 1) {
        ptsize = (float)SDL_atof(argv[1]);
    }
    if (ptsize == 0.0f) {
        i = 2;
        ptsize = DEFAULT_PTSIZE;
    } else {
        i = 3;
    }
    font = TTF_OpenFont(DEFAULT_FONT/*argv[0]*/, ptsize);
    if (font == NULL) {
        fprintf(stderr, "Couldn't load %g pt font from %s: %s\n",
                    ptsize, argv[0], SDL_GetError());
        cleanup(2);
    }
    TTF_SetFontStyle(font, renderstyle);

    if(dump) {
        for(i = 48; i < 123; i++) {
            SDL_Surface* glyph = NULL;

            glyph = TTF_RenderGlyph_Shaded(font, i, *forecol, *backcol);

            if(glyph) {
                char outname[64];
                sprintf(outname, "glyph-%d.bmp", i);
                SDL_SaveBMP(glyph, outname);
            }

        }
        cleanup(0);
    }

    /* Set a 640x480 video mode */
    window = SDL_CreateWindow("glfont", WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (window == NULL) {
        fprintf(stderr, "Couldn't create window: %s\n", SDL_GetError());
        cleanup(2);
    }

    context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        fprintf(stderr, "Couldn't create OpenGL context: %s\n", SDL_GetError());
        cleanup(2);
    }

    /* Render and center the message */
    if (argc > 2) {
        message = argv[2];
    } else {
        message = DEFAULT_TEXT;
    }
    text = TTF_RenderText_Blended(font, message, 0, *forecol);
    if (text == NULL) {
        fprintf(stderr, "Couldn't render text: %s\n", SDL_GetError());
        TTF_CloseFont(font);
        cleanup(2);
    }
    w = text->w;
    h = text->h;
    printf("Font is generally %d big, and string is %d big\n",
                        TTF_GetFontHeight(font), text->h);

    /* Convert the text into an OpenGL texture */
    glGetError();
    texture = SDL_GL_LoadTexture(text, texcoord);
    if ((gl_error = glGetError()) != GL_NO_ERROR) {
        /* If this failed, the text may exceed texture size limits */
        printf("Warning: Couldn't create texture: 0x%x\n", gl_error);
    }

    /* Make texture coordinates easy to understand */
    texMinX = texcoord[0];
    texMinY = texcoord[1];
    texMaxX = texcoord[2];
    texMaxY = texcoord[3];

    /* We don't need the original text surface anymore */
    SDL_DestroySurface(text);

    glShadeModel(GL_SMOOTH);

    /* Wait for a keystroke, and blit text on mouse press */
    done = 0;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    done = 1;
                    break;
                default:
                    break;
            }
        }

        /* Clear the screen */
        glClearColor(.3, .3, .3, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Show the text on the screen */
        SDL_GL_Enter2DMode(WIDTH, HEIGHT);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(texMinX, texMinY); glVertex2i(0,   0);
        glTexCoord2f(texMaxX, texMinY); glVertex2i(0+w, 0);
        glTexCoord2f(texMinX, texMaxY); glVertex2i(0,   0+h);
        glTexCoord2f(texMaxX, texMaxY); glVertex2i(0+w, 0+h);
        glEnd();
        SDL_GL_Leave2DMode();

        /* Swap the buffers so everything is visible */
        SDL_GL_SwapWindow(window);
    }
    SDL_GL_DestroyContext(context);
    TTF_CloseFont(font);
    cleanup(0);

    /* Not reached, but fixes compiler warnings */
    return 0;
}
