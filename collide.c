/*
 * collide.c
 * program which demonstrates sprites colliding with tiles
 */

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

/* include the background image we are using */
#include "finally.h"

/* include the sprite image we are using */
#include "FinalSprites.h"

/* include the tile map we are using */
#include "background.h"
#include "blocks.h"

/* the tile mode flags needed for display control register */
#define MODE0 0x00
/* enable bits for the four tile layers */
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800


/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;


/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;


/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank() {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) finally_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) finally_data,
            (finally_width * finally_height) / 2);

    /* set all control the bits in this register */
    *bg0_control = 3 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (16 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */
    *bg1_control = 2 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (24 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (3 << 14);        /* bg size, 0 is 256x256 */


    /* load the tile data into screen block 16 */
    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*) background, background_width * background_height);
    memcpy16_dma((unsigned short*) screen_block(24), (unsigned short*) blocks, blocks_width * blocks_height);
}

/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
        (0 << 8) |          /* rendering mode */
        (0 << 10) |         /* gfx mode */
        (0 << 12) |         /* mosaic */
        (1 << 13) |         /* color mode, 0:16, 1:256 */
        (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
        (0 << 9) |          /* affine flag */
        (h << 12) |         /* horizontal flip flag */
        (v << 13) |         /* vertical flip flag */
        (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
        (priority << 10) | // priority */
        (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) FinalSprites_palette, PALETTE_SIZE);

    /* load the image into sprite image memory */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) FinalSprites_data, (FinalSprites_width * FinalSprites_height) / 2);
}

/* Function to set the index of a sprite */
void sprite_set_index(struct Sprite* sprite, int index) {
    /* Calculate the tile offset based on the index */
    int tile_offset = index * (sprite->attribute2 & 0x03ff);

    /* Update the sprite's attribute2 with the new tile offset */
    sprite->attribute2 = (sprite->attribute2 & 0xfc00) | (tile_offset & 0x03ff);
}


/* a struct for the peach's logic and behavior */
struct Peach {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion in pixels */
    int x, y;

    /* the peach's y velocity in 1/256 pixels/second */
    int yvel;

    /* the peach's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* which animation she should stop on */
    int stopVal;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the peach is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the peach stays */
    int border;

    /* if the peach is currently falling */
    int falling;

    /* if the game has been won*/
    int won;
};

/* initialize the peach */
void Peach_init(struct Peach* peach) {
    peach->x = 100;
    peach->y = 80;
    peach->yvel = 0;
    peach->gravity = 50;
    peach->border = 40;
    peach->frame = 0;
    peach->stopVal = 0;
    peach->move = 0;
    peach->counter = 0;
    peach->falling = 0;
    peach->won = 0;
    peach->animation_delay = 8;
    peach->sprite = sprite_init(peach->x, peach->y, SIZE_32_64, 0, 0, peach->frame, 2);
}

/* Structure for the Yoshi sprite*/
struct Yoshi{
    struct Sprite* sprite;
    int x, y;
    int yvel;
    int gravity;
    int falling;
    int frame;
    int animation_delay;
    int counter;
    int move;
    int border;
};

/*Initialize Yoshi*/
void Yoshi_init(struct Yoshi* yoshi){
    yoshi->x = 65;
    yoshi->y = 112;
    yoshi->yvel = 0;
    yoshi->gravity = 50;
    yoshi->border = 40;
    yoshi->frame = 544;
    yoshi->move = 0;
    yoshi->falling = 0;
    yoshi->animation_delay = 8;
    yoshi->counter = 0;
    yoshi->sprite = sprite_init(yoshi->x, yoshi->y, SIZE_32_32, 0, 0, yoshi->frame, 2);
}

/* move the peach left or right returns if it is at edge of the screen */
int peach_left(struct Peach* peach, struct Yoshi* yoshi) {
    /* face left */
    sprite_set_horizontal_flip(peach->sprite, 1);
    peach->move = 1;

    /* if we are at the left end, just scroll the screen */
    if (peach->x < peach->border) {
        if (peach->x < yoshi->x - 25){
            sprite_set_horizontal_flip(yoshi->sprite, 1);
            yoshi->move = 1;
        }else{
            yoshi->x = peach->x + 27;
        }
        return 1;
    } else {
        /* else move left */
        peach->x--;
        if (peach->x < yoshi->x - 25){
            sprite_set_horizontal_flip(yoshi->sprite, 1);
            yoshi->move = 1;
            yoshi->x--;
        }else{
            yoshi->move = 0;
            sprite_set_offset(yoshi->sprite, 544);
        }
        return 0;
    }
}


