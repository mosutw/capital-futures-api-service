#include <stdio.h>
#include <windows.h>
#include "orderapi.h"

#ifdef DEBUG
#define _D(x)	printf x
#else
#define _D(x)
#endif

#define	_L(x)	printf x

/*
   int SKOrderLib_Initialize( [in]const char* lpszUser, [in]const char* lpszPassword)
   int GetUserAccount()
   int RegisterOnAccountCallBack( long lCallBackFunction)
   int SKOrderLib_ReadCertByID([in]char* lpszID)
   int SendFutureOrder( [in]const TCHAR* lpszAccount, [in]const TCHAR* lpszStockNo,
        [in]unsigned short usTradeType, [in]unsigned short usDayTrade, [in]unsigned short usBuySell,
        [in]const TCHAR* lpszPrice, [in]int nQty, [in,out] TCHAR* lpszMessage, [in,out] int*
        pnMessageBufferSize)
 */

//DLL function . use LoadLibrary / GetProcAddress to get function address
static int  __stdcall (*__pointer_SKOrderLib_Initialize)        (char* lpszUser,char* lpszPassword);
#define SKOrderLib_Initialize       (*__pointer_SKOrderLib_Initialize)

static int  __stdcall (*__pointer_GetUserAccount)   (void);
#define SKOrderLib_GetUserAccount   (*__pointer_GetUserAccount)

static int  __stdcall (*__pointer_RegisterOnAccountCallBack)    (long lCallBackFunction);
#define SKOrderLib_RegisterOnAccountCallBack    (*__pointer_RegisterOnAccountCallBack)

static int  __stdcall (*__pointer_SKOrderLib_ReadCertByID)      (char* lpszID);
#define SKOrderLib_ReadCertByID     (*__pointer_SKOrderLib_ReadCertByID)

static int  __stdcall (*__pointer_SendFutureOrder)  (
        char* lpszAccount,char* lpszStockNo
        ,unsigned short int usTradeType,unsigned short int usDayTrade
        ,unsigned short int usBuySell,char* lpszPrice
        ,int nQty,char* lpszMessage
        ,int* pnMessageBufferSize);
#define SKOrderLib_SendFutureOrder  (*__pointer_SendFutureOrder)

////////////////////////////////////////

static HINSTANCE __lib = NULL;
static UserAccount __ua[8];
static int  __account_total = 0;
static char __account[16];


char __order(char* name,unsigned short int type,unsigned short int dailyflag,unsigned short int buysell,char* price,int n);
char __order(char* name,unsigned short int type,unsigned short int dailyflag,unsigned short int buysell,char* price,int n)
{
	char __res_msg[128];
	int  __res_msg_size=128;

	char _r;

	_r = SKOrderLib_SendFutureOrder(
			__account
			,name   //contrace name
			,type   //ROD? IOK? FOK ?
			,dailyflag  //daily trade ?
			,buysell    //buy ? sell ?
			,price  //price ? or market ?
			,n      //amount
			,__res_msg
			,&__res_msg_size
			) ;

	printf("SKOrderLib_SendFutureOrder() return %d\n",_r);
	return _r;
}


static char __load_ol(void);
static char __load_ol(void)
{
	void* __funcp;

	__lib = LoadLibrary("SKOrderLib.dll");
	if(__lib == NULL) return -1;

	__funcp = GetProcAddress(__lib,"SKOrderLib_Initialize");
	if(__funcp == NULL) return -1;
	__pointer_SKOrderLib_Initialize = __funcp;

	__funcp = GetProcAddress(__lib,"GetUserAccount");
	if(__funcp == NULL) return -1;
	__pointer_GetUserAccount = __funcp;

	__funcp = GetProcAddress(__lib,"RegisterOnAccountCallBack");
	if(__funcp == NULL) return -1;
	__pointer_RegisterOnAccountCallBack = __funcp;

	__funcp = GetProcAddress(__lib,"SKOrderLib_ReadCertByID");
	if(__funcp == NULL) return -1;
	__pointer_SKOrderLib_ReadCertByID = __funcp;

	__funcp = GetProcAddress(__lib,"SendFutureOrder");
	if(__funcp == NULL) return -1;
	__pointer_SendFutureOrder = __funcp;

	return 1;
}

static void __free_ol(void);
static void __free_ol(void)
{
	if(__lib != NULL)
	{
		FreeLibrary(__lib);
		__lib = NULL;
	}
}

