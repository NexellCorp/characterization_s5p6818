// OGLEarthSphere.cpp : Defines the entry point for the application.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* for open/close .. */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "OGLEarthSphere.h"
#include <math.h>

#include "Fake_OS.h"

#include <semaphore.h> 
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "asv_type.h"
#include "V6_sphere_2000.h"
#include "GLFont.h"
#include <nx_alloc_mem.h>

///////////////////////////////////////////////////////////////////////////////
//#define TEST_ES1_FLOAT_EN
#define TEST_ES1_EN
#define TEST_ES2_EN

#define CALLOC	calloc
#define MALLOC 	malloc
#define FREE	free

#define TEST_FRAMES_MAX					100
#define TEST_GOLDEN_FRAME_INTERVAL		10

//#define glF(x)	((GLfixed)((x)*(1<<16)))
//#define glD(x)	glF(x)
//#define GL_F	GL_FIXED
//typedef GLfixed GLf;

#ifdef TEST_ES1_FLOAT_EN
#define X1_0	1.f
#define X0_0	0.f
#define X5_0	5.f
#define X0_5	0.5f
#define X0_2	0.2f
#define X0_1	0.1f
#define X0_3	0.3f
#define X1_2	1.2f
#define X0_8	0.8f
#define X0_9	0.9f
#else
#define glF(x)	((GLfixed)((x)*(1<<16)))
#define glD(x)	glF(x)
#define GL_F	GL_FIXED
typedef GLfixed GLf;
#define X1_0	65536
#define X0_0	0
#define X5_0	327680
#define X0_5	32768
#define X0_2	13107
#define X0_X	(6553/2)
#define X0_1	6553
#define X0_3	21845
#define X1_2	(X1_0+X0_2)
#define X0_8	(X1_0-X0_2)
#define X0_9	(X1_0-X0_1)
#endif

#define LCD_WIDTH  DISP_WIDTH
#define	LCD_HEIGHT DISP_HEIGHT

#define model	V6_sphere_980

/////////////////////////////////////////////////////////////////////////////////////
#define MAX_LOADSTRING 100

#define VR_MSG	printf
#define VR_ERR	printf

enum BUFFERTYPE{
	VERTEX_BUFFER,
	INDEX_BUFFER,
	TEXTURECOORD_BUFFER,
};
//VBO variables

typedef struct tagVRBaseRuntimeThread
{
	pthread_t thread;
	int args[3];
}VRBaseRuntimeThread;
typedef struct tagVRBaseRuntimeThread* HTHREAD;

///////////////////////////////////////////////////////////////////////////////
//state
static ASV_RESULT gState = ASV_RES_OK;
static int gRenderTimeoutState = 0;
static int gThreadExit = 0;

//VBO variables
//static GLuint nBufferName980[3];
static GLuint nBufferName9680[3];

#ifdef TEST_ES1_FLOAT_EN
static GLfloat g_Perspective[16] = 
{
		2.4142141f, 0.f, 0.f, 0.f,
		0.f, 2.4142141f, 0.f, 0.f,
		0.f, 0.f, -1.0010005f, -1.f,
		0.f, 0.f, -1.0005002f, 0.f
};
static GLfloat	xrot;							// X Rotation
static GLfloat	yrot;							// Y Rotation
static GLfloat xspeed = 1.f;					// X Rotation Speed
static GLfloat yspeed = 1.f;					// Y Rotation Speed
static GLfloat	g_depth=-5.f;					// Depth Into The Screen
#else
static GLfixed g_Perspective[16] = 
{
		glF(2.4142141f), 0, 0, 0,
		0, glF(2.4142141f), 0, 0,
		0, 0, glF(-1.0010005f), glF(-1.0),
		0, 0, glF(-1.0005002f), 0
};
static GLfixed	xrot;							// X Rotation
static GLfixed	yrot;							// Y Rotation
static GLfixed xspeed = X1_0;					// X Rotation Speed
static GLfixed yspeed = X1_0;					// Y Rotation Speed
static GLfixed	g_depth=-X5_0;					// Depth Into The Screen
#endif
static GLuint	gTexID1;

static int drawMode = GL_TRIANGLES;
static int nfirstTime = 0;
static int nLastTime = 0;
static int nCnt =0;			
static unsigned int nFrames;
static unsigned int* gResultImage;
static unsigned int* gGoldenImage;
static int err;

static CGLFont* m_pGLFont;
static float	fps = 0.0f;			// Holds The Current FPS (Frames Per Second) 

static EGLDisplay glesDisplay;  // EGL display
static EGLSurface glesSurfaceEs1;	 // EGL rendering surface
static EGLSurface glesSurfaceEs2;	 // EGL rendering surface
static EGLContext glesContextEs1;	 // EGL rendering context
static EGLContext glesContextEs2;	 // EGL rendering context
static EGLImageKHR eglImage;

static NativeWindowType hNativeWnd = 0;
static int		view_width	= 0;
static int		view_height	= 0;
static int gDemoMode = 1;
static int gRenderLoopCnt = 1;
static int gDrawEarthCnt = 1;

//es2.0
static GLuint vert_name = 0;
static GLuint frag_name = 0;
static GLuint prog_name = 0;
static GLint loc_position = -1;
static GLint loc_fill_color = -1;
static GLint loc_tex_coord = -1;
static GLint loc_uni_sampler = -1;
static GLint loc_uni_mvp = -1;
static GLint test_status = 0;

static float* pdata_vertex_pc_f32;
static float* pdata_texcoord_pv_f32;
static unsigned int texture_in_width = 1920, texture_in_height = 1080;
static NX_MEMORY_HANDLE hPixmap;
static EGLNativePixmapType pixmap_input;
static GLuint pixmapTextureName;
PFNEGLCREATEIMAGEKHRPROC _eglCreateImageKHR = NULL;
PFNEGLDESTROYIMAGEKHRPROC _eglDestroyImageKHR = NULL;
PFNGLEGLIMAGETARGETTEXTURE2DOESPROC _glEGLImageTargetTexture2DOES = NULL;
PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC _glEGLImageTargetRenderbufferStorageOES = NULL;
static HTHREAD hthread;
static NativeWindowType  m_nativeWindow;
EGLint		g_EGLXNumOfConfigs;
EGLint		g_EGLXAttributeList[] = {       
		EGL_DEPTH_SIZE, 16,  
		EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
		EGL_NONE
};

///////////////////////////////////////////////////////////////////////////////
static void GL_Draw(void);
static void Render(void);
static void InitTestFramework(void);
static void DeInitTestFramework(void);
static GLboolean SaveFrame(unsigned int frame_count);
static GLboolean CheckResult(unsigned int frame_count, FILE* fp100, FILE* fp1_9);
static void glPerspectivef(GLfloat fov, GLfloat aspect, GLfloat near_val, GLfloat far_val);
static void InitOGLES(void);
static void DeInitOGLES(void);
static float framerate(int Poly);
static bool LoadTGA(TGAImage *texture, char *filename);
static double GetTime();
///////////////////////////////////////////////////////////////////////////////
// EGL X
///////////////////////////////////////////////////////////////////////////////
static GLboolean AppeglxCreate (GLint width, GLint height, GLint mode);
static void AppeglxDelete(void);
static void AppeglxFlush(void);
static void AppSetStatus(ASV_RESULT state);
ASV_RESULT AvsGrahpicGetStatus(void);
static void SetOGLESCurrent_es1(void);
static void SetOGLESCurrent_es2(void);

#define EGL_CHECK(x) \
	x; \
	{ \
		EGLint eglError = eglGetError(); \
		if(eglError != EGL_SUCCESS) { \
			printf("eglGetError() = %i (0x%.8x) at %s:%i\n", eglError, eglError, __FILE__, __LINE__); \
			exit(1); \
		} \
	}

#define GL_CHECK(x) \
	x; \
	{ \
		GLenum glError = glGetError(); \
		if(glError != GL_NO_ERROR) { \
			printf( "glGetError() = %i (0x%.8x) at %s:%i\n", glError, glError, __FILE__, __LINE__); \
			exit(1); \
		} \
	}

