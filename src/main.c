#include <reg52.h>
#include <intrins.h>

#include " LCD12864.h "	//里面定义了12864液晶的端口接法  以及 12864程序声名
#include "Finger_Modle.h"//指纹模块的头文件

//=====P3的高4位接了按键，可以在keyscan里面改=============
sfr KEY=0xB0; //就是P3,参考reg52.h
#define G_upkey 0x80 //px.7 按键，向上键
#define G_downkey 0x40 //px.6 按键，向下键
#define G_entkey 0x20 //px.5 按键，确定键
#define G_cankey 0x10//px.4 按键，取消键
//===========按键======================================

sbit relay =P1^4; //继电器引脚
sbit buzzer=P1^5; //蜂鸣器引脚
sbit red=   P2^7;//录入模式指示灯 在板子靠近单片机处
sbit green= P2^0;//识别模式指示灯 在板子远离单片机处

void delayms(int ms);
void keyscan();//按键扫描

unsigned char Trg;//按键触发，一组p3里面只会出现一次
unsigned char Cont;//按键长按，被keyscan函数改变

unsigned char KeyFuncIndex=0;    //存放当前的菜单索引

void (*KeyFuncPtr)();            //定义按键功能指针
//定义类型 
typedef struct 
{
   unsigned char KeyStateIndex;   //当前的状态索引号
   unsigned char KeyUpState;      //按下向上键时的状态索引号
	 unsigned char KeyDownState;    //按下向下键时的状态索引号
   unsigned char KeyEnterState;   //按下回车键时的状态索引号
   unsigned char KeyCancle;       //按下取消退回上级菜单
	 void (*CurrentOperate)(void);      //当前状态应该执行的功能操作
}  StateTab;

//===========================================================================
//================下面是各菜单层的实施函数================================
//============================================================================

void Stat00(void){//第一个页面显示是否握手成功，成功的话按任意键进入下一个页面，不成功的话就不开定时键盘扫描中断
	unsigned char i;
	ET0=0;//先关掉定时器中断，避免在处理时响应了键盘，进入了下一个状态，而这这状态还没运行完
	dprintf(0,0,"  南牧指纹开关  ");
	for(i=0;i<6;i++)//开始握手6次，如果没有一次成功，表示模块通信不正常。只要成功就跳出此循环
	{
		if(FM_VefPSW())//与模块握手通过，绿灯亮起。进入识别模式
		{
      dprintf(1,0,"    握手成功    ");
			dprintf(3,0,"按任意键进入系统");
			//ET0=1;//成功的话开定时键盘扫描中断，可以进入下一个状态，否则的话就一直显示下面的失败界面
			break;//退出全部循环，执行下面for的语句，不同与continue
		}
	    else
		{
			dprintf(1,0,"    握手失败    ");//如果失败继续执行下一次for看会不会成功
			dprintf(3,0,"请检查线路模块");
			//break;
		}
	}
	ET0=1;
}
void Stat10(void){ //ui打开开关
	ET0=0;//关定时器中断避免响应按键，记得开回来
	dprintf(0,0,"    开关机器   ");//反白显示
	dprintf(1,0,"    录入指纹    ");
	dprintf(2,0,"    清空指纹    ");
	dprintf(3,0,"                ");
	Writecolor_hang_12864(0,1);//反白第一行，0代表第一行，1代表黑色
	Writecolor_hang_12864(1,0);
	ET0=1;
}
void Stat11(void){ //ui录入指纹
	ET0=0;
	dprintf(0,0,"    开关机器    ");//反白显示
	dprintf(1,0,"    录入指纹    ");
	dprintf(2,0,"    清空指纹    ");
	dprintf(3,0,"                ");
	Writecolor_hang_12864(0,0);//把上一行的填充察掉
	Writecolor_hang_12864(2,0);//确保重下往上按时下面那行擦掉
	Writecolor_hang_12864(1,1);//反白第二行
	ET0=1;
}
void Stat12(void){ //ui清空指纹
	ET0=0;
	dprintf(0,0,"    开关机器    ");//反白显示
	dprintf(1,0,"    录入指纹    ");
	dprintf(2,0,"    清空指纹    ");
	dprintf(3,0,"                ");
	Writecolor_hang_12864(1,0);
	Writecolor_hang_12864(3,0);
	Writecolor_hang_12864(2,1);
	ET0=1;
}
/*
void Stat13(void){ 
	ET0=0;
	dprintf(0,0,"    打开开关    ");//反白显示
	dprintf(1,0,"    录入指纹    ");
	dprintf(2,0,"    清空指纹    ");
	dprintf(3,0,"    打开电源    ");
	Writecolor_hang_12864(2,0);
	Writecolor_hang_12864(3,1);
	ET0=1;
}*/