//Void __stdcall OnAccount( BSTR bstrData);
static void __stdcall __account_pull_notify (void* bstrData);
static void __stdcall __account_pull_notify (void* bstrData)
{
	int i = 0;
	short int* _p = (short int*) bstrData;

	_D(("orderapi.c:Account callback notify.\n"));

	for(i = 0; _p[i] != ',' ; i++)
		__ua[__account_total].Market[i] = (char)(_p[i]);
	__ua[__account_total].Market[i]=0;
	_p+=(i+1);


	for(i = 0; _p[i] != ',' ; i++)
		__ua[__account_total].BranchName[i] = (char)(_p[i]);
	__ua[__account_total].BranchName[i] = 0;
	_p+=(i+1);


	for(i = 0; _p[i] != ',' ; i++)
		__ua[__account_total].BranchID[i] = (char)(_p[i]);
	__ua[__account_total].BranchID[i] = 0;
	_p+=(i+1);


	for(i = 0; _p[i] != ',' ; i++)
		__ua[__account_total].Account[i] = (char)(_p[i]);
	__ua[__account_total].Account[i] = 0;
	_p+=(i+1);


	for(i = 0; _p[i] != ',' ; i++)
		__ua[__account_total].IdentityNumber[i] = (char)(_p[i]);
	__ua[__account_total].IdentityNumber[i] = 0;


	_D(("[%s][%s]\n",__ua[__account_total].BranchName,__ua[__account_total].Account));
	__account_total++;
}

////////////////////////////////////////



//###################################################
char OL_LoginServer(char* username, char* password)
{
	char _r;
	_D(("orderapi.c:OL_LoginServer\n"));

	_r = __load_ol();
	_D(("__load_ol() return %d\n",_r));
	if (_r == -1) return -1;

	_r = SKOrderLib_Initialize(username,password);
	_D(("SKOrderLib_Initialize() return %d\n",_r));
	if (_r == -1) return -1;

	_r = SKOrderLib_RegisterOnAccountCallBack((long)__account_pull_notify);
	_D(("SKOrderLib_RegisterOnAccountCallBack() return %d\n",_r));
	if (_r == -1) return -1;

	_r = SKOrderLib_GetUserAccount();
	_D(("SKOrderLib_GetUserAccount() return %d\n",_r));
	if (_r == -1) return -1;

	_r = SKOrderLib_ReadCertByID(username);
	_D(("SKOrderLib_ReadCertByID() return %d\n",_r));
	if (_r == -1) return -1;

	return 1;
}

UserAccount* OL_GetUserAccount(int* number)
{
	if(number == NULL)          return NULL;
	if(__account_total <= 0)    return NULL;

	*number = __account_total;  return __ua;
}

char OL_SetTradeAccount(int index)
{

	_D(("orderapi.c:OL_SetTradeAccount\n"));

	if(index > 8 || index < 0)
	{
		_D(("orderapi.c:OL_SetTradeAccount():Out of index.total 8,and start is 1\n"));
		return -1;
	}

	sprintf(__account,"%s%s",__ua[index-1].BranchName,__ua[index-1].Account);
	_D(("orderapi.c:OL_SetTradeAccount():Accout string for order is [%s]\n",__account));
	return 1;
}

//char __order(name,ROD|IOC|FOK,Daily,buysell,price,amount);
char OL_OrderMarket(char* Stockname,unsigned char amount,char DailyFlag,char BSFlag)
{
	char _r;
	_D(("orderapi.c:OL_OrderMarket\n"));

	_r = __order(Stockname,TYPE_IOC,(DailyFlag>0?1:0),(BSFlag>0?1:0),"M",amount);
	_D(("orderapi.c:__order is return %d\n",_r));
	return _r;
}

char OL_OrderPrice (char* Stockname,char* price,unsigned char amount,char DailyFlag,char BSFlag)
{
	char _r;
	_D(("orderapi.c:OL_OrderPrice\n"));

	_r = __order(Stockname,TYPE_ROD,(DailyFlag>0?1:0),(BSFlag>0?1:0),price,amount);
	_D(("orderapi.c:__order is return %d\n",_r));
	return _r;
}

void OL_Bye(void)
{
	_D(("orderapi.c:OL_Bye\n"));
	__account_total  = 0;
	__free_ol();
}


//###################################################