//---------------------------------------------------------
//    Functions  
//---------------------------------------------------------
void InitVariables(void)
{
	gState = ASV_RES_OK;
	gRenderTimeoutState = 0;
	gThreadExit = 0;
	
#ifdef TEST_ES1_FLOAT_EN
	xrot = 0.f;							// X Rotation
	yrot = 0.f;							// Y Rotation
	xspeed = 1.f;					// X Rotation Speed
	yspeed = 1.f;					// Y Rotation Speed
	g_depth=-5.f;					// Depth Into The Screen
#else
	xrot = 0;							// X Rotation
	yrot = 0;							// Y Rotation
	xspeed = X1_0;					// X Rotation Speed
	yspeed = X1_0;					// Y Rotation Speed
	g_depth=-X5_0;					// Depth Into The Screen
#endif
	
	nfirstTime = 0;
	nLastTime = 0;
	nCnt =0; 		
	nFrames = 0;
	err = 0;
	
	fps = 0.0f; 		// Holds The Current FPS (Frames Per Second) 
	
	glesDisplay = NULL;	// EGL display
	glesSurfaceEs1 = NULL;	 // EGL rendering surface
	glesSurfaceEs2 = NULL;	 // EGL rendering surface
	glesContextEs1 = NULL;	 // EGL rendering context
	glesContextEs2 = NULL;	 // EGL rendering context
	eglImage = NULL;
	
	hNativeWnd = 0;
	view_width	= 0;
	view_height = 0;
	gDemoMode = 1;
	gRenderLoopCnt = 1;
	gDrawEarthCnt = 1;
	
	//es2.0
	vert_name = 0;
	frag_name = 0;
	prog_name = 0;
	loc_position = -1;
	loc_fill_color = -1;
	loc_tex_coord = -1;
	loc_uni_sampler = -1;
	loc_uni_mvp = -1;
	test_status = 0;
	
	pdata_vertex_pc_f32 = NULL;
	pdata_texcoord_pv_f32 = NULL;
	texture_in_width = 1920, texture_in_height = 1080;
	hPixmap = NULL;
	pixmap_input = NULL;
	_eglCreateImageKHR = NULL;
	_eglDestroyImageKHR = NULL;
	_glEGLImageTargetTexture2DOES = NULL;
	_glEGLImageTargetRenderbufferStorageOES = NULL;
	hthread = NULL;
	m_nativeWindow = NULL;
}

//#define FILE_CREATE

//---------------------------------------------------------
//    Functions  
//---------------------------------------------------------
void* base_runtime_grahpic_test_thread(void* arg)
{
	unsigned int count = 0;
	printf( "Grahpic processing start, frames(%d)\n", nFrames );

	InitTestFramework();
	
	InitOGLES();

	int err = GL_NO_ERROR;
	char sName[128];
	static int run_count;
	FILE* fp100; 
	FILE* fp1_9;
	unsigned int bytes_size = view_width * 4 * view_height;
	sprintf((char*)sName, "/mnt/mmc0/vr/%s_%d_%dx%d_%d.data", "golden", nFrames, view_width, view_height, 4);
	#ifdef FILE_CREATE
	fp100 = fopen((char*)sName, "w");	
	#else
	fp100 = fopen((char*)sName, "r");	
	#endif
	if(!fp100)
	{
		printf("can't open file(%s)\n", sName);
	}
	sprintf((char*)sName, "/mnt/mmc0/vr/%s_%dx%d_%d.data", "golden1-9", view_width, view_height, 4);
	#ifdef FILE_CREATE
	fp1_9 = fopen((char*)sName, "w");	
	#else
	fp1_9 = fopen((char*)sName, "r");	
	#endif
	if(!fp1_9)
	{
		printf("can't open file(%s)\n", sName);
	}
		
	//Message Loop
	if(!nFrames)
	{
	  while(!gThreadExit)
	  {		  	
		Render();		

		if(SaveFrame(count))
		{
			#ifdef FILE_CREATE
			if(count > 0 && count < 10)
			{
				fwrite(gResultImage, bytes_size , 1, fp1_9);
				printf("save count(%d)\n", count);
			}
			else
			{
				fwrite(gResultImage, bytes_size , 1, fp100);
				printf("save count(%d)\n", count);
			}
			#else	
			if(!CheckResult(count, fp100, fp1_9))
			{
				AppSetStatus(ASV_RES_ERR);
				break;
			}
			#endif
		}
	  	++count;
	  }
	}
	else
	{
	  while( !gThreadExit && nFrames-- )
	  {
		Render();
		
		if(SaveFrame(count))
		{
			#ifdef FILE_CREATE
			if(count > 0 && count < 10)
			{
				fwrite(gResultImage, bytes_size , 1, fp1_9);
				printf("save count(%d)\n", count);
			}
			else
			{
				fwrite(gResultImage, bytes_size , 1, fp100);
				printf("save count(%d)\n", count);
			}
			#else	
			if(!CheckResult(count, fp100, fp1_9))
			{
				AppSetStatus(ASV_RES_ERR);
				break;
			}
			#endif
		}
		++count;
	  }
	}	

	fflush(fp100); 	
	fclose(fp100);
	fflush(fp1_9); 	
	fclose(fp1_9);

	DeInitOGLES();

	DeInitTestFramework();

	if(AvsGrahpicGetStatus() == ASV_RES_OK)
	{
		printf( "Grahpic processing completed successfully, count(%d)\n", count );
	}
}

void base_runtime_test_mode_default(void)
{
	gDrawEarthCnt = 100;
}

//---------------------------------------------------------
//    Functions  
//---------------------------------------------------------
void* base_runtime_thread_callback(void* pArg)
{
	int* pargument = (int*)pArg;
	void* (*pthreadfunc)(void *) = (void* (*)(void* ))(pargument[0]);
	void* ptemp_arg = (void*)pargument[1];
	HTHREAD hthread = (HTHREAD)pargument[2];
	//VR_DBG("thread callback func start(0x%x), parg(0x%x)\n", (int)pThreadFunc, ptemp_arg);
	pthreadfunc(ptemp_arg);
	return (void*)NULL;
}

//---------------------------------------------------------
//    Functions  
//---------------------------------------------------------
HTHREAD base_runtime_thread_create(void* (*pThreadFunc)(void *), void* pArg)
{
	HTHREAD hthread = (HTHREAD)malloc(sizeof(VRBaseRuntimeThread));
	if( !hthread  ){ return 0; }
	hthread->args[0] = (int)pThreadFunc;
	hthread->args[1] = (int)pArg;
	hthread->args[2] = (int)hthread;
	VR_MSG("[thread] create start\n");
	if( pthread_create(&hthread->thread, NULL, base_runtime_thread_callback, &(hthread->args[0])) < 0 )
	{
		VR_MSG("thread create error\n");
		free(hthread); 
		return 0;
	}
	VR_MSG("[thread] create done\n");
	//VR_DBG("thread create 0x%x, 0x%x, 0x%x\n", hthread->thread, parg[1], parg[2]);
	return (HTHREAD)hthread;
}

//---------------------------------------------------------
//    Functions  
//---------------------------------------------------------
void base_runtime_thread_destroy(HTHREAD hThread)
{
	int* status;
	// 연결된 스레드의 자원을 정상적으로 반환시켜준다
	VR_MSG("[thread] join start\n");
	pthread_join(hThread->thread, (void **)&status);	
	if(0 != status)
	{
		VR_MSG("pthread_join return ERROR!(0x%x)\n", status);
	}
	VR_MSG("[thread] join done\n");
	//at api_gles_runtime_thread_create_SP, doesn't alloc memory for parg
	free(hThread);
}

///////////////////////////////////////////////////////////////////////////////
ASV_RESULT AvsGrahpicGetStatus(void)
{
	return (!gRenderTimeoutState)? gState : ASV_RES_ERR;
}

void AppSetStatus(ASV_RESULT state)
{
	gState = state;
}

///////////////////////////////////////////////////////////////////////////////
ASV_RESULT AvsGrahpicStart(void)
{
	InitVariables();

	AppSetStatus(ASV_RES_TESTING);

#if 1
	nFrames = TEST_FRAMES_MAX;
#else
	nFrames = 0;
#endif

	hNativeWnd = OS_CreateWindow(); 				  
#if 0
	OS_GetWindowSize(hNativeWnd, &view_width, &view_height);
#else
	view_width = LCD_WIDTH;
	view_height = LCD_HEIGHT;
#endif
	if(!hNativeWnd)
	{
		AppSetStatus(ASV_RES_ERR);
		return ASV_RES_ERR;
	}

	/*******************************/
	gThreadExit = 0;
	base_runtime_test_mode_default();
	hthread = base_runtime_thread_create(base_runtime_grahpic_test_thread, NULL);

	printf("AvsGrahpicStart() done\n");
	
	return ASV_RES_OK;
}

///////////////////////////////////////////////////////////////////////////////
ASV_RESULT AvsGrahpicStop()
{	
	gThreadExit = 1;
	printf("AvsGrahpicStop() start\n");

	if(hthread)
		base_runtime_thread_destroy(hthread);		

	OS_DestroyWindow(hNativeWnd);  
	hNativeWnd = NULL;

	printf("AvsGrahpicStop() done\n");

	return ASV_RES_OK;
}

///////////////////////////////////////////////////////////////////////////////
ASV_TEST_MODULE *Get3DTestModule(void)
{
/*
char name[64];					//	Module Name
ASV_RESULT (*run)(void);	//	Run Test
ASV_RESULT (*stop)(void);	//	Stop Test
ASV_RESULT (*status)(void); //	Current Status (ERR,OK,TESTING)
*/
	ASV_TEST_MODULE* gragphic_test_module = (ASV_TEST_MODULE*)malloc(sizeof(ASV_TEST_MODULE));
	if(!gragphic_test_module)	
		return NULL;

	strcpy(gragphic_test_module->name, "VR graphic");
	gragphic_test_module->run = AvsGrahpicStart;
	gragphic_test_module->stop = AvsGrahpicStop;
	gragphic_test_module->status = AvsGrahpicGetStatus;
	return gragphic_test_module;
}

