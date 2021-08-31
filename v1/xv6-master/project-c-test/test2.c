#include "types.h"
#include "stat.h"
#include "user.h"

#include "fs.h"

int main(int argc, char** argv)
{
	int x = uptime();
	
	printf(1, "Current uptime: %d\n", x);	
	
	if( (x % 10) == 0 ) {
		printf(1, "You win!\n");
	} else {
		printf(1, "You lose...\n");
	}
	exit();
}