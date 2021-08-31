#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

int main(int argc, char** argv)
{
	char dirname[20] = { 'd', 'i', 'r', 0, };
	int x = uptime();
	
	dirname[3] = 'a' + x % 20;
	dirname[4] = 'A' + x % 21;
	dirname[5] = 'a' + x % 22;
	dirname[6] = 'A' + x % 23;
	
	
	//printf(1, "folder name: %s\n", dirname);	
	mkdir(dirname);
	
	exit();
}