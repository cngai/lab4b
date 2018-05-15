// NAME: Christopher Ngai
// EMAIL: cngai1223@gmail.com
// ID: 404795904

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <math.h>
#include <mraa.h>
#include <mraa/aio.h>
#include <errno.h>
#include <getopt.h>
#include <time.h>
#include <ctype.h>

/********************/
/* GLOBAL VARIABLES */
/********************/

//command line argument variables
int period_duration = 1;		//default is 1 second
char scale_opt = 'F';			//default is Fahrenheit
char *log_filename = NULL;
int log_arg = 0;
int log_fd;

//temperature-related variables
const int B = 4275;				//B value of the thermistor
const int R0 = 100000;			//R0 = 100k
mraa_aio_context temp_sensor;
int input_temp = 0;

//button-related variables
mraa_gpio_context button;

//poll-related variables
struct pollfd pollfd_array[1];	//array that holds pollfd structs

//miscellaneous variables
int stop = 0;
int valid_command = 1;			//flag to see if command is valid or not

/********************/
/* HELPER FUNCTIONS */
/********************/

//takes input temperature and converts it to actual temperature based on F or C scale
float temp_converter(int init_temp, char scale){
	float R = 1023.0 / (init_temp - 1.0);
	R = R0 * R;
	float temp_cels = 1.0 / (log(R/R0) / B + 1/298.15) - 273.15;		//convert temperature in celsius

	//return temperature based on scale
	if (scale == 'F'){
		float temp_fahr = temp_cels * 9/5 + 32;
		return temp_fahr;
	}
	else{
		return temp_cels;
	}
}

//if button pushed, handle shutdown properly
void handle_shutdown(int fd){
	//initialize time struct
	struct tm* sd_time_values;
	time_t sd_timer;
	char sd_time_string[10];

	//get final time
	time_t sd_time_ret = time(&sd_timer);
	if (sd_time_ret == -1){
		fprintf(stderr, "Error getting shutdown time.\n");
		exit(1);
	}
	sd_time_values = localtime(&sd_timer);						//converts time in timer into local time
	strftime(sd_time_string, 10, "%H:%M:%S", sd_time_values);	//formats time and places into time_string array

	//create report for each sample
	printf("%s SHUTDOWN\n", sd_time_string);					//write report to STDOUT
	if (log_arg){
		dprintf(fd, "%s SHUTDOWN\n", sd_time_string);			//write report to log file
	}
}

//change scale based on input scale
void change_scale(char input_scale){
	scale_opt = input_scale;
}

//change stop based on input
void change_stop(int input){
	stop = input;				//1 means stop, 0 means start (don't stop)
}

//change period
void change_period(int new_period){
	period_duration = new_period;
}

//deal with invalid commands
void process_invalid_command(){
	printf("%s\n", "Error: invalid command.");
	valid_command = 0;			//command is no longer valid
}

//processes commands from STDIN and performs them
void perform_command(char* command){
	if (strcmp(command, "SCALE=F") == 0){
		change_scale('F');
	}
	else if (strcmp(command, "SCALE=C") == 0){
		change_scale('C');
	}
	else if (strcmp(command, "STOP") == 0){
		change_stop(1);
	}
	else if (strcmp(command, "START") == 0){
		change_stop(0);
	}
	else if (strcmp(command, "OFF") == 0){
		dprintf(log_fd, "OFF\n");
		handle_shutdown(log_fd);
		exit(0);
	}
	else{
		char period_string[] = "PERIOD=";
		int p_cnt = 0;							//iterates through period_string
		int matches_period = 1;

		if (strlen(command) > strlen(period_string)){
			//check to see if command starts with "PERIOD="
			while (command[p_cnt] != '\0' && period_string[p_cnt] != '\0'){
				if (command[p_cnt] != period_string[p_cnt]){
					matches_period = 0;
				}
				p_cnt++;
			}
		}
		else{
			matches_period = 0;
		}

		char log_string[] = "LOG ";
		int l_cnt = 0;							//iterates through log_string
		int matches_log = 1;

		if (strlen(command) > strlen(log_string)){
			//check to see if command starts with "LOG "
			while (command[l_cnt] != '\0' && log_string[l_cnt] != '\0'){
				if (command[l_cnt] != log_string[l_cnt]){
					matches_log = 0;
				}
				l_cnt++;
			}
		}
		else{
			matches_log = 0;
		}

		//if neither PERIOD nor LOG command
		if (matches_period == 0 && matches_log == 0){
			process_invalid_command();
		}
		else{
			if (matches_period){
				while (command[p_cnt] != '\0'){
					if (!isdigit(command[p_cnt])){
						process_invalid_command();
					}
					p_cnt++;
				}

				//change period
				change_period(atoi(&command[7]));
			}
		}
	}
}


/*****************/
/* MAIN FUNCTION */
/*****************/

