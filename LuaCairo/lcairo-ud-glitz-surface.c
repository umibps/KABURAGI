/* -*- C -*-
 *
 * Lua binding of Cairo
 *
 */
#ifndef LUACAIRO
# error private include file
#endif




//---------------------------------------------------------------------------
// GlitzSurface userdata for oo usage
//---------------------------------------------------------------------------



#if CAIRO_HAS_GLITZ_SURFACE
#if LUACAIRO_HAS_OO_INTERFACE


static int new_GlitzSurface (lua_State *L)
{
    lua_remove(L, 1); // remove cairo.GlitzSurface

    //{"create",                                   l_cairo_glitz_surface_create},
    glitz_surface_t *surface = (glitz_surface_t *) check_lightuserdata(L, 1);
    cairo_surface_t *cs = cairo_glitz_surface_create (surface);

    return new_Surface(L, LUACAIRO ".GlitzSurface.mt", cs, CAIRO_SURFACE_TYPE_GLITZ, 1);
}

static const struct luaL_Reg GlitzSurface_lib_f [] = {
    {"new", new_GlitzSurface},
    {NULL, NULL}
};

static const struct luaL_Reg GlitzSurface_lib_m [] = {
    {"__gc",                                     gc_Surface},
    {"__tostring",                               tostring_Surface},

    {"surface_create_similar",                   l_cairo_surface_create_similar},
    {"reference",                                l_cairo_surface_reference},
    {"finish",                                   l_cairo_surface_finish},
    {"destroy",                                  gc_Surface},
    {"get_reference_count",                      l_cairo_surface_get_reference_count},
    {"status",                                   l_cairo_surface_status},
    {"get_type",                                 l_cairo_surface_get_type},
    {"get_content",                              l_cairo_surface_get_content},
#if CAIRO_HAS_PNG_FUNCTIONS
    {"write_to_png",                             l_cairo_surface_write_to_png},
    {"write_to_png_stream",                      l_cairo_surface_write_to_png_stream},
#endif
    {"get_user_data",                            l_cairo_surface_get_user_data},
    {"set_user_data",                            l_cairo_surface_set_user_data},
    {"get_font_options",                         l_cairo_surface_get_font_options},
    {"flush",                                    l_cairo_surface_flush},
    {"mark_dirty",                               l_cairo_surface_mark_dirty},
    {"mark_dirty_rectangle",                     l_cairo_surface_mark_dirty_rectangle},
    {"set_device_offset",                        l_cairo_surface_set_device_offset},
    {"get_device_offset",                        l_cairo_surface_get_device_offset},
    {"set_fallback_resolution",                  l_cairo_surface_set_fallback_resolution},
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 8, 0)
    {"get_fallback_resolution",                  l_cairo_surface_get_fallback_resolution},
#endif
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 6, 0)
    {"copy_page",                                l_cairo_surface_copy_page},
    {"show_page",                                l_cairo_surface_show_page},
#endif
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 8, 0)
    {"has_show_text_glyphs",                     l_cairo_surface_has_show_text_glyphs},
#endif

  //{"create",                                   l_cairo_glitz_surface_create},

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 10, 0)
    {"get_device",                               l_cairo_surface_get_device},
#endif

    {NULL, NULL}
};

static int luaopen_lGlitzSurface (lua_State *L)
{
    luaL_newmetatable(L, LUACAIRO ".GlitzSurface.mt");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    //luaL_register(L, NULL, GlitzSurface_lib_m);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, GlitzSurface_lib_m, 0);
    //luaL_register(L, LUACAIRO ".GlitzSurface", GlitzSurface_lib_f);
	luaL_newlib(L, GlitzSurface_lib_f);
	lua_setglobal(L, "GlitzSurface");
    lua_newtable(L);
    lua_pushstring(L, "__call");
    lua_pushcfunction(L, new_GlitzSurface);
    lua_settable(L, -3);
    lua_setmetatable(L, -2);
    lua_pop(L, 1);
    lua_pop(L, 1);
    return 0;
}

#endif  /* LUACAIRO_HAS_OO_INTERFACE */
#endif  /* CAIRO_HAS_GLITZ_SURFACE */



static int luaopen_lcairo_ud_glitz_surface (lua_State* L)
{
#if CAIRO_HAS_GLITZ_SURFACE
#if LUACAIRO_HAS_OO_INTERFACE

    luaopen_lGlitzSurface (L);

#endif /* LUACAIRO_HAS_OO_INTERFACE */
#endif /* CAIRO_HAS_GLITZ_SURFACE */
    return 0;
}