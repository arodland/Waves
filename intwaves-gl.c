#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>

#ifdef __WIN32__
#include <SDL.h>
#include <SDL_opengl.h>
#include <windows.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#endif

#include "colors.i"


#define WIDTH (1280)
#define HEIGHT (720)


struct Vertex {
    float tu, tv;
    float x, y, z;
};

struct Vertex quad_vertices[] = {
    { 0.0f,0.0f, -1.0f,-1.0f, 0.0f },
    { 1.0f,0.0f,  1.0f,-1.0f, 0.0f },
    { 1.0f,1.0f,  1.0f, 1.0f, 0.0f },
    { 0.0f,1.0f, -1.0f, 1.0f, 0.0f }
};

GLuint tex = 0;

int VEL_LIMIT = 15 * 256;
const int TRANSFER = 8;
int DRAG = 0;
int RAIN = 0;
int NONLINEAR = 0;
int COLOROFFSET = 0;
int BRUSHTYPE = 1;
int BRUSHSOFT = 1;
int BRUSHADD = 1;
int MODE = 1;
int WRAP = 0;

int *data, *v_x, *v_y,*old;

int row_up[HEIGHT], row_down[HEIGHT], col_left[WIDTH], col_right[WIDTH];

SDL_Surface *screen;

void set_nowrap();

void init_data () {
    if ((data = malloc(WIDTH * HEIGHT * sizeof(*data))) == NULL) {
        perror("Allocating data");
        abort();
    }

#ifndef TWOLOOP
    if ((old = malloc(WIDTH * HEIGHT * sizeof(*data))) == NULL) {
        perror("Allocating data");
        abort();
    }
#endif

    if ((v_x = malloc(WIDTH * HEIGHT * sizeof(*v_x))) == NULL) {
        perror("Allocating v_x");
        abort();
    }

    if ((v_y = malloc(WIDTH * HEIGHT * sizeof(*v_y))) == NULL) {
        perror("Allocating v_y");
        abort();
    }

    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            data[y * WIDTH + x] = 16 * 256;
            v_x[y * WIDTH + x] = 0;
            v_y[y * WIDTH + x] = 0;
        }
    }

    for (int i = 0 ; i < HEIGHT ; i++) {
      row_up[i] = i - 1;
      row_down[i] = i + 1;
    }

    for (int i = 0 ; i < WIDTH ; i++) {
      col_left[i] = i - 1;
      col_right[i] = i + 1;
    }

    set_nowrap();

}

void set_nowrap() {
  row_up[0] = 1;
  row_down[HEIGHT - 1] = HEIGHT - 2;
  col_left[0] = 1;
  col_right[WIDTH - 1 ] = WIDTH - 2;
}

void set_wrap() {
  row_up[0] = HEIGHT - 1;
  row_down[HEIGHT - 1] = 0;
  col_left[0] = WIDTH - 1;
  col_right[WIDTH - 1] = 0;
}

