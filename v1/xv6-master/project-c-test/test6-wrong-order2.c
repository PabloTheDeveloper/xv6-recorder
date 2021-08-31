#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

int main(int argc, char** argv)
{
	int fd, ret1, ret2;
	ret1 = close(fd);
	fd = open("file2.txt", 0);
	ret2 = close(fd);	
	
	exit();
}