int peach_right(struct Peach* peach, struct Yoshi* yoshi) {
    /* face right */
    sprite_set_horizontal_flip(peach->sprite, 0);
    peach->move = 1;

    /* if we are at the right end, just scroll the screen */
    if (peach->x > (SCREEN_WIDTH - 16 - peach->border)) {
        if (peach->x > yoshi->x + 25){
            sprite_set_horizontal_flip(yoshi->sprite, 0);
            yoshi->move = 1;
        }else{
            yoshi->x = peach->x - 27;
        }
        return 1;
    } else {
        /* else move right */
        peach->x++;
        if (peach->x > yoshi->x + 25){
            sprite_set_horizontal_flip(yoshi->sprite, 0);
            yoshi->move = 1;
            yoshi->x++;
        }else{
            yoshi->move = 0;
            sprite_set_offset(yoshi->sprite, 544);
        }
        return 0;
    }
}


/* start the peach jumping, unless already fgalling */
void peach_jump(struct Peach* peach) {
    if (!peach->falling) {
        peach->yvel = -1350;
        peach->falling = 1;
    }
}

/* finds which tile a screen coordinate maps to, taking scroll into acco  unt */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* the larger screen maps (bigger than 32x32) are made of multiple stitched
       together - the offset is used for finding which screen block we are in
       for these cases */
    int offset = 0;

    /* if the width is 64, add 0x400 offset to get to tile maps on right   */
    if (tilemap_w == 64 && x >= 32) {
        x -= 32;
        offset += 0x400;
    }

    /* if height is 64 and were down there */
    if (tilemap_h == 64 && y >= 32) {
        y -= 32;

        /* if width is also 64 add 0x800, else just 0x400 */
        if (tilemap_w == 64) {
            offset += 0x800;
        } else {
            offset += 0x400;
        }
    }

    /* find the index in this tile map */
    int index = y * 32 + x;

    /* return the tile */
    return tilemap[index + offset];
}

/* stop the peach from walking left/right */
void peach_stop(struct Peach* peach) {
    peach->move = 0;
    peach->frame = peach->stopVal;
    peach->counter = 7;
    sprite_set_offset(peach->sprite, peach->frame);
}

/* stop yoshi from walking */
void yoshi_stop(struct Yoshi* yoshi){
    yoshi->move = 0;
    yoshi->frame = 544;
    yoshi->counter = 7;
    sprite_set_offset(yoshi->sprite, yoshi->frame);
}

