namespace asset
{
#ifdef __EMSCRIPTEN__
#define SHADER_DIR "/shader"
#else
#define SHADER_DIR "@CMAKE_SOURCE_DIR@/shader"
#endif
}
