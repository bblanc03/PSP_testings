

// Include Graphics Libraries
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

// PSP Module Info
PSP_MODULE_INFO("context", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

// Define PSP Width / Heightp
#define PSP_BUF_WIDTH (512)
#define PSP_SCR_WIDTH (480)  // screen width
#define PSP_SCR_HEIGHT (272) // screen height

// Global variables
int running = 1;
// GE LIST
static unsigned int __attribute__((aligned(16))) list[262144];

// calculates how many MB the application needs
static unsigned int getMemorySize(unsigned int width, unsigned int height, unsigned int psm)
{
    unsigned int size = width * height;

    switch (psm)
    {
    case GU_PSM_T4:
        return size / 2;
    case GU_PSM_T8:
        return size;

    case GU_PSM_5650:
    case GU_PSM_5551:
    case GU_PSM_4444:
    case GU_PSM_T16:
        return size * 2;

    case GU_PSM_8888:
    case GU_PSM_T32:
        return size * 4;

    default:
        return 0;
    }
}

void *getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm)
{
    static unsigned int staticOffset = 0;

    unsigned int memSize = getMemorySize(width, height, psm);

    void *result = (void *)staticOffset;

    staticOffset += memSize;

    return result;
}

void *getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm)
{
    void *result = getStaticVramBuffer(width, height, psm);
    return (void *)(((unsigned int)result) + ((unsigned int)sceGeEdramGetAddr()));
}

void initGraphics()
{
    void *fbp0 = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_8888);
    void *fbp1 = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_8888);
    void *zbp = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_4444);

    sceGuInit();

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, PSP_BUF_WIDTH);
    sceGuDispBuffer(PSP_SCR_WIDTH, PSP_SCR_HEIGHT, fbp1, PSP_BUF_WIDTH);
    sceGuDepthBuffer(zbp, PSP_BUF_WIDTH);

    // to tell the psp that we want to drawn in the middle of our coordonate space
    sceGuOffset(2048 - (PSP_SCR_WIDTH / 2), 2048 - (PSP_SCR_HEIGHT / 2));
    sceGuViewport(2048, 2048, PSP_SCR_WIDTH, PSP_SCR_HEIGHT);

    sceGuDepthRange(65535, 0); // this is to set the depth of the device. first number is near and secound is far since psp has inverted depth

    sceGuEnable(GU_SCISSOR_TEST);
    sceGuScissor(0, 0, PSP_SCR_WIDTH, PSP_SCR_HEIGHT); // to force it to only render within the limist fo the screen 480x272 (WxH)

    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);

    sceGuEnable(GU_CULL_FACE);
    sceGuFrontFace(GU_CW);

    sceGuShadeModel(GU_SMOOTH);

    sceGuEnable(GU_TEXTURE_2D);
    sceGuEnable(GU_CLIP_PLANES);

    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

void termGraphics()
{
    sceGuTerm();
}

int exit_callback(int arg1, int arg2, void *common)
{
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp)
{
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void)
{
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

void startFrame()
{
    sceGuStart(GU_DIRECT, list);
}

void endFrame()
{
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers(); // swaps displayBuffer with drawBuffer
}

void reset_translate(float x, float y, float z) // in 2d it resets the position of zero in the mapto the specified point like (objc2d.trancslate in webgl)
{
    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();

    ScePspFVector3 v = {x, y, z};
    sceGumTranslate(&v);
}

struct Vertex
{
    unsigned int color;
    float x, y, z;
};

///      2
///
///  1 <==== 0
///
struct Vertex __attribute__((aligned(16))) triangle[3] = {
    //[x] = number of traingles
    {0xFF0000FF, 0.35f, 0.0f, -1.0f},
    {0xFF00FF00, -0.35f, 0.0f, -1.0f},
    {0XFFFF0000, 0.0f, 0.5f, -1.0f},
};

struct Vertex __attribute__((aligned(16))) square[6] = {
    {0xFF0000FF, -0.25f, -0.25f, -1.0f}, // 0
    {0xFF0000FF, -0.25f, 0.25f, -1.0f},  // 1
    {0xFF00FF00, 0.25f, 0.25f, -1.0f},   // 2
    {0xFF00FF00, 0.25f, 0.25f, -1.0f},   // 2
    {0xFFFF0000, 0.25f, -0.25f, -1.0f}, // 3
    {0xFF0000FF, -0.25f, -0.25f, -1.0f}, // 0
};

//we use aligned(16) to limit the vertex to onl take 16 bites since the bites or each vertex is 16 (hexcode = 4, floats = 4)
struct Vertex __attribute__((aligned(16))) square_indexed[4] = { // this is like maillage from school. Probably gonna use this version the most
    {0xFF0000FF, -0.25f, -0.25f, -1.0f}, // 0
    {0xFF0000FF, -0.25f, 0.25f, -1.0f},  // 1
    {0xFF00FF00, 0.25f, 0.25f, -1.0f},   // 2
    {0xFFFF0000, 0.25f, -0.25f, -1.0f}, // 3
};

unsigned short __attribute__((aligned(16))) square_indices[6] = { // table to tell the order of wich to link the vertices
    0, 1, 2, 2, 3, 0
};

int main()
{
    // Boillerplate
    setup_callbacks(); // home button functionnality

    // Initialize Graphics
    initGraphics();

    // Initialize Matrices
    sceGumMatrixMode(GU_PROJECTION); // tell is i am in 2d(ortographic matrix) or 3d(perspective matrix)
    sceGumLoadIdentity();
    sceGumOrtho(-16.0f / 9.0f, 16.0f / 9.0f, -1.0f, 1.0f, -10.0f, 10.0f);

    sceGumMatrixMode(GU_VIEW); // camera transformations
    sceGumLoadIdentity();

    sceGumMatrixMode(GU_MODEL); // positions of current model
    sceGumLoadIdentity();

    // Main program loop
    while (running)
    {
        // TODO: Do something
        startFrame();

        sceGuDisable(GU_DEPTH_TEST);
        sceGuDisable(GU_TEXTURE_2D);

        sceGuClearColor(0xFF000000);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        reset_translate(-0.5f, 0.0f, 0.0f);
        sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 3, NULL, triangle);

        reset_translate(0.5f, 0.25f, 0.0f);
        sceGumDrawArray(GU_TRIANGLES, GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, NULL, square);

        reset_translate(0.0f, -0.5f, 0.0f);
        sceGumDrawArray(GU_TRIANGLES, GU_INDEX_16BIT | GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D, 6, square_indices, square_indexed);

        endFrame();
    }

    termGraphics();

    // Exit Game
    sceKernelExitGame();
    return 0;
}