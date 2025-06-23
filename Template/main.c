//#include "../common/callbacks.h"

//Include Graphics Libraries
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

//PSP Module Info
PSP_MODULE_INFO("context", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

// Define PSP Width / Heightp
#define PSP_BUF_WIDTH (512)
#define PSP_SCR_WIDTH (480) // screen width
#define PSP_SCR_HEIGHT (272) // screen height

//Global variables
int running = 1;
// GE LIST
static unsigned int __attribute__((aligned(16))) list[262144];

// calculates how many MB the application needs
static unsigned int getMemorySize(unsigned int width, unsigned int height, unsigned int psm){
    unsigned int size = width * height;

    switch(psm){
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

void* getStaticVramBuffer(unsigned int width, unsigned int height, unsigned int psm){
    static unsigned int staticOffset = 0;

    unsigned int memSize = getMemorySize(width, height, psm);

    void* result = (void*)staticOffset;

    staticOffset += memSize;

    return result;
}

void* getStaticVramTexture(unsigned int width, unsigned int height, unsigned int psm){
    void* result = getStaticVramBuffer(width, height, psm);
    return (void*)( ((unsigned int)result) + ((unsigned int) sceGeEdramGetAddr()) );
}

void initGraphics(){
    void* fbp0 = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_8888);
    void* fbp1 = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_8888);
    void* zbp = getStaticVramBuffer(PSP_BUF_WIDTH, PSP_SCR_HEIGHT, GU_PSM_4444);

    sceGuInit();

    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, fbp0, PSP_BUF_WIDTH);
    sceGuDispBuffer(PSP_SCR_WIDTH, PSP_SCR_HEIGHT, fbp1, PSP_BUF_WIDTH);
    sceGuDepthBuffer(zbp, PSP_BUF_WIDTH);

    // to tell the psp that we want to drawn in the middle of our coordonate space
    sceGuOffset(2048 - (PSP_SCR_WIDTH /2), 2048 - (PSP_SCR_HEIGHT / 2)); 
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

void termGraphics(){
    sceGuTerm();
}

int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int callback_thread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0)
        sceKernelStartThread(thid, 0, 0);
    return thid;
}

void startFrame() {
    sceGuStart(GU_DIRECT, list);
}

void endFrame(){
    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers(); // swaps displayBuffer with drawBuffer
} 

int main(){
    //Boillerplate
    setup_callbacks(); // home button functionnality

    initGraphics();

    //Main program loop
    while(running){
        //TODO: Do something
        startFrame();

        // normally we do RGBA
        //0-255 0-255 0-255 0-255
        // on psp its backwards
        // A = 255
        // B = 0
        // G = 255
        // R = 0
        sceGuClearColor(0xFF00FF00);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        endFrame();
    }

    termGraphics();

    // Exit Game
    sceKernelExitGame();
    return 0;
}