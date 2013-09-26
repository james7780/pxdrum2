#include <pspkernel.h>
#include <pspmodulemgr.h>
#include <pspdisplay.h>
#include <pspdebug.h>
#include <pspthreadman.h>
#include <pspctrl.h>
#include <pspsdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define printf pspDebugScreenPrintf

/* 0x1000 = kernel mode set */
PSP_MODULE_INFO("AdHoc", 0x1000, 0, 1);
PSP_MAIN_THREAD_ATTR(0);

/* exit callback */
int exit_callback(int arg1, int arg2, void *common){
adhocTerm();
sceKernelExitGame();
return 0;
}

/* callback thread */
int CallbackThread(SceSize args, void *argp){
int cbid;
cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
sceKernelRegisterExitCallback(cbid);
sceKernelSleepThreadCB();
return 0;
}

/* debug output */
void sdl_psp_exception_handler(PspDebugRegBlock *regs){
pspDebugScreenInit();

pspDebugScreenSetBackColor(0x00FF0000);
pspDebugScreenSetTextColor(0xFFFFFFFF);
pspDebugScreenClear();

printf("oh **** BSOD...\n\n");
printf("Exception Details:\n");
pspDebugDumpException(regs);
printf("\nPut this in your cygwin and smoke it:\n\n"
"\tpsp-addr2line -e target.elf -f -C 0x%x 0x%x 0x%x\n",
regs->epc, regs->badvaddr, regs->r[31]);
}

/* setup the callback thread */
int SetupCallbacks(void){
int thid = 0;
thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
if(thid >= 0)
sceKernelStartThread(thid, 0, 0);
return thid;
}

/* Alphabet Examples */
int user_main(SceSize args, void *argp){

/* var init */
int done, err, i, loop, server, size = 0;
unsigned int length;
char *data;
SceCtrlData pad;

pspDebugScreenInit();

/* adhoc initalise functions */
if((adhocInit("") >= 0) && ((server = adhocSelect()) >=0)){
printf("\n\n## AdHoc INITALISED ##\n\n");
sceKernelDelayThread(1000000);

/* Display server, or client */
if(server)
printf("Server Assigned\n\n");
else
printf("Client Assigned\n\n");

/* if this psp is the server */
if(server){
/* client must be waiting for server, so slight delay for server */
sceKernelDelayThread(1000000);
/* alphabet string */
data = "abcdefghijklmnopqrstuvwxyz";
size = sizeof(data);

printf("Sending data to the client.. size: %d \n", size);
err = adhocSendRecvAck(&size, 4); /* let client know the size of data */
printf("Done sending size: %d\ndata: %s\n", size, data);

sceKernelDelayThread(1000000);

err = adhocSendRecvAck(&data, 4);  /* send data as void */
printf("Done sending data: %s\nsize: %d\n", data, size);
} else {
printf("Waiting for data..\n");

/* get data from server */
size = 0;
length = 4;
err = adhocRecvSendAck(&size, &length); /* get size of data */
printf("Done receiving size: %d \nlength: %d \n", size, length);

length = size;
err = adhocRecvSendAck(&data, &length); /* get data */
data = (char *)data; /* type cast received data */
printf("Received data from server: %s \n", data);
}
}
/* terminate adhoc */
adhocTerm();

printf("Press START to exit..\n");
done = 0;
do {
sceCtrlReadBufferPositive(&pad, 1);
if(pad.Buttons != 0)
if(pad.Buttons & PSP_CTRL_START)
done = 1;
} while(!done);

}

/* Data Stream Examples *//*
int user_main(SceSize args, void *argp){

SetupCallbacks();

static char buffer[0x8000];
unsigned int length;
int err, bD = 0;
int i, done, mainDone, server=0;

char *data;
int size = 0;

SceCtrlData pad;

pspDebugScreenInit();

do {

if((adhocInit("") >= 0) && ((server = adhocSelect()) >=0)){

pspDebugScreenPrintf("\n\n## AdHoc INITALISED ##\n\n");

sceKernelDelayThread(1000000);

if(server)
pspDebugScreenPrintf("Server Assigned\n\n");
else
pspDebugScreenPrintf("Client Assigned\n\n");

if(server){
sceKernelDelayThread(5000000);
done = 0;
do {
sceCtrlReadBufferPositive(&pad, 1);
if(pad.Buttons != 0)
if(pad.Buttons & PSP_CTRL_CROSS)
done = 1;

if((i % 2) == 0){
data = ".";
i = 0;
} else
data = "|";
i++;

if(done == 1)
data = "x";
size = sizeof(data);

err = adhocSendRecvAck(&size, 4);
err = adhocSendRecvAck(&data, 4);  //# send data

pspDebugScreenPrintf("%s", data);
} while(done==0);

} else {
done = 0;
do {
//sceCtrlReadBufferPositive(&pad, 1);
//if(pad.Buttons != 0)
// if(pad.Buttons & PSP_CTRL_CROSS)
// done = 1;

size = 0;
length = 4;
err = adhocRecvSendAck(&size, &length);

length = size;
err = adhocRecvSendAck(&data, &length);
data = (char *)data;

if(data == "x")
done = 1;

pspDebugScreenPrintf("%s", data);
} while(done==0);

}

}

pspDebugScreenPrintf("\n\n");

adhocTerm();

done = 0;
pspDebugScreenInit();
pspDebugScreenPrintf("\n\nPress START to Exit, X to Continue\n\n");
do {
sceCtrlReadBufferPositive(&pad, 1);
if(pad.Buttons != 0){
if(pad.Buttons & PSP_CTRL_START){
done = 1; mainDone=1;
sceKernelExitGame();
}
if(pad.Buttons & PSP_CTRL_CROSS)
done = 1; mainDone=0;
}
} while(!done);

} while(!mainDone);

//pspDebugScreenPrintf("Press any key to exit..\n");
//done = 0;
//do {
// sceCtrlReadBufferPositive(&pad, 1);
// if(pad.Buttons != 0)
// done = 1;
//} while(!done);

sceKernelExitGame();
}*/

/* kernel mode thread */
int main(int argc, char *argp[]){

/* exit callback */
SetupCallbacks();

/* load adhoc modules */
    if (adhocLoadDrivers(&module_info) != 0){
        printf("Driver load error\n");
        return 0;
    }

/* setup debug info handler */
pspDebugInstallErrorHandler(sdl_psp_exception_handler);

    /* create user thread */
    SceUID thid = sceKernelCreateThread("User Mode Thread", user_main,
            0x11, // default priority
            256 * 1024, // stack size (256KB is regular default)
            PSP_THREAD_ATTR_USER, NULL); //# user mode

    // start user thread, then wait for it to do everything else
    sceKernelStartThread(thid, 0, 0);
    sceKernelWaitThreadEnd(thid, NULL);

/* quick clean exit */
sceKernelExitGame();

    return 0;
}