/* update the peach */
void peach_update(struct Peach* peach, struct Yoshi* yoshi, int xscroll, int yscroll) {
    /* update y position and speed if falling */
    if (peach->falling) {
        peach->y += (peach->yvel >> 8);
        peach->yvel += peach->gravity;
    }

    if (peach->y > 175){
        yscroll = 94;
    }
    /* check which tile the peach's feet are over */
    unsigned short tile = tile_lookup(peach->x + 16, peach->y + 64, xscroll, yscroll, blocks,
            blocks_width, blocks_height);
    
    unsigned short left = tile_lookup(peach->x + 10, peach->y + 48, xscroll, yscroll, blocks, blocks_width, 
                blocks_height);

    unsigned short right = tile_lookup(peach->x + 20, peach->y + 48, xscroll, yscroll, blocks, blocks_width, 
                blocks_height);
    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the peach can walk on */
    if ((tile >= 72 && tile <= 83) || (tile >= 96 && tile <= 99) || (tile >= 178 && tile <= 179) || 
        (tile >= 202 && tile <= 203) || (tile >= 246 && tile <= 247) || (tile >= 258 && tile <= 259)) {
        /* stop the fall! */
        peach->falling = 0;
        peach->yvel = 0;

        /* make him line up with the top of a block works by clearing out the lower bits to 0 */
        peach->y &= ~0x3;

        /* move him down one because there is a one pixel gap in the image */
        peach->y++;
       
        if (left == 246){
            peach->x += 1;
        }else if (right == 247 || right == 259){
            peach->x -= 1;
        }
 
        if ((tile == 82 || tile == 83)){
            peach->frame = 160;
            sprite_set_offset(peach->sprite, peach->frame);
            peach->stopVal = 160;
            peach->move = 0;
            peach->y = 250;
            yoshi->move = 0;
            yoshi->y = 250;
            wait_vblank();
            *bg0_y_scroll = 348;
            *bg1_y_scroll = 348;
        }else{
            peach->stopVal = 0;
        }

        if (tile == 178 || tile == 179){
            peach->won = 1;
            peach->move = 0;
        }

    } else {
        /* he is falling now */
        peach->falling = 1;
    }

    /* update animation if moving */
    if (peach->move) {
        peach->counter++;
        if (peach->counter >= peach->animation_delay) {
            peach->frame = peach->frame + 64;
            if (peach->frame > 64) {
                peach->frame = 0;
            }
            sprite_set_offset(peach->sprite, peach->frame);
            peach->counter = 0;
        }
    }

    if (peach->y < -60){
        peach->y = -60;
        peach->falling = 1;
    }
    if (peach->y > 150){
        if (peach->y < 200){
            peach->y = 200;
            peach->falling = 1;
        }
    }
    /* set on screen position */
    sprite_position(peach->sprite, peach->x, peach->y);
}

/* update the yoshi */
void Yoshi_update(struct Yoshi* yoshi, int xscroll, int yscroll) {
    /* update y position and speed if falling */
    if (yoshi->falling) {
        yoshi->y += (yoshi->yvel >> 8);
        yoshi->yvel += yoshi->gravity;
    }

    if (yoshi->y > 175){
        yscroll = 95;
    }
    /* check which tile the peach's feet are over */
    unsigned short tile = tile_lookup(yoshi->x + 16, yoshi->y + 32, xscroll, yscroll, blocks,
            blocks_width, blocks_height);

    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the peach can walk on */
    if ((tile >= 72 && tile <= 83) || (tile >= 96 && tile <= 99) || (tile >= 178 && tile <= 179) || (tile >= 202 && tile <= 203) || (tile >= 246 && tile <= 247) || (tile >= 258 && tile <= 259)) {
        /* stop the fall! */
        yoshi->falling = 0;
        yoshi->yvel = 0;

        /* make him line up with the top of a block works by clearing out the lower bits to 0 */
        yoshi->y &= ~0x3;

        /* move him down one because there is a one pixel gap in the image */
        //yoshi->y++;
       
        
    } else {
        /* he is falling now */
        yoshi->falling = 1;
    }

    /* update animation if moving */
    if (yoshi->move) {
        yoshi->counter++;
        if (yoshi->counter >= yoshi->animation_delay) {
            yoshi->frame = yoshi->frame - 32;
            if (yoshi->frame < 512) {
               yoshi->frame = 544;
            }
            sprite_set_offset(yoshi->sprite, yoshi->frame);
            yoshi->counter = 0;
        }
    }

    /* set on screen position */
    sprite_position(yoshi->sprite, yoshi->x, yoshi->y);
}


/* Animation for peach when the game is won*/
void peach_win(struct Peach* peach){
    peach->frame = 224;
    sprite_set_offset(peach->sprite, peach->frame);
}

/* Animation for peach when she loses */
void peach_lose(struct Peach* peach){
    peach->frame = 288;
    sprite_set_offset(peach->sprite, peach->frame);
}

/* Animation for yoshi when the game is won */
void yoshi_win(struct Yoshi* yoshi){
    yoshi->frame = 576;
    sprite_set_offset(yoshi->sprite, yoshi->frame);
}

/* Structure for the Mario sprite */
struct Mario{
    struct Sprite* sprite;
    int x, y;
};

/* Initialize Mario */
void Mario_init(struct Mario* mario){
    mario->x = 400;
    mario->y = 208;
    mario->sprite = sprite_init(mario->x, mario->y, SIZE_32_32, 0, 0, 352, 3);
}