///////////////////////////////////////////////////////////////////////////////
static void InitTestFramework(void)
{
	typedef struct ASV_TEST_MODULE
	{
		char name[64];					//	Module Name
		ASV_RESULT (*run)(void);	//	Run Test
		ASV_RESULT (*stop)(void);	//	Stop Test
		ASV_RESULT (*status)(void); //	Current Status (ERR,OK,TESTING)
	} ASV_TEST_MODULE;

	gResultImage = (unsigned int*)malloc(view_width * view_height * 4);
	gGoldenImage = (unsigned int*)malloc(view_width * view_height * 4);
}

static void DeInitTestFramework(void)
{
	free(gResultImage);
	gResultImage = NULL;
	free(gGoldenImage);
	gGoldenImage = NULL;
}

static int gFirstDondFlag = 0;

static GLboolean SaveFrame(unsigned int frame_count)
{
	if( (frame_count > 0 && frame_count < 10) || 
	    (!(frame_count % TEST_GOLDEN_FRAME_INTERVAL)) )
	{
		glReadPixels(0, 0, view_width, view_height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)gResultImage);	
		#if 0
		if(!gFirstDondFlag && 1 == frame_count)
		{
			char sName[128];	
			sprintf((char*)sName, "./save_read_%d_%dx%d_%d.data", frame_count, view_width, view_height, 4);
			FILE* fp = fopen(sName, "w");	
			if(!fp)
			{
				printf("can't open file\n");
			}
			
			fwrite(gResultImage, view_width * view_height * 4 , 1, fp);
			fflush(fp);	
			fclose(fp);
			gFirstDondFlag = 1;
		}
		#endif
		return GL_TRUE;
	}
	return GL_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
static GLboolean CheckResult(unsigned int frame_count, FILE* fp100, FILE* fp1_9)
{

	FILE* fp = NULL;
	if(frame_count > 0 && frame_count < 10)
	{
		fp = fp1_9;
	}
	else if(!(frame_count % TEST_GOLDEN_FRAME_INTERVAL))
	{
		fp = fp100;
	}

	if( fp )
	{	
		unsigned int bytes_size = view_width * 4 * view_height;
		//unsigned int* pdata_rst = (unsigned int*)malloc(bytes_size);
		//load file
		fread(gGoldenImage, bytes_size , 1, fp);

		#if !defined( FILE_CREATE )
		for(unsigned int j = 0 ; j < view_height ; j++)
		for(unsigned int i = 0 ; i < view_width ; i++)
		{
			#if 0
			//temp test
			gResultImage[100] = 0;			
			#endif

			#if 0
			if( frame_count && ((gResultImage[j*view_width + i] & 0xFF000000)>>24) != 0xFF)
			{
				printf(" Alpha : <i:%d, j:%d>ref(0x%08x), out(0x%08x)\n", i, j, gGoldenImage[j*view_width + i], gResultImage[j*view_width + i]);
			}
			#endif
						
			if((gGoldenImage[j*view_width + i] & 0xFFFFFFFF) != (gResultImage[j*view_width + i] & 0xFFFFFFFF))
			{
				#if 1 //temp test
				printf("======================================\n");
				printf(" <%d> CheckResult Fail!!!.\n", frame_count);
				printf(" <i:%d, j:%d>ref(0x%08x), out(0x%08x)\n", i, j, gGoldenImage[j*view_width + i], gResultImage[j*view_width + i]);
				printf("======================================\n");

				AppSetStatus(ASV_RES_ERR);
				return GL_FALSE;
				#endif
			}
		}	
		#endif
		printf(" <%d> CheckResult Success.\n", frame_count);
		return GL_TRUE;
	}
	else
	{
		printf(" <%d> CheckResult skiped.\n", frame_count);
		return GL_FALSE;
	}
}

