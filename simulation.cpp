#include<iostream>
#include<map>
#include<fstream>
#include<cstring>
#include<iomanip>
using namespace std;
/* 
指令32位，寄存器32个，mem

指令32位，前6位区分不同指令，
load ra, num(rb) ->  ra 5位  rb 5位  num 16位
EX算num+(rb),MEM算(num+(rb)) 
store num(rb) ,ra -> ra 5位  rb 5位  num 16位
EX算num+(rb),MEM算(num+(rb)) 
addi ra,rb,num ->ra 5位  rb 5位  num 16位
add  ra,rb,rc ->ra 5位  rb 5位  rc 5位
sub  ra,rb,rc ->ra 5位  rb 5位  rc 5位
beqz  ra ,addr -> ra 5位 addr 相对地址 21位
EX算结果地址并写回 
trap 陷入操作系统的指令 
IF识别出来就ban了流水线以后的，WB执行完就结束 
*/
struct Register{
	int r[32];
	int use[32];//-1表示没指令写，num表示num条指令写,-2表示能用定向 
	int dir_result[32];
	int timestamp[32];//表示r或者dir_result多久修改，如果t时修改，那么t时不能出结果
	//因为t时出结果，t+1才能用，t时给前面用就错了 
	Register()
	{
		for(int i=0;i<32;i++) use[i]=-1;
		for(int i=0;i<32;i++) r[i]=dir_result[i]=0;
		for(int i=0;i<32;i++)	timestamp[i]=0;
	}
	void write(int x,int y)
	{
		r[x]=y;
	}
	void show()
	{
		for(int i=0;i<4;i++)
		{
			for(int j=0;j<8;j++)
			{
				cout<<"r"<<i*8+j<<":"<<r[i*8+j]<<"  ";
			}
			cout<<endl;
		}
		cout<<endl;
	} 
}Reg;
struct Memory{
	int mem[100000];
	int use;
	Memory()
	{
		memset(mem,0,sizeof(mem));
	}
}Mem;
int PC;
int all=0;
struct Instruction{
	int inst;//区分不同指令
	int ra,rb,rc;
	int number;
	int addr;
	int result;
	void show();
};
void Instruction::show()
{
	switch(inst)
	{
		case 0: 
		{
			cout<<"load "<<"r"<<ra<<" "<<number<<"(r"<<rb<<")";
			break;
		}
		case 1: cout<<"store "<<number<<"(r"<<rb<<") "<<"r"<<ra; break;
		case 2: cout<<"addi "<<"r"<<ra<<" r"<<rb<<" "<<number;break;
		case 3:	cout<<"add "<<"r"<<ra<<" r"<<rb<<" r"<<rc;break;
		case 4: cout<<"sub "<<"r"<<ra<<" r"<<rb<<" r"<<rc;break;
		case 5: cout<<"beqz "<<"r"<<ra<<" "<<addr;break;
		case 6: cout<<"trap";
	}
}
Instruction Inst[10000];
int timeline[10000][6]; 
int time=0;
struct Pipline{
	int instid[6];//不同段指令编号，-1表示stall，0表示还没有指令
	string name[6]={"","IF","ID","EX","MEM","WB"} ;
	int num_WAW;//写后写冲突 
	int num_RAW;//读后写冲突 
	Pipline()
	{
		for(int i=1;i<=5;i++)
		{
			instid[i]=0;
		}
		num_WAW=0;
		num_RAW=0;
	}
	int deal_no_dir(int pc);//不定向 
	int deal_dir(int pc);//定向 
	int WB();
	void MEM();
	void EX();
	int ID();
	int IF(int signal); 
	void show();
	void MEM_dir();
	void EX_dir();
	int ID_dir(); 
}pipline;
void Pipline::show()
{
	for(int i=1;i<=5;i++)
	{
		cout<<name[i]<<":";
		if(instid[i]==-1)
		{
			cout<<"stall";
		}
		else if(instid[i]>0)
		{
			Inst[instid[i]].show();
		}
		cout<<endl;
	}
}
int Pipline::WB()
{
	if(instid[5]>0)
	{
		Instruction* temp=&(Inst[instid[5]]);
		switch(temp->inst)
		{
			case 2:
			case 3:
			case 4:	
			case 0: 
			{
				Reg.r[temp->ra]= temp->result;
				if(Reg.use[temp->ra]!=-2) Reg.timestamp[temp->ra]=time;
				Reg.use[temp->ra]=-1;
				//Reg.timestamp[temp->ra]=time;
				break;
			}
			case 1: Mem.mem[temp->result]=Reg.r[temp->ra]; break;
			case 6:
				return 1;
		}
		instid[5]=-1;
	}
	return 0;		
}
void Pipline::MEM()
{
	if(instid[4]>0)
	{
		switch(Inst[instid[4]].inst)
		{
			case 0: //load
			//case 1: //store
				Inst[instid[4]].result=Mem.mem[Inst[instid[4]].result];break;
		}
		instid[5]=instid[4];
		instid[4]=-1;
	}
	else if(!instid[4]) 
	{
		instid[5]=0;
	}
}
void Pipline::MEM_dir()
{
	if(instid[4]>0)
	{
		switch(Inst[instid[4]].inst)
		{
			case 0: //load
			{
				Inst[instid[4]].result=Mem.mem[Inst[instid[4]].result];
				Reg.use[Inst[instid[4]].ra]=-2;
				Reg.timestamp[Inst[instid[4]].ra]=time;
				Reg.dir_result[Inst[instid[4]].ra]=Inst[instid[4]].result;
				break;
			}
			/* 
			case 1: //store
			{
				Inst[instid[4]].result=Mem.mem[Inst[instid[4]].result];
				Reg.dir_result[Inst[instid[4]].ra]=Inst[instid[4]].result;
				break;
			}*/ 
				
				 
		}
		instid[5]=instid[4];
		instid[4]=-1;
	}
	else if(!instid[4]) 
	{
		instid[5]=0;
	}
}
void Pipline::EX()
{
	if(instid[3]>0)
	{
		Instruction *temp=&Inst[instid[3]];
		switch(temp->inst)
		{
			case 0://load
			case 1://store
			case 2://addi
			{
				temp->result=Reg.r[temp->rb]+temp->number;
				break;
			}
			case 3://add
				temp->result=Reg.r[temp->rb]+Reg.r[temp->rc]; break;
			case 4://sub
				temp->result=Reg.r[temp->rb]-Reg.r[temp->rc]; break;
			case 5://beqz
			{
				if(!Reg.r[temp->ra]) 
				{
					//PC+=temp->addr-2;//写回地址,这是符合常理的
					PC+=temp->addr-3;//之所以会多减一个，是因为我下文逻辑是PC+1 
					instid[1]=instid[2]=0;//清空流水线 
				}
			}
		}
		instid[4]=instid[3];
		instid[3]=-1;
	}
	else if(!instid[3])
	{
		instid[4]=0;
	}
}
void Pipline::EX_dir()
{
	if(instid[3]>0)
	{
		Instruction *temp=&Inst[instid[3]];
		switch(temp->inst)
		{
			case 0://load
			case 1://store
			{
				if(Reg.use[temp->rb]==-1||Reg.use[temp->rb]==instid[3])
					temp->result=Reg.r[temp->rb]+temp->number;
				else if(Reg.use[temp->rb]==-2)
					temp->result=Reg.dir_result[temp->rb]+temp->number;	
				//Reg.use[temp->ra]=-2;
				Reg.dir_result[temp->ra]=temp->result;
				//Reg.timestamp[temp->ra]=time;
				break;
			}
			case 2://addi
			{
				if(Reg.use[temp->rb]==-1||Reg.use[temp->rb]==instid[3])
					temp->result=Reg.r[temp->rb]+temp->number;
				else if(Reg.use[temp->rb]==-2)
					temp->result=Reg.dir_result[temp->rb]+temp->number;	
				Reg.use[temp->ra]=-2;
				Reg.dir_result[temp->ra]=temp->result;
				Reg.timestamp[temp->ra]=time;
				break;
			}
			case 3://add
			{
				int b,c;///!!!!!!!!!到这里 
				if(Reg.use[temp->rb]==-1) b=Reg.r[temp->rb];
				else if(Reg.use[temp->rb]==-2) b=Reg.dir_result[temp->rb];
				else if(Reg.use[temp->rb]==instid[3]) b=Reg.r[temp->rb];
				
				if(Reg.use[temp->rc]==-1) c=Reg.r[temp->rc];
				else if(Reg.use[temp->rc]==-2) c=Reg.dir_result[temp->rc];
				else if(Reg.use[temp->rc]==instid[3]) c=Reg.r[temp->rc];
				
				temp->result=b+c;
				Reg.use[temp->ra]=-2;
				Reg.dir_result[temp->ra]=temp->result;
				Reg.timestamp[temp->ra]=time; 
				break;
			}	
			case 4://sub
			{
				int b,c;///!!!!!!!!!到这里 
				if(Reg.use[temp->rb]==-1) b=Reg.r[temp->rb];
				else if(Reg.use[temp->rb]==-2) b=Reg.dir_result[temp->rb];
				//下面这个是为了防止sub ra,ra,rb的情况 
				else if(Reg.use[temp->rb]==instid[3]) b=Reg.r[temp->rb];
				
				if(Reg.use[temp->rc]==-1) c=Reg.r[temp->rc];
				else if(Reg.use[temp->rc]==-2) c=Reg.dir_result[temp->rc];
				else if(Reg.use[temp->rc]==instid[3]) c=Reg.r[temp->rc];
				
				temp->result=b-c;
				Reg.use[temp->ra]=-2;
				Reg.dir_result[temp->ra]=temp->result; 
				Reg.timestamp[temp->ra]=time;
				break;
			}		
			case 5://beqz
			{
				if((Reg.use[temp->ra]==-1&&!Reg.r[temp->ra])||(Reg.use[temp->ra]==-2&&!Reg.dir_result[temp->ra])) 
				{
					
					//PC+=temp->addr-2;//写回地址,这是符合常理的
					PC+=temp->addr-3;//之所以会多减一个，是因为我下文逻辑是PC+1 
					instid[1]=instid[2]=0;//清空流水线 
				}
			}
		}
		instid[4]=instid[3];
		instid[3]=-1;
	}
	else if(!instid[3])
	{
		instid[4]=0;
	}
}
int Pipline::ID()//返回值表示是否堵塞，如堵塞，则IF也应堵塞 
{
	if(instid[2]>0)
	{
		Instruction *temp=&Inst[instid[2]];
		int signal=0;
		switch(temp->inst)
		{
			case 0://load
			{
				if(Reg.use[temp->ra]!=-1)//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				}
				if(Reg.use[temp->rb]!=-1)//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) 
				{
					Reg.use[temp->ra]=instid[2];//写寄存器 
					Reg.timestamp[temp->ra]=time;
				}
				break;
			}
			case 1://store
			{ 
				if(Reg.use[temp->rb]!=-1)//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				break;
			}
			case 2://addi
			{
				if(Reg.use[temp->ra]!=-1)//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				}
				if(Reg.use[temp->rb]!=-1)//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) 
				{
					Reg.use[temp->ra]=instid[2];//写寄存器
					Reg.timestamp[temp->ra]=time;
				}
				break;
			}
			case 3://add
			case 4://sub
			{
				if(Reg.use[temp->ra]!=-1)//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				} 
				if(Reg.use[temp->rb]!=-1)//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(Reg.use[temp->rc]!=-1)//||Reg.timestamp[temp->rc]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) 
				{
					Reg.use[temp->ra]=instid[2];//写寄存器
					Reg.timestamp[temp->ra]=time;
				}
				break;
			}
			case 5://beqz
			{
				if(Reg.use[temp->ra]!=-1)//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				//if(!signal) Reg.use[temp->ra]=instid[2];//写寄存器
				break; 
			}
		}
		if(!signal) 	
		{
			instid[3]=instid[2];
			instid[2]=-1;
		}
		return signal;
	}
	else if(!instid[2])
	{
		instid[3]=0;
	}
	return 0;
}
int Pipline::ID_dir()//返回值表示是否堵塞，如堵塞，则IF也应堵塞 
{
	if(instid[2]>0)
	{
		Instruction *temp=&Inst[instid[2]];
		int signal=0;
		switch(temp->inst)
		{
			case 0://load
			{
				if((Reg.use[temp->ra]!=-1&&Reg.use[temp->ra]!=-2))//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				} 
				if((Reg.use[temp->rb]!=-1&&Reg.use[temp->rb]!=-2))//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) Reg.use[temp->ra]=instid[2];//写寄存器 
				break;
			}
			case 1://store
			{ 
				if((Reg.use[temp->rb]!=-1&&Reg.use[temp->rb]!=-2))//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				break;
			}
			case 2://addi
			{
				if((Reg.use[temp->ra]!=-1&&Reg.use[temp->ra]!=-2))//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				} 
				if((Reg.use[temp->rb]!=-1&&Reg.use[temp->rb]!=-2)||Reg.timestamp[temp->rb]==time)
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) Reg.use[temp->ra]=instid[2];//写寄存器
				break;
			}
			case 3://add
			case 4://sub
			{
				if((Reg.use[temp->ra]!=-1&&Reg.use[temp->ra]!=-2))//||Reg.timestamp[temp->ra]==time) 
				{
					signal=1;//写后写 
					num_WAW++;
				}  
				if((Reg.use[temp->rb]!=-1&&Reg.use[temp->rb]!=-2))//||Reg.timestamp[temp->rb]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if((Reg.use[temp->rc]!=-1&&Reg.use[temp->rc]!=-2))//||Reg.timestamp[temp->rc]==time) 
				{
					signal=1;//写后读
					num_RAW++;
				}
				if(!signal) Reg.use[temp->ra]=instid[2];//写寄存器
				break;
			}
			case 5://beqz
			{
				if((Reg.use[temp->ra]!=-1&&Reg.use[temp->ra]!=-2))//||Reg.timestamp[temp->ra]==time)
				{
					signal=1;//写后读
					num_RAW++;
				}
				//if(!signal) Reg.use[temp->ra]=instid[2];//写寄存器
				break; 
			}
		}
		if(!signal) 	
		{
			instid[3]=instid[2];
			instid[2]=-1;
		}
		return signal;
	}
	else if(!instid[2])
	{
		instid[3]=0;
	}
	return 0;
}
int Pipline::IF(int signal)
{
	int ans=0;
	if(signal) return 0; //堵塞，啥也不用干
	if(instid[1]>0) 
	{
		if(Inst[instid[1]].inst==6) //trap
			ans=1;
		instid[2]=instid[1];
		instid[1]=0; 
	}
	return ans;
}
int Pipline::deal_no_dir(int pc)
{
	//WB
	//Inst[instid[5]].
	int y=WB();
	MEM();
	EX();
	int signal=ID(),x;
	x=IF(signal);
	if(!x)//正常 
		instid[1]=pc;
	if(!y&&signal) return 2;//这个是为了阻塞的ID走到EX后，后一条指令覆盖IF 
	return y; 
}
int Pipline::deal_dir(int pc)
{
	//WB
	//Inst[instid[5]].
	int y=WB();
	MEM_dir();
	EX_dir();
	int signal=ID_dir(),x;
	x=IF(signal);
	if(!x)//正常 
		instid[1]=pc;
	if(!y&&signal) return 2;//这个是为了阻塞的ID走到EX后，后一条指令覆盖IF 
	//判断！y是保证当结束时不会因signal而继续执行 
	return y; 
}
struct Op{
	int break_list[10000];//1表示有断点
	int flag;//1表示有定向，0表示没有
	int run;//1表示跑整个程序，0表示单步执行（1优先级小于断点）
	Op()
	{
		memset(break_list,0,sizeof(break_list));
		flag=0;
		run=0;
	}
	void clear_break()
	{
		memset(break_list,0,sizeof(break_list));
	}
	void show_reg()
	{
		Reg.show();
	}
	void show_pipline()
	{
		pipline.show();
	}
	void show_timeline();
	void show_statistic()
	{
		cout<<"写后写冲突周期数:"<<pipline.num_WAW<<endl;
		cout<<"写后读冲突周期数:"<<pipline.num_RAW<<endl;
	}
	void show_config();
	void change(int x);//x为时钟周期，方便显示 
};
/*
string to_string(int x)
{
	char temp[100]={0};
	int i=0;
	while(x)
	{
		t
	}
}
*/
void mycout(string str,int len)
{
	cout<<str;
	int i=len-str.size();
	for(;i;i--)cout<<" ";
}
void Op::show_timeline()
{
	int table[100][100]={0};
	int ins[100];
	int i,j=1,k,stage;
	for(i=0;i<time;i++)
	{
		if(i==0||(timeline[i][1]>0&&timeline[i][1]!=timeline[i-1][1])) 
		{
			ins[j]=timeline[i][1];
			k=i+1;
			stage=1;
			table[j][i]=1;
			while(1)
			{
				if(stage==5) break; 
				if(timeline[k][stage+1]==ins[j])//进入了下一阶段 
				{
					table[j][k]=stage+1;
					stage++; 
				}
				else if(timeline[k][stage]!=ins[j])//没救了，肯定没了
				{
					break;
				} 
				else if(timeline[k][stage]==-1)
				{
					table[j][k]=-1;
				}
				k++;
			}
			j++;
		}
	}
	int all_inst=j;
	//cout<<"Inst			";
	for(i=1;i<time;i++) cout<<setw(6)<<setiosflags(ios::left)<<i;//cout<<i<<"	";
	cout<<"	Inst";
	cout<<endl;
	for(i=1;i<all_inst;i++)
	{
		
		for(j=1;j<time;j++)
		{
			if(table[i][j-1]!=0) //cout<<setw(5)<<setiosflags(ios::left)<<table[i][j-1];
			{
				switch(table[i][j-1])
				{
					case -1:mycout("stall",6);break;
					case 1:mycout("IF",6);break;
					case 2:mycout("ID",6);break;
					case 3:mycout("EX",6);break;
					case 4:mycout("MEM",6);break;
					case 5:mycout("WB",6);break;
				}
			}
			else if(table[i][j-1]==0) mycout("",6);//cout<<"		";
		}
		Inst[ins[i]].show();
		cout<<endl;
	 } 
	/* 
	for(i=1;i<all_inst;i++)
	{
		Inst[ins[i]].show();
		cout<<"	";
		for(j=1;j<time;j++)
		{
			if(table[i][j]>0)
			{
				switch(table[i][j])
				{
					case 1:cout<<"IR	";break;
					case 2:cout<<"ID	";break;
					case 3:cout<<"EX	";break;
					case 4:cout<<"MEM	";break;
					case 5:cout<<"WB	";
				}
			}
			else if(table[i][j]==-1)
			{
				cout<<"stall	";
			}
			else
			{
				cout<<"		";
			}
		}
		cout<<endl;
	}*/ 
}
void Op::show_config()
{
	cout<<"断点:"<<endl;
	for(int i=1;i<=all;i++)
	{
		if(break_list[i])
		{
			cout<<"		";
			Inst[i].show();
			cout<<endl;
		}
	}
	if(flag==1) cout<<"有定向功能";
	else cout<<"无定向功能";
	cout<<endl;
	if(run==1) cout<<"跑完整个程序";
	else cout<<"单步执行";
	cout<<endl;
}
int strtoint(string str)
{
	int ans=0,i;
	for(i=0;i<str.size();i++)
	{
		ans*=10;
		ans+=str[i]-'0';
	}
	return ans;
}
void Op::change(int x)
{
	string str,temp;
	int number;
	while(1)
	{
		cout<<"[OP:T"<<x<<"]# ";
		str="";
		cin>>str;
		if(str=="run")
		{
			cin>>temp;
			number=strtoint(temp);
			if(number==1) run=1;
			else if(number==0)run=0;
		}
		else if(str=="clear_break")
		{
			clear_break();
		}
		else if(str=="break")
		{
			cin>>temp;
			number=strtoint(temp);
			break_list[number]=1;
		}
		else if(str=="clear")
		{
			cin>>temp;
			number=strtoint(temp);
			break_list[number]=0;
		} 
		else if(str=="show")
		{
			cin>>temp;
			if(temp=="reg") show_reg();
			else if(temp=="pipline") show_pipline();
			else if(temp=="config") show_config();
			else if(temp=="statistic") show_statistic(); 
			else if(temp=="timeline")	show_timeline();
			else if(temp=="all")	
			{
				for(int i=1;i<time;i++)
				{
					for(int j=1;j<=5;j++)
						cout<<timeline[i][j]<<" ";
					cout<<endl;
				}
			}
		}
		else if(str=="direction")
		{
			cin>>temp;
			number=strtoint(temp);
			if(number==1) flag=1;
			else if(number==0) flag=0;
		}
		else if(str=="exit"||str=="n")
		{
			return;
		}
		else if(str=="help")
		{
			cout<<"run [num]:如果num=1,表示运行整个程序；num=0表示单步执行，默认为单步执行"<<endl;
			cout<<"clear_break:表示删除所有断点"<<endl;
			cout<<"break [num]:设置断点，num表示设置第几行代码为断点，以1开始，同时到该代码IF段就停止"<<endl;
			cout<<"clear [num]:清除断点，num表示清除第几行代码的断点"<<endl;
			cout<<"show reg:表示显示寄存器表"<<endl;
			cout<<"show pipline:显示流水线各个段状态"<<endl;
			cout<<"show config:显示目前配置"<<endl;
			cout<<"show statistic:显示统计数据"<<endl;
			cout<<"show timeline:显示目前所有代码的段表"<<endl;
			cout<<"direction [num]:num=1表示开启定向，num=0表示关闭定向，默认为关闭"<<endl;
			cout<<"exit/n:下一步"<<endl;
			cout<<"help:就像它的英文名一样"<<endl;
		}	
		//cout<<endl;
	}
} 
void translate(string str,int t)
{
	int ans=0,i;
	for(i=0;i<str.size();i++)
	{
		ans*=2;
		ans+=str[i]-'0';
	}
	Inst[t].inst=ans>>26;
	Inst[t].ra=(ans>>21)&31;
	Inst[t].rb=(ans>>16)&31;
	Inst[t].rc=(ans>>11)&31;
	Inst[t].number=ans&65535;//2^16-1
	if(Inst[t].number&32768) 
	{
		Inst[t].number=65536-Inst[t].number;
	}
	Inst[t].addr=ans&2097151;//2^21-1
	if(Inst[t].addr&1048576) 
	{
		Inst[t].addr=2097152-Inst[t].addr;
	}
}
int main()
{
	//freopen("rubbish.txt","r",stdin);
	//string inst2="-1";//2进制指令 
	ifstream in("rubbish.txt");
	//freopen("rubbish.txt","r",in);
	//
	//string filename;
	string line;
	int i=1;
	if(in) // 有该文件
	{
		while (getline (in, line)) // line中不包括每行的换行符
		{ 
			translate(line,i);
			i++;
		}
		all=i-1;//总行数 
	}
	else // 没有该文件
	{
		cout <<"no such file" << endl;
		return 0;
	}
	PC=0;
	Op op;
	op.change(0);
	int x;
 	while(1)
 	{
 		PC++;
 		time++;
 		pipline.instid[1]=PC;
 		for(int k=1;k<=5;k++)
 			timeline[time-1][k]=pipline.instid[k];
 		if(op.break_list[PC]||!op.run)
 		{
 			op.change(time);
		}
		if(op.flag==0)
 			x=pipline.deal_no_dir(PC);
 		else if(op.flag==1)
 			x=pipline.deal_dir(PC);
 		
 		if(x==1) break;//x==1表示trap到WB了，该结束了 
 		else if(x==2) PC--;//这个是为了保持同一条指令，但我不知道如果beqz怎么办，不过这么做似乎没错。 
	}
	time++;
	op.change(time);
	return 0;	
}