void Stat20(void){//打开开关--确定
	unsigned char num=0;
	unsigned char strnum[4]={0};//数量转换为字符串
	ET0=0;
  LcmClearTXT();//清除文本
	LcmClearBMP();//清除图像
	if(relay==0){//如果继电器是开启的
		dprintf(0,0,"开关已经开启    ");//keil bug用内码
		dprintf(1,0,"输入指纹关闭    ");
		dprintf(2,0,"                ");
		dprintf(3,0,"等\xB4\xFD输入........");
	}else{
		dprintf(0,0,"开关已经关闭    ");//keil bug用内码
		dprintf(1,0,"输入指纹开启    ");
		dprintf(2,0,"                ");
		dprintf(3,0,"等\xB4\xFD输入........");
	}
	num=FM_Search();
	strnum[0]=32;//acsii码空格
	strnum[1]= num/100+48;     //+48是为了转换在ASCII码  百
	strnum[2]= (num%100)/10+48;//+48是为了转换在ASCII码  十
	strnum[3]= num%10+48;      //+48是为了转换在ASCII码  个?
	if(num==0xff){//操作失败
		dprintf(0,0,"该指纹不存在    ");//keil bug用内码
		dprintf(1,0,"操作失败        ");
		dprintf(2,0,"                ");
		dprintf(3,0,"按任意键返回    ");
		buzzer=0;
		delayms(500);
		buzzer=1;
	}else{//该指纹存在操作成功
		dprintf(0,0,"操作成功        ");
		dprintf(1,0,"                ");
	  dprintf(2,0,"用户号: ");
		dprintf(2,4,strnum);
		dprintf(2,6,"   ");
		dprintf(3,0,"按任意键返回    ");
		relay=~relay;//继电器翻转
		buzzer=0;
		delayms(500);
		buzzer=1;
	}
	ET0=1;
}

