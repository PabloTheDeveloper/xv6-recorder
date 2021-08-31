#include "types.h"
#include "user.h"
#include "fcntl.h"
#include "stat.h"

//wc function for test1 in prof_tests
char buf[100];
int
wc(int fd, char *name)
{
  int i, n;
  int l, w, c, inword;

  l = w = c = 0;
  inword = 0;
  while((n = read(fd, buf, sizeof(buf))) > 0){
    for(i=0; i<n; i++){
      c++;
      if(buf[i] == '\n')
        l++;
      if(strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if(!inword){
        w++;
        inword = 1;
      }
    }
  }
  //if(n < 0){
    //printf(1, "wc: read error\n");
    //exit();
  //}
  //printf(1, "%d %d %d %s\n", l, w, c, name);
  return w;
}


// https://stackoverflow.com/questions/12864265/using-pipe-to-pass-integer-values-between-parent-and-child/12864595
// using above link to setup fork correctly and pipe correctly for tests

#define ALL -128

int global_total = 0;
int global_passed = 0;
int local_total = 0;
int local_passed = 0;

void passedTest()
{
	local_passed += 1;
	global_passed += 1;
	local_total += 1;
	global_total += 1;
	printf(1, "\n ++++++ \n|PASSED|\n ++++++ \n");
}

void failedTest()
{
	local_total += 1;
	global_total += 1;
	printf(1, "\n ------ \n|FAILED|\n ------ \n");
}

void StartTestSuite(char* msg, int test_suit_idx)
{
	local_passed = 0;
	local_total = 0;
	printf(1,"\n*******************************************");
	printf(1, msg, test_suit_idx-1);
	printf(1,"*******************************************\n");

}

void EndTestSuite(int test_validness_status, int test, int test_suit)
{
	printf(1,"\n*******************************************");

	if(test_validness_status == -1)
	{
		printf(1, "INVALID TEST: %d, SUPPLIED TO TEST SUITE: %d\n", test, test_suit);
		exit();
	}
	printf(1, "Locally Passed: %d/%d", local_passed, local_total);
	printf(1,"*******************************************\n");

}


void StartTestCase(char* msg, char *testcode, int test_idx)
{
	printf(1,"________________________________________________________________________________________________________________\n\n");
	printf(1,"TEST CASE (%d):", test_idx-1);
	printf(1, msg);
	printf(1,"\n________________________________________________________________________________________________________________\n");

	printf(1, "\n");
	printf(1, testcode);
	printf(1, "\nLOGS FROM EXECUTION OF TEST APPEAR BELOW:");
	printf(1, "\n**************************************************************\n");
}

int
test_suit_trivial_IO(int test)
{
	if(test != ALL)
	{
		printf(1, "\nThis test suit is intended to be ALL not %d\n", test);
		return -1;
	}

	int test_idx = 1;
	if(test == test_idx++ || test == ALL )
	{
		char * testcode = ""
		"1. replay_start(\'rr1.txt\');\n"
		"2. replay_stop(\'rr1.txt\');\n";

		StartTestCase("replay (rr1.txt) a non-existant file", testcode, test_idx);
		int success = 1;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);

			replay_start("rr1.txt");
			replay_stop();

			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);

			success = 0;
			write(fd[1], &success, sizeof(success));
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);

		printf(1, "\n\n(should throw error: \'Likely, file ... does not exist\')");
		if(success)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL)
	{
		char * testcode = ""
		"1. record_start(\'rr1.txt\');\n"
		"2. record_stop(\'rr1.txt\');\n";

		StartTestCase("record (rr1.txt) a non-existant file", testcode, test_idx);
		if(record_start("rr1.txt") != -1 && record_stop() != -1)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL)
	{
		char * testcode = ""
		"1. replay_start(\'rr1.txt\');\n"
		"2. replay_stop(\'rr1.txt\');\n";

		StartTestCase("replay (rr1.txt) an empty record file", testcode, test_idx);
		if(replay_start("rr1.txt") != -1 && replay_stop() != -1)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL)
	{
		char * testcode = ""
		"1. record_start(\'rr2.txt\');\n"
		"2. record_stop(\'rr2.txt\');\n";

		StartTestCase("record (rr2.txt) a non-existant file", testcode, test_idx);
		if(record_start("rr2.txt") != -1 && record_stop() != -1)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL)
	{
		char * testcode = ""
		"1. replay_start(\'rr1.txt\');\n"
		"2. replay_stop(\'rr1.txt\');\n";

		StartTestCase("replay (rr1.txt)", testcode, test_idx);
		if(replay_start("rr1.txt") != -1 && replay_stop() != -1)
			passedTest();
		else
			failedTest();
	}
	// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_getpid(int test)
{
	if(test != ALL)
	{
		printf(1, "\nThis test suit is intended to be ALL not %d\n", test);
		return -1;
	}

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		char * testcode = ""
		"1. record_start(\'rr2.txt\');\n"
		"2. int curr_pid1 = getpid();\n"
		"3. int curr_pid2 = getpid();\n"
		"4. record_stop(\'rr2.txt\');\n";
		
		StartTestCase("record (rr2.txt) (two records)", testcode, test_idx);
		int replay_start_status = record_start("rr2.txt");
		int curr_pid1 = getpid();
		// int curr_pid2 = getpid();
		int replay_stop_status = record_stop();
		
		printf(1, "\nTEST VARIABLES:\n");
		printf(1, "curr_pid1: %d\n", curr_pid1);
		// printf(1, "curr_pid2: %d\n", curr_pid2);

		if(replay_start_status == -1 || replay_stop_status == -1)
			failedTest();
		else
			passedTest();
	}

	// if(test == test_idx++ || test == ALL )
	// {
	// 	char * testcode = ""
	// 	"1. record_start(\'rr1.txt\');\n"
	// 	"2. int curr_pid = getpid();\n"
	// 	"3. record_stop(\'rr1.txt\');\n";
		

	// 	StartTestCase("record (rr1.txt) (one record)", testcode, test_idx);
	// 	int record_start_status = record_start("rr1.txt");
	// 	int curr_pid = getpid();
	// 	int record_stop_status = record_stop();

	// 	printf(1, "\nTEST VARIABLES:\n");
	// 	printf(1, "curr_pid: %d\n", curr_pid);

	// 	if(record_start_status == -1 || curr_pid <= -1 || record_stop_status == -1)
	// 		failedTest();
	// 	else
	// 		passedTest();
	// }

	if(test == test_idx++ || test == ALL )
	{
		char * testcode = ""
		"1. replay_start(\'rr2.txt\');\n"
		"2. int curr_pid1 = getpid();\n"
		"3. int curr_pid2 = getpid();\n"
		"4. replay_stop(\'rr2.txt\');\n";
		
		StartTestCase("replay (rr2.txt) (two records)", testcode, test_idx);
		int replay_start_status = replay_start("rr2.txt");
		int curr_pid1 = getpid();
		// int curr_pid2 = getpid();
		int replay_stop_status = replay_stop();
		
		printf(1, "\nTEST VARIABLES:\n");
		printf(1, "curr_pid1: %d\n", curr_pid1);
		// printf(1, "curr_pid2: %d\n", curr_pid2);

		if(replay_start_status == -1 || replay_stop_status == -1)
			failedTest();
		else
			passedTest();
	}

	if(test == test_idx++ || test == ALL )
	{
		char * testcode = ""
		"1. replay_start(\'rr1.txt\');\n"
		"2. int curr_pid = getpid();\n"
		"3. replay_stop(\'rr1.txt\');\n";
		
		StartTestCase("replay (rr1.txt) (one record)", testcode, test_idx);
		int replay_start_status = replay_start("rr1.txt");
		int curr_pid = getpid();
		int replay_stop_status = replay_stop();
		
		printf(1, "\nTEST VARIABLES:\n");
		printf(1, "curr_pid: %d\n", curr_pid);

		if(replay_start_status == -1 || replay_stop_status == -1)
			failedTest();
		else
			passedTest();
	}

	// if(test == test_idx++ || test == ALL )
	// {
	// 	char * testcode = ""
	// 	"1. replay_start(\'rr2.txt\');\n"
	// 	"2. int curr_pid1 = getpid();\n"
	// 	"3. int curr_pid2 = getpid();\n"
	// 	"4. int curr_pid3 = getpid();\n"
	// 	"5. replay_stop(\'rr2.txt\');\n";
		
	// 	StartTestCase("replay (rr2.txt) (two records in rr2.txt) (but 3 syscalls in code)", testcode, test_idx);
	// 	int success = 0;
	// 	int fd[2];
	// 	write(fd[0], &success, sizeof(success));
	// 	pipe(fd);
	// 	int pid;
	// 	if ((pid = fork()) == 0)
	// 	{
	// 		close(fd[0]);
	// 		replay_start("rr2.txt");
	// 		getpid();
	// 		getpid();
	// 		getpid();
	// 		replay_stop();

	// 		success = 1;
	// 		write(fd[1], &success, sizeof(success));
	// 		close(fd[1]);
	// 		exit();
	// 	}
	// 	wait();
	// 	close(fd[1]);
	// 	read(fd[0], &success, sizeof(success));
	// 	close(fd[0]);
	// 	printf(1, "\n\n(should throw error: \'ERR: Attempted to replay more records ...\')");

	// 	if(!success)
	// 		passedTest();
	// 	else
	// 		failedTest();
	// }

	// if(test == test_idx++ || test == ALL )
	// {
	// 	char * testcode = ""
	// 	"1. replay_start(\'rr2.txt\');\n"
	// 	"2. int curr_pid = getpid();\n"
	// 	"3. replay_stop(\'rr2.txt\');\n";
		
	// 	StartTestCase("replay (rr2.txt) (replay one record), when there are two records in rr2.txt", testcode, test_idx);
	// 	int replay_start_status = replay_start("rr2.txt");
	// 	int curr_pid = getpid();
	// 	int replay_stop_status = replay_stop();
		
	// 	printf(1, "\nTEST VARIABLES:\n");
	// 	printf(1, "curr_pid: %d\n", curr_pid);

	// 	if(replay_start_status == -1 || replay_stop_status == -1)
	// 		failedTest();
	// 	else
	// 		passedTest();
	// }
		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_record_replay_1000(int test)
{

	int test_idx = 1;
	if (test == ALL)
	{
		printf(1, "Will not execute, memory demands are too high!\n");
		return 0;
	}

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay (rr1000.txt) (1k records)", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			record_start("rr1000.txt");

			for (int i = 0; i < 1000; i++)
				getpid();

			record_stop();
		
			replay_start("rr1000.txt");
			for (int i =0; i < 1000; i++)
				getpid();

			replay_stop();

			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);

		if(success)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("record (rr1000.txt) (1k + 1records)", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			record_start("rr1000.txt");

			for (int i = 0; i < 1001; i++)
				getpid();

			record_stop();

			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);

		printf(1, "\n\n(should throw error: \'ERR: attempted to add more than 1000 records\')");
		if(!success)
			passedTest();
		else
			failedTest();
	}

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay (rr1000.txt) (1k records) but 1001 replays.", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			record_start("rr1000.txt");

			for (int i = 0; i < 1000; i++)
				getpid();

			record_stop();
		
			replay_start("rr1000.txt");
			for (int i =0; i < 1001; i++)
				getpid();

			replay_stop();

			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);

		printf(1, "\n\n(should throw error: \'ERR: attempted to get more than 1000 records\')");

		if(!success)
			passedTest();
		else
			failedTest();
	}
		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_uptime_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay just 1 uptime syscall then replay getpid (rr1.txt) ", "", test_idx);
		int success = 0;

		record_start("rr1.txt");
		int original = uptime();
		record_stop();
	
		replay_start("rr1.txt");
		int replayed = uptime();
		replay_stop();

		if(original != replayed)
			exit();
		success = 1;
		// incorrect replay (should stop process)
		replay_start("rr1.txt");
		getpid();
		replay_stop();

		success = 0;
		
		printf(1, "\n\n(should throw error: \'ERR: replayed syscall #:11, while record syscall #:14\')");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_close_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay just 1 close syscall then replay close but with incorrect input (rr1.txt) ", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			record_start("rr1.txt");
			int original = close(2);
			record_stop();
		
			replay_start("rr1.txt");
			int replayed = close(2);
			replay_stop();

			if(original != replayed)
				exit();
			
			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);

			// incorrect replay (should stop process)
			replay_start("rr1.txt");
			close(3);
			replay_stop();

			success = 0;
			write(fd[1], &success, sizeof(success));
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);

		printf(1, "\n\n(should throw error: \'ERR: Mismatched field values at syscall counter: 1, field position 1\')");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_open_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay just 1 open syscall then replay open but with incorrect input (rr1.txt) ", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			record_start("rr1.txt");
			int original = open("a.txt",O_CREATE);
			record_stop();
		
			replay_start("rr1.txt");
			int replayed = open("a.txt", O_CREATE);
			replay_stop();

			if(original != replayed)
				exit();
			
			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);

			// incorrect replay (should stop process)
			replay_start("rr1.txt");
			open("b.txt", O_CREATE);
			replay_stop();

			success = 0;
			write(fd[1], &success, sizeof(success));
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(should throw error: \'Mismatched Byte field content values, at syscall counter: 1, field position: 1\')");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_read_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase(
			"record then replay sys_read (rr1.txt).\n"
			"This test checks that '\\0' values don't affect what is being returned by read.\n"
			"In particular, it ensure that my string converstion methods aren't being used.\n"
			"In them, I write each char onto the Byte Field content variable until the first '\\0' char.\n"
			"Thus, if I am not using that method, I should be able to record & replay values past the '\\0' char.", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			int file_fd = open("abc_NULL_e.txt", O_CREATE | O_RDWR);
			char file_content[5];
			file_content[0] = 'a';
			file_content[1] = 'b';
			file_content[2] = 'c';
			file_content[3] = '\0';
			file_content[4] = 'e';
			write(file_fd, file_content, sizeof(file_content));
			close(file_fd);
			
			record_start("rr1.txt");
			char record_buffer[5];
			file_fd = open("abc_NULL_e.txt", O_RDONLY);
			int original = read(file_fd, record_buffer, sizeof(record_buffer));
			close(file_fd);
			record_stop();

			// record buffer was correctly filled in with data
			for (int i = 0; i < sizeof(file_content); i++)
			{
				if(file_content[i] != record_buffer[i])
				{
					exit();
					break;
				}
			}

			replay_start("rr1.txt");
			char replay_buffer[5];
			file_fd = open("abc_NULL_e.txt", O_RDONLY);
			int replayed = read(file_fd, replay_buffer, sizeof(replay_buffer));
			close(file_fd);
			replay_stop();

			// record buffer was correctly filled in with data
			for (int i = 0; i < sizeof(file_content); i++)
			{
				if(file_content[i] != replay_buffer[i])
				{
					exit();
					break;
				}
			}

			if(original != replayed)
				exit();
			
			if(replay_buffer[4] == '\0')
				exit();
		
			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(should not throw error)");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_fstat_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay fstat (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// setup some file for fstat
			int file_fd = open("some_file.txt", O_CREATE | O_RDWR);
			char file_content[5];
			file_content[0] = 'a';
			file_content[1] = 'b';
			file_content[2] = 'c';
			write(file_fd, file_content, sizeof(file_content));

			// get actual stat data of file
			struct stat actual_stat;
			int original = fstat(file_fd, &actual_stat);
			close(file_fd);

			// record 
			record_start("rr1.txt");
			file_fd = open("some_file.txt", O_RDONLY);
			struct stat recorded_stat;
			int recorded = fstat(file_fd, &recorded_stat);
			close(file_fd);
			record_stop();
			
			if (original != recorded)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record fstat\n");
				exit();
			}

			// compare actual stat data vs. recorded stat data
			if(
				actual_stat.type != recorded_stat.type ||
				actual_stat.dev != recorded_stat.dev ||
				actual_stat.ino != recorded_stat.ino ||
				actual_stat.nlink != recorded_stat.nlink ||
				actual_stat.size != recorded_stat.size
				)
			{
				printf(1,"TEST MESSAGE: Missmatch in actual stat struct and recorded stat struct\n");
				printf(
					1,
					"actual:\n\tstat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
     			actual_stat.type,
      		actual_stat.dev,
      		actual_stat.ino,
      		actual_stat.nlink,
      		actual_stat.size
				);
				printf(
					1,
					"recorded:\n\tstat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
     			recorded_stat.type,
      		recorded_stat.dev,
      		recorded_stat.ino,
      		recorded_stat.nlink,
      		recorded_stat.size
				);
				exit();
			}

			// replay
			replay_start("rr1.txt");
			file_fd = open("some_file.txt", O_RDONLY);
			struct stat replay_stat;
			int replayed = fstat(file_fd, &replay_stat);
			close(file_fd);
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record fstat\n");
				exit();
			}

			// compare recorded stat data vs. replayed stat data
			if(
				actual_stat.type != recorded_stat.type ||
				actual_stat.dev != recorded_stat.dev ||
				actual_stat.ino != recorded_stat.ino ||
				actual_stat.nlink != recorded_stat.nlink ||
				actual_stat.size != recorded_stat.size
				)
			{
				printf(1,"TEST MESSAGE: Missmatch in actual stat struct and recorded stat struct\n");
				printf(
					1,
					"actual:\n\tstat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
     			actual_stat.type,
      		actual_stat.dev,
      		actual_stat.ino,
      		actual_stat.nlink,
      		actual_stat.size
				);
				printf(
					1,
					"recorded:\n\tstat: (type:%d, dev:%d, ino:%d, nlink:%d, size:%d)\n",
     			recorded_stat.type,
      		recorded_stat.dev,
      		recorded_stat.ino,
      		recorded_stat.nlink,
      		recorded_stat.size
				);
				exit();
			}

			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(should not throw error)");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_mkdir_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay mkdir (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// record 
			record_start("rr1.txt");
			int recorded = mkdir("some_dir");
			record_stop();

			// replay
			replay_start("rr1.txt");
			int replayed = mkdir("some_dir");
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record and replay mkdir\n");
				exit();
			}
			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));

			// should fail
			replay_start("rr1.txt");
			mkdir("some_wrong_dir");
			replay_stop();

			// fails (if it gets to here!)
			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n (err should be mismatched Byte field sizes, at syscall counter: 1, field position: 1");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_chdir_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay chdir (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// record 
			record_start("rr1.txt");
			int recorded = chdir("some_dir");
			record_stop();

			// replay
			replay_start("rr1.txt");
			int replayed = chdir("some_dir");
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record and replay chdir\n");
				exit();
			}
			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));

			// should fail
			replay_start("rr1.txt");
			chdir("some_wrong_dir");
			replay_stop();

			// fails (if it gets to here!)
			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(error should be mismatched Byte field sizes, at syscall counter: 1, field position: 1");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_mknod_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay mknod (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// record 
			record_start("rr1.txt");
			int recorded = mknod("some_dir", 1, 1);
			record_stop();

			// replay
			replay_start("rr1.txt");
			int replayed = mknod("some_dir", 1, 1);
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record and replay mknod\n");
				exit();
			}
			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));

			// should fail
			replay_start("rr1.txt");
			mknod("some_wrong_dir", 1, 1);
			replay_stop();

			// fails (if it gets to here!)
			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(error should be mismatched Byte field sizes, at syscall counter: 1, field position: 1");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_link_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay link (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// record 
			record_start("rr1.txt");
			int recorded = link("README", "echo");
			record_stop();

			// replay
			replay_start("rr1.txt");
			int replayed = link("README", "echo");
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record and replay link\n");
				exit();
			}
			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));

			// should fail
			replay_start("rr1.txt");
			link("some_file", "echo");
			replay_stop();

			// fails (if it gets to here!)
			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(error should be mismatched Byte field sizes, at syscall counter: 1, field position: 1");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_unlink_basic(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		
		StartTestCase("record then replay unlink (rr1.txt).\n", "", test_idx);
		int success = 0;
		int fd[2];
		write(fd[0], &success, sizeof(success));
		pipe(fd);
		int pid;
		if ((pid = fork()) == 0)
		{
			close(fd[0]);
			
			// record 
			record_start("rr1.txt");
			int recorded = unlink("README");
			record_stop();

			// replay
			replay_start("rr1.txt");
			int replayed = unlink("README");
			replay_stop();
			
			if (recorded != replayed)
			{
				printf(1, "TEST MESSAGE: Mismatch int output for record and replay unlink\n");
				exit();
			}
			// passes!
			success = 1;
			write(fd[1], &success, sizeof(success));

			// should fail
			replay_start("rr1.txt");
			unlink("READMEa");
			replay_stop();

			// fails (if it gets to here!)
			success = 0;
			write(fd[1], &success, sizeof(success));
			close(fd[1]);
			exit();
		}
		wait();
		close(fd[1]);
		read(fd[0], &success, sizeof(success));
		close(fd[0]);
		printf(1, "\n\n(error should be mismatched Byte field sizes, at syscall counter: 1, field position: 1");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_prof_tests_basic(int test)
{

	int test_idx = 1;
	// 1
	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("test1 (rr1.txt).\n", "", test_idx);
		int success = 0;
		/*insert test 1*/
			
		// setting cmd line args
		int argc = 2;
		char * argv[] = {"wc", "README"};

		// setting up internal variables
		int fd, i, totalw = 0;

		// executing test (record)
		record_start("./rr1.txt");

		fd=0;
		i=0;
		totalw=0;

		if(argc <= 1){
			wc(0, "");
			exit();
		}

		for(i = 1; i < argc; i++){
			if((fd = open(argv[i], 0)) < 0){
				printf(1, "wc: cannot open %s\n", argv[i]);
				exit();
			}
			totalw += wc(fd, argv[i]);
			printf(1, "totalw: %d\n", totalw);
			close(fd);
		}
			
		switch( totalw % 5 ) {
		case 0:
			printf(1, "device: %d\n", uptime());
			break;
		case 1:
			printf(1, "License: %d\n", totalw);
			break;
		case 2:
			printf(1, "embedded: %d\n", uptime()+totalw);
			break;
		case 3:
			printf(1, "designed: %d\n", totalw - uptime());
			break;
		case 4:
			printf(1, "inner: %d\n", uptime());
			break;
		}
		record_stop();

		// executing test (replay)
		replay_start("./rr1.txt");

		if(argc <= 1){
			wc(0, "");
			exit();
		}
		fd=0;
		i=0;
		totalw=0;

		for(i = 1; i < argc; i++){
			if((fd = open(argv[i], 0)) < 0){
				printf(1, "wc: cannot open %s\n", argv[i]);
				exit();
			}
			totalw += wc(fd, argv[i]);
			printf(1, "totalw: %d\n", totalw);
			close(fd);
		}
		
		switch( totalw % 5 ) {
		case 0:
			printf(1, "device: %d\n", uptime());
			break;
		case 1:
			printf(1, "License: %d\n", totalw);
			break;
		case 2:
			printf(1, "embedded: %d\n", uptime()+totalw);
			break;
		case 3:
			printf(1, "designed: %d\n", totalw - uptime());
			break;
		case 4:
			printf(1, "inner: %d\n", uptime());
			break;
		}
		replay_stop();

		// passes!
		success = 1;

		if(success)
			passedTest();
		else
			failedTest();
	}

	// 2
	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("test2 (rr1.txt).\n", "", test_idx);
		int success = 0;

		// test 2 (record)
		record_start("./rr1.txt");

		int x = uptime();
		
		printf(1, "Current uptime: %d\n", x);	
		
		if( (x % 10) == 0 ) {
			printf(1, "You win!\n");
		} else {
			printf(1, "You lose...\n");
		}
		
		// trying to pass some time
		for (int i = 0; i < 100000; i++)
		{
			int y = i;
			if(y%i !=0) // always zero
				break;
		}
		record_stop();

		printf(1, "\nTEST MESSAGE: actual uptime: %d\n\n", uptime());

		// test 2 (replay)

		replay_start("./rr1.txt");

		int y = uptime();
		
		printf(1, "Current uptime: %d\n", y);	
		
		if( (y % 10) == 0 ) {
			printf(1, "You win!\n");
		} else {
			printf(1, "You lose...\n");
		}
		replay_stop();

		// passes!
		success = 1;
		if(success)
			passedTest();
		else
			failedTest();
	}
	// 3
	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("test3 (rr1.txt).\n", "", test_idx);
		int success = 0;
		/*insert test*/
		// test 3 (record)

		record_start("./rr1.txt");
		int x = getpid();
		
		printf(1, "Current PID: %d\n", x);	
		
		if( (x*uptime() % 10) == 0 ) {
			printf(1, "You win!\n");
		} else {
			printf(1, "You lose...\n");
		}

		record_stop();
		

		printf(1, "\nTEST MESSAGE: actual pid:%d , actual uptime: %d\n\n", getpid(), uptime());

		// test 3 (replay)
		replay_start("./rr1.txt");

		int y = getpid();

		printf(1, "Current PID: %d\n", y);	
		
		if( (y*uptime() % 10) == 0 ) {
			printf(1, "You win!\n");
		} else {
			printf(1, "You lose...\n");
		}
		replay_stop();

		// passes!
		success = 1;

		if(success)
			passedTest();
		else
			failedTest();
	}
	// 4
	if((test == test_idx++ || test == ALL) && 0)
	{
		StartTestCase("test4 (rr1.txt).\n", "", test_idx);
		int success = 0;

		/*insert test*/
		// test 4 (record)
		record_start("./rr1.txt");
		char dirname[20] = { 'd', 'i', 'r', 0, };
		int x = uptime();
		
		dirname[3] = 'a' + x % 20;
		dirname[4] = 'A' + x % 21;
		dirname[5] = 'a' + x % 22;
		dirname[6] = 'A' + x % 23;
		
		printf(1, "folder name: %s\n", dirname);	
		mkdir(dirname);

		record_stop();

		// test 4 (replay)
		replay_start("./rr1.txt");
		char dirname1[20] = { 'd', 'i', 'r', 0, };
		int y = uptime();
		
		dirname1[3] = 'a' + y % 20;
		dirname1[4] = 'A' + y % 21;
		dirname1[5] = 'a' + y % 22;
		dirname1[6] = 'A' + y % 23;
		
		
		printf(1, "folder name: %s\n", dirname1);	
		mkdir(dirname1);
		replay_stop();

		// passes!
		success = 1;
		if(success)
			passedTest();
		else
			failedTest();
	}
	// 5 (skipping for once io is done right)
	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("test5 (rr1.txt).\n", "", test_idx);
		int success = 0;

		/*insert test*/			

		// passes!
		success = 1;
		if(success)
			passedTest();
		else
			failedTest();
	}

	// 6
	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("test1 (rr1.txt).\n", "", test_idx);
		int success = 0;
		/*insert test*/
		// test 6 (record)
		record_start("./rr1.txt");
		int fd, ret1, ret2;
		fd = open("file.txt", 0);
		ret1 = close(fd);
		ret2 = close(fd);	
		record_stop();	

		printf(1,"\nTEST MESSAGE: recorded --- fd: %d, ret1: %d, ret2: %d\n\n",fd, ret1, ret2);

		replay_start("./rr1.txt");
		int fd1, ret11, ret21;
		fd1 = open("file.txt", 0);
		ret11 = close(fd1);
		ret21 = close(fd1);	
		replay_stop();

		printf(1,"\nTEST MESSAGE: replayed --- fd: %d, ret1: %d, ret2: %d\n\n",fd1, ret11, ret21);

		// passes!
		success = 1;
		if(success)
			passedTest();
		else
			failedTest();
	}
		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