int main (int argc, char* argv[]){
	int opt;

	static struct option long_options[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'},
		{0, 0, 0, 0}
	};

	while((opt = getopt_long(argc, argv, "p:s:l", long_options, NULL)) != -1){
    	switch(opt){
      		case 'p':
        		period_duration = atoi(optarg);
        		break;
        	case 's':
        		if (strlen(optarg) == 1){
        			if (optarg[0] == 'F' || optarg[0] == 'C'){
        				scale_opt = optarg[0];
        			}
        		}
        		else{
        			fprintf(stderr, "Error: invalid scale argument. %s.\n", strerror(errno));
        			exit(1);
        		}
        		break;
        	case 'l':
        		log_filename = optarg;
        		log_arg = 1;
        		break;
      		default:
       			fprintf(stderr, "Unrecognized argument.\n");
        		exit(1);
    	}
	}

	//create log file
	if (log_arg == 1){
		//give everyone R/W permissions
        log_fd = open(log_filename, O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if (log_fd == -1){
                fprintf(stderr, "Error creating log file. %s.\n", strerror(errno));
                exit(1);
        }
	}

	//initialize temperature sensor
	temp_sensor = mraa_aio_init(1);							//A0 corresponds to pin 1

	//initialize button
	button = mraa_gpio_init(62);							//GPIO pin 51 is initialized as pin 62
	mraa_gpio_dir(button, MRAA_GPIO_IN);					//configure button GPIO interface to be an input pin

	//initialize time struct
	struct tm* time_values;									//struct that includes values like sec, mins, hours, day of month, etc.
	time_t curr_time;										//current time
	time_t p_start, p_end;									//start of period time, end of period time
	char time_string[10];									//holds properly formatted time in HH:MM:SS

	//initialize poll struct
	pollfd_array[0].fd = 0;									//file descriptor is STDIN
	pollfd_array[0].events = POLLIN | POLLHUP | POLLERR;

	int poll_rv;											//holds return value of poll()

	while (1){
		//read input temp
		input_temp = mraa_aio_read(temp_sensor);
		if (input_temp == -1){
			fprintf(stderr, "Error while reading input temperature. %s.\n", strerror(errno));
			exit(1);
		}
		//convert to input temperature to celsius/fahrenheit temperature
		float converted_temp = temp_converter(input_temp, scale_opt);

		//get time
		time_t time_ret = time(&curr_time);
		if (time_ret == -1){
			fprintf(stderr, "Error getting current time.\n");
			exit(1);
		}
		time_values = localtime(&curr_time);				//converts time in timer into local time
		strftime(time_string, 10, "%H:%M:%S", time_values);	//formats time and places into time_string array

		//create report for each sample
		printf("%s %.1f\n", time_string, converted_temp);	//write report to STDOUT
		if (log_arg){
			dprintf(log_fd, "%s %.1f\n", time_string, converted_temp);	//write report to log file
		}

		//check if period taken place
		time_ret = time(&p_start);
		if (time_ret == -1){
			fprintf(stderr, "Error getting start time.\n");
			exit(1);
		}
		time_ret = time(&p_end);
		if (time_ret == -1){
			fprintf(stderr, "Error getting end time.\n");
			exit(1);
		}


		double time_passed = difftime(p_end, p_start);			//gets difference between start and end time
		while (time_passed < period_duration){
			//get state of button once per period
			int button_ret = mraa_gpio_read(button);			//read GPIO value
			if (button_ret == -1){
				fprintf(stderr, "Error getting state of button. %s.\n", strerror(errno));
				exit(1);
			}
			//if button is pushed
			else if (button_ret == 1){
				handle_shutdown(log_fd);						//output and log final sample
				exit(0);
			}

			//check if input read from STDIN
			poll_rv = poll(pollfd_array, 1, 0);
			if (poll_rv < 0){
				fprintf(stderr, "Error while polling. %s.\n", strerror(errno));
				exit(1);
			}

			//if input available
			if (pollfd_array[0].revents & POLLIN){
				//holds characters stored from keyboard input
	            char buff[256];
	            memset(buff, 0, 256*sizeof(char));				//refill buffer with 0 so doesn't have leftover input from previous read

	            int bytesRead = read(0, &buff, 256);
	            if (bytesRead < 0){
	                    fprintf(stderr, "Error reading from STDIN. %s.\n", strerror(errno));
	                    exit(1);
	            }

	            char* comm = strtok(buff, "\n");				//breaks buff into strings when newline read

	            //process commands until no commands left
	            while (comm != NULL && bytesRead > 0){
	            	//process command
	            	perform_command(comm);
	            	//log command
	            	if (log_arg && valid_command){
	            		dprintf(log_fd, "%s\n", comm);
	            	}

	            	comm = strtok(NULL, "\n");					//keep reading from buff, if no input left set to NULL
	            	valid_command = 1;							//assume next command is valid
	            }
	        }

	        //if stop flag set,stop generating reports but keep generating input
	        if (!stop){
	        	time_ret = time(&p_end);
				if (time_ret == -1){
					fprintf(stderr, "Error getting end time.\n");
					exit(1);
				}
	        }

	        //update time passed
	        time_passed = difftime(p_end, p_start);
		}
	}

	return 0;
}