void Mario_update(struct Mario* mario, struct Peach* peach, int xscroll, int yscroll){
    sprite_position(mario->sprite, mario->x, mario->y);
    if (peach->y > 175){
        yscroll = 94;
        xscroll *= -1;
    }
        sprite_move(mario->sprite, xscroll, yscroll);
}

void mario_win(struct Mario* mario){
    sprite_set_offset(mario->sprite, 480);
}

/* Structure for the Toad sprite*/
struct Toad{
    struct Sprite* sprite;
    int x, y;
};

/* Initialize Toad */
void Toad_init(struct Toad* toad){
    toad->x = 365;
    toad->y = 178;
    toad->sprite = sprite_init(toad->x, toad->y, SIZE_32_64, 0, 0, 672, 3);
}

void Toad_update(struct Toad* toad, struct Peach* peach, int xscroll, int yscroll){
    sprite_position(toad->sprite, toad->x, toad->y);
    if (peach->y > 175){
        yscroll = 94;
        xscroll *= -1;
    }
    sprite_move(toad->sprite, xscroll, yscroll);
}

void toad_win(struct Toad* toad){
    sprite_set_offset(toad->sprite, 608);
}

/* Structure for the Goomba sprite */
struct Goomba {
    struct Sprite* sprite;
    int x, y;
    int direction; // -1 for left, 1 for right
    int frame;
    int animation_delay;
    int counter;
    int move;
};

/* Initialize the Goomba */
void Goomba_init(struct Goomba* goomba) {
    goomba->x = 10; // Initial x-coordinate
    goomba->y = 121; // Initial y-coordinate
    goomba->direction = 1; // Initial direction (right)
    goomba->frame = 744;
    goomba->animation_delay = 8;
    goomba->counter = 0;
    goomba->move = 1;
    goomba->sprite = sprite_init(goomba->x, goomba->y, SIZE_32_32, 0, 0, goomba->frame, 2); // Create the sprite
}

/* Structure for heart sprite */
struct Heart {
    struct Sprite* sprite;
    int x, y;
    int active; // Flag to indicate if the heart is active
};

/* Initialize a heart sprite */
void Heart_init(struct Heart* heart, int x, int y) {
    heart->x = x;
    heart->y = y;
    heart->active = 1; // Initially, the heart is active
    heart->sprite = sprite_init(x, y, SIZE_32_16, 0, 0, 846, 2);
}

/* Update the heart sprite position */
void Heart_update(struct Heart* hearts, int num_hearts, int lives) {
    for (int i = 0; i < num_hearts; i++) {
        if (i < lives) {
            // Heart is active
            hearts[i].active = 1;
            sprite_position(hearts[i].sprite, hearts[i].x, hearts[i].y);
        } else {
            // Heart is inactive
            hearts[i].active = 0;
            sprite_position(hearts[i].sprite, -32, -32); // Move off screen
        }
    }
}

/* Decrement Peach's lives when she collides with a Goomba */
void Peach_collide(struct Peach* peach, struct Goomba* goomba, struct Heart* hearts, int* num_lives, int* cooldown_timer) {
    // Check if Peach is currently on cooldown
    if (*cooldown_timer > 0) {
        // Peach is on cooldown, do not decrement lives
        return;
    }

    // Check collision between Peach and Goomba
    if (peach->x < goomba->x + 32 &&
        peach->x + 32 > goomba->x &&
        peach->y < goomba->y + 32 &&
        peach->y + 64 > goomba->y) {
        // Reduce Peach's lives
        (*num_lives)--;
        // Update heart sprites
        Heart_update(hearts, 3, *num_lives);
        // Reset Peach's position or do other necessary actions

        // Set the cooldown timer to prevent immediate subsequent collisions
        *cooldown_timer = 60;
    }
}


/* Update the cooldown timer */
void Update_cooldown(int* cooldown_timer) {
    if (*cooldown_timer > 0) {
        (*cooldown_timer)--;
    }
}

/* Structure for number */
struct Number {
    struct Sprite* ones_sprite;
    struct Sprite* tens_sprite;
    int x, y;
    int current_ones_index; // Store the current index of the ones place sprite
    int current_tens_index; // Store the current index of the tens place sprite
};