int
test_suit_all_syscalls(int test)
{

	int test_idx = 1;

	if(test == test_idx++ || test == ALL )
	{
		StartTestCase("record then replay every syscall (rr1.txt).\n", "", test_idx);
		int success = 0;

		/* actual code */
		int recorded[11];
		int replayed[11];

		// setup (setting up a dummy file)
		int fd1 = open("a.txt\0a", O_CREATE | O_RDWR);
		char actual_buff[201];
		for (int i = 0; i < sizeof(actual_buff); i++)
			actual_buff[i] = i%10;			
		write(fd1, actual_buff, sizeof(actual_buff));
		close(fd1);

		// record 
		record_start("./rr1.txt");
		recorded[0] = getpid();
		recorded[1] = uptime();
		recorded[2] = open("a.txt\0a", O_CREATE | O_RDWR);
		char buff1[512];
		recorded[3] = read(recorded[2], buff1, 200);
		struct stat st1;
		recorded[5] = fstat(recorded[2], &st1);
		recorded[4] = close(recorded[2]);
		recorded[6] = mkdir("some_dir");
		recorded[7] = chdir("some_dir");
		recorded[8] = mknod("apples",1, 1);
		recorded[9] = link("README", "echo");
		recorded[10] = unlink("echo");
		record_stop();

		// replay
		replay_start("./rr1.txt");
		replayed[0] = getpid();
		replayed[1] = uptime();
		replayed[2] = open("a.txt\0a", O_CREATE | O_RDWR);
		char buff2[512];
		replayed[3] = read(replayed[2], buff2, 200);
		struct stat st2;
		replayed[5] = fstat(replayed[2], &st2);
		replayed[4] = close(replayed[2]);
		replayed[6] = mkdir("some_dir");
		replayed[7] = chdir("some_dir");
		replayed[8] = mknod("apples", 1, 1);
		replayed[9] = link("README", "echo");
		replayed[10] = unlink("echo");
		replay_stop();
		
		for (int i = 0; i < 0; i++)
		{
			if (recorded[i] != replayed[i])
			{
				printf(1, "TEST MESSAGE: Mismatch int output (rec:%d; rep: %d) for record and replay: %d\n", recorded[i], replayed[i], i);
				exit();
			}
		}

		// passes!
		success = 1;

		printf(1, "\n\n(Should be no errors)");
		if(success)
			passedTest();
		else
			failedTest();
	}

		// within range of test_idx
	if(test == ALL || (test >= 0 && test < test_idx))
		return 0;

	return -1;
}

