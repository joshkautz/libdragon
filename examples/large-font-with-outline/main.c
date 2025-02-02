// Library Imports
#include <libdragon.h>
#include <math.h>

#define FONT_20 1
#define FONT_20_OUTLINED 2
#define FONT_30 3
#define FONT_30_OUTLINED 4
#define FONT_50 5
#define FONT_50_OUTLINED 6

#define BLACK RGBA32(0, 0, 0, 255)
#define WHITE RGBA32(255, 255, 255, 255)
#define RED RGBA32(255, 0, 0, 255)
#define YELLOW RGBA32(243, 243, 116, 255)
#define BLUE RGBA32(106, 185, 242, 255)

int main(void)
{
	joypad_init();
	debug_init_isviewer();
	debug_init_usblog();
	dfs_init(DFS_DEFAULT_LOCATION);
	rdpq_init();

	rdpq_fontstyle_t fontstyle_white = {
		.color = WHITE,
	};
	rdpq_fontstyle_t fontstyle_white_outlined = {
		.color = WHITE,
		.outline_color = BLACK,
	};
	rdpq_fontstyle_t fontstyle_yellow = {
		.color = YELLOW,
	};
	rdpq_fontstyle_t fontstyle_blue = {
		.color = BLUE,
	};

	rdpq_font_t *font_20 = rdpq_font_load("rom:/font-20.font64");
	rdpq_font_t *font_20_outlined = rdpq_font_load("rom:/font-20-outlined.font64");
	rdpq_font_t *font_30 = rdpq_font_load("rom:/font-30.font64");
	rdpq_font_t *font_30_outlined = rdpq_font_load("rom:/font-30-outlined.font64");
	rdpq_font_t *font_50 = rdpq_font_load("rom:/font-50.font64");
	rdpq_font_t *font_50_outlined = rdpq_font_load("rom:/font-50-outlined.font64");

	rdpq_font_style(font_20, 0, &fontstyle_white);
	rdpq_font_style(font_20, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_20, 2, &fontstyle_yellow);
	rdpq_font_style(font_20, 3, &fontstyle_blue);

	rdpq_font_style(font_20_outlined, 0, &fontstyle_white);
	rdpq_font_style(font_20_outlined, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_20_outlined, 2, &fontstyle_yellow);
	rdpq_font_style(font_20_outlined, 3, &fontstyle_blue);

	rdpq_font_style(font_30, 0, &fontstyle_white);
	rdpq_font_style(font_30, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_30, 2, &fontstyle_yellow);
	rdpq_font_style(font_30, 3, &fontstyle_blue);

	rdpq_font_style(font_30_outlined, 0, &fontstyle_white);
	rdpq_font_style(font_30_outlined, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_30_outlined, 2, &fontstyle_yellow);
	rdpq_font_style(font_30_outlined, 3, &fontstyle_blue);

	rdpq_font_style(font_50, 0, &fontstyle_white);
	rdpq_font_style(font_50, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_50, 2, &fontstyle_yellow);
	rdpq_font_style(font_50, 3, &fontstyle_blue);

	rdpq_font_style(font_50_outlined, 0, &fontstyle_white);
	rdpq_font_style(font_50_outlined, 1, &fontstyle_white_outlined);
	rdpq_font_style(font_50_outlined, 2, &fontstyle_yellow);
	rdpq_font_style(font_50_outlined, 3, &fontstyle_blue);

	rdpq_text_register_font(FONT_20, font_20);
	rdpq_text_register_font(FONT_20_OUTLINED, font_20_outlined);
	rdpq_text_register_font(FONT_30, font_30);
	rdpq_text_register_font(FONT_30_OUTLINED, font_30_outlined);
	rdpq_text_register_font(FONT_50, font_50);
	rdpq_text_register_font(FONT_50_OUTLINED, font_50_outlined);

	display_init((resolution_t){
					 // Initialize a framebuffer resolution which precisely matches the video
					 .width = 320,
					 .height = 240,
					 .interlaced = INTERLACE_OFF,
					 // Set the desired aspect ratio to that of the video. By default,
					 // display_init would force 4:3 instead, which would be wrong here.
					 // eg: if a video is 320x176, we want to display it as 16:9-ish.
					 .aspect_ratio = (float)320 / 240,
					 // Uncomment this line if you want to have some additional black
					 // borders to fully display the video on real CRTs.
					 // .overscan_margin = VI_CRT_MARGIN,
				 },
				 // 32-bit display mode is mandatory for video playback.
				 DEPTH_32_BPP, 2, GAMMA_NONE,
				 // Activate bilinear filtering while rescaling the video
				 FILTERS_RESAMPLE);

	surface_t menuSelect = surface_alloc(FMT_RGBA16, 320, 240);
	rdpq_attach(&menuSelect, NULL);
	rdpq_text_print(NULL, 1, 106, 115, "$01^02NEW GAME");
	rdpq_detach();

	while (1)
	{
		float pulseFactor = (sin((float)TICKS_READ() / 7500000.00) / 5) + 1;

		rdpq_attach(display_get(), NULL);
		rdpq_clear(RED);

		rdpq_set_mode_standard();
		rdpq_mode_alphacompare(128);

		rdpq_tex_blit(&menuSelect, 106, 110, &(rdpq_blitparms_t){
												 .s0 = 0,
												 .t0 = 0,
												 .cx = 106,
												 .cy = 110,
												 .scale_y = pulseFactor,
											 });

		rdpq_text_print(NULL, 1, 42, 60, "$06^01MALLARD");
		rdpq_text_print(NULL, 1, 122, 75, "$04^012025");
		// rdpq_text_print(NULL, 1, 106, 115, "$01^02NEW GAME");
		rdpq_text_print(NULL, 1, 104, 155, "$01^03CONTINUE");
		rdpq_text_print(NULL, 1, 113, 195, "$01^03CREDITS");

		rdpq_detach_show();
	}

	return 0;
}
