#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "n64sys.h"
#include "rspq.h"
#include "rdpq.h"
#include "rdpq_rect.h"
#include "surface.h"
#include "sprite.h"
#include "rdpq_mode.h"
#include "rdpq_tex.h"
#include "rdpq_sprite.h"
#include "rdpq_font.h"
#include "rdpq_text.h"
#include "rdpq_paragraph.h"
#include "rdpq_font_internal.h"
#include "rdpq_internal.h"
#include "asset.h"
#include "fmath.h"
#include "utils.h"

// Include the built-in font data
#include "rdpq_font_builtin.c"

#define MAX_STYLES   256

_Static_assert(sizeof(glyph_t) == 8, "glyph_t size is wrong");
_Static_assert(sizeof(atlas_t) == 12, "atlas_t size is wrong");
_Static_assert(sizeof(kerning_t) == 3, "kerning_t size is wrong");

#define PTR_DECODE(font, ptr)    ((void*)(((uint8_t*)(font)) + (uint32_t)(ptr)))
#define PTR_ENCODE(font, ptr)    ((void*)(((uint8_t*)(ptr)) - (uint32_t)(font)))

static void setup_render_mode(int font_type, tex_format_t fmt)
{
    switch (font_type) {
    case FONT_TYPE_ALIASED:
        // Atlases are simple I4 textures, so we just need to colorize
        // them using the PRIM color. We also use PRIM alpha to control
        // additional transparency of the text.
        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (TEX0,0,PRIM,0)));
            rdpq_mode_alphacompare(1);
        rdpq_mode_end();
        break;
    case FONT_TYPE_MONO:
        // Monochrome fonts use CI4 textures with 4 1bpp layers, and special
        // palettes to isolate each layer. PRIM control the color and
        // the transparency of the text.
        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (TEX0,0,PRIM,0)));
            rdpq_mode_alphacompare(1);
            rdpq_mode_tlut(TLUT_RGBA16);
        rdpq_mode_end();
        break;
    case FONT_TYPE_MONO_OUTLINE:
        // Mono-outline fonts are CI4 with IA16 palettes. Each texel is
        // a IA16 color as follows: 0x0000=transparent, 0xFFFF=fill, 0x00FF=outline
        // So TEX will become either 0x00 or 0xFF, and TEX_ALPHA will be 0x20 (or 0 for transparent)
        // We set a combiner that does PRIM*TEX + ENV*(1-TEX), so that we can
        // select between the fill and the outline color, in PRIM and ENV respectively.
        // Unfortunately, we can't use alpha compare with a two-stage combiner because of
        // a RDP bug; so we simulate it using SOM_BLALPHA_CVG_TIMES_CC which multiplies
        // the alpha by the coverage (which should be full on all pixels) before hitting
        // the blender, and this causes a similar transparent effect.
        // to turn on AA for this to work (for unknown reasons).
        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((PRIM,ENV,TEX0,ENV), (TEX0,0,PRIM,0)));
            rdpq_mode_antialias(AA_REDUCED);
            rdpq_mode_tlut(TLUT_IA16);
        rdpq_mode_end();
        rdpq_change_other_modes_raw(SOM_BLALPHA_MASK, SOM_BLALPHA_CVG_TIMES_CC);
        break;
    case FONT_TYPE_ALIASED_OUTLINE:
        // Atlases are IA8, where I modulates between the fill color and the outline color.
        rdpq_mode_begin();
            rdpq_set_mode_standard();
            rdpq_mode_combiner(RDPQ_COMBINER1((PRIM,ENV,TEX0,ENV), (TEX0,0,PRIM,0)));
        rdpq_mode_end();
        rdpq_change_other_modes_raw(SOM_BLALPHA_MASK, SOM_BLALPHA_CVG_TIMES_CC);
        break;
    case FONT_TYPE_BITMAP:
        switch (fmt) {
        case FMT_RGBA16:
        case FMT_CI4:
        case FMT_CI8:
            rdpq_mode_begin();
                rdpq_mode_alphacompare(1);
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,0,PRIM,0), (TEX0,0,PRIM,0)));
            rdpq_mode_end();
            break;
        case FMT_RGBA32:
            rdpq_mode_begin();
                rdpq_set_mode_standard();
                rdpq_mode_combiner(RDPQ_COMBINER1((TEX0,0,PRIM,0), (TEX0,0,PRIM,0)));
            rdpq_mode_end();
            break;
        default:
            assertf(0, "unsupported bitmap font format %s", tex_format_name(fmt));
            break;
        }
        break;
    default:
        assert(0);
    }
}