static EGLNativePixmapType vrCreatePixmap(unsigned int uiWidth, unsigned int uiHeight, void* pData, 
								int is_video_dma_buf, unsigned int pixel_bits)
{	
	fbdev_pixmap* pPixmap = (fbdev_pixmap *)CALLOC(1, sizeof(fbdev_pixmap));
	if(pPixmap == NULL)
	{
		printf("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}
	
	pPixmap->width = uiWidth;
	pPixmap->height = uiHeight;
	if(32 == pixel_bits)
	{
		pPixmap->bytes_per_pixel = 4;
		pPixmap->buffer_size = 32;
		pPixmap->red_size = 8;
		pPixmap->green_size = 8;
		pPixmap->blue_size = 8;
		pPixmap->alpha_size = 8;
		pPixmap->luminance_size = 0;
	}
	else if(16 == pixel_bits)
	{
		pPixmap->bytes_per_pixel = 2;
		pPixmap->buffer_size = 16;
		pPixmap->red_size = 0;
		pPixmap->green_size = 0;
		pPixmap->blue_size = 0;
		pPixmap->alpha_size = 8;
		pPixmap->luminance_size = 8;
	}		
	else if(8 == pixel_bits)
	{
		pPixmap->bytes_per_pixel = 1;
		pPixmap->buffer_size = 8;
		pPixmap->red_size = 0;
		pPixmap->green_size = 0;
		pPixmap->blue_size = 0;
		pPixmap->alpha_size = 0;
		pPixmap->luminance_size = 8;
	}	
	else
	{
		VR_ERR("Error: pixel_bits(%d) is not valid. %s:%i\n", pixel_bits, __FILE__, __LINE__);
		free(pPixmap);
		return NULL;
	}

	int fd_handle;
	if(is_video_dma_buf)
	{
		fd_handle = (int)(((NX_MEMORY_HANDLE)pData)->privateDesc);
	}
	else
	{
		fd_handle = (int)pData;
	}
	
	pPixmap->flags = (fbdev_pixmap_flags)FBDEV_PIXMAP_COLORSPACE_sRGB;
	pPixmap->flags = (fbdev_pixmap_flags)(pPixmap->flags |FBDEV_PIXMAP_DMA_BUF);
	pPixmap->data = (unsigned short *)calloc(1, sizeof(int));
	if(pPixmap->data == NULL)
	{
		VR_ERR("Error: NULL memory at %s:%i\n", __FILE__, __LINE__);
		free(pPixmap);
		return NULL;
	}
	*((int*)pPixmap->data) = (int)fd_handle;
	return pPixmap;
}

void vrDestroyPixmap(EGLNativePixmapType pPixmap)
{
	if(((fbdev_pixmap*)pPixmap)->data)
		FREE(((fbdev_pixmap*)pPixmap)->data);
	if(pPixmap)
		FREE(pPixmap);
}

static NX_MEMORY_HANDLE mem_alloc(unsigned int size, unsigned int align)
{	
	NX_MEMORY_HANDLE handle = NX_AllocateMemory(size, align);
	if(!handle)
	{	
		fprintf(stderr, "Alloc Err! %s:%i\n", __FILE__, __LINE__);
		return NULL;
	}
	memset((void*)handle->virAddr, 0x3C, size);
	//printf("mem_alloc, fd(0x%x, 0x%x, 0x%x)\n", (int)handle, (int)handle->privateDesc, handle->virAddr);
	return handle;
}

static void mem_free(NX_MEMORY_HANDLE handle)
{
	NX_FreeMemory((NX_MEMORY_HANDLE)handle);
}

///////////////////////////////////////////////////////////////////////////////
int vrInitEglExtensions(void)
{
	_eglCreateImageKHR = (PFNEGLCREATEIMAGEKHRPROC) eglGetProcAddress("eglCreateImageKHR");
	if ( NULL == _eglCreateImageKHR ) return -1;

	_eglDestroyImageKHR = (PFNEGLDESTROYIMAGEKHRPROC) eglGetProcAddress("eglDestroyImageKHR");
	if ( NULL == _eglDestroyImageKHR ) return -1;

	_glEGLImageTargetTexture2DOES = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC) eglGetProcAddress("glEGLImageTargetTexture2DOES");
	if ( NULL == _glEGLImageTargetTexture2DOES ) return -1;

	_glEGLImageTargetRenderbufferStorageOES = (PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC) eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
	if ( NULL == _glEGLImageTargetRenderbufferStorageOES ) return -1;

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
static GLboolean AppeglxCreate (GLint width, GLint height, GLint mode) 
{
	EGLNativeWindowType win;
	EGLint major, minor, num_config, max_num_config;
	int i;	
    EGLint attrib_list_es1[] = { EGL_RED_SIZE, 8,  EGL_GREEN_SIZE, 8,  EGL_BLUE_SIZE, 8, 
			     EGL_ALPHA_SIZE, 8,  EGL_RENDERABLE_TYPE,  EGL_OPENGL_ES_BIT, 
			     EGL_SURFACE_TYPE,  EGL_PIXMAP_BIT,  EGL_NONE };
    EGLint attrib_list_es2[] = { EGL_RED_SIZE, 8,  EGL_GREEN_SIZE, 8,  EGL_BLUE_SIZE, 8, 
			     EGL_ALPHA_SIZE, 8,  EGL_RENDERABLE_TYPE,  /*EGL_OPENGL_ES_BIT |*/ EGL_OPENGL_ES2_BIT, 
			     EGL_SURFACE_TYPE,  EGL_WINDOW_BIT,  EGL_NONE };
	EGLint context_attrib_list_es1[] = {EGL_CONTEXT_CLIENT_VERSION, 1, 
									EGL_NONE};
	EGLint context_attrib_list_es2[] = {EGL_CONTEXT_CLIENT_VERSION, 2, 
									EGL_NONE};
	glesDisplay = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    if ( EGL_NO_DISPLAY == glesDisplay )
    {
		printf( "eglGetDisplay() failed (error 0x%x)\n", eglGetError() );
		return GL_FALSE;
    }
    if ( EGL_FALSE == eglInitialize( glesDisplay, &major, &minor ) )
    {
		printf( "eglInitialize() failed (error 0x%x)\n", eglGetError() );
		return GL_FALSE;
    }

	/* step2 - find the number of configs */
 	if ( EGL_FALSE == eglGetConfigs(glesDisplay, NULL, 0, &max_num_config) )
	{
		printf( "eglGetConfigs() failed to retrive the number of configs (error 0x%x)\n", eglGetError() );
		return GL_FALSE;
	}

	if(max_num_config <= 0)
	{
		printf( "No EGLconfigs found\n" );
		return GL_FALSE;
	}

	vrInitEglExtensions();

	/* step5 - create a context */
	//es1 context
	#if 1
	{
		EGLConfig config;
		EGLConfig *configs = (EGLConfig *)malloc( sizeof( EGLConfig) * max_num_config );
		if ( NULL == configs )
		{
			printf( "Out of memory\n" );
			return GL_FALSE;
		}
		
		/* eglBindAPI( EGL_OPENGL_ES_API ); */
		/* step3 - find a suitable config */
		if ( EGL_FALSE == eglChooseConfig( glesDisplay, attrib_list_es1, configs, max_num_config, &num_config ) )
		{
			printf( "eglChooseConfig() failed (error 0x%x)\n", eglGetError() );
			return GL_FALSE;
		}
		if ( 0 == num_config )
		{
			printf( "eglChooseConfig() was unable to find a suitable config\n" );
			return GL_FALSE;
		}
		
		for ( i=0; i<num_config; i++ )
		{
			EGLint value;
			
			/*Use this to explicitly check that the EGL config has the expected color depths */
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_RED_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_GREEN_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_BLUE_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_ALPHA_SIZE, &value );
			if ( 8 != value ) continue; 
			config = configs[i];
			break;
		}	 
		free(configs);
	
		hPixmap = mem_alloc(texture_in_width * texture_in_height * 4, 4096);
		pixmap_input = (fbdev_pixmap*)vrCreatePixmap(texture_in_width, texture_in_height, hPixmap, EGL_TRUE, 32);
		if(pixmap_input == NULL || ((fbdev_pixmap*)pixmap_input)->data == NULL)
		{
			printf("Error: Out of memory at %s:%i\n", __FILE__, __LINE__);
			return GL_FALSE;
		}

		glesSurfaceEs1 = eglCreatePixmapSurface( glesDisplay, config, (EGLNativePixmapType)pixmap_input, NULL );
		if ( EGL_NO_SURFACE == glesSurfaceEs1 )
		{
			printf( "eglCreatePixmapSurface failed (error 0x%x\n", eglGetError() );
			return GL_FALSE;
		}
		
		//es1 context
	    glesContextEs1 = eglCreateContext( glesDisplay, config, EGL_NO_CONTEXT, context_attrib_list_es1 );
	    if ( EGL_NO_CONTEXT == glesContextEs1 )
	    {
			printf( "eglCreateContext failed (error 0x%x)\n", eglGetError() );
			return GL_FALSE;
	    }
    }
	#endif

	//es2 context
	#if 1
	{	
		EGLConfig config;	
		EGLConfig *configs = (EGLConfig *)malloc( sizeof( EGLConfig) * max_num_config );
		if ( NULL == configs )
		{
			printf( "Out of memory\n" );
			return GL_FALSE;
		}

	    /* eglBindAPI( EGL_OPENGL_ES_API ); */
	 	/* step3 - find a suitable config */
	    if ( EGL_FALSE == eglChooseConfig( glesDisplay, attrib_list_es2, configs, max_num_config, &num_config ) )
	    {
			printf( "eglChooseConfig() failed (error 0x%x)\n", eglGetError() );
			return GL_FALSE;
	    }
	    if ( 0 == num_config )
	    {
			printf( "eglChooseConfig() was unable to find a suitable config\n" );
			return GL_FALSE;
	    }

		for ( i=0; i<num_config; i++ )
		{
			EGLint value;
			
			/*Use this to explicitly check that the EGL config has the expected color depths */
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_RED_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_GREEN_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_BLUE_SIZE, &value );
			if ( 8 != value ) continue;
			eglGetConfigAttrib( glesDisplay, configs[i], EGL_ALPHA_SIZE, &value );
			if ( 8 != value ) continue;	
			config = configs[i];
			break;
		}    
	    free(configs);

		/* step4 - create a window surface (512x512 pixels) */
		win = (EGLNativeWindowType)hNativeWnd;
		glesSurfaceEs2 = eglCreateWindowSurface( glesDisplay, config, win, NULL );
		if ( EGL_NO_SURFACE == glesSurfaceEs2 )
		{
			printf( "eglCreateWindowSurface failed (error 0x%x\n", eglGetError() );
			return GL_FALSE;
		}
		
		//es1 context
	    glesContextEs2 = eglCreateContext( glesDisplay, config, EGL_NO_CONTEXT, context_attrib_list_es2 );
	    if ( EGL_NO_CONTEXT == glesContextEs2 )
	    {
			printf( "eglCreateContext failed (error 0x%x)\n", eglGetError() );
			return GL_FALSE;
	    }

		EGLint imageAttributes[] = {
			EGL_IMAGE_PRESERVED_KHR, /*EGL_TRUE*/EGL_FALSE, 
			EGL_NONE
		};	
		eglImage = EGL_CHECK(_eglCreateImageKHR( glesDisplay, EGL_NO_CONTEXT, 
										   EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)pixmap_input, imageAttributes)); 	    
	}
    #endif
	return GL_TRUE;
}

///////////////////////////////////////////////////////////////////////////////
static void AppeglxDelete() {
	//glDeleteBuffers(3, nBufferName980);
	glDeleteBuffers(3, nBufferName9680);
	
	eglMakeCurrent (glesDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);	
	eglDestroyContext	(glesDisplay, glesContextEs1);
	eglDestroyContext	(glesDisplay, glesContextEs2);
	
	eglDestroySurface	(glesDisplay, glesSurfaceEs1);	
	_eglDestroyImageKHR(glesDisplay, eglImage);
	vrDestroyPixmap(pixmap_input); 
	pixmap_input = NULL;
	mem_free(hPixmap);
	hPixmap = NULL;
	eglDestroySurface	(glesDisplay, glesSurfaceEs2);

	eglTerminate (glesDisplay);
}

///////////////////////////////////////////////////////////////////////////////
static void AppeglxFlush() {	
	//SetOGLESCurrent_es1();
	//eglSwapBuffers(glesDisplay, glesSurfaceEs1);
	SetOGLESCurrent_es2();
	eglSwapBuffers(glesDisplay, glesSurfaceEs2);

	//temp test
	//getchar();
}

static unsigned int gRenderFrameCount;
static float frames_per_sec = 0.f;
static float poly_per_sec = 0.f;
static float pixels_per_sec = 0.f;

///////////////////////////////////////////////////////////////////////////////
static void Render(void)
{
	char text_fps[32];	
	char text_poly_cnt[32];	
	char text_polyps[32];	
	char text_object_cnt[32];	
	float font_scale_x = 1.2f;
	float font_scale_y = 1.5f;
	unsigned int test_start_time, test_end_time;
	test_start_time = (unsigned int)OS_GetTickCount();	
	gRenderTimeoutState = 0;

	//printf("GL_Draw start\n");

	GL_Draw();
		
	AppeglxFlush();

	test_end_time = (unsigned int)OS_GetTickCount();	 
	if(test_end_time >  test_start_time)
	{
		if((test_end_time - test_start_time) < 2000000)
		{
			gRenderTimeoutState = 0;//Success			
			//printf("GL_Draw end0(%d)\n", (test_end_time - test_start_time));
		}
		else
		{
			gRenderTimeoutState = 1;//Timeout			
		}
	}
	else
	{
		if((0xFFFFFFFFUL - test_start_time + test_end_time) < 2000000)
		{
			gRenderTimeoutState = 0;//Success
			//printf("GL_Draw end1(%d)\n", (0xFFFFFFFFUL - test_start_time + test_end_time));
		}
		else
		{
			gRenderTimeoutState = 1;//Timeout			
		}
	}
	
 }

