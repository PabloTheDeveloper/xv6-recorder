#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(int argc, char *argv[]) {
	int success = 0;
	// record 
	record_start("./rr1.txt");
	int recorded = mkdir("some_dir");
	int recorded1 = chdir("some_dir");
	record_stop();

	// replay
	replay_start("./rr1.txt");
	int replayed = mkdir("some_dir");
	int replayed1 = chdir("some_dir");
	replay_stop();
	
	if (recorded != replayed)
	{
		printf(1, "TEST MESSAGE: Mismatch int output for record and replay mkdir\n");
		exit();
	}
	if (recorded1 != replayed1)
	{
		printf(1, "TEST MESSAGE: Mismatch int output for record and replay mkdir\n");
		exit();
	}
	// passes!
	success = 1;
	printf(1, "past test 1: %d", success);

	// should fail
	printf(1, "\n\nerr is suppose to below here\n\n");
	replay_start("rr1.txt");
	mkdir("some_wrong_dir");
	replay_stop();
	printf(1, "failed test 2");
	exit();
}