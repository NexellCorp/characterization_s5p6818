#ifndef	__DbgMsg_h__
#define	__DbgMsg_h__

#ifndef	DTAG
#define	DTAG	"[NXP-TS] "
#endif

#ifndef	DBG_FUNC
#define	DBG_FUNC	1
#endif

#define DbgMsg(FLAG, fmt...)	if(FLAG) do{					\
									printf("%s" , DTAG);		\
									printf(fmt);				\
								}while(0)

#define	ErrMsg(fmt...)			do{																	\
									printf("%s%s(line:%d) %s:", DTAG, __FILE__,__LINE__,__func__);	\
									printf(fmt);													\
								}while(0)

#if (DBG_FUNC==1)
#define	FUNC_IN					printf("%s%s() IN\n", DTAG, __func__)
#define	FUNC_OUT				printf("%s%s() OUT\n", DTAG, __func__)
#else
#define	FUNC_IN					do{}while(0)
#define	FUNC_OUT				do{}while(0)
#endif

#endif	//	__DbgMsg_h__
