#ifndef H_LANGUAGE_GL
#define H_LANGUAGE_GL


// Math
#define mul(M, V) M * V

#define STRUCT(NAME)
#define END_STRUCT

#define ATTRIBUTE_VERETX_IN(NUM, TYPE, NAME, SEMANTIC) layout(location = NUM) in TYPE NAME;
#ifdef ENG_SHADER_VERTEX
#define ATTRIBUTE(TYPE, NAME, SEMANTIC) smooth out TYPE NAME;
#elif ENG_SHADER_PIXEL
#define ATTRIBUTE(TYPE, NAME, SEMANTIC) smooth in TYPE NAME;
#endif

// Uniform Blocks
// Note:
//		- Use only types multiplied 2 or 4 (not vec3, mat3!)
//		- By default in OpenGL (and DirectX) CPU-GPU transfer implemented in column-major style.
//		We change this behaviour by keyword "row_major" for all uniform buffers
//		to match C++ math lib wich keeps matrix in rom_major style.
//		- Don't use ifdef in uniform buffer block to match C++ side struct
#define UNIFORM_BUFFER_BEGIN(SLOT) layout (std140, binding = SLOT, row_major) uniform const_buffer_ ## SLOT { 
#define UNIFORM_BUFFER_END };
#define UNIFORM(TYPE, NAME) uniform TYPE NAME;

// Textures
#define TEXTURE2D_IN(NAME, NUM) uniform sampler2D NAME;
#define TEXTURE(NAME, UV) texture(NAME, UV)

// Vertex in/out
#define IN_ATTRIBUTE(NAME) NAME
#define INIT_POSITION
#define OUT_ATTRIBUTE(NAME) NAME
#define OUT_POSITION gl_Position

// Fragment in/out
#define GET_ATRRIBUTE(NAME)NAME
#define OUT_COLOR color

// Main functions
#define MAIN_VERTEX(VERTEX_IN_, VERTEX_OUT_) void main() {
#define MAIN_VERTEX_END }
#define MAIN_FRAG(FRAG_IN) out vec4 color; void main() {
#define MAIN_FRAG_END }
#define MAIN_FRAG_UI(FRAG_IN) out uint color; void main() {

#endif // H_LANGUAGE_GL
