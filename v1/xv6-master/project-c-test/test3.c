#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

int main(int argc, char** argv)
{
	int x = getpid();
	
	printf(1, "Current PID: %d\n", x);	
	
	if( (x*uptime() % 10) == 0 ) {
		printf(1, "You win!\n");
	} else {
		printf(1, "You lose...\n");
	}
	exit();
}