static void SetPosition(int* x_offset, int* y_offset, int* z_offset, unsigned int drawEarthCnt)
{
	int offset_unit;
	float size_offset;
	size_offset = (drawEarthCnt / 14) * 1.5;
	drawEarthCnt = drawEarthCnt % 14;
	offset_unit = ((X1_0 * 1.5) + (X1_0 * size_offset));
	switch(drawEarthCnt)
	{
		case 0 : *x_offset = -offset_unit, *y_offset = offset_unit, *z_offset = offset_unit;
			break;
		case 1 : *x_offset = offset_unit, *y_offset = -offset_unit, *z_offset = -offset_unit;
			break;
		case 2 : *x_offset = offset_unit, *y_offset = -offset_unit, *z_offset = offset_unit;
			break;
		case 3 : *x_offset = -offset_unit, *y_offset = offset_unit, *z_offset = -offset_unit;
			break;
		case 4 : *x_offset = offset_unit, *y_offset = offset_unit, *z_offset = offset_unit;
			break;
		case 5 : *x_offset = -offset_unit, *y_offset = -offset_unit, *z_offset = -offset_unit;
			break;
		case 6 : *x_offset = -offset_unit, *y_offset = -offset_unit, *z_offset = offset_unit;
			break;
		case 7 : *x_offset = offset_unit, *y_offset = offset_unit, *z_offset = -offset_unit;
			break;

		case 8 : *x_offset = 0, *y_offset = 0, *z_offset = offset_unit*2;
			break;
		case 9 : *x_offset = 0, *y_offset = 0, *z_offset = -offset_unit*2;
			break;
		case 10 : *x_offset = 0, *y_offset = offset_unit*2, *z_offset = 0;
			break;
		case 11 : *x_offset = 0, *y_offset = -offset_unit*2, *z_offset = 0;
			break;
		case 12 : *x_offset = offset_unit*2, *y_offset = 0, *z_offset = 0;
			break;
		case 13 : *x_offset = -offset_unit*2, *y_offset = 0, *z_offset = 0;
			break;
		default:
			break;
	}
}


#define MALI_PI					3.14159265358979323
#define MALI_DEGREES_TO_RADIANS	(MALI_PI / 180.0f)
#define MALI_RADIANS_TO_DEGREES	(180.0f / MALI_PI)
#define MALI_SW_EPSILON 1e-10

typedef float mali_float;

typedef struct
{
	mali_float x, y, z;
} mali_float_vector3;

typedef struct
{
	mali_float x, y, z, w;
} mali_float_vector4;

typedef mali_float mali_float_matrix4x4[4][4];

/** a wrapper-type for mali_float / mali_fixed */
typedef mali_float mali_ftype;

/** a wrapper-type for mali_float_vector3 / mali_fixed_vector3 */
typedef mali_float_vector3 mali_vector3;

/** a wrapper-type for mali_float_vector4 / mali_fixed_vector4 */
typedef mali_float_vector4 mali_vector4;

/** a wrapper-type for mali_float_matrix4x4 / mali_fixed_matrix4x4 */
typedef mali_float_matrix4x4 mali_matrix4x4;

void api_matrix4_set_identity(mali_matrix4x4* mat)
{
	memset(mat, 0, sizeof(float)*16);
	(*mat)[0][0] = 1.f;
	(*mat)[1][1] = 1.f;
	(*mat)[2][2] = 1.f;
	(*mat)[3][3] = 1.f;
}

int _api_matrix4_is_identity(mali_matrix4x4* mat)
{
	if((1.f == (*mat)[0][0]) && ((*mat)[0][0] == (*mat)[1][1]) && ((*mat)[1][1] == (*mat)[2][2]) && ((*mat)[2][2] == (*mat)[3][3]) )
	{
		if( (       0.f == (*mat)[0][1]) && ((*mat)[0][1] == (*mat)[0][2]) && ((*mat)[0][2] == (*mat)[0][3]) && 
		    ((*mat)[0][3] == (*mat)[1][0]) && ((*mat)[1][0] == (*mat)[1][2]) && ((*mat)[1][2] == (*mat)[1][3]) && 
		    ((*mat)[1][3] == (*mat)[2][0]) && ((*mat)[2][0] == (*mat)[2][1]) && ((*mat)[2][1] == (*mat)[2][3]) && 
		    ((*mat)[2][3] == (*mat)[3][0]) && ((*mat)[3][0] == (*mat)[3][1]) && ((*mat)[3][1] == (*mat)[3][2]) )		
		{
			return 1;
		}		
	}
	return 0;
}

mali_ftype __mali_vector3_dot3(mali_vector3 v0, mali_vector3 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
mali_vector3 __mali_vector3_scale(mali_vector3 v, mali_float scalar)
{
	mali_float_vector3 temp;
	temp.x = v.x * scalar;
	temp.y = v.y * scalar;
	temp.z = v.z * scalar;
	return temp;
}

void _gles1_rotate(float angle, float x, float y, float z, mali_matrix4x4* mat)
{
	mali_vector3 axis;
	mali_float len2;
	int is_identity;

	is_identity = _api_matrix4_is_identity(mat);

	angle *= (mali_float) MALI_DEGREES_TO_RADIANS;

	/*
	 * normalize rotation axis if necessary
	 */

	axis.x = x;
	axis.y = y;
	axis.z = z;

	len2 = __mali_vector3_dot3(axis, axis);

	/* len2 must be zero or positive - hence no need for fabs() */

	if (len2 < (1.0f - MALI_SW_EPSILON) ||
	        len2 > (1.0f + MALI_SW_EPSILON))
	{
		mali_float len = sqrt(len2);

		/* ensure len >= MALI_SW_EPSILON */

		if (len < MALI_SW_EPSILON)
		{
			len = (mali_float)MALI_SW_EPSILON;
		}

		axis = __mali_vector3_scale(axis, 1.0f / len);
	}

	/*
	 * apply rotation to current matrix
	 */
	{
		/* precompute sine/cosine of angle since it will be used several times */

		const mali_float c = cos(angle);
		const mali_float s = sin(angle);
		const mali_float ci = 1.0f - c;

		/* precompute subexpressions */

		const mali_float xs =   axis.x * s;
		const mali_float ys =   axis.y * s;
		const mali_float zs =   axis.z * s;

		const mali_float yci =  axis.y * ci;
		const mali_float zci =  axis.z * ci;

		const mali_float xyci = axis.x * yci;
		const mali_float xzci = axis.x * zci;
		const mali_float yzci = axis.y * zci;

		/* a rotation matrix */
		const mali_matrix4x4 rot_mat = {   {axis.x *axis.x *ci + c,   xyci + zs, xzci - ys,  0},
			{xyci - zs, axis.y *yci + c,  yzci + xs,  0},
			{xzci + ys, yzci - xs, axis.z *zci + c,  0},
			{0,  0, 0, 1}
		};

		if (is_identity == 1)
		{
			(*mat)[0][0] = rot_mat[0][0];
			(*mat)[0][1] = rot_mat[0][1];
			(*mat)[0][2] = rot_mat[0][2];
			(*mat)[1][0] = rot_mat[1][0];
			(*mat)[1][1] = rot_mat[1][1];
			(*mat)[1][2] = rot_mat[1][2];
			(*mat)[2][0] = rot_mat[2][0];
			(*mat)[2][1] = rot_mat[2][1];
			(*mat)[2][2] = rot_mat[2][2];
		}
		else
		{
			/* optimised matrix multiply: no need to unroll since
			 * branches are predictable and smaller code is kinder to
			 * I-cache
			 */
			int i;

			for (i = 0; i < 4; ++i)
			{
				const mali_float m[3] = { (*mat)[0][i], (*mat)[1][i], (*mat)[2][i] };
				(*mat)[0][i] = m[0] * rot_mat[0][0] + m[1] * rot_mat[0][1] + m[2] * rot_mat[0][2];
				(*mat)[1][i] = m[0] * rot_mat[1][0] + m[1] * rot_mat[1][1] + m[2] * rot_mat[1][2];
				(*mat)[2][i] = m[0] * rot_mat[2][0] + m[1] * rot_mat[2][1] + m[2] * rot_mat[2][2];
			}
		}
	}
}

void _gles1_translate(float x, float y, float z, mali_matrix4x4* mat)
{
	int is_identity;

	is_identity = _api_matrix4_is_identity(mat);

	if (is_identity == 1)
	{
		(*mat)[3][0] = x;
		(*mat)[3][1] = y;
		(*mat)[3][2] = z;
	}
	else
	{
		(*mat)[3][0] = (*mat)[0][0] * x + (*mat)[1][0] * y + (*mat)[2][0] * z + (*mat)[3][0];
		(*mat)[3][1] = (*mat)[0][1] * x + (*mat)[1][1] * y + (*mat)[2][1] * z + (*mat)[3][1];
		(*mat)[3][2] = (*mat)[0][2] * x + (*mat)[1][2] * y + (*mat)[2][2] * z + (*mat)[3][2];
		(*mat)[3][3] = (*mat)[0][3] * x + (*mat)[1][3] * y + (*mat)[2][3] * z + (*mat)[3][3];
	}
}

void _gles1_scale(float x, float y, float z, mali_matrix4x4* mat)
{
	(*mat)[0][0] *= x;
	(*mat)[1][0] *= y;
	(*mat)[2][0] *= z;

	(*mat)[0][1] *= x;
	(*mat)[1][1] *= y;
	(*mat)[2][1] *= z;

	(*mat)[0][2] *= x;
	(*mat)[1][2] *= y;
	(*mat)[2][2] *= z;

	(*mat)[0][3] *= x;
	(*mat)[1][3] *= y;
	(*mat)[2][3] *= z;
}

void __mali_float_matrix4x4_make_frustum(mali_float_matrix4x4 dst, float left, float right, float bottom, float top, float near, float far)
{
	float wr = 1.f / (right - left); /* reciprocal width */
	float hr = 1.f / (top - bottom); /* reciprocal height */
	float lr = 1.f / (far - near);   /* reciprocal length */

	dst[0][0] = 2.f * near * wr;
	dst[0][1] = 0.f;
	dst[0][2] = 0.f;
	dst[0][3] =  0.f;
	dst[1][0] = 0.f;
	dst[1][1] = 2.f * near * hr;
	dst[1][2] = 0.f;
	dst[1][3] =  0.f;
	dst[2][0] = (right + left) * wr;
	dst[2][1] = (top + bottom) * hr;
	dst[2][2] = -(far + near) * lr;
	dst[2][3] = -1.f;
	dst[3][0] = 0.f;
	dst[3][1] = 0.f;
	dst[3][2] = -(2.f * far * near) * lr;
	dst[3][3] =  0.f;
}

void __mali_float_matrix4x4_make_perspective(mali_float_matrix4x4 dst, GLfloat fov, GLfloat aspect, GLfloat near_val, GLfloat far_val)
{
	GLfloat top = (GLfloat)(tan(fov*0.5) * near_val);
	GLfloat bottom = -top;
	GLfloat left = aspect * bottom;
	GLfloat right = aspect * top;

	__mali_float_matrix4x4_make_frustum(dst, left, right, bottom, top, near_val, far_val);
}

///////////////////////////////////////////////////////////////////////////////

static void InitOGLES_es1(void)
{	
	//glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	GL_CHECK(glClearColor(0.3f, 0.7f, 0.4f, 1.0f));
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);		
	glShadeModel(GL_SMOOTH);  
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glClearDepthf(1.f); 
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	
	glViewport(0, 0, texture_in_width, texture_in_height);

	// Set Projection 
	glMatrixMode(GL_PROJECTION);			
	glLoadIdentity();
	glPerspectivef( 3.141592654f/4.0f, 1.0f, 1.0f, 1000.0f );	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	
	GL_CHECK(glGenTextures(1, &gTexID1)); 		
	GL_CHECK(glActiveTexture(GL_TEXTURE0));
	glBindTexture(GL_TEXTURE_2D , gTexID1 ); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	glTexImage2D(GL_TEXTURE_2D, model.tl, model.ti, ( GLsizei )model.tw, ( GLsizei )model.th, 0, model.tf, model.tt, &DATA_globe2_0[0]);

	/*************Create VBO********************/		
	//9680 Sphere
	glGenBuffers(3,&nBufferName9680[0]);
	pdata_texcoord_pv_f32 = (float*)malloc(15132 * sizeof(int));
	for(int i = 0 ; i < 15132 ; i++)
	{
		pdata_texcoord_pv_f32[i] = (float)sphere_9680pv[i] / (float)(1<<16);
	}
	glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[VERTEX_BUFFER]);
