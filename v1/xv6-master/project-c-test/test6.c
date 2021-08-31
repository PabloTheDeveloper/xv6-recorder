#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

int main(int argc, char** argv)
{
	int fd, ret1, ret2;
	fd = open("file.txt", 0);
	ret1 = close(fd);
	ret2 = close(fd);	
	
	exit();
}