static void apply_style(int font_type, style_t *s, tex_format_t fmt)
{
    switch (font_type) {
    case FONT_TYPE_MONO_OUTLINE:
    case FONT_TYPE_ALIASED_OUTLINE:
        rdpq_set_env_color(s->outline_color);
        // fallthrough
    case FONT_TYPE_ALIASED:
    case FONT_TYPE_MONO:
    case FONT_TYPE_BITMAP:
        rdpq_set_prim_color(s->color);
        break;
    default:
        assert(0);
    }
    //Blender setup
    switch (font_type) {
        case FONT_TYPE_MONO:
            if(s->color.a != 255) {
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            } else {
                rdpq_mode_blender(0);
            }
            break;

        case FONT_TYPE_MONO_OUTLINE:
        case FONT_TYPE_ALIASED_OUTLINE:
        case FONT_TYPE_ALIASED:
            rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            break;
        
        case FONT_TYPE_BITMAP:
            if(s->color.a != 255 || fmt == FMT_RGBA32) {
                rdpq_mode_blender(RDPQ_BLENDER_MULTIPLY);
            } else {
                rdpq_mode_blender(0);
            }
            break;
    }
    if (s->custom)
        s->custom(s->custom_arg);
}

rdpq_font_t* rdpq_font_load_buf(void *buf, int sz)
{
    assertf(__rdpq_inited, "rdpq_init() must be called before loading a font");
    rdpq_font_t *fnt = buf;
    assertf(sz >= sizeof(rdpq_font_t), "Font buffer too small (sz=%d)", sz);
    assertf(memcmp(fnt->magic, FONT_MAGIC_LOADED, 3), "Trying to load already loaded font data (buf=%p, sz=%08x)", buf, sz);
    assertf(!memcmp(fnt->magic, FONT_MAGIC, 3), "invalid font data (magic: %c%c%c)", fnt->magic[0], fnt->magic[1], fnt->magic[2]);
    assertf(fnt->version == 11, "unsupported font version: %d\nPlease regenerate fonts with an updated mkfont tool", fnt->version);
    fnt->ranges = PTR_DECODE(fnt, fnt->ranges);
    if (fnt->sparse_range) {
        fnt->sparse_range = PTR_DECODE(fnt, fnt->sparse_range);
        fnt->sparse_range->g = PTR_DECODE(fnt, fnt->sparse_range->g);
        fnt->sparse_range->values = PTR_DECODE(fnt, fnt->sparse_range->values);
    }
    fnt->glyphs = PTR_DECODE(fnt, fnt->glyphs);
    if (fnt->glyphs_kranges) fnt->glyphs_kranges = PTR_DECODE(fnt, fnt->glyphs_kranges);
    fnt->atlases = PTR_DECODE(fnt, fnt->atlases);
    fnt->kerning = PTR_DECODE(fnt, fnt->kerning);
    fnt->styles = &fnt->builtin_style;
    fnt->num_styles = 1;
    for (int i = 0; i < fnt->num_atlases; i++) {
        void *buf = PTR_DECODE(fnt, fnt->atlases[i].sprite);
        fnt->atlases[i].sprite = sprite_load_buf(buf, fnt->atlases[i].size);
        sprite_t *spr = fnt->atlases[i].sprite;
        rspq_block_begin();
            // Setup the render mode for this font type
            int font_type = fnt->flags & FONT_FLAG_TYPE_MASK;
            setup_render_mode(font_type, sprite_get_format(spr));

            // Get the atlas surface and check if it fits into TMEM or not,
            // as we will need different rendering strategies.
            surface_t surf = sprite_get_pixels(spr);
            bool fits_tmem = sprite_fits_tmem(spr);
            spr->hslices = fits_tmem ? 1 : 0; // Store it in an unused filed of the header

            switch (font_type) {
            case FONT_TYPE_MONO: {
                rdpq_tex_upload_tlut(sprite_get_palette(spr), 0, font_type == FONT_TYPE_MONO ? 64 : 32);
                if (fits_tmem) {
                    rdpq_tex_multi_begin();
                    rdpq_tex_upload(TILE0, &surf, NULL);
                    rdpq_tex_reuse(TILE1, &(rdpq_texparms_t){ .palette = 1 });
                    rdpq_tex_reuse(TILE2, &(rdpq_texparms_t){ .palette = 2 });
                    rdpq_tex_reuse(TILE3, &(rdpq_texparms_t){ .palette = 3 });
                    rdpq_tex_multi_end();
                } else {
                    rdpq_set_texture_image_raw(0, PhysicalAddr(surf.buffer), FMT_CI8, surf.width/2, surf.height);
                    rdpq_set_tile(TILE0, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 0 });
                    rdpq_set_tile(TILE1, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 1 });
                    rdpq_set_tile(TILE2, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 2 });
                    rdpq_set_tile(TILE3, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 3 });
                    rdpq_set_tile(TILE4, FMT_CI8, 0    , 48, NULL);
                }
                break;
            }
            case FONT_TYPE_MONO_OUTLINE: {
                rdpq_tex_upload_tlut(sprite_get_palette(spr), 0, font_type == FONT_TYPE_MONO ? 64 : 32);
                if (fits_tmem) {
                    rdpq_tex_multi_begin();
                    rdpq_tex_upload(TILE0, &surf, NULL);
                    rdpq_tex_reuse(TILE1, &(rdpq_texparms_t){ .palette = 1 });
                    rdpq_tex_multi_end();
                } else {
                    rdpq_set_texture_image_raw(0, PhysicalAddr(surf.buffer), FMT_CI8, surf.width/2, surf.height);
                    rdpq_set_tile(TILE0, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 0 });
                    rdpq_set_tile(TILE1, sprite_get_format(spr), 0    , 48, &(rdpq_tileparms_t){ .palette = 1 });
                    rdpq_set_tile(TILE4, FMT_CI8, 0    , 48, NULL);
                }
                break;
            }
            case FONT_TYPE_ALIASED:
                if (fits_tmem) {
                    rdpq_sprite_upload(TILE0, spr, NULL);
                } else {
                    rdpq_set_texture_image_raw(0, PhysicalAddr(surf.buffer), FMT_CI8, surf.width/2, surf.height);
                    rdpq_set_tile(TILE0, sprite_get_format(spr), 0, 48, NULL);
                    rdpq_set_tile(TILE4, FMT_CI8, 0    , 48, NULL);
                }
                break;
            case FONT_TYPE_ALIASED_OUTLINE:
            case FONT_TYPE_BITMAP:
                if (fits_tmem) {
                    rdpq_sprite_upload(TILE0, spr, NULL);
                } else {
                    if (TEX_FORMAT_BITDEPTH(sprite_get_format(spr)) == 4) {
                        rdpq_set_texture_image(&surf);
                        rdpq_set_tile(TILE0, sprite_get_format(spr), 0, 48, NULL);
                        rdpq_set_tile(TILE4, FMT_CI8, 0    , 48, NULL);
                    } else {
                        rdpq_set_texture_image(&surf);
                        for (int i=0; i<8; i++)
                            rdpq_set_tile(TILE0+i, sprite_get_format(spr), 0, 48, NULL);
                    }
                }
                break;
            }

        rdpq_sync_load(); // FIXME: revisit once we have the new auto-sync engine
        fnt->atlases[i].up = rspq_block_end();
    }

    memcpy(fnt->magic, FONT_MAGIC_LOADED, 3);
    data_cache_hit_writeback(fnt, sz);
    return fnt;
}