/* Initialize number sprites */
void Number_init(struct Number* number, int x, int y, int ones_sprite_index, int tens_sprite_index) {
    number->x = x;
    number->y = y;
    number->ones_sprite = sprite_init(x, y, SIZE_8_32, 0, 0, ones_sprite_index, 2);
    number->tens_sprite = sprite_init(x - 8, y, SIZE_8_32, 0, 0, tens_sprite_index, 2);
    number->current_ones_index = ones_sprite_index; // Initial ones place index
    number->current_tens_index = tens_sprite_index; // Initial tens place index
}

void sprite_set_tile_offset(struct Sprite* sprite, int tile_offset) {
    /* Update the sprite's attribute2 with the new tile offset */
    sprite->attribute2 = (sprite->attribute2 & 0xfc00) | (tile_offset & 0x03ff);
}

/* Function to update the number sprites based on the score */
void Number_update(struct Number* number, int score) {
    int ones_digit = score % 10;
    int tens_digit = (score >= 10) ? 1 : 0; // Tens place is 1 if score is 10 or more

    // Update ones place sprite
    sprite_set_tile_offset(number->ones_sprite, 864 + (ones_digit * 8));
    
    // Update tens place sprite
    sprite_set_tile_offset(number->tens_sprite, 864 + (tens_digit * 8));

    // Show tens place sprite
    sprite_position(number->tens_sprite, number->x - 8, number->y); // Move to the left of ones place
}


/* Structure for coin sprite */
struct Coin {
    struct Sprite* sprite;
    int x, y;
    int collected; // Flag to indicate if the coin is collected
};

/* Initialize a coin sprite */
void Coin_init(struct Coin* coin, int x, int y) {
    coin->x = x;
    coin->y = y;
    coin->collected = 0;
    coin->sprite = sprite_init(x, y, SIZE_32_16, 0, 0, 830, 2); // Create the sprite
}

int increment_score();
int score = 0;

void Coin_update(struct Coin* coins, int num_coins, struct Peach* peach, int xscroll, int yscroll, struct Number* number) {
    for (int i = 0; i < num_coins; i++) {
        struct Coin* coin = &coins[i];
        if (!coin->collected) {
            // Update the coin's position based on background scroll
            int coin_x = (coin->x - xscroll) % SCREEN_WIDTH;
            int coin_y = coin->y - yscroll;

            // Adjust negative x values
            if (coin_x < 0) {
                coin_x += 300;
            }

            // Check for collision with Peach
            if (peach->x < coin_x && peach->x + 32 > coin_x &&
                peach->y < coin_y  && peach->y + 32 > coin_y) {
                coin->collected = 1; // Set collected flag
                // Move the coin off screen
                coin->x = -16; // Move off screen
                coin->y = -16; // Move off screen

                // Hide the coin's sprite
                sprite_position(coin->sprite, SCREEN_WIDTH, SCREEN_HEIGHT);

                int new_score = increment_score();
                Number_update(number, new_score);
                score = new_score;
            } else {
                // Update the coin's sprite position
                sprite_position(coin->sprite, coin_x, coin_y);
            }
        }
    }
}


// Function to get the current background scroll X position
int get_background_scroll_x() {
    return *bg0_x_scroll;
}

// Function to get the current background scroll Y position
int get_background_scroll_y() {
    return *bg0_y_scroll;
}

/* Call assembly function to keep the Goomba moving left and right */
void GoombaMove(int* x, int* direction); // Assembly function declaration

