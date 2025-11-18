#include "sensor_db.h"
#include <time.h>
#include "logger.h"


FILE * open_db(char * filename, bool append){
	if(filename==NULL) return NULL;

	FILE *f = fopen(filename, append ? "a" : "w");

	if (f==NULL){
		perror("open_db : file open failed");
		return NULL;
	}

	create_log_process();
	write_to_log_process("Data file opened.");

	return f;
}


int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts){
	if (f== NULL){
		return -1;
	}
	int write = fprintf(f, "%u,%.6f,%ld\n",(unsigned int)id,(double)value,(long)ts);

	if(write<0){
		perror("insert_sensor : fprintf failed");
		return -1;
	}
	if (fflush(f) != 0) {
        	perror("insert_sensor: fflush failed");
        	return -1;
	}
        
        write_to_log_process("Data inserted.");

	return 0;
}


int close_db(FILE * f){

        if (f == NULL) {
                return -1;
        } 
        fflush(f);

        write_to_log_process("Data file closed.");
        end_log_process();

        int file = fclose(f);
        if (file == EOF) {
                perror("close_db: fclose failed");
                return -1;
        }

        return 0;  
}