void update_slice (int firstrow, int lastrow) {

#if TWOLOOP
    for (int y = firstrow ; y < lastrow ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            int north = row_up[y] * WIDTH + x, south = row_down[y] * WIDTH + x;
            int west = y * WIDTH + col_left[x], east = y * WIDTH + col_right[x];
            if (NONLINEAR) {
                if (data[west] > data[east]) {
                    v_x[cur] -= ((data[west] - data[east]) * (v_x[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_x[cur] += ((data[west] - data[east]) * (v_x[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }

                if (data[north] > data[south]) {
                    v_y[cur] -= ((data[north] - data[south]) * (v_y[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_y[cur] += ((data[north] - data[south]) * (v_y[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }
                if (v_x[cur] > VEL_LIMIT) v_x[cur] = VEL_LIMIT;
                if (v_x[cur] < -VEL_LIMIT) v_x[cur] = -VEL_LIMIT;
                if (v_y[cur] > VEL_LIMIT) v_y[cur] = VEL_LIMIT;
                if (v_y[cur] < -VEL_LIMIT) v_y[cur] = -VEL_LIMIT;

            } else {
                v_x[cur] += (data[west] - data[east]) / TRANSFER;
                //                if (v_x[cur] < -VEL_LIMIT) v_x[cur] = -VEL_LIMIT; 
                //                if (v_x[cur] > VEL_LIMIT) v_x[cur] = VEL_LIMIT;
                v_x[cur] = (v_x[cur] * (1024 - DRAG) / 1024);


                v_y[cur] += (data[north] - data[south]) / TRANSFER;
                //                if (v_y[cur] < -VEL_LIMIT) v_y[cur] = -VEL_LIMIT;
                //                if (v_y[cur] > VEL_LIMIT) v_y[cur] = VEL_LIMIT;
                v_y[cur] = (v_y[cur] * (1024 - DRAG) / 1024);
            }

        }
    }

    for (int y = firstrow ; y < lastrow ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            int north = row_up[y] * WIDTH + x, south = row_down[y] * WIDTH + x;
            int west = y * WIDTH + col_left[x], east = y * WIDTH + col_right[x];
            data[west] -= v_x[cur]; data[east] += v_x[cur];
            data[north] -= v_y[cur]; data[south] += v_y[cur];
        }
    }

#else

    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            int north = row_up[y] * WIDTH + x, south = row_down[y] * WIDTH + x;
            int west = y * WIDTH + col_left[x], east = y * WIDTH + col_right[x];
            if (NONLINEAR) {
                if (old[west] > old[east]) {
                    v_x[cur] -= ((old[west] - old[east]) * (v_x[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_x[cur] += ((old[west] - old[east]) * (v_x[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }

                if (old[north] > old[south]) {
                    v_y[cur] -= ((old[north] - old[south]) * (v_y[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_y[cur] += ((old[north] - old[south]) * (v_y[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }
            } else {
                v_x[cur] += (old[west] - old[east]) / TRANSFER;
                v_y[cur] += (old[north] - old[south]) / TRANSFER;
            }

            data[west] -= v_x[cur]; data[east] += v_x[cur];
            data[north] -= v_y[cur]; data[south] += v_y[cur];

        }
    }

#endif
}

void update () {
#if ! TWOLOOP
  memcpy(old, data, WIDTH * HEIGHT * sizeof(*data));
#endif

  update_slice(0, HEIGHT);
}

void set_palette() {
    GLfloat red[256], green[256], blue[256];

    for (int i = 0; i < 256 ; i++) {
        // red[i] = (float)i / 256;
        // green[i] = (float)i / 256;
        // blue[i] = 0.5 + (float)i / 512;
        red[i] = colors[i+COLOROFFSET][0];
        green[i] = colors[i+COLOROFFSET][1];
        blue[i] = colors[i+COLOROFFSET][2];
    }

    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, red);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, green);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, blue);

    glPixelTransferi(GL_INDEX_SHIFT, -8);


    // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    // glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);


}

void draw() {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, WIDTH, HEIGHT, 0, GL_COLOR_INDEX, GL_INT, data);

    glInterleavedArrays( GL_T2F_V3F, 0, quad_vertices );
    glDrawArrays( GL_QUADS, 0, 4 );

    SDL_GL_SwapBuffers();


}

void invert() {
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            data[WIDTH * y + x] = (256 * 256) - data[WIDTH * y + x];
        }
    }
}

void invert_vel() {
    for (int y = 0 ; y < HEIGHT; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            v_x[WIDTH * y + x] = 0 - v_x[WIDTH * y + x];
            v_y[WIDTH * y + x] = 0 - v_y[WIDTH * y + x];
        }
    }
}

void clear() {
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            data[WIDTH * y + x] = 16 * 256;
        }
    }
}

void clear_vel() {
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            v_x[WIDTH * y + x] = 0;
            v_y[WIDTH * y + x] = 0;
        }
    }
}

void randomize() {
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            data[WIDTH * y + x] = (rand() % 65536);
        }
    }
}

char status_buf[100];

#define status(fmt, args... ) \
{ \
    snprintf(status_buf, 100, fmt, ## args); \
    SDL_WM_SetCaption(status_buf, "Waves"); \
    fprintf(stderr, "%s\n", status_buf); \
}

static int fullscreen = 0;
static int scr_width = WIDTH;
static int scr_height = HEIGHT;

void init_gl () {

    glClearColor( 0.0f, 0.0f, 0.5f, 1.0f );
    glEnable( GL_TEXTURE_2D );

    glOrtho(0.0f, (float)scr_width, (float)scr_height, 0.0f, 0.0f, -2.0f);

    glViewport(0, 0, scr_width, scr_height);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    set_palette();
}

void init_screen () {
    const SDL_VideoInfo *vid_info = SDL_GetVideoInfo();

    int flags = SDL_OPENGL | SDL_HWPALETTE | SDL_RESIZABLE;

    if (fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    if (vid_info->hw_available) 
        flags |= SDL_HWSURFACE;
    else
        flags |= SDL_SWSURFACE;

    if (vid_info->blit_hw)
        flags |= SDL_HWACCEL;

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    screen = SDL_SetVideoMode(scr_width, scr_height, 32, flags);
    init_gl();

    if (!screen) {
        fprintf(stderr, "Couldn't get a screen.\n");
        exit(1);
    }


}

int main (int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL died.\n");
        exit(1);
    }

    init_screen();

    init_data();

    status("Waves running...");

    int x, y, down = 0;
    int pause = 0;
    int brushsize = 32;
    int fps = 0, frames = 0;
    int prev_ticks = SDL_GetTicks();

    while (1) {
        SDL_Event e;

        if (!pause) {
            update();
        }


        draw();
        if (fps) {
            int ticks = SDL_GetTicks();
            frames++;
            if (ticks - prev_ticks >= 1000) {
                status("Waves: %d FPS", (int)(frames * 1000 / (ticks - prev_ticks)));
                prev_ticks = ticks;
                frames = 0;
            }
        }


        while (SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_VIDEORESIZE:
                    scr_width = e.resize.w;
                    scr_height = e.resize.h;
                    init_screen();
                    break;

                case SDL_MOUSEMOTION:
                    x = e.motion.x * WIDTH / scr_width;
                    y = HEIGHT - e.motion.y * HEIGHT / scr_height;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    down = e.button.button;
                    break;
                case SDL_MOUSEBUTTONUP:
                    down = 0;
                    break;
                case SDL_KEYDOWN:
                    switch(e.key.keysym.sym) {
/*                        case SDLK_MINUS:
                            TRANSFER -= 1;
                            status("Viscosity: %d", TRANSFER);
                            break; */
/*                        case SDLK_EQUALS:
                            TRANSFER += 1;
                            status("Viscosity: %d", TRANSFER);
                            break; */
/*                        case SDLK_z:
                            TRANSFER *= -1;
                            status("Viscosity: %d", TRANSFER);
                            break; */
                        case SDLK_t:
                            BRUSHTYPE = !BRUSHTYPE ;
                            status("Brushtype: %d", BRUSHTYPE);
                            break;

                        case SDLK_s:
                            BRUSHSOFT = !BRUSHSOFT ;
                            status("Brushsoft: %d", BRUSHSOFT);
                            break;
                        case SDLK_a:
                            BRUSHADD = !BRUSHADD ;
                            status("BrushAdd: %d", BRUSHADD);
                            break;              
                        case SDLK_f:
                            fps = 1 - fps;
                            status("FPS: %s", fps?"on":"off")
                                break;
                        case SDLK_SEMICOLON:
                            VEL_LIMIT = 4 * VEL_LIMIT / 5;
                            status("VLimit: %d", VEL_LIMIT);
                            break;
                        case SDLK_QUOTE:
                            VEL_LIMIT = 5 * VEL_LIMIT / 4 ;
                            status("VLimit: %d", VEL_LIMIT);
                            break;
                        case SDLK_w:
                            WRAP = 1 - WRAP;
                            if (WRAP) {
                              set_wrap();
                            } else {
                              set_nowrap();
                            }
                            status("Wrap: %d", WRAP);
                            break;
                        case SDLK_i:
                            invert();
                            break;
                        case SDLK_v:
                            invert_vel();
                            break;
                        case SDLK_r:
                            randomize();
                            break;
                        case SDLK_n:
                            NONLINEAR = (NONLINEAR + 1) % 2;
                            status("Nonlinear mode: %d", NONLINEAR)
                                break;	
                        case SDLK_p:
                            pause = 1 - pause;
                            if(pause) status("Waves Paused") 
                            else status("Waves Running") 
                                break;
                        case SDLK_SLASH:
                            clear();
                            clear_vel();
                            break;
                        case SDLK_PERIOD:
                            clear();
                            break;
                        case SDLK_COMMA:
                            clear_vel();
                            break;
                        case SDLK_q: case SDLK_ESCAPE:
                            fullscreen = 0;
                            init_screen();
                            return(0);
                            break;
                        case SDLK_LEFTBRACKET:
                            brushsize--;
                            status("Brush size: %d", brushsize);
                            break;
                        case SDLK_RIGHTBRACKET:
                            brushsize++;
                            status("Brush size: %d", brushsize);
                            break;
                        case SDLK_F11:
                            DRAG--;
                            status("Drag: %d/1024", DRAG);
                            break;
                        case SDLK_F12:
                            DRAG++;
                            status("Drag: %d/1024", DRAG);
                            break;
                        case SDLK_F1:
                            RAIN--;
                            if (RAIN<0) RAIN=0;
                            status("Rain: %d", RAIN);
                            break;
                        case SDLK_F2:
                            RAIN++;
                            status("Rain: %d", RAIN);
                            break;

                        case SDLK_F3:
                            COLOROFFSET -=8 ;
                            if (COLOROFFSET<0) COLOROFFSET=0;
                            status("ColorOffset: %d", COLOROFFSET);
                            set_palette();
                            break;
                        case SDLK_F4:
                            COLOROFFSET += 8;
                            if (COLOROFFSET > num_colors - 256) COLOROFFSET = num_colors - 256;
                            status("ColorOffset: %d", COLOROFFSET);
                            set_palette();
                            break;
                        case SDLK_F9:
                            fullscreen = 1 - fullscreen;
                            init_screen();
                            break;


                    }
                    break;
                case SDL_QUIT:
                    fullscreen = 0;
                    init_screen();
                    return(0);
                    break;
            }
        }

        if (down) {
            for (int j = y - brushsize ; j < y + brushsize ; j++) {
                if (j < 0 || j >= HEIGHT) continue;
                for (int k = x - brushsize ; k < x + brushsize ; k++) {
                    if (k < 0 || k >= WIDTH) continue;
                    float disty = sqrt( ((k-x)*(k-x)) + ((j-y)*(j-y)) ) ;
                    float ity = 1 ;
                    if (BRUSHTYPE == 1)
                    {
                        if (disty>(brushsize) ) continue ;
                    }

                    if (BRUSHSOFT ==1)
                    {
                        ity = 1-(disty/brushsize);
                    }

                    if (down == 1) {
                        if (BRUSHADD==1)
                        { data[WIDTH * j + k]  += (256 * 256 *ity) ;}
                        else
                        {data[WIDTH * j + k]  = (256 * 256 *ity) ;}
                        if (data[WIDTH * j + k] <0 ||data[WIDTH * j + k] > (256*256) ) { data[WIDTH * j + k] = (256*256);}

                    }
                    else {
                        data[WIDTH * j + k] = 0;
                        v_x[WIDTH * j + k] = 0; v_y[WIDTH * j + k] = 0;
                    }

                }
            }
        }
        if (RAIN && ((rand() % 100) < RAIN)) {
            int x = 1 + rand() % (WIDTH - 2);
            int y = 1 + rand() % (HEIGHT - 2);

            for (int j = y - brushsize ; j < y + brushsize ; j++) {
                if (j < 0 || j >= HEIGHT) continue;
                for (int k = x - brushsize ; k < x + brushsize ; k++) {
                    if (k < 0 || k >= WIDTH) continue;
                    float disty = sqrt( ((k-x)*(k-x)) + ((j-y)*(j-y)) ) ;
                    float ity = 1 ;
                    if (BRUSHTYPE == 1)
                    {
                        if (disty>(brushsize) ) continue ;
                    }

                    if (BRUSHSOFT ==1)
                    {
                        ity = 1-(disty/brushsize);
                    }
                    if (BRUSHADD==1)
                    { data[WIDTH * j + k]  += (256 * 256 *ity) ;}
                    else
                    {data[WIDTH * j + k]  = (256 * 256 *ity) ;}
                    if (data[WIDTH * j + k] <0 ||data[WIDTH * j + k] > (256*256) ) 
                    { data[WIDTH * j + k] = (256*256);}
                }
            }      
        }
    }
    return 0;
}

#ifdef __WIN32__
int WINAPI WinMain (HINSTANCE hThisInstance, HINSTANCE hPrevInstance, LPSTR lpszArgument,
        int nFunsterStil) {
    return main(0, NULL);
}
#endif