/* Update the Goomba's position and direction */
void Goomba_update(struct Goomba* goomba, struct Peach* peach, int xscroll, int yscroll) {

    GoombaMove(&goomba->x, &goomba->direction);

    // Check boundaries (adjust as needed)
    if (goomba->x <= 0 || goomba->x >= 100) {
        goomba->direction *= -1; // Reverse direction if Goomba reaches screen edges
    }

    if(peach->y > 175){
        goomba->y = 160;   
    }

    // Update Goomba sprite position
    sprite_position(goomba->sprite, goomba->x, goomba->y);

    //Hopefully flip the sprite if walking left
    if (goomba->direction == -1) {
        sprite_set_horizontal_flip(goomba->sprite, 1); // Flip horizontally
    } else {
        sprite_set_horizontal_flip(goomba->sprite, 0); // Reset horizontal flip
    }

    // update animation if moving
    if (goomba->move) {
        goomba->counter++;
        if (goomba->counter >= goomba->animation_delay) {
            goomba->frame = goomba->frame + 32;
            if (goomba->frame > 776) {
                goomba->frame = 744;
            }
            sprite_set_offset(goomba->sprite, goomba->frame);
            goomba->counter = 0;
        }
    }

    if (peach->y > 175){
        yscroll = 220;
        xscroll *= -1;
    }
    sprite_move(goomba->sprite, xscroll, yscroll);
}

/* the main function */
int main() {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG0_ENABLE |BG1_ENABLE | SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

    /* create the peach */
    struct Peach peach;
    Peach_init(&peach);

    /* create Yoshi*/
    struct Yoshi yoshi;
    Yoshi_init(&yoshi);

   /* create the Goomba */
    struct Goomba goomba;
    Goomba_init(&goomba);

    /* Create heart sprites */
    struct Heart hearts[3];
    for (int i = 0; i < 3; i++) {
        Heart_init(&hearts[i], 1 + i * 17, 5);
    }
    int num_lives = 3;
    int cooldown_timer = 0;

    /* Initialize score number */
    struct Number number;
    Number_init(&number, 220, 5, 864, 864);    

    int score = 0;

    /* Initialize coins */
    struct Coin coins[11];
    Coin_init(&coins[0], 5, 30); // Example: Coin at (50, 50)

    // Add more coins on top of different tiles
    Coin_init(&coins[1], 30, 80);
Coin_init(&coins[2], 85, 60);
Coin_init(&coins[3], 110, 40);
Coin_init(&coins[4], 135, 100);
Coin_init(&coins[5], 170, 35);
Coin_init(&coins[6], 200, 80);
Coin_init(&coins[7], 215, 80);

    /* create Mario */
    struct Mario mario;
    Mario_init(&mario);

    /* create Toad */
    struct Toad toad;
    Toad_init(&toad);

   /* set initial scroll to 0 */
    int xscroll = 0;
    int yscroll = 0;
    /* loop forever */
    while (1) {
        if(peach.won){
            wait_vblank();
            peach_win(&peach);
            yoshi_win(&yoshi);
            toad_win(&toad);
            mario_win(&mario);
            sprite_update_all();
            delay(300);
        }else{
            if(num_lives == 0) {
                wait_vblank();
                peach_lose(&peach);
                sprite_update_all();
                delay(300);
            } else {
                // Check collision between Peach and Goomba
                Update_cooldown(&cooldown_timer);
                Peach_collide(&peach, &goomba, hearts, &num_lives, &cooldown_timer);

                Mario_update(&mario, &peach, 2*xscroll, yscroll); 

                Toad_update(&toad, &peach, 2*xscroll, yscroll);        

                /* update the peach */
                peach_update(&peach, &yoshi, 2*xscroll, yscroll);

                Yoshi_update(&yoshi, 2*xscroll, yscroll);

                /* now the arrow keys move the peach */
                if (button_pressed(BUTTON_RIGHT)) {
                    if (peach_right(&peach, &yoshi)) {
                        xscroll++;
                    }
                } else if (button_pressed(BUTTON_LEFT)) {
                    if (peach_left(&peach, &yoshi)) {
                        xscroll--;
                    }
                }else {
                    peach_stop(&peach);
                    yoshi_stop(&yoshi);
                }

                /* check for jumping */
                if (button_pressed(BUTTON_A)) {
                    peach_jump(&peach);
                }
            
                /* wait for vblank before scrolling and moving sprites */
                 wait_vblank();
                *bg0_x_scroll = xscroll;
                *bg1_x_scroll = 2 * xscroll;

                Coin_update(coins, 11, &peach, 2*xscroll, yscroll, &number);

                sprite_update_all();
                /* delay some */

                delay(300);
    
                /* update the Goomba */
                Goomba_update(&goomba, &peach, 2*xscroll, yscroll);
            }
        }
   }
}