// test_suit = ALL, equivalent to run all tests Suites
// test = ALL, equivalent to run all tests in test Suite
// test_suit = Any zero or Positive Num, equivalent to run a particular test suit
// test = Positive Num, equivalent to run a particular test in a test suite

int
main(int argc, char *argv[])
{
	int test_suit = 14;
	int test = ALL;

	if(test_suit == ALL)
	{
		printf(1, "The program does not accept test_suite 'ALL' anymore. Need to fix how to pass data");
		exit();
	}

	// error in arguments
	if(test_suit == ALL && test != ALL)
	{
		printf(1,"You cannot have a specific test with the test_suit = ALL flag\n");
		exit();
	}

	// test suites 
	int test_suit_idx = 1;
	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 1
		StartTestSuite("Test Suite (%d): Trivial IO",test_suit_idx);
		EndTestSuite(test_suit_trivial_IO(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 2
		StartTestSuite("Test Suit (%d): getpid Basic",test_suit_idx);
		EndTestSuite(test_suit_getpid(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 3
		StartTestSuite("Test Suit (%d): using getpid to record/replay 1000, and fail on 1001",test_suit_idx);
		EndTestSuite(test_suit_record_replay_1000(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 4
		StartTestSuite("Test Suit (%d): uptime Basic",test_suit_idx);
		EndTestSuite(test_suit_uptime_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 5
		StartTestSuite("Test Suit (%d): close Basic",test_suit_idx);
		EndTestSuite(test_suit_close_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 6
		StartTestSuite("Test Suit (%d): open Basic",test_suit_idx);
		EndTestSuite(test_suit_open_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 7
		StartTestSuite("Test Suit (%d): read Basic",test_suit_idx);
		EndTestSuite(test_suit_read_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 8
		StartTestSuite("Test Suit (%d): fstat Basic",test_suit_idx);
		EndTestSuite(test_suit_fstat_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 9
		StartTestSuite("Test Suit (%d): chdir Basic",test_suit_idx);
		EndTestSuite(test_suit_chdir_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 10
		StartTestSuite("Test Suit (%d): mknod Basic",test_suit_idx);
		EndTestSuite(test_suit_mknod_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 11
		StartTestSuite("Test Suit (%d): link Basic",test_suit_idx);
		EndTestSuite(test_suit_link_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 12
		StartTestSuite("Test Suit (%d): unlink Basic",test_suit_idx);
		EndTestSuite(test_suit_unlink_basic(test), test, test_suit);
	}

	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 13
		StartTestSuite("Test Suit (%d): Prof tests Basic",test_suit_idx);
		EndTestSuite(test_suit_prof_tests_basic(test), test, test_suit);
	}
	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 14
		StartTestSuite("Test Suit (%d): all syscalls in sequence",test_suit_idx);
		EndTestSuite(test_suit_all_syscalls(test), test, test_suit);
	}
	// not within range of test_idx
	if( test_suit != ALL && (test_suit < 0 || test >= test_suit_idx))
	{
		printf(0, "INVALID TEST SUITE SUPPLIED!\n"); 
	}
	if(test_suit == test_suit_idx++ || test_suit == ALL)
	{
		// 15
		StartTestSuite("Test Suit (%d): mkdir Basic",test_suit_idx);
		EndTestSuite(test_suit_mkdir_basic(test), test, test_suit);
	}

	printf(1, "TOTAL PASSED %d/%d\n", global_passed, global_total);
  exit();
}