rdpq_font_t* rdpq_font_load(const char *fn)
{
    int sz;
    void *buf = asset_load(fn, &sz);
    rdpq_font_t *fnt = rdpq_font_load_buf(buf, sz);
    memcpy(fnt->magic, FONT_MAGIC_OWNED, 3);
    return fnt;
}

static void font_unload(rdpq_font_t *fnt)
{
    for (int i = 0; i < fnt->num_atlases; i++) {
        sprite_free(fnt->atlases[i].sprite);
        rspq_block_free(fnt->atlases[i].up); fnt->atlases[i].up = NULL;
        fnt->atlases[i].sprite = PTR_ENCODE(fnt, fnt->atlases[i].sprite);
    }
    if (fnt->num_styles > 1) {
        free(fnt->styles);
        fnt->styles = &fnt->builtin_style;
        fnt->num_styles = 1;
    }
    fnt->ranges = PTR_ENCODE(fnt, fnt->ranges);
    if (fnt->sparse_range) {
        fnt->sparse_range->g = PTR_ENCODE(fnt, fnt->sparse_range->g);
        fnt->sparse_range->values = PTR_ENCODE(fnt, fnt->sparse_range->values);
        fnt->sparse_range = PTR_ENCODE(fnt, fnt->sparse_range);
    }
    fnt->glyphs = PTR_ENCODE(fnt, fnt->glyphs);
    if (fnt->glyphs_kranges) fnt->glyphs_kranges = PTR_ENCODE(fnt, fnt->glyphs_kranges);
    fnt->atlases = PTR_ENCODE(fnt, fnt->atlases);
    fnt->kerning = PTR_ENCODE(fnt, fnt->kerning);
    memcpy(fnt->magic, FONT_MAGIC, 3);
}

