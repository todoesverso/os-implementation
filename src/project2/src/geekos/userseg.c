/*
 * Segmentation-based user mode implementation
 * Copyright (c) 2001,2003 David H. Hovemeyer <daveho@cs.umd.edu>
 * $Revision: 1.23 $
 * 
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "COPYING".
 */

#include <geekos/ktypes.h>
#include <geekos/kassert.h>
#include <geekos/defs.h>
#include <geekos/mem.h>
#include <geekos/string.h>
#include <geekos/malloc.h>
#include <geekos/int.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/tss.h>
#include <geekos/kthread.h>
#include <geekos/argblock.h>
#include <geekos/user.h>

#include <geekos/errno.h>

/* ----------------------------------------------------------------------
 * Variables
 * ---------------------------------------------------------------------- */

#define DEFAULT_USER_STACK_SIZE 8192
#define LDTCS 0
#define LDTDS 1


/* ----------------------------------------------------------------------
 * Private functions
 * ---------------------------------------------------------------------- */


/*
 * Create a new user context of given size
 */

/* TODO: Implement
*/
static struct User_Context* Create_User_Context(ulong_t size)
{   
    struct User_Context *userContext = 0;

    /* Allocate space for User_Context 'user.h' */
    userContext = (struct User_Context*) Malloc(sizeof(struct User_Context));
    if(userContext == NULL){
        Print(" [!] Failed allocating memory for userContext\n");
        goto error;
    }

    /* Allocate 'size' memory and fill size fild of userContext 
     */
    userContext->memory = (char *) Malloc(sizeof(char) * size);
    memset((char *) userContext->memory, '\0', size); 
    userContext->size   = size;

    if(userContext->memory == NULL){
        Print(" [!] Failed allocating memory for userContext->memory\n");
        goto error;
    }

    /* Allocate a LDT */
    /* METODO - Handle NULL (could not allocate descriptor) */
    userContext->ldtDescriptor = Allocate_Segment_Descriptor();
    if(userContext->ldtDescriptor == NULL){
       Print(" [!] Could not allocate descriptor\n");
       goto error;
    } 

    /* Init LDT */
    Init_LDT_Descriptor(userContext->ldtDescriptor, 
            userContext->ldt,
            NUM_USER_LDT_ENTRIES);    

    /* Create a selector that contains the location of the LDT descriptor
     * whitin the GDT 
     */
    userContext->ldtSelector = Selector(KERNEL_PRIVILEGE, true, 
            Get_Descriptor_Index(userContext->ldtDescriptor));

    /* Create descriptors for the code and the data segments of the
     * user program and add these descriptors to the LDT
     */
    Init_Code_Segment_Descriptor(&userContext->ldt[LDTCS], 
            (ulong_t) userContext->memory,
            (size/PAGE_SIZE), 
            USER_PRIVILEGE);

    Init_Data_Segment_Descriptor(&userContext->ldt[LDTDS], 
            (ulong_t) userContext->memory,
            (size/PAGE_SIZE), 
            USER_PRIVILEGE);


    userContext->csSelector = Selector(USER_PRIVILEGE, 
                                        false, 
                                        0);
                                        //(int)&userContext->ldt[LDTCS]);

    userContext->dsSelector = Selector(USER_PRIVILEGE, 
                                        false, 
                                        1);
                                        //(int)&userContext->ldt[LDTDS]);

    KASSERT(userContext != NULL);
    userContext->refCount = 0;

    return userContext;

error:
    Free(userContext);
    Free(userContext->memory);
    Free(userContext);
    return NULL;
}    