void Stat21(void){//录入指纹--确�
 
	unsigned char strnum[4]={0};//数量转换为字符串
	unsigned char FM_model_num=0,search_num=0;//模块里面的指纹存储数量
  bit ok1=0,ok2=0;
	ET0=0;//关按键扫描
	LcmClearTXT();//清除文本
	LcmClearBMP();//清除图像
	FM_model_num=FM_ValidTempleteNum(0);//读取指纹模块里面的模版数量低位，低位就够?

	
	if((FM_model_num==0xff)){//如果读模版数指令返回错误值
		dprintf(0,0,"读取模版\xCA\xFD错误  ");//keil bug用内码
		dprintf(1,0,"                ");
		dprintf(2,0,"                ");
		dprintf(3,0,"请退回重试      ");
		goto end;//跳过了读取指纹
	}else{//如果是正常读取模版数
		if((FM_model_num==0)||(FM_model_num==1)){//你是第一个，第2个输入指纹的，具有最高权限
			dprintf(0,0,"    最高权限    ");
			goto begin;
		}else{//你是第三个或其他输入的人，你的录入是不同的，，你的输入需要最高权限的同意，你只有普通权限
			dprintf(0,0,"    用户权限    ");
			dprintf(1,0,"请管理员输入指纹");
			dprintf(2,0,"等\xB4\xFD输入........");
			search_num=FM_Search();
			if((search_num==1)||(search_num==2)){//如果是第	1，2号指纹就是管理员，具有权限运行下面的录入指纹
				strnum[0]=32;//acsii码空格
				strnum[1]= search_num/100+48;     //+48是为了转换在ASCII码  百
				strnum[2]= (search_num%100)/10+48;//+48是为了转换在ASCII码  十
				strnum[3]= search_num%10+48;      //+48是为了转换在ASCII码  个?
				dprintf(1,0,"验证成功        ");
				dprintf(2,0,"管理员: ");
				dprintf(2,4,strnum);
				dprintf(2,6,"   ");
				buzzer=0;
				delayms(500);//蜂鸣
				buzzer=1;
				goto begin;
			}else if(search_num==0xff){//不能搜索到该指纹
				dprintf(0,0,"    用户权限    ");
				dprintf(1,0,"该指纹还没录入  ");
				dprintf(2,0,"                ");
				dprintf(3,0,"按任意键退回重试");
				buzzer=0;
				delayms(500);//蜂鸣
				buzzer=1;
				goto end;
			}else{//指纹是录入的但不是1，2号指纹，没有最高权限
				strnum[0]=32;//acsii码空格
				strnum[1]= search_num/100+48;     //+48是为了转换在ASCII码  百
				strnum[2]= (search_num%100)/10+48;//+48是为了转换在ASCII码  十
				strnum[3]= search_num%10+48;      //+48是为了转换在ASCII码  个?
				dprintf(1,0,"你的权限不足    ");
				dprintf(2,0,"用户号: ");
				dprintf(2,4,strnum);
				dprintf(2,6,"   ");
				dprintf(3,0,"按任意键退回重试");
				buzzer=0;
				delayms(500);//蜂鸣
				buzzer=1;
				goto end;
			}
		}
	}
	    //--------------------读取两次指纹保存到模版---------------------------------
			begin:
		//数字转为char类型显示
			strnum[0]=32;//acsii码空格
			strnum[1]= FM_model_num/100+48;     //+48是为了转换在ASCII码  百
			strnum[2]= (FM_model_num%100)/10+48;//+48是为了转换在ASCII码  十
			strnum[3]= FM_model_num%10+48;      //+48是为了转换在ASCII码  个?
			dprintf(1,0,"指纹\xCA\xFD: ");//keil里面的bug要用汉字内码代替对于fd的汉字,指纹数
			dprintf(1,4,strnum);
			dprintf(1,6,"    ");//一行内补空格避免了显示乱码
			dprintf(2,0,"第一次录入      ");
			dprintf(3,0,"等\xB4\xFD中..........");//等待中......
			if(FM_GetImage()==1){//获取指纹图像
				dprintf(1,0,"第一次获取成功  ");
				dprintf(2,0,"                ");
				dprintf(3,0,"\xD5\xFD在保存到寄存器");
				if(FM_CreatChar_buffer(1)==1){//保存到寄存器1
					if(FM_Searchfinger1()==0xff){//用buffer1搜索指纹如果没有收到那就是该指纹还没录入
						goto con_tinue;//继续正常的程序流
					}else{
						dprintf(1,0,"该指纹已经存在  ");
						dprintf(2,0,"                ");
						dprintf(3,0,"任意键退回重试  ");
						buzzer=0;
						delayms(500);//蜂鸣
						buzzer=1;
						goto end;
					}
					con_tinue:
					dprintf(1,0,"保存成功        ");
					dprintf(2,0,"                ");
					dprintf(3,0,"                ");
					buzzer=0;
					delayms(500);//蜂鸣器100ms
					buzzer=1;
					delayms(1000);//让字幕显示2秒
					dprintf(3,0,"开始第二次录入  ");
					if(FM_GetImage()==1){//获取指纹图像
						dprintf(1,0,"第二次获取成功  ");
						dprintf(2,0,"                ");
						dprintf(3,0,"\xD5\xFD在保存到寄存器");
						if(FM_CreatChar_buffer(2)==1){//将图像保存到寄存器2
							dprintf(1,0,"保存成功        ");
							dprintf(2,0,"                ");
							dprintf(3,0,"                ");
							buzzer=0;
							delayms(500);//蜂鸣器100ms
							buzzer=1;
							delayms(1000);//让字幕显示2秒
							dprintf(3,0,"开始生成模版    ");
							if(FM_RegModel_Charbuffer()==1){//根据录入的两个指纹特征码生成模版
								dprintf(1,0,"生成模版成功    ");
								dprintf(2,0,"                ");
								dprintf(3,0,"正在保存模版    ");
								FM_model_num=FM_model_num+1;//用当前有效指纹数加1
							  //数字转为char类型显示
								strnum[0]=32;//acsii码空格
								strnum[1]= FM_model_num/100+48;     //+48是为了转换在ASCII码  百
								strnum[2]= (FM_model_num%100)/10+48;//+48是为了转换在ASCII码  十
								strnum[3]= FM_model_num%10+48;      //+48是为了转换在ASCII码  个?
								if(FM_Save_model(FM_model_num)==1){//保存模版成功，根据录入的两个指纹特征码生成模版，保存到FM_model_num+1中
									//dprintf(0,0,"    最高权限    ");
									dprintf(1,0,"  保存指纹成功  ");
									dprintf(2,0,"指纹号:");
									dprintf(2,4,strnum);
									dprintf(2,6,"    ");//一行内补空格避免了显示乱码
									dprintf(3,0,"按任意键返回    ");
									goto end;
								}else{//保存模版失败
									dprintf(1,0,"保存指纹失败    ");
									dprintf(2,0,"                ");
									dprintf(3,0,"请退回重试      ");
									goto end;
								}
							}else{
								dprintf(1,0,"生成指纹模版失败");
								dprintf(2,0,"                ");
								dprintf(3,0,"请退回重试      ");
								goto end;
							}
						}else{
							dprintf(1,0,"保存寄存器二失败");
							dprintf(2,0,"                ");
							dprintf(3,0,"请退回重试      ");
							goto end;
						}
					}else{
						dprintf(1,0,"获取指纹二失败  ");
						dprintf(2,0,"                ");
						dprintf(3,0,"请退回重试      ");
						goto end;
					}
				}else
				{
					dprintf(1,0,"保存寄存器一失败");
					dprintf(2,0,"                ");
					dprintf(3,0,"请退回重试      ");
					goto end;
				}
			}else{//获取指纹失败
				dprintf(1,0,"获取指纹一失败  ");
				dprintf(2,0,"                ");
				dprintf(3,0,"请退回重试      ");
				goto end;
			}
			//----------------读取两次指纹并生成模版，保存模版结束-------------------------
	end:  
	ET0=1;//开扫描
}