#ifdef TEST_ES1_FLOAT_EN
	glBufferData(GL_ARRAY_BUFFER,15132*sizeof(GLfloat),pdata_texcoord_pv_f32,GL_STATIC_DRAW);
#else
   glBufferData(GL_ARRAY_BUFFER,15132*sizeof(GLfixed),sphere_9680pv,GL_STATIC_DRAW);
#endif

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,nBufferName9680[INDEX_BUFFER]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,29040*sizeof(GLshort),sphere_9680pf,GL_STATIC_DRAW);
	pdata_vertex_pc_f32 = (float*)malloc(10088 * sizeof(int));
	for(int i = 0 ; i < 10088 ; i++)
	{
		pdata_vertex_pc_f32[i] = (float)sphere_9680pc[i] / (float)(1<<16);
	}
	glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[TEXTURECOORD_BUFFER]);
#ifdef TEST_ES1_FLOAT_EN
	glBufferData(GL_ARRAY_BUFFER,10088*sizeof(GLfloat),pdata_vertex_pc_f32,GL_STATIC_DRAW);
#else
	glBufferData(GL_ARRAY_BUFFER,10088*sizeof(GLfixed),sphere_9680pc,GL_STATIC_DRAW);
#endif
	
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
}
	
static void InitOGLES_es2(void)
{
#if 0
	const char test_vertex_shader_A[] = {
	"	attribute vec4 a_position; \n\
		attribute vec4 a_color; \n\
		varying vec4 v_v4FillColor; \n\
									\n\
		void main() \n\
		{				\n\
			v_v4FillColor = a_color;	\n\
			gl_Position = a_position;		\n\
		}	\n\
	"
	};
	
	const char test_fragment_shader_A[] = {
	"	precision mediump float; \n\
		varying vec4 v_v4FillColor; \n\
		void main() \n\
		{	\n\
			gl_FragColor = v_v4FillColor;	\n\
		}	\n\
	"
	};
	const char* pstrings;
	int err;
			
	// Create shader and load into GL.
	vert_name = glCreateShader(GL_VERTEX_SHADER);
	pstrings = test_vertex_shader_A;
	glShaderSource(vert_name, 1, &pstrings, NULL);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	
	frag_name = glCreateShader(GL_FRAGMENT_SHADER);
	pstrings = test_fragment_shader_A;
	glShaderSource(frag_name, 1, &pstrings, NULL);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }

	// Try compiling the shader. 
	glCompileShader(vert_name);
	glGetShaderiv(vert_name, GL_COMPILE_STATUS, &test_status);
	printf("vert: test_status(0x%x)\n", test_status);		
	glCompileShader(frag_name);
	glGetShaderiv(frag_name, GL_COMPILE_STATUS, &test_status);
	printf("frag: test_status(0x%x)\n", test_status);		

	/* Set up shaders. */
	prog_name = glCreateProgram();
	glAttachShader(prog_name, vert_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glAttachShader(prog_name, frag_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glLinkProgram(prog_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glUseProgram(prog_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }

	/* Vertex positions. */
	loc_position = glGetAttribLocation(prog_name, "a_position");
	if(loc_position == -1)
	{		
		printf("Error(%d): Attribute loc_position not found\n", __LINE__);
	}
	else
	{
		glEnableVertexAttribArray(loc_position);
	}
	
	/* loc_fill_color. */
	loc_fill_color = glGetAttribLocation(prog_name, "a_color");
	if(loc_fill_color == -1)
	{		
		printf("Error(%d): Attribute loc_fill_color not found\n", __LINE__);
	}
	else
	{
		glEnableVertexAttribArray(loc_fill_color);
	}
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }

#else

	const char test_vertex_shader_A[] = {
	"	precision mediump float; \n\
		attribute vec4 a_position; \n\
		attribute vec2 a_v2TexCoord;	\n\
		//uniform mat4 u_mvp_mat; \n\
		varying vec2 v_v2TexCoord;	\n\
		void main()	\n\
		{			\n\
			//vec4 pos; \n\
			//pos.x = a_position.x; \n\
			//pos.y = a_position.y; \n\
			//pos.z = a_position.z; \n\
			//pos.w = 1.0; \n\
			v_v2TexCoord = a_v2TexCoord;	\n\
			gl_Position = a_position;		\n\
		}	\n\
	"
	};

	const char test_fragment_shader_A[] = {
	"	precision mediump float; \n\
		uniform sampler2D u_s2dTexture; \n\
		varying vec2 v_v2TexCoord;	\n\
		void main()	\n\
		{	\n\
			vec4 tval0 = vec4(0.0, 0.0, 0.0, 0.0);	\n\
			vec2 tex0 = vec2(0.0, 1.0/1080.0);	\n\
			vec2 tex1 = vec2(-1.0/1920.0, 0.0);	\n\
			vec2 tex2 = vec2(0.0, 0.0);	\n\
			vec2 tex3 = vec2(1.0/1920.0, 0.0);	\n\
			vec2 tex4 = vec2(0.0, -1.0/1080.0);	\n\
			//gl_FragColor = texture2D(u_s2dTexture, v_v2TexCoord);	\n\
			tval0 += texture2D(u_s2dTexture, v_v2TexCoord + tex0) * (-1.0/8.0);						\n\
			tval0 += texture2D(u_s2dTexture, v_v2TexCoord + tex1) * ( 4.0/8.0);						\n\
			tval0 += texture2D(u_s2dTexture, v_v2TexCoord + tex2) * ( 2.0/8.0);						\n\
			tval0 += texture2D(u_s2dTexture, v_v2TexCoord + tex3) * ( 4.0/8.0);						\n\
			tval0 += texture2D(u_s2dTexture, v_v2TexCoord + tex4) * (-1.0/8.0);						\n\
			gl_FragColor = tval0;													\n\
		}	\n\
	"
	};
	const char* pstrings;
	int err;
			
	// Create shader and load into GL.
	vert_name = glCreateShader(GL_VERTEX_SHADER);
	pstrings = test_vertex_shader_A;
	glShaderSource(vert_name, 1, &pstrings, NULL);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	
	frag_name = glCreateShader(GL_FRAGMENT_SHADER);
	pstrings = test_fragment_shader_A;
	glShaderSource(frag_name, 1, &pstrings, NULL);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }

	// Try compiling the shader. 
	glCompileShader(vert_name);
	glGetShaderiv(vert_name, GL_COMPILE_STATUS, &test_status);
	printf("vert: test_status(0x%x)\n", test_status);		
	glCompileShader(frag_name);
	glGetShaderiv(frag_name, GL_COMPILE_STATUS, &test_status);
	printf("frag: test_status(0x%x)\n", test_status);		

	/* Set up shaders. */
	prog_name = glCreateProgram();
	glAttachShader(prog_name, vert_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glAttachShader(prog_name, frag_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glLinkProgram(prog_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	glUseProgram(prog_name);
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }

	/* Vertex positions. */
	loc_position = glGetAttribLocation(prog_name, "a_position");
	if(loc_position == -1)
	{		
		printf("Error(%d): Attribute loc_position not found\n", __LINE__);
	}
	else
	{
		glEnableVertexAttribArray(loc_position);
	}
	
	/* Tex coords. */
	loc_tex_coord = glGetAttribLocation(prog_name, "a_v2TexCoord");
	if(loc_tex_coord == -1)
	{		
		printf("Error(%d): Attribute loc_tex_coord not found\n", __LINE__);
	}
	else
	{
		glEnableVertexAttribArray(loc_tex_coord);
	}
	/* Sampler. */
	loc_uni_sampler = glGetUniformLocation(prog_name, "u_s2dTexture");
	if(loc_uni_sampler == -1)
	{
		printf("Error(%d): Uniform loc_uni_sampler not found\n", __LINE__);
	}

	#if 0
	/* matrix. */
	loc_uni_mvp = glGetUniformLocation(prog_name, "u_mvp_mat");
	if(loc_uni_mvp == -1)
	{
		printf("Error(%d): Uniform loc_uni_mvp not found\n", __LINE__);
	}	
	#endif

	#if 1	
	GL_CHECK(glGenTextures(1, &pixmapTextureName));
	GL_CHECK(glActiveTexture(GL_TEXTURE1));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, pixmapTextureName));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL_CHECK(_glEGLImageTargetTexture2DOES( GL_TEXTURE_2D, (GLeglImageOES)eglImage));				
	#else //temp test
	glGenTextures(1, &gTexID1); 	
	glBindTexture(GL_TEXTURE_2D , gTexID1 ); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	
	glTexImage2D(GL_TEXTURE_2D, model.tl, model.ti, ( GLsizei )model.tw, ( GLsizei )model.th, 0, model.tf, model.tt, &DATA_globe2_0[0]);
	#endif
	
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
#endif
}