static bool Validate_User_Memory(struct User_Context* userContext,
    ulong_t userAddr, ulong_t bufSize)
{
    ulong_t avail;

    if (userAddr >= userContext->size)
        return false;

    avail = userContext->size - userAddr;
    if (bufSize > avail)
        return false;

    return true;
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */

/*
 * Destroy a User_Context object, including all memory
 * and other resources allocated within it.
 */
void Destroy_User_Context(struct User_Context* userContext)
{
    /*
     * Hints:
     * - you need to free the memory allocated for the user process
     * - don't forget to free the segment descriptor allocated
     *   for the process's LDT
     */
    //TODO("Destroy a User_Context");


    Free_Segment_Descriptor(userContext->ldtDescriptor);
    //Free(userContext->memory);
    userContext->memory = 0;
    Free(userContext);

    Print(" [*] User Context Destroyed\n");
    userContext = NULL;
}

/*
 * Load a user executable into memory by creating a User_Context
 * data structure.
 * Params:
 * exeFileData - a buffer containing the executable to load
 * exeFileLength - number of bytes in exeFileData
 * exeFormat - parsed ELF segment information describing how to
 *   load the executable's text and data segments, and the
 *   code entry point address
 * command - string containing the complete command to be executed:
 *   this should be used to create the argument block for the
 *   process
 * pUserContext - reference to the pointer where the User_Context
 *   should be stored
 *
 * Returns:
 *   0 if successful, or an error code (< 0) if unsuccessful
 */
int Load_User_Program(char *exeFileData, ulong_t exeFileLength,
    struct Exe_Format *exeFormat, const char *command,
    struct User_Context **pUserContext)
{
    /*
     * Hints:
     * - Determine where in memory each executable segment will be placed
     * - Determine size of argument block and where it memory it will
     *   be placed
     * - Copy each executable segment into memory
     * - Format argument block in memory
     * - In the created User_Context object, set code entry point
     *   address, argument block address, and initial kernel stack pointer
     *   address
     */
   // TODO("Load a user executable into a user memory space using segmentation");
    unsigned long virtSize;
    int i = 0;
    int ret = -1;
    ulong_t maxva = 0;
    unsigned numArgs = 0;
    ulong_t argBlockSize = 0;
    struct Argument_Block *pArgBlock = NULL;
    ulong_t start_vaddr_argblock = 0;
    ulong_t max_vaddr_argblock = 0;
    struct User_Context *userContext = 0;
    
    /* Find maximum virtual address */
    for (i = 0; i < exeFormat->numSegments; ++i) {
        struct Exe_Segment *segment = &exeFormat->segmentList[i];
        ulong_t topva = segment->startAddress + segment->sizeInMemory;

        if (topva > maxva)
            maxva = topva;
    }

    /* Find Argument Block Size */
   // Get_Argument_Block_Size(command, &numArgs, &argBlockSize);
    if(numArgs < 0 || argBlockSize < 0){
        Print(" [!] numArgs < 0 || argBlockSize < 0 !!!\n");
        ret = EUNSPECIFIED;
        goto error;
    }

    /*
    virtSize = Round_Up_To_Page(maxva) + 
        Round_Up_To_Page(DEFAULT_USER_STACK_SIZE +
                argBlockSize); 
    */
    virtSize = Round_Up_To_Page(maxva);
    Get_Argument_Block_Size(command, &numArgs, &argBlockSize);
    virtSize += Round_Up_To_Page(DEFAULT_USER_STACK_SIZE);
    start_vaddr_argblock = virtSize;
    virtSize += Round_Up_To_Page(argBlockSize);
    max_vaddr_argblock = virtSize;


    userContext = Create_User_Context(virtSize);
    if(pUserContext == NULL){
        Print(" [!] Failed Create_User_Context\n");
        ret = ENOMEM;
        goto error;
    }

    /* Copy segments over into process' memory space */
    for (i = 0; i < exeFormat->numSegments; i++){
        struct Exe_Segment *segment = &exeFormat->segmentList[i];

        memcpy(userContext->memory + segment->startAddress,
            exeFileData + segment->offsetInFile,
            segment->lengthInFile);
    }


    pArgBlock = Malloc(sizeof(struct Argument_Block)); 
    if(pArgBlock == NULL){
        Print(" [!] Failed allocating memory for temp argBlock\n");
        ret = ENOMEM;
        goto error;
    }
/*
    Format_Argument_Block((char *) pArgBlock,  
            numArgs, 
            (ulong_t) (*pUserContext)->memory + maxva,
            command);
            */  

    Free(pArgBlock);
    Format_Argument_Block(userContext->memory + start_vaddr_argblock, 
                            numArgs, 
                            start_vaddr_argblock,
                            command);


    /* Create the user context */
    userContext->entryAddr          = exeFormat->entryAddr;
    //(*pUserContext)->argBlockAddr       = (ulong_t)(*pUserContext)->memory + maxva;
    userContext->argBlockAddr       = start_vaddr_argblock;
    //(*pUserContext)->stackPointerAddr   = (ulong_t)virtSize;
    userContext->stackPointerAddr   = max_vaddr_argblock;

/*    
Print("    command:\t\t%s numArgs:\t%d argBlockSize:\t%li\n", command, numArgs, argBlockSize);
Print("    seg_size:\t\t%li\n",maxva); 
Print("    seg pages:\t\t%li\n\n",(maxva)/PAGE_SIZE); 
Print("    arg_size:\t\t%li\n",argBlockSize); 

Print("    entry_addr:\t\t%li\n",userContext->entryAddr);
Print("    stack_addr:\t\t%li\n", userContext->stackPointerAddr);
Print("    argb_addr:\t\t%li\n", userContext->argBlockAddr);
Print("    max_vaddr:\t\t%li\n\n",virtSize);

Print("    mem address:\t%ld\n\n", (ulong_t)userContext->memory);
Print("    size:\t\t%ld\n", userContext->size);
Print("    pages used:\t\t%lu\n", userContext->size/PAGE_SIZE);
Print("    LDT Descriptor:\t%d\n", userContext->ldtSelector);
Print("    code selector:\t%d\n", userContext->csSelector);
Print("    data selector:\t%d\n", userContext->dsSelector);
*/
    *pUserContext = userContext;

    return 0;

error:
    Free(pArgBlock);
    return ret;
}

/*
 * Copy data from user memory into a kernel buffer.
 * Params:
 * destInKernel - address of kernel buffer
 * srcInUser - address of user buffer
 * bufSize - number of bytes to copy
 *
 * Returns:
 *   true if successful, false if user buffer is invalid (i.e.,
 *   doesn't correspond to memory the process has a right to
 *   access)
 */
bool Copy_From_User(void* destInKernel, ulong_t srcInUser, ulong_t bufSize)
{
    /*
     * Hints:
     * - the User_Context of the current process can be found
     *   from g_currentThread->userContext
     * - the user address is an index relative to the chunk
     *   of memory you allocated for it
     * - make sure the user buffer lies entirely in memory belonging
     *   to the process
     */
    //TODO("Copy memory from user buffer to kernel buffer");
    //Validate_User_Memory(NULL,0,0); /* delete this; keeps gcc happy */
    bool ret = false;

    ret = Validate_User_Memory(g_currentThread->userContext,
                                srcInUser, bufSize);
    if (ret){
        memcpy(destInKernel, 
                (void *)(g_currentThread->userContext->memory + srcInUser),
                bufSize);
    }

    return ret;

}

/*
 * Copy data from kernel memory into a user buffer.
 * Params:
 * destInUser - address of user buffer
 * srcInKernel - address of kernel buffer
 * bufSize - number of bytes to copy
 *
 * Returns:
 *   true if successful, false if user buffer is invalid (i.e.,
 *   doesn't correspond to memory the process has a right to
 *   access)
 */
bool Copy_To_User(ulong_t destInUser, void* srcInKernel, ulong_t bufSize)
{
    /*
     * Hints: same as for Copy_From_User()
     */
    //TODO("Copy memory from kernel buffer to user buffer");
    bool ret = false;

    ret = Validate_User_Memory(g_currentThread->userContext,
                                destInUser, bufSize);
    if (ret)
        memcpy((void *) (g_currentThread->userContext->memory + destInUser),
                srcInKernel,
                bufSize);

    return ret;
}

/*
 * Switch to user address space belonging to given
 * User_Context object.
 * Params:
 * userContext - the User_Context
 */
void Switch_To_Address_Space(struct User_Context *userContext)
{
    /*
     * Hint: you will need to use the lldt assembly language instruction
     * to load the process's LDT by specifying its LDT selector.
     */
    //TODO("Switch to user address space using segmentation/LDT");

    __asm__ __volatile__ ("lldt %0" :: "a" (userContext->ldtSelector));
    //Print(" [*] Switch to user address space using segmentation/LDT\n");
}