void Stat22(void){//清空指纹--确定
	unsigned char search_num=0;
	//unsigned int time_count=0;
	ET0=0;
  LcmClearTXT();//清除文本
	LcmClearBMP();//清除图像
	dprintf(0,0,"注意该操作会删\xB3\xFD");
	dprintf(1,0,"所有指纹包括管理");
	dprintf(2,0,"员指纹!!        ");
	dprintf(3,0,"确定继续退出取消");
	while(1){//等待用户按键选择继续还是退�,�
		keyscan();//按键扫描
		switch(Trg){
			case  G_cankey:			  //按下取消键,P3.4
			{
				goto end;//退出
			}
			case G_entkey://按下确定键盘
			{

				goto begin;
			}
			default:break;
		}
		//time_count++;
	}
	goto end;//如果是while因为超时退出的下面的验证就不运行了
	begin://如果用户按下确定键盘
	dprintf(0,0,"删\xB3\xFD所有指纹    ");
	dprintf(1,0,"请管理员输入指纹");
	dprintf(2,0,"                ");
	dprintf(3,0,"等\xB4\xFD输入........");
	search_num=FM_Search();
	buzzer=0;
	delayms(300);
	buzzer=1;
	if(search_num==1||search_num==2){//你是管理员
		if(FM_Empty()==1){//成功
			  dprintf(0,0,"清空指纹库成功  ");
				dprintf(1,0,"                ");
				dprintf(2,0,"                ");
				dprintf(3,0,"按任意键盘返回  ");
				goto end;
		}else{
				dprintf(0,0,"清空指纹库失败  ");
				dprintf(1,0,"                ");
				dprintf(2,0,"                ");
				dprintf(3,0,"按任意键盘返回  ");
				goto end;
		}
	}else if(search_num=0xff){//你的指纹输入有错，不存在
		dprintf(0,0,"你的指纹不存在  ");
		dprintf(1,0,"                ");
		dprintf(2,0,"                ");
		dprintf(3,0,"按任意键返回    ");
		goto end;
	}else{//其他不具有管理员权限的指纹
		dprintf(0,0,"你的指纹权限不够");
		dprintf(1,0,"仅管理员可以操作");
		dprintf(2,0,"                ");
		dprintf(3,0,"按任意键返回    ");
		goto end;
	}
	
	end:
	ET0=1;
}

/*
void Stat23(void){//清空指纹--确�
	unsigned char num;
	unsigned char strnum[4]={0};//数量转换为字符串
	ET0=0;
	LcmClearBMP();//清除图像
	num=FM_Search();
								strnum[0]=32;//acsii码空格
								strnum[1]= num/100+48;     //+48是为了转换在ASCII码  百
								strnum[2]= (num%100)/10+48;//+48是为了转换在ASCII码  十
								strnum[3]= num%10+48;      //+48是为了转换在ASCII码  个?
	if(num==0xff){//失败
			  dprintf(0,0,"清空指纹失败    ");
				dprintf(1,0,"                ");
				dprintf(2,0,"                ");
				dprintf(3,0,"请退回          ");
	}else{//成功
				dprintf(0,0,"清空指纹成功    ");
				dprintf(1,0,"                ");
				dprintf(2,0,strnum);
				dprintf(2,4,"            ");
				dprintf(3,0,"请退回重试      ");
	}
	ET0=1;
}
*/
/*-------------------------------------------------------------*/
 //数据结构数组,数字代表每个按键按下是的进入的状态
