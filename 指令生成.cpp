#include<iostream>
#include<cstring>
using namespace std;
/* 
ָ��32λ���Ĵ���32����mem

ָ��32λ��ǰ6λ���ֲ�ָͬ�
load ra, num(rb) ->  ra 5λ  rb 5λ  num 16λ
EX��num+(rb),MEM��(num+(rb)) 
store num(rb) ,ra -> ra 5λ  rb 5λ  num 16λ
EX��num+(rb),MEM��(num+(rb)) 
addi ra,rb,num ->ra 5λ  rb 5λ  num 16λ
add  ra,rb,rc ->ra 5λ  rb 5λ  rc 5λ
sub  ra,rb,rc ->ra 5λ  rb 5λ  rc 5λ
beqz  ra ,addr -> ra 5λ addr ��Ե�ַ 21λ
EX������ַ��д�� 
trap �������ϵͳ��ָ�� 
IFʶ�������ban����ˮ���Ժ�ģ�WBִ����ͽ��� 
*/
int out[100];
void deal(int len,int x)
{
	int b[100]={0},i=0;
	while(x)
	{
		b[i]=x%2;
		x/=2;
		i++;
	}
	for(i=len-1;i>=0;i--)
	{
		cout<<b[i];
	}
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
int main()
{
	string str,temp1,temp2,temp3;
	int ra,rb,rc,number,addr;
	while(1)
	{
		cin>>str;
		if(str=="load")
		{
			cin>>temp1>>temp2>>temp3;//ra,rb,num
			ra=strtoint(temp1);
			rb=strtoint(temp2);
			number=strtoint(temp3);
			deal(6,0);
			deal(5,ra);
			deal(5,rb);
			deal(16,number);
		}
		else if(str=="store")
		{
			cin>>temp1>>temp2>>temp3;//ra,rb,num
			ra=strtoint(temp1);
			rb=strtoint(temp2);
			number=strtoint(temp3);
			deal(6,1);
			deal(5,ra);
			deal(5,rb);
			deal(16,number);
		}
		else if(str=="addi")
		{
			cin>>temp1>>temp2>>temp3;//ra,rb,num
			ra=strtoint(temp1);
			rb=strtoint(temp2);
			number=strtoint(temp3);
			deal(6,2);
			deal(5,ra);
			deal(5,rb);
			deal(16,number);
		}
		else if(str=="add")
		{
			cin>>temp1>>temp2>>temp3;//ra,rb,num
			ra=strtoint(temp1);
			rb=strtoint(temp2);
			rc=strtoint(temp3);
			deal(6,3);
			deal(5,ra);
			deal(5,rb);
			deal(5,rc);
			deal(11,0);
		}
		else if(str=="sub")
		{
			cin>>temp1>>temp2>>temp3;//ra,rb,num
			ra=strtoint(temp1);
			rb=strtoint(temp2);
			rc=strtoint(temp3);
			deal(6,4);
			deal(5,ra);
			deal(5,rb);
			deal(5,rc);
			deal(11,0);
		}
		else if(str=="beqz")
		{
			cin>>temp1>>temp2;//ra,rb,num
			ra=strtoint(temp1);
			addr=strtoint(temp2);
			deal(6,5);
			deal(5,ra);
			deal(21,addr);
		}
		else if(str=="trap")
		{
			deal(6,6);
			deal(26,0);
		}
		cout<<endl;
	}
}