static void SetOGLESCurrent_es1(void)
{
 	/* es1 - make the context and surface current */
    if ( EGL_FALSE == eglMakeCurrent( glesDisplay, glesSurfaceEs1, glesSurfaceEs1, glesContextEs1 ) )
    {
		printf( "ES1 eglMakeCurrent failed (error 0x%x)\n", eglGetError() );
		return;
    }	
}

static void SetOGLESCurrent_es2(void)
{
 	/* es1 - make the context and surface current */
    if ( EGL_FALSE == eglMakeCurrent( glesDisplay, glesSurfaceEs2, glesSurfaceEs2, glesContextEs2 ) )
    {
		printf( "ES2 eglMakeCurrent failed (error 0x%x)\n", eglGetError() );
		return;
    }	
}

static void InitOGLES(void)
{
	InitTestFramework();

	AppeglxCreate(view_width, view_height, 0);


	#ifdef TEST_ES1_EN
	SetOGLESCurrent_es1();
	InitOGLES_es1();
	#endif
	
	#ifdef TEST_ES2_EN
	SetOGLESCurrent_es2();
	InitOGLES_es2();
	#endif
}

static void DeInitOGLES(void)
{
	#ifdef TEST_ES1_EN
	SetOGLESCurrent_es1();
	glDeleteTextures (1, &gTexID1);
	if(m_pGLFont)
		SAFE_DELETE(m_pGLFont);
	#endif
	
	#ifdef TEST_ES2_EN
	SetOGLESCurrent_es2();
	glDeleteShader(vert_name);
	glDeleteShader(frag_name);
	glDeleteProgram(prog_name);
	#endif
	if(pdata_vertex_pc_f32) free(pdata_vertex_pc_f32);
	if(pdata_texcoord_pv_f32) free(pdata_texcoord_pv_f32);
	pdata_vertex_pc_f32 = NULL;
	pdata_texcoord_pv_f32 = NULL;
	AppeglxDelete();	
}


///////////////////////////////////////////////////////////////////////////////
//******* Main Render Loop *******************/
static void GL_Draw_es1(void)
{
	static float r=1.0f;
	static float g=1.0f;
	static float b=1.0f;

	GL_CHECK(glActiveTexture(GL_TEXTURE0));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, gTexID1));

	glColor4f( r,g,b,0.7f );
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear The Screen And The Depth Buffer
	
	glPushMatrix();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D , gTexID1 ); 

	{	//9860*1
		//VBO Bind
		glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[VERTEX_BUFFER]);//Vertex buffer
#ifdef TEST_ES1_FLOAT_EN		
		glVertexPointer(3, GL_FLOAT, 0, 0);//vertex buffer Disable
#else
		glVertexPointer(3, GL_FIXED, 0, 0);//vertex buffer Disable
#endif
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,nBufferName9680[INDEX_BUFFER]);//Index buffer

		glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[TEXTURECOORD_BUFFER]);//Texture coord buffer
#ifdef TEST_ES1_FLOAT_EN				
		glTexCoordPointer(2, GL_FLOAT, 0, 0);//Texture coord Pointer Disable
#else
		glTexCoordPointer(2, GL_FIXED, 0, 0);//Texture coord Pointer Disable
#endif
		//VBO Bind
		
		#ifdef TEST_ES1_FLOAT_EN		
		glGetFloatv(GL_MODELVIEW_MATRIX, (float*)mat16);
		_gles1_translate(X0_0,X0_9,g_depth/2.f, &mat16);
		//_gles1_scale(2.f,2.f,2.f, &mat16);
		_gles1_rotate(yrot,X0_0,X1_0,X0_0, &mat16);
		glLoadMatrixf((float*)mat16);
		#else
		glTranslatex(X0_0,X0_0,g_depth/2);
		glRotatex(yrot,X0_0,X1_0,X0_0);
		glTranslatex(X0_0,X0_9,0);
		#endif
		glDrawElements(/*drawMode*/GL_LINES, 9680 * 3.f, GL_UNSIGNED_SHORT, 0); 
	}
	glPopMatrix();
	
	if((err = glGetError()) != GL_NO_ERROR){ printf("err(0x%x) line(%d)\n", err, __LINE__); return; }
	
#if 1
	{					
		//9680*10
		//VBO Bind
		glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[VERTEX_BUFFER]);//Vertex buffer
		#ifdef TEST_ES1_FLOAT_EN		
		glVertexPointer(3, GL_FLOAT, 0, 0);//vertex Pointer Disable
		#else
		glVertexPointer(3, GL_FIXED, 0, 0);//vertex Pointer Disable
		#endif
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,nBufferName9680[INDEX_BUFFER]);//index buffer

		glBindBuffer(GL_ARRAY_BUFFER,nBufferName9680[TEXTURECOORD_BUFFER]);//Texture Coord buffer
		#ifdef TEST_ES1_FLOAT_EN		
		glTexCoordPointer(2, GL_FLOAT, 0, 0);//Texture coord Pointer Disable
		#else
		glTexCoordPointer(2, GL_FIXED, 0, 0);//Texture coord Pointer Disable
		#endif
		//VBO Bind

		#ifdef TEST_ES1_FLOAT_EN		
		mali_float_matrix4x4 mat16;
		mali_float_matrix4x4 loop_mat16;
		api_matrix4_set_identity(&mat16);
		_gles1_translate(X0_0,X0_0,g_depth/2.f, &mat16);
		_gles1_rotate(xrot,X1_0,X0_0,X0_0, &mat16);
		_gles1_rotate(yrot,X0_0,X1_0,X0_0, &mat16);
		_gles1_scale(X0_1,X0_1,X0_1, &mat16);
		#else
		glTranslatex(X0_0,X0_0,g_depth/2);
		glRotatex(xrot,X1_0,X0_0,X0_0);
		glRotatex(yrot,X0_0,X1_0,X0_0);
		glScalex(X0_1,X0_1,X0_1);
		#endif
		
		for(int m = 0 ; m < gDrawEarthCnt ; m++)
		{
			int x_offset, y_offset, z_offset;
			glPushMatrix(); 		
			SetPosition(&x_offset, &y_offset, &z_offset, m);//(X1_0 * 3.f)
			#ifdef TEST_ES1_FLOAT_EN		
			memcpy(loop_mat16, mat16, sizeof(float) * 16);
			_gles1_translate((float)x_offset, (float)y_offset, (float)z_offset, &loop_mat16);
			glLoadMatrixf((float*)loop_mat16);
			#else
			glTranslatex(x_offset, y_offset, z_offset);
			#endif
			glDrawElements(drawMode, 9680 * 3.f, GL_UNSIGNED_SHORT, 0); 
			glPopMatrix();			
		}
	}	