void rdpq_font_free(rdpq_font_t *fnt)
{
    bool owned = memcmp(fnt->magic, FONT_MAGIC_OWNED, 3) == 0;
    font_unload(fnt);

    if (owned) {
        #ifndef NDEBUG
        // To help debugging, zero the font structure
        memset(fnt, 0, sizeof(rdpq_font_t));
        #endif

        free(fnt);
    }
}

static uint32_t rotl(uint32_t x, int k)
{
    return (x << k) | (x >> (-k & 31));
}

__attribute__((noinline))
static uint32_t phf_round32(uint32_t k1, uint32_t h1) {
	k1 *= 0xcc9e2d51;
	k1 = rotl(k1, 15);
	k1 *= 0x1b873593;
	h1 ^= k1;
	h1 = rotl(h1, 13);
	h1 = h1 * 5 + 0xe6546b64;
	return h1;
}
__attribute__((noinline))
static uint32_t phf_mix32(uint32_t h1) {
	h1 ^= h1 >> 16;
	h1 *= 0x85ebca6b;
	h1 ^= h1 >> 13;
	h1 *= 0xc2b2ae35;
	h1 ^= h1 >> 16;
	return h1;
}

__attribute__((noinline))
static int16_t __rdpq_font_glyph_sparse(const rdpq_font_t *fnt, uint32_t codepoint)
{
    uint32_t seed = fnt->sparse_range->seed;
    uint32_t g = phf_mix32(phf_round32(codepoint, seed));
    uint32_t gidx = ((uint64_t)g * fnt->sparse_range->r) >> 32;
    uint32_t d = fnt->sparse_range->g[gidx];
    uint32_t f = phf_mix32(phf_round32(codepoint, phf_round32(d, seed)));
    uint32_t idx = ((uint64_t)f * fnt->sparse_range->m) >> 32;
    int16_t glyph = fnt->sparse_range->values[idx];
    return glyph;
}

