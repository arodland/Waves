#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include "colors.i"


#define SCR_WIDTH (1280)
#define SCR_HEIGHT (768)

#define WIDTH (960)
#define HEIGHT (540)

#define TWOLOOP 1


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
int COLOROFFSET = 760;
int BRUSHTYPE = 1;
int BRUSHSOFT = 1;
int BRUSHADD = 1;
int MODE = 1;

struct {
	int w;
	int e;
	int n;
	int s;
} *dir;

int *data, *v_x, *v_y,*new;

SDL_Surface *screen;

void init_data () {
    if ((data = malloc(WIDTH * HEIGHT * sizeof(*data))) == NULL) {
        perror("Allocating data");
        abort();
    }
    if ((new = malloc(WIDTH * HEIGHT * sizeof(*data))) == NULL) {
        perror("Allocating data");
        abort();
    }

    if ((v_x = malloc(WIDTH * HEIGHT * sizeof(*v_x))) == NULL) {
        perror("Allocating v_x");
        abort();
    }

    if ((v_y = malloc(WIDTH * HEIGHT * sizeof(*v_y))) == NULL) {
        perror("Allocating v_y");
        abort();
    }

	if ((dir = malloc(WIDTH * HEIGHT * sizeof(*dir))) == NULL) {
		perror("Allocating dir");
		abort();
	}

    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
			int cur = WIDTH * y + x;
            data[cur] = 128 * 256;
            v_x[cur] = 0;
            v_y[cur] = 0;

			dir[cur].n = (y == 0 ? cur + WIDTH : cur - WIDTH);
			dir[cur].s = (y == HEIGHT - 1 ? cur - WIDTH : cur + WIDTH);
			dir[cur].w = (x == 0 ? cur + 1 : cur - 1);
			dir[cur].e = (x == WIDTH - 1 ? cur - 1 : cur + 1);
        }
    }
}

void update () {

#if TWOLOOP
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            if (NONLINEAR) {
                if (data[dir[cur].w] > data[dir[cur].e]) {
                    v_x[cur] -= ((data[dir[cur].w] - data[dir[cur].e]) * (v_x[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_x[cur] += ((data[dir[cur].w] - data[dir[cur].e]) * (v_x[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }

                if (data[dir[cur].n] > data[dir[cur].s]) {
                    v_y[cur] -= ((data[dir[cur].n] - data[dir[cur].s]) * (v_y[cur] - VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                } else {
                    v_y[cur] += ((data[dir[cur].n] - data[dir[cur].s]) * (v_y[cur] + VEL_LIMIT) / VEL_LIMIT) / TRANSFER;
                }
            } else {
                v_x[cur] += (data[dir[cur].w] - data[dir[cur].e]) / TRANSFER;
//                if (v_x[cur] < -VEL_LIMIT) v_x[cur] = -VEL_LIMIT; 
//                if (v_x[cur] > VEL_LIMIT) v_x[cur] = VEL_LIMIT;
                //v_x[cur] = (v_x[cur] * (1024 - DRAG) / 1024);


                v_y[cur] += (data[dir[cur].n] - data[dir[cur].s]) / TRANSFER;
//                if (v_y[cur] < -VEL_LIMIT) v_y[cur] = -VEL_LIMIT;
//                if (v_y[cur] > VEL_LIMIT) v_y[cur] = VEL_LIMIT;
                //v_y[cur] = (v_y[cur] * (1024 - DRAG) / 1024);
            }

        }
    }

    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            data[dir[cur].w] -= v_x[cur]; data[dir[cur].e] += v_x[cur];
            data[dir[cur].n] -= v_y[cur]; data[dir[cur].s] += v_y[cur];
        }
    }
#else

    memcpy(new, data, WIDTH * HEIGHT * sizeof(*data));
    for (int y = 0 ; y < HEIGHT ; y++) {
        for (int x = 0 ; x < WIDTH ; x++) {
            int cur = WIDTH * y + x;

            v_x[cur] += (data[dir[cur].w] - data[dir[cur].e]) / TRANSFER;
            //v_x[cur] = (v_x[cur] * (1024 - DRAG) / 1024);
            // if (v_x[cur] < 0) v_x[cur] = 0; 
            // if (v_x[cur] > VEL_LIMIT) v_x[cur] = VEL_LIMIT;
            new[dir[cur].w] -= v_x[cur]; new[dir[cur].e] += v_x[cur];

            v_y[cur] += (data[dir[cur].n] - data[dir[cur].s]) / TRANSFER;
            //v_y[cur] = (v_y[cur] * (1024 - DRAG) / 1024);

            // if (v_y[cur] < 0 ) v_y[cur] = 0;
            // if (v_y[cur] > VEL_LIMIT) v_y[cur] = VEL_LIMIT;
            new[dir[cur].n] -= v_y[cur]; new[dir[cur].s] += v_y[cur];
        }
    }

    int *tmp = data;
    data = new;
    new = tmp;
#endif
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

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
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

void init_gl () {
    glClearColor( 0.0f, 0.0f, 0.5f, 1.0f );
    glEnable( GL_TEXTURE_2D );

    glOrtho(0.0f, (float)SCR_WIDTH, (float)SCR_HEIGHT, 0.0f, 0.0f, -2.0f);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

    set_palette();


}

int main (int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL died.\n");
        exit(1);
    }

    const SDL_VideoInfo *vid_info = SDL_GetVideoInfo();

    int flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE;
    if (vid_info->hw_available) 
        flags |= SDL_HWSURFACE;
    else
        flags |= SDL_SWSURFACE;

    if (vid_info->blit_hw)
        flags |= SDL_HWACCEL;

    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    screen = SDL_SetVideoMode(SCR_WIDTH, SCR_HEIGHT, 32, flags);

    if (!screen) {
        fprintf(stderr, "Couldn't get a screen.\n");
        exit(1);
    }

    init_gl();

    init_data();

    status("Waves running...");

    int x, y, down = 0;
    int pause = 0;
    int brushsize = 32;
    int fps = 0, frames = 0;
    time_t prev_sec;

    while (1) {
        SDL_Event e;

        if (!pause) {
            update();
        }


        draw();
        if (fps) {
            struct timeval tv;
            frames++;
            gettimeofday(&tv, NULL);
            if (tv.tv_sec != prev_sec) {
                status("Waves: %d FPS", frames);
                prev_sec = tv.tv_sec;
                frames = 0;
            }
        }


        while (SDL_PollEvent(&e)) {
            switch(e.type) {
                case SDL_MOUSEMOTION:
                    x = e.motion.x * WIDTH / SCR_WIDTH;
                    y = HEIGHT - e.motion.y * HEIGHT / SCR_HEIGHT;
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
                        case SDLK_q:
                            return(0);
                            break;
                        case SDLK_ESCAPE:
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
                            if (COLOROFFSET>1300) COLOROFFSET=1300;
                            status("ColorOffset: %d", COLOROFFSET);
                            set_palette();
                            break;

                    }
                    break;
                case SDL_QUIT:
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