#endif

#if 0
	xrot+=xspeed*30;
	yrot+=yspeed*30;
#else
	xrot+=xspeed;
	yrot+=yspeed;
#endif

	//VBO Mode Disable
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); 
	glBindBuffer(GL_ARRAY_BUFFER, 0);		
}

#define VERT_X 2.f
static void GL_Draw_es2(void)
{				
	/* Simple triangle. */
	const float vertex_data[] =
	{
		-0.5f*VERT_X, -0.5f*VERT_X, 0.0f,
		 0.5f*VERT_X, -0.5f*VERT_X, 0.0f,
		 0.5f*VERT_X,  0.5f*VERT_X, 0.0f,
		-0.5f*VERT_X,  0.5f*VERT_X, 0.0f,
	};

	const float color_data[] =
	{
		1.0, 0.0, 0.0, 1.0,
		0.0, 1.0, 0.0, 1.0,
		0.0, 0.0, 1.0, 1.0,
	};

	const GLfloat tex_coord_data[] = 
	{
		0.f,0.f,	1.f,0.f,  1.f,1.f,	0.f,1.f,
		1.f,1.f ,	0.f,1.f,  0.f,0.f,	1.f,0.f
	};

	GL_CHECK(glActiveTexture(GL_TEXTURE1));
	GL_CHECK(glBindTexture(GL_TEXTURE_2D, pixmapTextureName));

	/* Set clear screen color. */
	glClearColor(0.1f, 0.2f, 0.4f, 1.0);

	/* Test drawing. */
	for(int i = 0 ; i < 5; i++)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* Draw triangle. */
		glUseProgram(prog_name);


		if(loc_position != -1)
		{
			glEnableVertexAttribArray(loc_position);
			glVertexAttribPointer(loc_position, 3, GL_FLOAT, GL_FALSE, 0, vertex_data);
		}				
		else
		{
			printf("Warning: Attribute loc_position not found\n");
		}

		#if 0
		if(loc_fill_color != -1)
		{
			glEnableVertexAttribArray(loc_fill_color);
			glVertexAttribPointer(loc_fill_color, 4, GL_FLOAT, GL_FALSE, 0, color_data);
		}				
		else
		{
			printf("Warning: Attribute loc_fill_color not found\n");
		}
		#endif

		#if 1
		if(loc_tex_coord != -1)
		{
			glEnableVertexAttribArray(loc_tex_coord);
			glVertexAttribPointer(loc_tex_coord, 2, GL_FLOAT, GL_TRUE, 0, tex_coord_data);
		}				
		else
		{
			printf("Warning: Attribute loc_tex_coord not found\n");
		}
		
		if(loc_uni_sampler != -1)
		{
			glUniform1i(loc_uni_sampler, 1/*unit 0*/);
		}
		else
		{
			printf("Warning: Uniform loc_uni_sampler not found\n");
		}
		#endif
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
	}	
}


static void GL_Draw(void)
{
	#ifdef TEST_ES1_EN	
	SetOGLESCurrent_es1();
	GL_Draw_es1();	
	EGL_CHECK(eglWaitGL());
	#endif

	#ifdef TEST_ES2_EN	
	SetOGLESCurrent_es2();
	GL_Draw_es2();
	#endif
}

///////////////////////////////////////////////////////////////////////////////
static void glPerspectivef(GLfloat fov, GLfloat aspect, GLfloat near_val, GLfloat far_val)
{
	GLfloat top = (GLfloat)(tan(fov*0.5) * near_val);
	GLfloat bottom = -top;
	GLfloat left = aspect * bottom;
	GLfloat right = aspect * top;

	glFrustumf(left, right, bottom, top, near_val, far_val);
}


//////////////////////////////////////////////////////////////////////////////////////
/*************Calculate frame rate********/
static float framerate(int Poly)
{
    static float previous = 0;
    static int framecount = 0;
	static float finalfps = 0;
    framecount++;

    if ( framecount == 10 )
    {
        float time = (float)GetTime();
        float seconds = time - previous;
        float fps = framecount / seconds;
        previous = time;
		finalfps = fps;
        framecount = 0;
    }

	return finalfps;
}

/************* Texture ********/
static bool LoadTGA(TGAImage *texture, char *filename)				// Loads A TGA File Into Memory
{    
	GLubyte		TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};		// Uncompressed TGA Header
	GLubyte		TGAcompare[12];									// Used To Compare TGA Header
	GLubyte		header[6];										// First 6 Useful Bytes From The Header
	GLuint		bytesPerPixel;									// Holds Number Of Bytes Per Pixel Used In The TGA File
	GLuint		imageSize;										// Used To Store The Image Size When Setting Aside Ram
	GLuint		temp;											// Temporary Variable
	GLuint		type=GL_RGBA;									// Set The Default GL Mode To RBGA (32 BPP)

	OS_FILE file = OS_fopen(filename, "rb");							// Open The TGA File

	if(	file==NULL ||											// Does File Even Exist?
		OS_fread(TGAcompare,1,sizeof(TGAcompare),file)!=sizeof(TGAcompare) ||	// Are There 12 Bytes To Read?
		memcmp(TGAheader,TGAcompare,sizeof(TGAheader))!=0				||	// Does The Header Match What We Want?
		OS_fread(header,1,sizeof(header),file)!=sizeof(header))				// If So Read Next 6 Header Bytes
	{
		if (file == NULL)										// Did The File Even Exist? *Added Jim Strong*
			return GL_FALSE;										// Return False
		else													// Otherwise
		{
			OS_fclose(file);										// If Anything Failed, Close The File
			return GL_FALSE;										// Return False
		}
	}

	texture->width  = header[1] * 256 + header[0];				// Determine The TGA Width	(highbyte*256+lowbyte)
	texture->height = header[3] * 256 + header[2];				// Determine The TGA Height	(highbyte*256+lowbyte)
   
	if(	texture->width	<=0	||									// Is The Width Less Than Or Equal To Zero
		texture->height	<=0	||									// Is The Height Less Than Or Equal To Zero
		(header[4]!=24 && header[4]!=32))						// Is The TGA 24 or 32 Bit?
	{
		OS_fclose(file);											// If Anything Failed, Close The File
		return GL_FALSE;											// Return False
	}

	texture->bpp	= header[4];								// Grab The TGA's Bits Per Pixel (24 or 32)
	bytesPerPixel	= texture->bpp/8;							// Divide By 8 To Get The Bytes Per Pixel
	imageSize		= texture->width*texture->height*bytesPerPixel;	// Calculate The Memory Required For The TGA Data

	texture->imageData=(GLubyte *)malloc(imageSize);			// Reserve Memory To Hold The TGA Data

	if(	texture->imageData==NULL ||								// Does The Storage Memory Exist?
		OS_fread(texture->imageData, 1, imageSize, file)!=imageSize)	// Does The Image Size Match The Memory Reserved?
	{
		if(texture->imageData!=NULL)							// Was Image Data Loaded
			free(texture->imageData);							// If So, Release The Image Data

		OS_fclose(file);											// Close The File
		return GL_FALSE;											// Return False
	}

	for(GLuint i=0; i<int(imageSize); i+=bytesPerPixel)			// Loop Through The Image Data
	{															// Swaps The 1st And 3rd Bytes ('R'ed and 'B'lue)
		temp=texture->imageData[i];								// Temporarily Store The Value At Image Data 'i'
		texture->imageData[i] = texture->imageData[i + 2];		// Set The 1st Byte To The Value Of The 3rd Byte
		texture->imageData[i + 2] = temp;						// Set The 3rd Byte To The Value In 'temp' (1st Byte Value)
	}

	OS_fclose (file);												// Close The File


	glGenTextures(1, &texture->texID);							// Generate OpenGL texture IDs
	glBindTexture(GL_TEXTURE_2D, texture->texID);						// Bind Our Texture
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);	// Linear Filtered
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// Linear Filtered
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


	if (texture[0].bpp==24)										// Was The TGA 24 Bits
	{
		type=GL_RGB;											// If So Set The 'type' To GL_RGB
	}

	glTexImage2D(GL_TEXTURE_2D, 0, type, texture->width, texture->height, 0, type, GL_UNSIGNED_BYTE, texture->imageData);

	return GL_TRUE;												// Texture Building Went Ok, Return True
}

static double GetTime()
{
	return 0.001 * OS_GetTickCount();
}

//int OGLEarthSphere_VB()