int16_t __rdpq_font_glyph(const rdpq_font_t *fnt, uint32_t codepoint)
{
    // Search for the range that contains this codepoint (if any)
    for (int i = 0; i < fnt->num_ranges; i++) {
        range_t *r = &fnt->ranges[i];
        if (codepoint >= r->first_codepoint && codepoint < r->first_codepoint + r->num_codepoints) {
            if (UNLIKELY(r->first_glyph < 0)) {
                assert(fnt->sparse_range);
                return __rdpq_font_glyph_sparse(fnt, codepoint);
            }
            return r->first_glyph + codepoint - r->first_codepoint;
        }
    }
    return -1;
}

float __rdpq_font_kerning(const rdpq_font_t *fnt, int16_t glyph1, int16_t glyph2)
{
    glyph_krange_t *gk = &fnt->glyphs_kranges[glyph1];
    float kerning_scale = fnt->point_size / 127.0f;

    // Do a binary search in the kerning table to look for the next glyph
    int l = gk->kerning_lo, r = gk->kerning_hi;
    while (l <= r) {
        int m = (l + r) / 2;
        if (fnt->kerning[m].glyph2 == glyph2) {
            // Found the kerning value
            return fnt->kerning[m].kerning * kerning_scale;
        }
        if (fnt->kerning[m].glyph2 < glyph2)
            l = m + 1;
        else
            r = m - 1;
    }

    return 0;
}

void rdpq_font_style(rdpq_font_t *fnt, uint8_t style_id, const rdpq_fontstyle_t *style)
{
    if (UNLIKELY(style_id >= fnt->num_styles)) {
        assertf(style_id < MAX_STYLES, "style_id %d exceeds maximum %d", style_id, MAX_STYLES);

        if (fnt->num_styles == 1) {
            fnt->num_styles = 16;
            fnt->styles = calloc(16, sizeof(style_t));
            memcpy(&fnt->styles[0], &fnt->builtin_style, sizeof(style_t));
        } else {
            int old_styles = fnt->num_styles;
            fnt->num_styles = MAX(fnt->num_styles*2, MAX_STYLES);
            fnt->styles = realloc(fnt->styles, fnt->num_styles * sizeof(style_t));
            memset(&fnt->styles[old_styles], 0, (fnt->num_styles - old_styles) * sizeof(style_t));
        }
    }

    style_t *s = &fnt->styles[style_id];
    s->color = style->color;
    s->outline_color = style->outline_color;
    s->custom = style->custom;
    s->custom_arg = style->custom_arg;
}

