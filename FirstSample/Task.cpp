#include <chrono>
#include "Task.h"

#include "Protocol.h"
#include <conio.h>

void rc17::Correct()
{
	
	bool readyToShoot = true;
	bool LastHadBall = false;
	while(ThreadFlag::run)
	{
		if(ThreadFlag::t_Num == true)
		{
			if (PillarState::hasBall() == true)
			{
				cout << "ball" << endl;
				readyToShoot = PillarState::lockPillar(PillarState::WithBall);
			}
			else
			{
				readyToShoot = PillarState::lockPillar(PillarState::NoBall);
			}

			if (readyToShoot == true && ThreadFlag::t_Num != 2)//发射状态,非云台调整状态
			{
				if (PillarState::hasBall() == true)
					Protocol::sendCmd(Protocol::BallPara);
				else
					Protocol::sendCmd(Protocol::NoBallPara);
				this_thread::sleep_for(chrono::milliseconds(50));
				Protocol::sendCmd(Protocol::Shoot);

				if (PillarState::hasBall() == true)
					this_thread::sleep_for(chrono::milliseconds(2000));//打球的延时大一些
				else
					this_thread::sleep_for(chrono::milliseconds(1300));//等一发飞盘发射完毕
				continue;
			}
		}
		//500ms 执行一次
		this_thread::sleep_for(chrono::milliseconds(500));
	}
}

void rc17::scanfKey()
{
	while (rc17::ThreadFlag::run == 0);
	while (rc17::ThreadFlag::run)
	{
		int key = getch();
		if (key == 'w')
		{
			try
			{
				std::ofstream datafile;
				datafile.open("C:\\Users\\robocon2017\\Desktop\\datafile.txt", std::ios::app);
				datafile << "飞盘相比得到的偏差偏后" << endl;
				cout << "检测到按键w 已向文件写入 飞盘相比得到的偏差偏后！" << endl;
				datafile.close();
			}
			catch (...)
			{
				cout << "文件写入失败， 也许文件被占用.";
			}
		}
		if (key == 'a')
		{	
			try
			{
				std::ofstream datafile;
				datafile.open("C:\\Users\\robocon2017\\Desktop\\datafile.txt", std::ios::app);
				datafile << "飞盘相比得到的偏差偏左" << endl;
				cout << "检测到按键a 已向文件写入 飞盘相比得到的偏差偏左！" << endl;
				datafile.close();
			}
			catch (...)
			{
				cout << "文件写入失败， 也许文件被占用.";
			}
		}
		if (key == 's')
		{
			try
			{
				std::ofstream datafile;
				datafile.open("C:\\Users\\robocon2017\\Desktop\\datafile.txt", std::ios::app);
				datafile << "飞盘相比得到的偏差偏前" << endl;
				cout << "检测到按键s 已向文件写入 飞盘相比得到的偏差偏前！" << endl;
				datafile.close();
			}
			catch (...)
			{
				cout << "文件写入失败， 也许文件被占用.";
			}
		}
		if (key == 'd')
		{
			try
			{
				std::ofstream datafile;
				datafile.open("C:\\Users\\robocon2017\\Desktop\\datafile.txt", std::ios::app);
				datafile << "飞盘相比得到的偏差偏右" << endl;
				cout << "检测到按键d 已向文件写入 飞盘相比得到的偏差偏右！" << endl;
				datafile.close();
			}
			catch (...)
			{
				cout << "文件写入失败， 也许文件被占用.";
			}
		}
	}
}

void rc17::coutDatas()
{
	cout << "ctrl + 1:under pan pos" << endl;
	cout << "ctrl + 2:" << endl;
	cout << "ctrl + 3:" << endl;
	cout << "ctrl + 4:" << endl;
	bool keyState[4], flags[4] = { 0 };
	while (rc17::ThreadFlag::run)
	{
		while (GetKeyState(VK_CONTROL) < 0)//ctrl按下
		{
			for (int i = 0; i < sizeof(keyState); i++)
				if(GetKeyState(i + 48) >= 0)
					keyState[i] = GetKeyState(i + 48);
			Sleep(30);
			for (int i = 0; i < sizeof(keyState); i++)
				if (GetKeyState(i + 48) >= 0 && !(GetKeyState(i + 48) == keyState[i]))
				{
					flags[i] = !flags[i];
					keyState[i] = !keyState[i];
				}
		}
		Sleep(200);
	}
}
