/* -*- C -*-
 *
 * Lua binding of Cairo
 *
 */
#ifndef LUACAIRO
# error private include file
#endif


/*
 * Quartz font support
 */

#if CAIRO_HAS_QUARTZ_FONT
#include <cairo-quartz.h>


#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 6, 0)
// cairo_public cairo_font_face_t *
// cairo_quartz_font_face_create_for_cgfont (CGFontRef font);
static int l_cairo_quartz_font_face_create_for_cgfont(lua_State* L)
{
    luaL_error(L, "not implemented!");
    return 0;
}
#endif


#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 6, 0)
#ifndef __LP64__
// cairo_public cairo_font_face_t *
// cairo_quartz_font_face_create_for_atsu_font_id (ATSUFontID font_id);
static int l_cairo_quartz_font_face_create_for_atsu_font_id(lua_State* L)
{
    luaL_error(L, "not implemented!");
    return 0;
}
#endif
#endif



static const struct luaL_Reg quartz_font_face_f [] = {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 6, 0)
    {"quartz_font_face_create_for_cgfont",           l_cairo_quartz_font_face_create_for_cgfont},
#ifndef __LP64__
    {"quartz_font_face_create_for_atsu_font_id",     l_cairo_quartz_font_face_create_for_atsu_font_id},
#endif
#endif
	{NULL, NULL} /* sentinel */
};


#endif /* CAIRO_HAS_QUARTZ_FONT */



static int luaopen_lcairo_quartz_font_face(lua_State* L)
{
#if CAIRO_HAS_QUARTZ_FONT

    // register functions
	set_functions(L, quartz_font_face_f);
    //luaL_register(L, LUACAIRO, quartz_font_face_f);

#endif /* CAIRO_HAS_QUARTZ_FONT */
    return 0;
}