int rdpq_font_render_paragraph(const rdpq_font_t *fnt, const rdpq_paragraph_char_t *chars, float x0, float y0)
{
    uint8_t font_id = chars[0].font_id;
    int cur_atlas = -1;
    int cur_style = -1;
    int rdram_loading = 0;
    int tile_offset = 0;

    const rdpq_paragraph_char_t *ch = chars;
    while (ch->font_id == font_id) {
        bool force_apply_style = false;
        const glyph_t *g = &fnt->glyphs[ch->glyph];
        if (UNLIKELY(g->natlas != cur_atlas)) {
            atlas_t *a = &fnt->atlases[g->natlas];
            rspq_block_run(a->up);
            if (a->sprite->hslices == 0) { // check if the atlas is in RDRAM instead of TMEM
                tile_offset = 0;
                switch (fnt->flags & FONT_FLAG_TYPE_MASK) {
                case FONT_TYPE_MONO:            rdram_loading = 1; break;
                case FONT_TYPE_MONO_OUTLINE:    rdram_loading = 1; break;
                case FONT_TYPE_ALIASED:         rdram_loading = 1; break;
                case FONT_TYPE_ALIASED_OUTLINE: rdram_loading = 2; break;
                case FONT_TYPE_BITMAP: switch (TEX_FORMAT_BITDEPTH(sprite_get_format(a->sprite))) {
                    case 4:     rdram_loading = 1; break;
                    default:    rdram_loading = 2; break;
                    } break;
                default: assert(0);
                }
            } else {
                rdram_loading = 0;
            }
            cur_atlas = g->natlas;
            force_apply_style = true;
        }
        if (force_apply_style || UNLIKELY(ch->style_id != cur_style)) {
            assertf(ch->style_id < fnt->num_styles,
                 "style %d not defined in this font", ch->style_id);
            apply_style(fnt->flags & FONT_FLAG_TYPE_MASK, &fnt->styles[ch->style_id], sprite_get_format(fnt->atlases[g->natlas].sprite));
            cur_style = ch->style_id;
        }

        // Draw the glyph
        float x = x0 + (ch->x + g->xoff);
        float y = y0 + (ch->y + g->yoff);
        int width = g->xoff2 - g->xoff;
        int height = g->yoff2 - g->yoff;
        int ntile = g->ntile;

        // Check if the atlas is in RDRAM (rather than TMEM). If so, we need
        // to load each glyph into TMEM before drawing.
        if (UNLIKELY(rdram_loading)) {
            switch (rdram_loading) {
            case 1: // 4bpp format: TILE4 is for loading, TILE0-3 are for rendering
                // If the atlas is 4bpp, we need to load the glyph as CI8 (usual trick)
                // TILE4 is the CI8 tile configured for loading
                rdpq_load_tile(TILE4, g->s/2, g->t, (g->s+width+1)/2, g->t+height);
                rdpq_set_tile_size(ntile+tile_offset, g->s & ~1, g->t, (g->s+width+1) & ~1, g->t+height);
                break;
            case 2: // 8bpp: all tiles can be used for both loading and rendering. ntile is always 0
                rdpq_load_tile(tile_offset, g->s, g->t, g->s+width, g->t+height);
                tile_offset = (tile_offset + 1) & 7;
                break;
            default:
                assertf(0, "invalid rdram_loading value %d", rdram_loading);
            }
        }

        rdpq_texture_rectangle(ntile,
            x, y, x+width, y+height,
            g->s, g->t);

        ch++;
    }

    return ch - chars;
}

bool rdpq_font_get_glyph_ranges(const rdpq_font_t *fnt, int idx, uint32_t *start, uint32_t *end, bool *sparse)
{
    if (idx < 0 || idx >= fnt->num_ranges)
        return false;
    *start = fnt->ranges[idx].first_codepoint;
    *end = fnt->ranges[idx].first_codepoint + fnt->ranges[idx].num_codepoints - 1;
    if (sparse) *sparse = fnt->ranges[idx].first_glyph < 0;
    return true;
}

int rdpq_font_get_glyph_index(const rdpq_font_t *fnt, uint32_t codepoint)
{
    return __rdpq_font_glyph(fnt, codepoint);
}

bool rdpq_font_get_glyph_metrics(const rdpq_font_t *fnt, uint32_t codepoint, rdpq_font_gmetrics_t *metrics)
{
    int16_t glyph = __rdpq_font_glyph(fnt, codepoint);
    if (glyph < 0)
        return false;

    glyph_t *g = &fnt->glyphs[glyph];
    metrics->xadvance = g->xadvance;
    metrics->x0 = g->xoff;
    metrics->y0 = g->yoff;
    metrics->x1 = g->xoff2;
    metrics->y1 = g->yoff2;
    return true;
}

rdpq_font_t *__rdpq_font_load_builtin_1(void)
{
    return rdpq_font_load_buf((void*)__fontdb_monogram, __fontdb_monogram_len);
}

rdpq_font_t *__rdpq_font_load_builtin_2(void)
{
    return rdpq_font_load_buf((void*)__fontdb_at01, __fontdb_at01_len);
}


extern inline void __rdpq_font_glyph_metrics(const rdpq_font_t *fnt, int16_t index, float *xadvance, int8_t *xoff, int8_t *xoff2, bool *has_kerning, uint8_t *sort_key);
extern inline rdpq_font_t* rdpq_font_load_builtin(rdpq_font_builtin_t font);