StateTab code KeyTab[12]=
{
	{0,1,1,1,1,   (*Stat00)}, //握手检测，ok的话可以显示下个状态不行的话就关键盘中断扫描
	{1,1,2,4,1,   (*Stat10)},    //顶层，打开开关  当前的状态索引号,按下向下键时的状态索引号,按下向上键时的状态索引号,按下回车键时的状态索引号,当前状态应该执行的功能操作
	{2,1,3,5,2,   (*Stat11)},	   //顶层，录入指纹
	{3,2,3,6,3,   (*Stat12)},	   //顶层，清空指纹
	//{4,3,4,8,4,   (*Stat13)},	   //顶层，测试
	
	{4,1,1,1,1,   (*Stat20)},//第二层，打开开关的子菜单
	{5,2,2,2,2,   (*Stat21)},//第二层,录入指纹
	{6,3,3,3,3,   (*Stat22)},//第二层 清空指纹
	//{8,2,2,2,2,   (*Stat23)},//第二层 清空指纹	
};

//==================================================================================
//========状态机结构结束======================================
//=================================================

//**************?????***************************
 void delayms(int ms) 
{      
 unsigned char j;
 while(ms--)
 {
  	for(j =0;j<120;j++);
 }
}
//=======按键扫描=============
void keyscan(){//返回是哪个按键
	unsigned char ReadDate=KEY^0xff;;
	Trg=ReadDate&(ReadDate^Cont);
	Cont=ReadDate;
}
/*-------------------------------------------------------------*/
void MenuOperate() interrupt 1
{
   keyscan(); //键盘扫描，改不了Trg，Cont的值
    switch(Trg) //检测按键触发
	{
	    case  G_upkey:		       //向上的键，Trg为1000 0000 代表P3.7触发了一次
		{
		    KeyFuncIndex=KeyTab[KeyFuncIndex].KeyUpState;
				//下面是执行按键的操作
			  KeyFuncPtr=KeyTab[KeyFuncIndex].CurrentOperate;
				(*KeyFuncPtr)();     //执行当前的按键操作
			buzzer=0;delayms(20);buzzer=1;//蜂鸣器短响起
			break; 
		}
		case  G_entkey:			  //回车键,P3.5
		{
			KeyFuncIndex=KeyTab[KeyFuncIndex].KeyEnterState;
				//下面是执行按键的操作
			KeyFuncPtr=KeyTab[KeyFuncIndex].CurrentOperate;
			(*KeyFuncPtr)();     //执行当前的按键操作
			buzzer=0;delayms(20);buzzer=1;//蜂鸣器短响起
			break; 
		}
		case  G_downkey:			  //向下的键,P3.6
		{
			KeyFuncIndex=KeyTab[KeyFuncIndex].KeyDownState;
				//下面是执行按键的操作
			KeyFuncPtr=KeyTab[KeyFuncIndex].CurrentOperate;
			(*KeyFuncPtr)();     //执行当前的按键操作
			buzzer=0;delayms(20);buzzer=1;//蜂鸣器短响起
			break; 
		}
		case  G_cankey:			  //按下取消键,P3.4
		{
			KeyFuncIndex=KeyTab[KeyFuncIndex].KeyCancle;
				//下面是执行按键的操作
			KeyFuncPtr=KeyTab[KeyFuncIndex].CurrentOperate;
			(*KeyFuncPtr)();     //执行当前的按键操作
			buzzer=0;delayms(20);buzzer=1;//蜂鸣器短响起
			break; 
		}
		//此处添加按键错误代码,定时扫描没有检测到按键按下
		default:return;
	}
}	
void main(void){
	delayms(10);//等待单片机复位
	PSB=0;      //液晶为串口显示模式，将PSB引脚设置为0
	
	//开定时中断-------------------------
	TMOD=0x01;//设置定时器0位工作模式1
	TH0=(65536-22936)/256;//装初值11.0592Mhz晶振定时50ms数为45872,25ms为22936
	TL0=(65536-22936)%256;//
	EA=1;//开总中断
	ET0=1;//开定时器0中断
	TR0=1;//启动定时器0
	//================================
	
	LcmInit(); //12864初始化
	LcmClearTXT();//清除文本
	LcmClearBMP();//清除图像
	FM_Init();//初始化指纹模块主要是串口
	//运行一次界面显示
	KeyFuncPtr=KeyTab[KeyFuncIndex].CurrentOperate;
	(*KeyFuncPtr)();     //执行当前的按键操作，就是stat00();
	
	while(1){
		//relay=1;
		//50ms定时中断，扫描键盘输入,不用定时响应会很慢
	}
}


//-------------------各种字函数-----------------------------















